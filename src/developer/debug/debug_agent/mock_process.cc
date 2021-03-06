// Copyright 2019 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "src/developer/debug/debug_agent/mock_process.h"

#include "src/developer/debug/debug_agent/mock_thread.h"

namespace debug_agent {

MockProcess::MockProcess(DebugAgent* debug_agent, zx_koid_t koid, std::string name,
                         std::shared_ptr<arch::ArchProvider> arch_provider,
                         std::shared_ptr<ObjectProvider> object_provider)
    : DebuggedProcess(debug_agent, {koid, std::move(name), zx::process(), std::move(arch_provider),
                                    std::move(object_provider)}) {}
MockProcess::~MockProcess() = default;

MockThread* MockProcess::AddThread(zx_koid_t thread_koid) {
  auto mock_thread =
      std::make_unique<MockThread>(this, thread_koid, arch_provider_, object_provider_);
  MockThread* thread_ptr = mock_thread.get();
  threads_[thread_koid] = std::move(mock_thread);
  return thread_ptr;
}

DebuggedThread* MockProcess::GetThread(zx_koid_t koid) const {
  auto it = threads_.find(koid);
  if (it == threads_.end())
    return nullptr;
  return it->second.get();
}

std::vector<DebuggedThread*> MockProcess::GetThreads() const {
  std::vector<DebuggedThread*> threads;
  threads.reserve(threads_.size());
  for (auto& kv : threads_)
    threads.emplace_back(kv.second.get());
  return threads;
}

}  // namespace debug_agent
