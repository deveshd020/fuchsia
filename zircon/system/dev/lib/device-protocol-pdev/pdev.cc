// Copyright 2018 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <lib/device-protocol/pdev.h>

#include <ddk/debug.h>
#include <lib/mmio/mmio.h>

namespace ddk {

void PDev::ShowInfo() {
    pdev_device_info_t info;
    if (GetDeviceInfo(&info) == ZX_OK) {
        zxlogf(INFO, "VID:PID:DID         = %04x:%04x:%04x\n", info.vid, info.pid, info.did);
        zxlogf(INFO, "mmio count          = %d\n", info.mmio_count);
        zxlogf(INFO, "irq count           = %d\n", info.irq_count);
        zxlogf(INFO, "clk count           = %d\n", info.clk_count);
        zxlogf(INFO, "bti count           = %d\n", info.bti_count);
    }
}

zx_status_t PDev::MapMmio(uint32_t index, std::optional<MmioBuffer>* mmio) {
    pdev_mmio_t pdev_mmio;

    zx_status_t status = GetMmio(index, &pdev_mmio);
    if (status != ZX_OK) {
        return status;
    }
    return MmioBuffer::Create(pdev_mmio.offset, pdev_mmio.size, zx::vmo(pdev_mmio.vmo),
                              ZX_CACHE_POLICY_UNCACHED_DEVICE, mmio);
}

ClockProtocolClient PDev::GetClk(uint32_t index) {
    clock_protocol_t clk;
    size_t actual;
    zx_status_t res = GetProtocol(ZX_PROTOCOL_CLOCK, index, &clk, sizeof(clk), &actual);
    if (res != ZX_OK || actual != sizeof(clk)) {
        return {};
    }
    return ClockProtocolClient(&clk);
}

} // namespace ddk
