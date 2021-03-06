// Copyright 2019 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "src/ledger/bin/app/page_manager.h"

#include <lib/async/cpp/task.h>
#include <lib/fidl/cpp/optional.h>
#include <lib/fit/function.h>
#include <zircon/errors.h>
#include <zircon/syscalls.h>

#include <cstdint>
#include <memory>
#include <set>
#include <utility>
#include <vector>

#include "gtest/gtest.h"
#include "src/ledger/bin/app/constants.h"
#include "src/ledger/bin/app/disk_cleanup_manager_impl.h"
#include "src/ledger/bin/encryption/fake/fake_encryption_service.h"
#include "src/ledger/bin/environment/environment.h"
#include "src/ledger/bin/fidl/include/types.h"
#include "src/ledger/bin/storage/fake/fake_db_factory.h"
#include "src/ledger/bin/storage/fake/fake_ledger_storage.h"
#include "src/ledger/bin/storage/fake/fake_page_storage.h"
#include "src/ledger/bin/storage/impl/ledger_storage_impl.h"
#include "src/ledger/bin/storage/public/constants.h"
#include "src/ledger/bin/storage/public/ledger_storage.h"
#include "src/ledger/bin/sync_coordinator/public/ledger_sync.h"
#include "src/ledger/bin/sync_coordinator/testing/fake_ledger_sync.h"
#include "src/ledger/bin/testing/fake_disk_cleanup_manager.h"
#include "src/ledger/bin/testing/test_with_environment.h"
#include "src/ledger/lib/callback/capture.h"
#include "src/ledger/lib/callback/set_when_called.h"
#include "src/ledger/lib/convert/convert.h"
#include "third_party/abseil-cpp/absl/strings/string_view.h"

namespace ledger {
namespace {

constexpr absl::string_view kLedgerName = "ledger_under_test";

class PageManagerTest : public TestWithEnvironment {
 public:
  PageManagerTest() = default;
  PageManagerTest(const PageManagerTest&) = delete;
  PageManagerTest& operator=(const PageManagerTest&) = delete;

  ~PageManagerTest() override = default;

  // gtest::TestWithEnvironment:
  void SetUp() override {
    TestWithEnvironment::SetUp();
    page_id_ = RandomId();
    ledger_merge_manager_ = std::make_unique<LedgerMergeManager>(&environment_);
    storage_ = std::make_unique<storage::fake::FakeLedgerStorage>(&environment_);
    sync_ = std::make_unique<sync_coordinator::FakeLedgerSync>();
    disk_cleanup_manager_ = std::make_unique<FakeDiskCleanupManager>();
    page_manager_ = std::make_unique<PageManager>(
        &environment_, convert::ToString(kLedgerName), convert::ToString(page_id_.id),
        std::vector<PageUsageListener*>{disk_cleanup_manager_.get()}, storage_.get(), sync_.get(),
        ledger_merge_manager_.get());
  }

  PageId RandomId() {
    PageId result;
    environment_.random()->Draw(&result.id);
    return result;
  }

 protected:
  std::unique_ptr<storage::fake::FakeLedgerStorage> storage_;
  std::unique_ptr<sync_coordinator::FakeLedgerSync> sync_;
  std::unique_ptr<LedgerMergeManager> ledger_merge_manager_;
  std::unique_ptr<FakeDiskCleanupManager> disk_cleanup_manager_;
  std::unique_ptr<PageManager> page_manager_;
  PageId page_id_;
};

class StubConflictResolverFactory : public ConflictResolverFactory {
 public:
  explicit StubConflictResolverFactory(fidl::InterfaceRequest<ConflictResolverFactory> request)
      : binding_(this, std::move(request)) {
    binding_.set_error_handler([this](zx_status_t status) { disconnected = true; });
  }

  bool disconnected = false;

 private:
  void GetPolicy(PageId page_id, fit::function<void(MergePolicy)> callback) override {}

  void NewConflictResolver(PageId page_id,
                           fidl::InterfaceRequest<ConflictResolver> resolver) override {}

  fidl::Binding<ConflictResolverFactory> binding_;
};

TEST_F(PageManagerTest, OnDiscardableCalled) {
  bool get_page_callback_called;
  Status get_page_status;
  bool on_discardable_called;

  page_manager_->SetOnDiscardable(Capture(SetWhenCalled(&on_discardable_called)));

  PagePtr page;
  page_manager_->GetPage(LedgerImpl::Delegate::PageState::NEW, page.NewRequest(),
                         Capture(SetWhenCalled(&get_page_callback_called), &get_page_status));
  RunLoopUntilIdle();
  EXPECT_TRUE(get_page_callback_called);
  EXPECT_EQ(get_page_status, Status::OK);
  EXPECT_FALSE(on_discardable_called);

  fit::closure detacher = page_manager_->CreateDetacher();
  RunLoopUntilIdle();
  EXPECT_FALSE(on_discardable_called);

  page.Unbind();
  RunLoopUntilIdle();
  EXPECT_FALSE(on_discardable_called);

  detacher();
  RunLoopUntilIdle();
  EXPECT_TRUE(on_discardable_called);
}

TEST_F(PageManagerTest, PageIsClosedAndSyncedCheckNotFound) {
  bool called;
  Status status;
  PagePredicateResult is_closed_and_synced;

  // Check for a page that doesn't exist.
  storage_->should_get_page_fail = true;
  page_manager_->PageIsClosedAndSynced(
      Capture(SetWhenCalled(&called), &status, &is_closed_and_synced));
  RunLoopUntilIdle();
  EXPECT_TRUE(called);
  EXPECT_EQ(status, Status::PAGE_NOT_FOUND);
}

// Check for a page that exists, is synced and open. PageIsClosedAndSynced
// should be false.
TEST_F(PageManagerTest, PageIsClosedAndSyncedCheckClosed) {
  bool get_page_callback_called;
  Status get_page_status;
  bool called;
  PagePredicateResult is_closed_and_synced;

  storage_->should_get_page_fail = false;
  PagePtr page;
  storage::PageIdView storage_page_id = convert::ExtendedStringView(page_id_.id);
  page_manager_->GetPage(LedgerImpl::Delegate::PageState::NAMED, page.NewRequest(),
                         Capture(SetWhenCalled(&get_page_callback_called), &get_page_status));
  RunLoopUntilIdle();
  EXPECT_TRUE(get_page_callback_called);
  EXPECT_EQ(get_page_status, Status::OK);

  Status storage_status;
  storage_->set_page_storage_synced(storage_page_id, true);
  page_manager_->PageIsClosedAndSynced(
      Capture(SetWhenCalled(&called), &storage_status, &is_closed_and_synced));
  RunLoopUntilIdle();
  EXPECT_TRUE(called);
  EXPECT_EQ(storage_status, Status::OK);
  EXPECT_EQ(is_closed_and_synced, PagePredicateResult::PAGE_OPENED);

  // Close the page. PageIsClosedAndSynced should now be true.
  page.Unbind();
  RunLoopUntilIdle();

  page_manager_->PageIsClosedAndSynced(
      Capture(SetWhenCalled(&called), &storage_status, &is_closed_and_synced));
  RunLoopUntilIdle();
  EXPECT_TRUE(called);
  EXPECT_EQ(storage_status, Status::OK);
  EXPECT_EQ(is_closed_and_synced, PagePredicateResult::YES);
}

// Check for a page that exists, is closed, but is not synced.
// PageIsClosedAndSynced should be false.
TEST_F(PageManagerTest, PageIsClosedAndSyncedCheckSynced) {
  bool get_page_callback_called;
  Status get_page_status;
  bool called;
  PagePredicateResult is_closed_and_synced;

  storage_->should_get_page_fail = false;
  PagePtr page;
  storage::PageIdView storage_page_id = convert::ExtendedStringView(page_id_.id);

  page_manager_->GetPage(LedgerImpl::Delegate::PageState::NAMED, page.NewRequest(),
                         Capture(SetWhenCalled(&get_page_callback_called), &get_page_status));
  RunLoopUntilIdle();
  EXPECT_TRUE(get_page_callback_called);
  EXPECT_EQ(get_page_status, Status::OK);

  // Mark the page as unsynced and close it.
  storage_->set_page_storage_synced(storage_page_id, false);
  page.Unbind();
  RunLoopUntilIdle();

  Status storage_status;
  page_manager_->PageIsClosedAndSynced(
      Capture(SetWhenCalled(&called), &storage_status, &is_closed_and_synced));
  RunLoopUntilIdle();
  EXPECT_TRUE(called);
  EXPECT_EQ(storage_status, Status::OK);
  EXPECT_EQ(is_closed_and_synced, PagePredicateResult::NO);
}

// Check for a page that exists, is closed, and synced, but was opened during
// the PageIsClosedAndSynced call. Expect an |PAGE_OPENED| result.
TEST_F(PageManagerTest, PageIsClosedAndSyncedCheckPageOpened) {
  bool get_page_callback_called;
  Status get_page_status;
  PagePredicateResult is_closed_and_synced;

  storage_->should_get_page_fail = false;
  PagePtr page;
  storage::PageIdView storage_page_id = convert::ExtendedStringView(page_id_.id);

  page_manager_->GetPage(LedgerImpl::Delegate::PageState::NAMED, page.NewRequest(),
                         Capture(SetWhenCalled(&get_page_callback_called), &get_page_status));
  RunLoopUntilIdle();
  EXPECT_TRUE(get_page_callback_called);
  EXPECT_EQ(get_page_status, Status::OK);

  // Mark the page as synced and close it.
  storage_->set_page_storage_synced(storage_page_id, true);
  page.Unbind();
  RunLoopUntilIdle();

  // Call PageIsClosedAndSynced but don't let it terminate.
  bool page_is_closed_and_synced_called = false;
  storage_->DelayIsSyncedCallback(storage_page_id, true);
  Status storage_status;
  page_manager_->PageIsClosedAndSynced(Capture(SetWhenCalled(&page_is_closed_and_synced_called),
                                               &storage_status, &is_closed_and_synced));
  RunLoopUntilIdle();
  EXPECT_FALSE(page_is_closed_and_synced_called);

  // Open and close the page.
  page_manager_->GetPage(LedgerImpl::Delegate::PageState::NAMED, page.NewRequest(),
                         Capture(SetWhenCalled(&get_page_callback_called), &get_page_status));
  RunLoopUntilIdle();
  EXPECT_TRUE(get_page_callback_called);
  EXPECT_EQ(get_page_status, Status::OK);
  page.Unbind();
  RunLoopUntilIdle();

  // Make sure PageIsClosedAndSynced terminates with a |PAGE_OPENED| result.
  storage_->CallIsSyncedCallback(storage_page_id);
  RunLoopUntilIdle();

  EXPECT_TRUE(page_is_closed_and_synced_called);
  EXPECT_EQ(storage_status, Status::OK);
  EXPECT_EQ(is_closed_and_synced, PagePredicateResult::PAGE_OPENED);
}

// Check for a page that exists, is closed, and synced. Test two concurrent
// calls to PageIsClosedAndSynced, where the second one will start and terminate
// without the page being opened by external requests.
TEST_F(PageManagerTest, PageIsClosedAndSyncedConcurrentCalls) {
  bool get_page_callback_called;
  Status get_page_status;
  storage_->should_get_page_fail = false;
  PagePtr page;
  storage::PageIdView storage_page_id = convert::ExtendedStringView(page_id_.id);

  page_manager_->GetPage(LedgerImpl::Delegate::PageState::NAMED, page.NewRequest(),
                         Capture(SetWhenCalled(&get_page_callback_called), &get_page_status));
  RunLoopUntilIdle();
  EXPECT_TRUE(get_page_callback_called);
  EXPECT_EQ(get_page_status, Status::OK);

  // Mark the page as synced and close it.
  storage_->set_page_storage_synced(storage_page_id, true);
  page.Unbind();
  RunLoopUntilIdle();

  // Make a first call to PageIsClosedAndSynced but don't let it terminate.
  bool called1 = false;
  Status status1;
  PagePredicateResult is_closed_and_synced1;
  storage_->DelayIsSyncedCallback(storage_page_id, true);
  page_manager_->PageIsClosedAndSynced(
      Capture(SetWhenCalled(&called1), &status1, &is_closed_and_synced1));
  RunLoopUntilIdle();

  // Prepare for the second call: it will return immediately and the expected
  // result is |YES|.
  bool called2 = false;
  Status status2;
  PagePredicateResult is_closed_and_synced2;
  storage_->DelayIsSyncedCallback(storage_page_id, false);
  page_manager_->PageIsClosedAndSynced(
      Capture(SetWhenCalled(&called2), &status2, &is_closed_and_synced2));
  RunLoopUntilIdle();
  EXPECT_FALSE(called1);
  EXPECT_TRUE(called2);
  EXPECT_EQ(status2, Status::OK);
  EXPECT_EQ(is_closed_and_synced2, PagePredicateResult::YES);

  // Open and close the page.
  page_manager_->GetPage(LedgerImpl::Delegate::PageState::NAMED, page.NewRequest(),
                         Capture(SetWhenCalled(&get_page_callback_called), &get_page_status));
  RunLoopUntilIdle();
  EXPECT_TRUE(get_page_callback_called);
  EXPECT_EQ(get_page_status, Status::OK);
  page.Unbind();
  RunLoopUntilIdle();

  // Call the callback and let the first call to PageIsClosedAndSynced
  // terminate. The expected returned result is |PAGE_OPENED|.
  storage_->CallIsSyncedCallback(storage_page_id);
  RunLoopUntilIdle();

  EXPECT_TRUE(called1);
  EXPECT_EQ(status1, Status::OK);
  EXPECT_EQ(is_closed_and_synced1, PagePredicateResult::PAGE_OPENED);
}

TEST_F(PageManagerTest, PageIsClosedOfflineAndEmptyCheckNotFound) {
  bool called;
  Status status;
  PagePredicateResult is_closed_offline_empty;

  // Check for a page that doesn't exist.
  storage_->should_get_page_fail = true;
  page_manager_->PageIsClosedOfflineAndEmpty(
      Capture(SetWhenCalled(&called), &status, &is_closed_offline_empty));
  RunLoopUntilIdle();
  EXPECT_TRUE(called);
  EXPECT_EQ(status, Status::PAGE_NOT_FOUND);
}

TEST_F(PageManagerTest, PageIsClosedOfflineAndEmptyCheckClosed) {
  bool get_page_callback_called;
  Status get_page_status;
  bool called;
  PagePredicateResult is_closed_offline_empty;

  storage_->should_get_page_fail = false;
  PagePtr page;
  storage::PageIdView storage_page_id = convert::ExtendedStringView(page_id_.id);

  page_manager_->GetPage(LedgerImpl::Delegate::PageState::NAMED, page.NewRequest(),
                         Capture(SetWhenCalled(&get_page_callback_called), &get_page_status));
  RunLoopUntilIdle();
  EXPECT_TRUE(get_page_callback_called);
  EXPECT_EQ(get_page_status, Status::OK);

  storage_->set_page_storage_offline_empty(storage_page_id, true);
  Status storage_status;
  page_manager_->PageIsClosedOfflineAndEmpty(
      Capture(SetWhenCalled(&called), &storage_status, &is_closed_offline_empty));
  RunLoopUntilIdle();
  EXPECT_TRUE(called);
  EXPECT_EQ(storage_status, Status::OK);
  EXPECT_EQ(is_closed_offline_empty, PagePredicateResult::PAGE_OPENED);

  // Close the page. PagePredicateResult should now be true.
  page.Unbind();
  RunLoopUntilIdle();

  page_manager_->PageIsClosedOfflineAndEmpty(
      Capture(SetWhenCalled(&called), &storage_status, &is_closed_offline_empty));
  RunLoopUntilIdle();
  EXPECT_TRUE(called);
  EXPECT_EQ(storage_status, Status::OK);
  EXPECT_EQ(is_closed_offline_empty, PagePredicateResult::YES);
}

TEST_F(PageManagerTest, PageIsClosedOfflineAndEmptyCanDeletePageOnCallback) {
  bool page_is_empty_called = false;
  Status page_is_empty_status;
  PagePredicateResult is_closed_offline_empty;
  bool delete_page_called = false;
  Status delete_page_status;

  // The page is closed, offline and empty. Try to delete the page storage in
  // the callback.
  storage_->set_page_storage_offline_empty(page_id_.id, true);
  page_manager_->PageIsClosedOfflineAndEmpty([&](Status status, PagePredicateResult result) {
    page_is_empty_called = true;
    page_is_empty_status = status;
    is_closed_offline_empty = result;

    page_manager_->DeletePageStorage(
        Capture(SetWhenCalled(&delete_page_called), &delete_page_status));
  });
  RunLoopUntilIdle();
  // Make sure the deletion finishes successfully.
  ASSERT_NE(nullptr, storage_->delete_page_storage_callback);
  storage_->delete_page_storage_callback(Status::OK);
  RunLoopUntilIdle();

  EXPECT_TRUE(page_is_empty_called);
  EXPECT_EQ(page_is_empty_status, Status::OK);
  EXPECT_EQ(is_closed_offline_empty, PagePredicateResult::YES);

  EXPECT_TRUE(delete_page_called);
  EXPECT_EQ(delete_page_status, Status::OK);
}

// Verifies that two successive calls to GetPage do not create 2 storages.
TEST_F(PageManagerTest, CallGetPageTwice) {
  PagePtr page1;
  bool get_page_callback_called1;
  Status get_page_status1;
  page_manager_->GetPage(LedgerImpl::Delegate::PageState::NAMED, page1.NewRequest(),
                         Capture(SetWhenCalled(&get_page_callback_called1), &get_page_status1));
  RunLoopUntilIdle();
  EXPECT_TRUE(get_page_callback_called1);
  EXPECT_EQ(get_page_status1, Status::OK);
  PagePtr page2;
  bool get_page_callback_called2;
  Status get_page_status2;
  page_manager_->GetPage(LedgerImpl::Delegate::PageState::NAMED, page2.NewRequest(),
                         Capture(SetWhenCalled(&get_page_callback_called2), &get_page_status2));
  RunLoopUntilIdle();
  EXPECT_TRUE(get_page_callback_called2);
  EXPECT_EQ(get_page_status2, Status::OK);
  EXPECT_EQ(storage_->create_page_calls.size(), 0u);
  ASSERT_EQ(storage_->get_page_calls.size(), 1u);
  EXPECT_EQ(storage_->get_page_calls[0], convert::ToString(page_id_.id));
}

TEST_F(PageManagerTest, OnExternallyUsedUnusedCalls) {
  PagePtr page1;
  bool get_page_callback_called1;
  Status get_page_status1;
  PagePtr page2;
  bool get_page_callback_called2;
  Status get_page_status2;

  EXPECT_EQ(disk_cleanup_manager_->externally_used_count, 0);
  EXPECT_EQ(disk_cleanup_manager_->externally_unused_count, 0);
  EXPECT_EQ(disk_cleanup_manager_->internally_used_count, 0);
  EXPECT_EQ(disk_cleanup_manager_->internally_unused_count, 0);
  disk_cleanup_manager_->ResetCounters();

  // Open a page and check that OnExternallyUsed was called once.
  page_manager_->GetPage(LedgerImpl::Delegate::PageState::NAMED, page1.NewRequest(),
                         Capture(SetWhenCalled(&get_page_callback_called1), &get_page_status1));
  RunLoopUntilIdle();
  EXPECT_TRUE(get_page_callback_called1);
  EXPECT_EQ(get_page_status1, Status::OK);
  EXPECT_EQ(disk_cleanup_manager_->externally_used_count, 1);
  EXPECT_EQ(disk_cleanup_manager_->externally_unused_count, 0);
  // GetPage may or may not have triggered internal requests. If it did, the page must now be
  // internally unused, i.e. have the same number of OnInternallyUsed/Unused calls.
  EXPECT_EQ(disk_cleanup_manager_->internally_used_count,
            disk_cleanup_manager_->internally_unused_count);
  disk_cleanup_manager_->ResetCounters();

  // Open the page again and check that there is no new call to OnExternallyUsed.
  page_manager_->GetPage(LedgerImpl::Delegate::PageState::NAMED, page2.NewRequest(),
                         Capture(SetWhenCalled(&get_page_callback_called2), &get_page_status2));
  RunLoopUntilIdle();
  EXPECT_TRUE(get_page_callback_called2);
  EXPECT_EQ(get_page_status2, Status::OK);
  EXPECT_EQ(disk_cleanup_manager_->externally_used_count, 0);
  EXPECT_EQ(disk_cleanup_manager_->externally_unused_count, 0);
  EXPECT_EQ(disk_cleanup_manager_->internally_used_count, 0);
  EXPECT_EQ(disk_cleanup_manager_->internally_unused_count, 0);
  disk_cleanup_manager_->ResetCounters();

  // Close one of the two connections and check that there is still no call to OnExternallyUnused.
  page1.Unbind();
  RunLoopUntilIdle();
  EXPECT_EQ(disk_cleanup_manager_->externally_used_count, 0);
  EXPECT_EQ(disk_cleanup_manager_->externally_unused_count, 0);
  EXPECT_EQ(disk_cleanup_manager_->internally_used_count, 0);
  EXPECT_EQ(disk_cleanup_manager_->internally_unused_count, 0);
  disk_cleanup_manager_->ResetCounters();

  // Close the second connection and check that OnExternallyUnused was called once.
  page2.Unbind();
  RunLoopUntilIdle();
  EXPECT_EQ(disk_cleanup_manager_->externally_used_count, 0);
  EXPECT_EQ(disk_cleanup_manager_->externally_unused_count, 1);
  EXPECT_EQ(disk_cleanup_manager_->internally_used_count, 0);
  EXPECT_EQ(disk_cleanup_manager_->internally_unused_count, 0);
}

TEST_F(PageManagerTest, OnInternallyUsedUnusedCalls) {
  PagePtr page;

  EXPECT_EQ(disk_cleanup_manager_->externally_used_count, 0);
  EXPECT_EQ(disk_cleanup_manager_->externally_unused_count, 0);
  EXPECT_EQ(disk_cleanup_manager_->internally_used_count, 0);
  EXPECT_EQ(disk_cleanup_manager_->internally_unused_count, 0);
  disk_cleanup_manager_->ResetCounters();

  // Make an internal request by calling PageIsClosedAndSynced.
  bool called;
  Status storage_status;
  PagePredicateResult page_state;
  page_manager_->PageIsClosedAndSynced(
      Capture(SetWhenCalled(&called), &storage_status, &page_state));
  RunLoopUntilIdle();
  EXPECT_TRUE(called);
  EXPECT_EQ(storage_status, Status::OK);
  EXPECT_EQ(page_state, PagePredicateResult::NO);
  EXPECT_EQ(disk_cleanup_manager_->externally_used_count, 0);
  EXPECT_EQ(disk_cleanup_manager_->externally_unused_count, 0);
  // GetPage may or may not have triggered internal requests. If it did, the page must now be
  // internally unused, i.e. have the same number of OnInternallyUsed/Unused calls.
  EXPECT_EQ(disk_cleanup_manager_->internally_used_count,
            disk_cleanup_manager_->internally_unused_count);
  disk_cleanup_manager_->ResetCounters();

  // Open the same page with an external request and check that OnExternallyUsed was called once.
  bool get_page_callback_called;
  Status get_page_status;
  page_manager_->GetPage(LedgerImpl::Delegate::PageState::NAMED, page.NewRequest(),
                         Capture(SetWhenCalled(&get_page_callback_called), &get_page_status));
  RunLoopUntilIdle();
  EXPECT_TRUE(get_page_callback_called);
  EXPECT_EQ(get_page_status, Status::OK);
  EXPECT_EQ(disk_cleanup_manager_->externally_used_count, 1);
  EXPECT_EQ(disk_cleanup_manager_->externally_unused_count, 0);
  EXPECT_EQ(disk_cleanup_manager_->internally_used_count,
            disk_cleanup_manager_->internally_unused_count);
}

TEST_F(PageManagerTest, OnPageInternallyExternallyUsedUnused) {
  PagePtr page;
  storage::PageIdView storage_page_id = convert::ExtendedStringView(page_id_.id);

  EXPECT_EQ(disk_cleanup_manager_->externally_used_count, 0);
  EXPECT_EQ(disk_cleanup_manager_->externally_unused_count, 0);
  EXPECT_EQ(disk_cleanup_manager_->internally_used_count, 0);
  EXPECT_EQ(disk_cleanup_manager_->internally_unused_count, 0);

  // Open and close the page through an external request.
  bool get_page_callback_called;
  Status get_page_status;
  page_manager_->GetPage(LedgerImpl::Delegate::PageState::NAMED, page.NewRequest(),
                         Capture(SetWhenCalled(&get_page_callback_called), &get_page_status));
  RunLoopUntilIdle();
  EXPECT_TRUE(get_page_callback_called);
  EXPECT_EQ(get_page_status, Status::OK);
  // Mark the page as synced and close it.
  storage_->set_page_storage_synced(storage_page_id, true);
  page.Unbind();
  RunLoopUntilIdle();
  EXPECT_EQ(disk_cleanup_manager_->externally_used_count, 1);
  EXPECT_EQ(disk_cleanup_manager_->externally_unused_count, 1);
  // GetPage may or may not have triggered internal requests. If it did, the page must now be
  // internally unused, i.e. have the same number of OnInternallyUsed/Unused calls.
  EXPECT_EQ(disk_cleanup_manager_->internally_used_count,
            disk_cleanup_manager_->internally_unused_count);
  disk_cleanup_manager_->ResetCounters();

  // Start an internal request but don't let it terminate.
  PagePredicateResult is_synced;
  bool page_is_synced_called = false;
  storage_->DelayIsSyncedCallback(storage_page_id, true);
  Status storage_status;
  page_manager_->PageIsClosedAndSynced(
      Capture(SetWhenCalled(&page_is_synced_called), &storage_status, &is_synced));
  RunLoopUntilIdle();
  EXPECT_FALSE(page_is_synced_called);
  EXPECT_EQ(disk_cleanup_manager_->externally_used_count, 0);
  EXPECT_EQ(disk_cleanup_manager_->externally_unused_count, 0);
  EXPECT_EQ(disk_cleanup_manager_->internally_used_count, 1);
  EXPECT_EQ(disk_cleanup_manager_->internally_unused_count, 0);
  disk_cleanup_manager_->ResetCounters();

  // Open the same page with an external request and check that OnExternallyUsed was called once.
  page_manager_->GetPage(LedgerImpl::Delegate::PageState::NAMED, page.NewRequest(),
                         Capture(SetWhenCalled(&get_page_callback_called), &get_page_status));
  RunLoopUntilIdle();
  EXPECT_TRUE(get_page_callback_called);
  EXPECT_EQ(get_page_status, Status::OK);
  EXPECT_EQ(disk_cleanup_manager_->externally_used_count, 1);
  EXPECT_EQ(disk_cleanup_manager_->externally_unused_count, 0);
  EXPECT_EQ(disk_cleanup_manager_->internally_used_count, 0);
  EXPECT_EQ(disk_cleanup_manager_->internally_unused_count, 0);
  disk_cleanup_manager_->ResetCounters();

  // Close the page. We should get the externally unused notification.
  page.Unbind();
  RunLoopUntilIdle();
  EXPECT_EQ(disk_cleanup_manager_->externally_used_count, 0);
  EXPECT_EQ(disk_cleanup_manager_->externally_unused_count, 1);
  EXPECT_EQ(disk_cleanup_manager_->internally_used_count, 0);
  EXPECT_EQ(disk_cleanup_manager_->internally_unused_count, 0);
  disk_cleanup_manager_->ResetCounters();

  // Terminate the internal request. We should now see the internally unused notification.
  storage_->CallIsSyncedCallback(storage_page_id);
  RunLoopUntilIdle();

  EXPECT_EQ(disk_cleanup_manager_->externally_used_count, 0);
  EXPECT_EQ(disk_cleanup_manager_->externally_unused_count, 0);
  EXPECT_EQ(disk_cleanup_manager_->internally_used_count, 0);
  EXPECT_EQ(disk_cleanup_manager_->internally_unused_count, 1);
}

TEST_F(PageManagerTest, DeletePageStorageWhenPageOpenFails) {
  bool get_page_callback_called;
  Status get_page_status;
  PagePtr page;
  bool called;

  page_manager_->GetPage(LedgerImpl::Delegate::PageState::NAMED, page.NewRequest(),
                         Capture(SetWhenCalled(&get_page_callback_called), &get_page_status));
  RunLoopUntilIdle();
  EXPECT_TRUE(get_page_callback_called);
  EXPECT_EQ(get_page_status, Status::OK);

  // Try to delete the page while it is open. Expect to get an error.
  Status storage_status;
  page_manager_->DeletePageStorage(Capture(SetWhenCalled(&called), &storage_status));
  RunLoopUntilIdle();
  EXPECT_TRUE(called);
  EXPECT_EQ(storage_status, Status::ILLEGAL_STATE);
}

// Verify that the PageManager opens a closed page and triggers the synchronization with the cloud
// for it.
TEST_F(PageManagerTest, StartPageSyncCheckSyncCalled) {
  bool get_page_callback_called;
  Status get_page_status;

  storage_->should_get_page_fail = false;
  PagePtr page;
  storage::PageId storage_page_id = convert::ToString(page_id_.id);

  // Opens the page and starts the sync with the cloud for the first time.
  page_manager_->GetPage(LedgerImpl::Delegate::PageState::NAMED, page.NewRequest(),
                         Capture(SetWhenCalled(&get_page_callback_called), &get_page_status));
  RunLoopUntilIdle();
  EXPECT_TRUE(get_page_callback_called);
  EXPECT_EQ(get_page_status, Status::OK);
  EXPECT_EQ(sync_->GetSyncCallsCount(storage_page_id), 1);

  page.Unbind();
  RunLoopUntilIdle();

  // Reopens closed page and starts the sync.
  page_manager_->StartPageSync();
  RunLoopUntilIdle();

  EXPECT_EQ(sync_->GetSyncCallsCount(storage_page_id), 2);
}

// Verify that the PageManager does not trigger the synchronization with the cloud for the currently
// opened page.
TEST_F(PageManagerTest, StartPageSyncCheckWithOpenedPage) {
  bool get_page_callback_called;
  Status get_page_status;

  storage_->should_get_page_fail = false;
  PagePtr page;
  storage::PageId storage_page_id = convert::ToString(page_id_.id);

  // Opens the page and starts the sync with the cloud for the first time.
  page_manager_->GetPage(LedgerImpl::Delegate::PageState::NAMED, page.NewRequest(),
                         Capture(SetWhenCalled(&get_page_callback_called), &get_page_status));
  RunLoopUntilIdle();
  EXPECT_TRUE(get_page_callback_called);
  EXPECT_EQ(get_page_status, Status::OK);
  EXPECT_EQ(sync_->GetSyncCallsCount(storage_page_id), 1);

  // Tries to reopen the already-opened page to start the sync.
  page_manager_->StartPageSync();
  RunLoopUntilIdle();

  EXPECT_EQ(sync_->GetSyncCallsCount(storage_page_id), 1);
}

}  // namespace
}  // namespace ledger
