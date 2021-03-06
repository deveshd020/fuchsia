// Copyright 2016 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "src/ledger/bin/cloud_sync/impl/batch_download.h"

#include <lib/fit/function.h>
#include <lib/trace/event.h>

#include <utility>

#include "src/ledger/bin/cloud_sync/impl/constants.h"
#include "src/ledger/bin/storage/public/types.h"
#include "src/ledger/lib/callback/scoped_callback.h"
#include "src/ledger/lib/callback/waiter.h"
#include "src/ledger/lib/convert/convert.h"
#include "src/ledger/lib/logging/logging.h"
#include "src/ledger/lib/memory/ref_ptr.h"

namespace cloud_sync {

BatchDownload::BatchDownload(storage::PageStorage* storage,
                             encryption::EncryptionService* encryption_service,
                             std::vector<cloud_provider::Commit> remote_commits,
                             std::unique_ptr<cloud_provider::PositionToken> position_token,
                             fit::closure on_done, fit::closure on_error)
    : storage_(storage),
      encryption_service_(encryption_service),
      remote_commits_(std::move(remote_commits)),
      position_token_(std::move(position_token)),
      on_done_(std::move(on_done)),
      on_error_(std::move(on_error)),
      weak_ptr_factory_(this) {
  LEDGER_DCHECK(storage);
  TRACE_ASYNC_BEGIN("ledger", "batch_download", reinterpret_cast<uintptr_t>(this));
}

BatchDownload::~BatchDownload() {
  TRACE_ASYNC_END("ledger", "batch_download", reinterpret_cast<uintptr_t>(this));
}

void BatchDownload::Start() {
  LEDGER_DCHECK(!started_);
  started_ = true;
  auto waiter = ledger::MakeRefCounted<
      ledger::Waiter<encryption::Status, storage::PageStorage::CommitIdAndBytes>>(
      encryption::Status::OK);
  for (auto& remote_commit : remote_commits_) {
    if (!remote_commit.has_id() || !remote_commit.has_data()) {
      LEDGER_LOG(ERROR) << "Received invalid commits from the cloud provider";
      on_error_();
      return;
    }
    encryption_service_->DecryptCommit(
        remote_commit.data(),
        ledger::MakeScoped(
            weak_ptr_factory_.GetWeakPtr(),
            [this, id = std::move(remote_commit.id()), callback = waiter->NewCallback()](
                encryption::Status status, std::string content) mutable {
              if (status != encryption::Status::OK) {
                LEDGER_LOG(ERROR) << "Failed to decrypt the commit.";
                on_error_();
                return;
              }
              storage::CommitId local_id = storage::ComputeCommitId(content);
              if (convert::ToArray(encryption_service_->EncodeCommitId(local_id)) != id) {
                LEDGER_LOG(ERROR) << "Commit content doesn't match the received id.";
                on_error_();
                return;
              }

              callback(status, storage::PageStorage::CommitIdAndBytes(std::move(local_id),
                                                                      std::move(content)));
            }));
  }
  waiter->Finalize(ledger::MakeScoped(
      weak_ptr_factory_.GetWeakPtr(),
      [this](encryption::Status status,
             std::vector<storage::PageStorage::CommitIdAndBytes> commits) {
        if (status != encryption::Status::OK) {
          on_error_();
          return;
        }

        storage_->AddCommitsFromSync(
            std::move(commits), storage::ChangeSource::CLOUD,
            ledger::MakeScoped(weak_ptr_factory_.GetWeakPtr(), [this](ledger::Status status) {
              if (status != ledger::Status::OK) {
                on_error_();
                return;
              }

              UpdateTimestampAndQuit();
            }));
      }));
}

void BatchDownload::UpdateTimestampAndQuit() {
  if (!position_token_) {
    // Can be deleted within.
    on_done_();
    return;
  }

  storage_->SetSyncMetadata(
      kTimestampKey, convert::ToString(position_token_->opaque_id),
      ledger::MakeScoped(weak_ptr_factory_.GetWeakPtr(), [this](ledger::Status status) {
        if (status != ledger::Status::OK) {
          on_error_();
          return;
        }

        // Can be deleted within.
        on_done_();
      }));
}

}  // namespace cloud_sync
