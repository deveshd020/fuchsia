// Copyright 2019 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "skip-block.h"

#include <optional>

#include <ddktl/protocol/badblock.h>
#include <ddktl/protocol/nand.h>
#include <fbl/vector.h>
#include <lib/fake_ddk/fake_ddk.h>
#include <lib/fzl/vmo-mapper.h>
#include <lib/zx/vmo.h>
#include <zircon/types.h>
#include <zxtest/zxtest.h>

constexpr uint32_t kPageSize = 1024;
constexpr uint32_t kOobSize = 8;
constexpr uint32_t kNumPages = 20;
constexpr uint32_t kBlockSize = kPageSize * kNumPages;
constexpr uint32_t kNumBlocks = 10;
constexpr uint32_t kEccBits = 10;

fuchsia_hardware_nand_Info kInfo = {kPageSize, kNumPages, kNumBlocks, kEccBits, kOobSize, 0, {}};

// We inject a special context as parent so that we can store the device.
struct Context {
  nand::SkipBlockDevice* dev = nullptr;
};

class Binder : public fake_ddk::Bind {
 public:
  zx_status_t DeviceRemove(zx_device_t* dev) override {
    Context* context = reinterpret_cast<Context*>(dev);
    if (context->dev) {
      context->dev->DdkRelease();
    }
    context->dev = nullptr;
    return ZX_OK;
  }
  zx_status_t DeviceAdd(zx_driver_t* drv, zx_device_t* parent, device_add_args_t* args,
                        zx_device_t** out) override {
    *out = parent;
    Context* context = reinterpret_cast<Context*>(parent);
    context->dev = reinterpret_cast<nand::SkipBlockDevice*>(args->ctx);

    if (args && args->ops) {
      if (args->ops->message) {
        zx_status_t status;
        if ((status = fidl_.SetMessageOp(args->ctx, args->ops->message)) < 0) {
          return status;
        }
      }
    }
    return ZX_OK;
  }
  zx_status_t DeviceGetProtocol(const zx_device_t* device, uint32_t proto_id,
                                void* protocol) override {
    auto out = reinterpret_cast<fake_ddk::Protocol*>(protocol);
    for (const auto& proto : protocols_) {
      if (proto_id == proto.id) {
        out->ops = proto.proto.ops;
        out->ctx = proto.proto.ctx;
        return ZX_OK;
      }
    }
    return ZX_ERR_NOT_SUPPORTED;
  }
};

// Fake for the nand protocol.
class FakeNand : public ddk::NandProtocol<FakeNand> {
 public:
  FakeNand() : proto_({&nand_protocol_ops_, this}) {
    ASSERT_OK(mapper_.CreateAndMap(kNumBlocks * kNumPages * kPageSize,
                                   ZX_VM_PERM_READ | ZX_VM_PERM_WRITE));
  }

  const nand_protocol_t* proto() const { return &proto_; }

  void set_result(zx_status_t result) { result_.push_back(result); }

  // Nand protocol:
  void NandQuery(fuchsia_hardware_nand_Info* info_out, size_t* nand_op_size_out) {
    *info_out = nand_info_;
    *nand_op_size_out = sizeof(nand_operation_t);
  }

  void NandQueue(nand_operation_t* op, nand_queue_callback completion_cb, void* cookie) {
    last_op_ = op->command;

    auto result = result_[call_++];

    if (result != ZX_OK) {
      completion_cb(cookie, result, op);
      return;
    }

    switch (op->command) {
      case NAND_OP_READ: {
        if (op->rw.offset_nand >= num_nand_pages_ || !op->rw.length ||
            (num_nand_pages_ - op->rw.offset_nand) < op->rw.length) {
          result = ZX_ERR_OUT_OF_RANGE;
          break;
        }
        if (op->rw.data_vmo == ZX_HANDLE_INVALID && op->rw.oob_vmo == ZX_HANDLE_INVALID) {
          result = ZX_ERR_BAD_HANDLE;
          break;
        }
        zx::unowned_vmo data_vmo(op->rw.data_vmo);
        if (*data_vmo) {
          auto* data = static_cast<uint8_t*>(mapper_.start()) + (op->rw.offset_nand * kPageSize);
          data_vmo->read(data, op->rw.offset_data_vmo, op->rw.length * kPageSize);
        }
        break;
      }
      case NAND_OP_WRITE: {
        if (op->rw.offset_nand >= num_nand_pages_ || !op->rw.length ||
            (num_nand_pages_ - op->rw.offset_nand) < op->rw.length) {
          result = ZX_ERR_OUT_OF_RANGE;
          break;
        }
        if (op->rw.data_vmo == ZX_HANDLE_INVALID && op->rw.oob_vmo == ZX_HANDLE_INVALID) {
          result = ZX_ERR_BAD_HANDLE;
          break;
        }
        zx::unowned_vmo data_vmo(op->rw.data_vmo);
        if (*data_vmo) {
          fzl::VmoMapper mapper;
          result = mapper.Map(*data_vmo, 0, 0, ZX_VM_PERM_READ);
          if (result != ZX_OK) {
            break;
          }
          auto* src = static_cast<uint8_t*>(mapper.start()) + (op->rw.offset_data_vmo * kPageSize);
          auto* dst = static_cast<uint8_t*>(mapper_.start()) + (op->rw.offset_nand * kPageSize);
          memcpy(dst, src, op->rw.length * kPageSize);
        }
        break;
      }
      case NAND_OP_ERASE: {
        if (!op->erase.num_blocks || op->erase.first_block >= nand_info_.num_blocks ||
            (op->erase.num_blocks > (nand_info_.num_blocks - op->erase.first_block))) {
          result = ZX_ERR_OUT_OF_RANGE;
          break;
        }
        auto* data = static_cast<uint8_t*>(mapper_.start()) +
                     (op->erase.first_block * kPageSize * kNumPages);
        memset(data, 0, op->erase.num_blocks * kPageSize * kNumPages);
        break;
      }
      default:
        result = ZX_ERR_NOT_SUPPORTED;
        break;
    }

    completion_cb(cookie, result, op);
    return;
  }

  zx_status_t NandGetFactoryBadBlockList(uint32_t* bad_blocks, size_t bad_block_len,
                                         size_t* num_bad_blocks) {
    *num_bad_blocks = 0;
    return ZX_OK;
  }

  const fzl::VmoMapper& mapper() { return mapper_; }
  const nand_op_t& last_op() { return last_op_; }

 private:
  size_t call_ = 0;
  nand_protocol_t proto_;
  fuchsia_hardware_nand_Info nand_info_ = kInfo;
  fbl::Vector<zx_status_t> result_;
  size_t num_nand_pages_ = kNumPages * kNumBlocks;

  fzl::VmoMapper mapper_;

  nand_op_t last_op_ = {};
};

// Fake for the bad block protocol.
class FakeBadBlock : public ddk::BadBlockProtocol<FakeBadBlock> {
 public:
  FakeBadBlock() : proto_({&bad_block_protocol_ops_, this}) {}

  const bad_block_protocol_t* proto() const { return &proto_; }

  void set_result(zx_status_t result) { result_ = result; }

  // Bad Block protocol:
  zx_status_t BadBlockGetBadBlockList(uint32_t* bad_block_list, size_t bad_block_list_len,
                                      size_t* bad_block_count) {
    *bad_block_count = grown_bad_blocks_.size();
    if (bad_block_list_len < *bad_block_count) {
      return bad_block_list == nullptr ? ZX_OK : ZX_ERR_BUFFER_TOO_SMALL;
    }
    memcpy(bad_block_list, grown_bad_blocks_.data(), grown_bad_blocks_.size());
    return result_;
  }
  zx_status_t BadBlockMarkBlockBad(uint32_t block) {
    if (result_ == ZX_OK) {
      grown_bad_blocks_.push_back(block);
    }
    return result_;
  }

  const fbl::Vector<uint32_t>& grown_bad_blocks() { return grown_bad_blocks_; }

 private:
  fbl::Vector<uint32_t> grown_bad_blocks_;
  bad_block_protocol_t proto_;
  zx_status_t result_ = ZX_OK;
};

class SkipBlockTest : public zxtest::Test {
 protected:
  SkipBlockTest() {
    fbl::Array<fake_ddk::ProtocolEntry> protocols(new fake_ddk::ProtocolEntry[2], 2);
    protocols[0] = {ZX_PROTOCOL_NAND, *reinterpret_cast<const fake_ddk::Protocol*>(nand_.proto())};
    protocols[1] = {ZX_PROTOCOL_BAD_BLOCK,
                    *reinterpret_cast<const fake_ddk::Protocol*>(bad_block_.proto())};
    ddk_.SetProtocols(std::move(protocols));
    ddk_.SetSize(kPageSize * kNumPages * kNumBlocks);
    ddk_.SetMetadata(&count_, sizeof(count_));
  }

  ~SkipBlockTest() { ddk_.DeviceRemove(parent()); }

  void CreatePayload(size_t size, zx::vmo* out) {
    zx::vmo vmo;
    fzl::VmoMapper mapper;
    ASSERT_OK(mapper.CreateAndMap(fbl::round_up(size, ZX_PAGE_SIZE),
                                  ZX_VM_PERM_READ | ZX_VM_PERM_WRITE, nullptr, &vmo));
    memset(mapper.start(), 0x4a, mapper.size());
    *out = std::move(vmo);
  }

  void Write(nand::ReadWriteOperation op, bool* bad_block_grown, zx_status_t expected = ZX_OK) {
    if (!client_) {
      client_.emplace(std::move(ddk().FidlClient()));
    }

    auto result = client_->Write(std::move(op));
    ASSERT_OK(result.status());
    ASSERT_STATUS(result.value().status, expected);
    *bad_block_grown = result.value().bad_block_grown;
  }

  void Read(nand::ReadWriteOperation op, zx_status_t expected = ZX_OK) {
    if (!client_) {
      client_.emplace(std::move(ddk().FidlClient()));
    }
    auto result = client_->Read(std::move(op));
    ASSERT_OK(result.status());
    ASSERT_STATUS(result.value().status, expected);
  }

  void WriteBytes(nand::WriteBytesOperation op, bool* bad_block_grown,
                  zx_status_t expected = ZX_OK) {
    if (!client_) {
      client_.emplace(std::move(ddk().FidlClient()));
    }
    auto result = client_->WriteBytes(std::move(op));
    ASSERT_OK(result.status());
    ASSERT_OK(result.value().status);
    *bad_block_grown = result.value().bad_block_grown;
  }

  void ValidateWritten(size_t offset, size_t size) {
    const uint8_t* start = static_cast<uint8_t*>(nand_.mapper().start()) + offset;
    for (size_t i = 0; i < size; i++) {
      ASSERT_EQ(start[i], 0x4a, "i = %zu", i);
    }
  }

  void ValidateUnwritten(size_t offset, size_t size) {
    const uint8_t* start = static_cast<uint8_t*>(nand_.mapper().start()) + offset;
    for (size_t i = 0; i < size; i++) {
      ASSERT_EQ(start[i], 0x00, "i = %zu", i);
    }
  }

  zx_device_t* parent() { return reinterpret_cast<zx_device_t*>(&ctx_); }
  nand::SkipBlockDevice& dev() { return *ctx_.dev; }
  Binder& ddk() { return ddk_; }
  FakeNand& nand() { return nand_; }
  FakeBadBlock& bad_block() { return bad_block_; }

 private:
  const uint32_t count_ = 1;
  Context ctx_ = {};
  Binder ddk_;
  FakeNand nand_;
  FakeBadBlock bad_block_;
  std::optional<::llcpp::fuchsia::hardware::skipblock::SkipBlock::SyncClient> client_;
};

TEST_F(SkipBlockTest, Create) { ASSERT_OK(nand::SkipBlockDevice::Create(nullptr, parent())); }

TEST_F(SkipBlockTest, GrowBadBlock) {
  ASSERT_OK(nand::SkipBlockDevice::Create(nullptr, parent()));

  nand().set_result(ZX_OK);
  nand().set_result(ZX_ERR_IO);
  nand().set_result(ZX_OK);
  nand().set_result(ZX_OK);

  zx::vmo vmo;
  ASSERT_OK(zx::vmo::create(fbl::round_up(kBlockSize, ZX_PAGE_SIZE), 0, &vmo));
  nand::ReadWriteOperation op = {};
  op.vmo = std::move(vmo);
  op.block = 5;
  op.block_count = 1;

  bool bad_block_grown;
  ASSERT_NO_FATAL_FAILURES(Write(std::move(op), &bad_block_grown));
  ASSERT_TRUE(bad_block_grown);
  ASSERT_EQ(bad_block().grown_bad_blocks().size(), 1);
  ASSERT_EQ(bad_block().grown_bad_blocks()[0], 5);
  ASSERT_EQ(nand().last_op(), NAND_OP_WRITE);
}

TEST_F(SkipBlockTest, GrowMultipleBadBlock) {
  ASSERT_OK(nand::SkipBlockDevice::Create(nullptr, parent()));

  // Erase Block 5
  nand().set_result(ZX_OK);
  // Write Block 5
  nand().set_result(ZX_ERR_IO);
  // Erase Block 6
  nand().set_result(ZX_ERR_IO);
  // Erase Block 7
  nand().set_result(ZX_OK);
  // Write Block 7
  nand().set_result(ZX_OK);

  zx::vmo vmo;
  ASSERT_OK(zx::vmo::create(fbl::round_up(kBlockSize, ZX_PAGE_SIZE), 0, &vmo));
  nand::ReadWriteOperation op = {};
  op.vmo = std::move(vmo);
  op.block = 5;
  op.block_count = 1;

  bool bad_block_grown;
  ASSERT_NO_FATAL_FAILURES(Write(std::move(op), &bad_block_grown));
  ASSERT_TRUE(bad_block_grown);
  ASSERT_EQ(bad_block().grown_bad_blocks().size(), 2);
  ASSERT_EQ(bad_block().grown_bad_blocks()[0], 5);
  ASSERT_EQ(bad_block().grown_bad_blocks()[1], 6);
  ASSERT_EQ(nand().last_op(), NAND_OP_WRITE);
}

TEST_F(SkipBlockTest, MappingFailure) {
  ASSERT_OK(nand::SkipBlockDevice::Create(nullptr, parent()));

  // Erase Block 5
  nand().set_result(ZX_OK);
  // Write Block 5
  nand().set_result(ZX_ERR_INVALID_ARGS);

  zx::vmo vmo;
  ASSERT_OK(zx::vmo::create(fbl::round_up(kBlockSize, ZX_PAGE_SIZE), 0, &vmo));
  nand::ReadWriteOperation op = {};
  op.vmo = std::move(vmo);
  op.block = 5;
  op.block_count = 1;

  bool bad_block_grown;
  ASSERT_NO_FATAL_FAILURES(Write(std::move(op), &bad_block_grown, ZX_ERR_INVALID_ARGS));
  ASSERT_FALSE(bad_block_grown);
  ASSERT_EQ(bad_block().grown_bad_blocks().size(), 0);
  ASSERT_EQ(nand().last_op(), NAND_OP_WRITE);
}

TEST_F(SkipBlockTest, ReadSuccess) {
  ASSERT_OK(nand::SkipBlockDevice::Create(nullptr, parent()));

  // Read Block 5.
  nand().set_result(ZX_OK);

  zx::vmo vmo;
  ASSERT_OK(zx::vmo::create(fbl::round_up(kBlockSize, ZX_PAGE_SIZE), 0, &vmo));
  nand::ReadWriteOperation op = {};
  op.vmo = std::move(vmo);
  op.block = 5;
  op.block_count = 1;

  ASSERT_NO_FATAL_FAILURES(Read(std::move(op), ZX_OK));
  ASSERT_EQ(bad_block().grown_bad_blocks().size(), 0);
  ASSERT_EQ(nand().last_op(), NAND_OP_READ);
}

TEST_F(SkipBlockTest, ReadFailure) {
  ASSERT_OK(nand::SkipBlockDevice::Create(nullptr, parent()));

  // Read Block 7.
  nand().set_result(ZX_ERR_INVALID_ARGS);

  zx::vmo vmo;
  ASSERT_OK(zx::vmo::create(fbl::round_up(kBlockSize, ZX_PAGE_SIZE), 0, &vmo));
  nand::ReadWriteOperation op = {};
  op.vmo = std::move(vmo);
  op.block = 7;
  op.block_count = 1;

  ASSERT_NO_FATAL_FAILURES(Read(std::move(op), ZX_ERR_INVALID_ARGS));
  ASSERT_EQ(bad_block().grown_bad_blocks().size(), 0);
  ASSERT_EQ(nand().last_op(), NAND_OP_READ);
}

TEST_F(SkipBlockTest, WriteBytesSingleBlock) {
    ASSERT_OK(nand::SkipBlockDevice::Create(nullptr, parent()));

    // Read block 5.
    nand().set_result(ZX_OK);
    // Erase block 5.
    nand().set_result(ZX_OK);
    // Write block 5.
    nand().set_result(ZX_OK);

    constexpr size_t kOffset = kPageSize;
    constexpr size_t kSize = kBlockSize - (2 * kPageSize);
    constexpr size_t kNandOffset = 5 * kBlockSize;

    zx::vmo vmo;
    ASSERT_NO_FATAL_FAILURES(CreatePayload(kSize, &vmo));
    nand::WriteBytesOperation op = {};
    op.vmo = std::move(vmo);
    op.offset = kNandOffset + kOffset;
    op.size = kSize;

    bool bad_block_grown;
    ASSERT_NO_FATAL_FAILURES(WriteBytes(std::move(op), &bad_block_grown));
    ASSERT_FALSE(bad_block_grown);
    ASSERT_EQ(bad_block().grown_bad_blocks().size(), 0);
    ASSERT_EQ(nand().last_op(), NAND_OP_WRITE);
    ASSERT_NO_FATAL_FAILURES(ValidateUnwritten(kNandOffset, kPageSize));
    ASSERT_NO_FATAL_FAILURES(ValidateWritten(kNandOffset + kOffset, kSize));
    ASSERT_NO_FATAL_FAILURES(ValidateUnwritten(kNandOffset + kOffset + kSize, kPageSize));
}

TEST_F(SkipBlockTest, WriteBytesMultipleBlocks) {
    ASSERT_OK(nand::SkipBlockDevice::Create(nullptr, parent()));

    // Read block 4.
    nand().set_result(ZX_OK);
    // Read block 6.
    nand().set_result(ZX_OK);
    // Erase block 4 - 6.
    nand().set_result(ZX_OK);
    nand().set_result(ZX_OK);
    nand().set_result(ZX_OK);
    // Write block  4 - 6.
    nand().set_result(ZX_OK);
    nand().set_result(ZX_OK);
    nand().set_result(ZX_OK);

    constexpr size_t kOffset = kPageSize;
    constexpr size_t kSize = (kBlockSize * 3) - (2 * kPageSize);
    constexpr size_t kNandOffset = 4 * kBlockSize;

    zx::vmo vmo;
    ASSERT_NO_FATAL_FAILURES(CreatePayload(kSize, &vmo));
    nand::WriteBytesOperation op = {};
    op.vmo = std::move(vmo);
    op.offset = kNandOffset + kOffset;
    op.size = kSize;

    bool bad_block_grown;
    ASSERT_NO_FATAL_FAILURES(WriteBytes(std::move(op), &bad_block_grown));
    ASSERT_FALSE(bad_block_grown);
    ASSERT_EQ(bad_block().grown_bad_blocks().size(), 0);
    ASSERT_EQ(nand().last_op(), NAND_OP_WRITE);
    ASSERT_NO_FATAL_FAILURES(ValidateUnwritten(kNandOffset, kPageSize));
    ASSERT_NO_FATAL_FAILURES(ValidateWritten(kNandOffset + kOffset, kSize));
    ASSERT_NO_FATAL_FAILURES(ValidateUnwritten(kNandOffset + kOffset + kSize, kPageSize));
}

TEST_F(SkipBlockTest, WriteBytesAligned) {
    ASSERT_OK(nand::SkipBlockDevice::Create(nullptr, parent()));

    // Erase block 4 - 5.
    nand().set_result(ZX_OK);
    nand().set_result(ZX_OK);
    nand().set_result(ZX_OK);
    // Write block  4 - 5.
    nand().set_result(ZX_OK);
    nand().set_result(ZX_OK);
    nand().set_result(ZX_OK);

    constexpr size_t kSize = (kBlockSize * 2);
    constexpr size_t kNandOffset = 4 * kBlockSize;

    zx::vmo vmo;
    ASSERT_NO_FATAL_FAILURES(CreatePayload(kSize, &vmo));
    nand::WriteBytesOperation op = {};
    op.vmo = std::move(vmo);
    op.offset = kNandOffset;
    op.size = kSize;

    bool bad_block_grown;
    ASSERT_NO_FATAL_FAILURES(WriteBytes(std::move(op), &bad_block_grown));
    ASSERT_FALSE(bad_block_grown);
    ASSERT_EQ(bad_block().grown_bad_blocks().size(), 0);
    ASSERT_EQ(nand().last_op(), NAND_OP_WRITE);
    ASSERT_NO_FATAL_FAILURES(ValidateWritten(kNandOffset, kSize));
}
