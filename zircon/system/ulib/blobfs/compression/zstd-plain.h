// Copyright 2019 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ZIRCON_SYSTEM_ULIB_BLOBFS_COMPRESSION_ZSTD_PLAIN_H_
#define ZIRCON_SYSTEM_ULIB_BLOBFS_COMPRESSION_ZSTD_PLAIN_H_

#include <zircon/types.h>

#include <memory>

#include <zstd/zstd.h>

#include "compressor.h"

namespace blobfs {

class ZSTDCompressor : public Compressor {
 public:
  // Returns the maximum possible size a buffer would need to be
  // in order to compress data of size |input_length|.
  static size_t BufferMax(size_t input_length);

  static zx_status_t Create(size_t input_size, void* compression_buffer,
                            size_t compression_buffer_length, std::unique_ptr<ZSTDCompressor>* out);
  ~ZSTDCompressor();

  ////////////////////////////////////////
  // Compressor interface
  size_t Size() const final;
  zx_status_t Update(const void* input_data, size_t input_length) final;
  zx_status_t End() final;

 private:
  ZSTDCompressor(ZSTD_CCtx* ctx, void* compression_buffer, size_t compression_buffer_length);

  ZSTD_CCtx* stream_ = nullptr;
  ZSTD_outBuffer output_ = {};
};

class AbstractZSTDDecompressor {
 public:
  AbstractZSTDDecompressor() = default;
  AbstractZSTDDecompressor(const AbstractZSTDDecompressor&) = delete;
  AbstractZSTDDecompressor& operator=(const AbstractZSTDDecompressor&) = delete;
  virtual ~AbstractZSTDDecompressor() {}

  // Decompress the source buffer into the target buffer, until either the source is drained or
  // the target is filled (or both). Internally invokes `DecompressStream` to perform streaming
  // decompression.
  zx_status_t Decompress(void* target_buf, size_t* target_size, const void* src_buf,
                         size_t* src_size);

 private:
  virtual size_t DecompressStream(ZSTD_DStream* zds, ZSTD_outBuffer* output,
                                  ZSTD_inBuffer* input) const = 0;
};

class ZSTDDecompressor : public AbstractZSTDDecompressor {
 public:
  ZSTDDecompressor() = default;

  // AbstractZSTDDecompressor interface.
  size_t DecompressStream(ZSTD_DStream* zds, ZSTD_outBuffer* output,
                          ZSTD_inBuffer* input) const final;

 private:
  DISALLOW_COPY_ASSIGN_AND_MOVE(ZSTDDecompressor);
};

// Production implementation of `DoZSTDDecompress(ZSTDDecompressor{}, ...)`.
zx_status_t ZSTDDecompress(void* target_buf, size_t* target_size, const void* src_buf,
                           size_t* src_size);

}  // namespace blobfs

#endif  // ZIRCON_SYSTEM_ULIB_BLOBFS_COMPRESSION_ZSTD_PLAIN_H_
