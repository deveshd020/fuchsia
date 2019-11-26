// Copyright 2019 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SRC_UI_LIB_ESCHER_VK_CHAINED_SEMAPHORE_GENERATOR_H_
#define SRC_UI_LIB_ESCHER_VK_CHAINED_SEMAPHORE_GENERATOR_H_

#include "src/lib/fxl/memory/weak_ptr.h"
#include "src/ui/lib/escher/renderer/semaphore.h"

#include <vulkan/vulkan.hpp>

namespace escher {

// Forward declaration.
class ChainedSemaphoreGenerator;
using ChainedSemaphoreGeneratorWeakPtr = fxl::WeakPtr<ChainedSemaphoreGenerator>;

// ChainedSemaphoreGenerator is used for generating VkSemaphores for
// synchronizing between gfx::Engine, gfx::Screenshotter, and gfx::Snaphshotter.
//
// Each time the caller calls TakeLastAndCreateNextSemaphore() to get a pair of
// VkSemaphore, which it will wait on |semaphore_to_wait| before executing
// its command and signal |semaphore_to_signal| after finishing execution. When
// the next caller calls this function, the caller will wait on the semaphore
// signalled by the previous caller. In this way all components are chained
// together and we can ensure the execution sequence of command buffers.
//
// Therefore, the caller MUST always signal |semaphore_to_signal| in their
// command buffer, otherwise the next caller will wait on this semaphore
// forever.
//
// Also, the caller MUST ensure that the submission sequence MUST be of the
// same order of the semaphore chain, otherwise it will cause deadlock on most
// platforms.
//
// TODO(41980): Add integration tests to test synchronization of gfx Engine,
// Screenshotter and Snapshotter as well.
//
class ChainedSemaphoreGenerator {
 public:
  struct SemaphorePair {
    // The |semaphore_to_wait| was the last semaphore generated by this class.
    SemaphorePtr semaphore_to_wait;
    // The |semaphore_to_signal| signal will be waited on by its next caller.
    SemaphorePtr semaphore_to_signal;
  };

  ChainedSemaphoreGenerator(vk::Device device);

  SemaphorePair TakeLastAndCreateNextSemaphore(bool exportable = false) {
    return SemaphorePair{.semaphore_to_wait = TakeLastSemaphore(),
                         .semaphore_to_signal = CreateNextSemaphore(exportable)};
  }

  ChainedSemaphoreGeneratorWeakPtr GetWeakPtr() { return weak_factory_.GetWeakPtr(); }

 private:
  SemaphorePtr CreateNextSemaphore(bool exportable);
  SemaphorePtr TakeLastSemaphore();

  vk::Device device_;
  SemaphorePtr last_semaphore_ = nullptr;

  fxl::WeakPtrFactory<ChainedSemaphoreGenerator> weak_factory_;  // must be last
};

}  // namespace escher

#endif  // SRC_UI_LIB_ESCHER_VK_CHAINED_SEMAPHORE_GENERATOR_H_