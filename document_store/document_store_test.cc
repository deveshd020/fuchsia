// Copyright 2016 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <mojo/system/main.h>

#include <sstream>
#include <string>
#include <utility>

#include "apps/ledger/api/ledger.mojom-sync.h"
#include "apps/maxwell/document_store/interfaces/document_store.mojom-sync.h"
#include "mojo/public/cpp/application/application_impl_base.h"
#include "mojo/public/cpp/application/connect.h"
#include "mojo/public/cpp/application/run_application.h"
#include "mojo/public/cpp/application/service_provider_impl.h"
#include "mojo/public/cpp/bindings/binding_set.h"
#include "mojo/public/cpp/bindings/interface_ptr_set.h"
#include "mojo/public/cpp/bindings/strong_binding.h"
#include "mojo/public/cpp/bindings/synchronous_interface_ptr.h"

namespace {

using namespace document_store;

std::string bytestohex(const mojo::Array<uint8_t>& arr) {
  std::stringstream ss;
  for (size_t i = 0; i < arr.size(); ++i) {
    ss << std::hex << static_cast<int>(arr[i]);
  }
  return ss.str();
}

// TODO(azani): Make more generally available.
std::string statustostr(const Status status) {
  std::string status_str;
  switch (status) {
    case Status::OK:
      status_str = "OK";
      break;
    case Status::PAGE_NOT_FOUND:
      status_str = "PAGE_NOT_FOUND";
      break;
    case Status::DOCUMENT_NOT_FOUND:
      status_str = "DOCUMENT_NOT_FOUND";
      break;
    case Status::DOCUMENT_ALREADY_EXISTS:
      status_str = "DOCUMENT_ALREADY_EXISTS";
      break;
    case Status::TRANSACTION_ALREADY_IN_PROGRESS:
      status_str = "TRANSACTION_ALREADY_IN_PROGRESS";
      break;
    case Status::UNKNOWN_ERROR:
      status_str = "UNKNOWN_ERROR";
      break;
  };
  return status_str;
}

class DocumentStoreTest : public mojo::ApplicationImplBase {
  void OnInitialize() override {
    // TODO(azani): Make sure the data is not persisted unnecessarily across
    // runs.
    mojo::SynchronousInterfacePtr<ledger::LedgerFactory> ledger_factory;
    ConnectToService(shell(), "mojo:ledger",
                     GetSynchronousProxy(&ledger_factory));
    MOJO_LOG(INFO) << "Connected to " << ledger::LedgerFactory::Name_;
    MOJO_CHECK(ledger_factory.is_bound());

    ledger::IdentityPtr id(ledger::Identity::New());
    // Currently, any user_id is valid as long as it's not the size-0 array.
    id->user_id = mojo::Array<uint8_t>::New(1);

    ledger::Status ledger_status;
    mojo::InterfaceHandle<ledger::Ledger> ledger;

    MOJO_CHECK(
        ledger_factory->GetLedger(std::move(id), &ledger_status, &ledger));
    MOJO_LOG(INFO) << "Got a ledger with status: " << ledger_status;
    MOJO_CHECK(ledger.is_valid());
    MOJO_CHECK(ledger_status == ledger::Status::OK);

    mojo::SynchronousInterfacePtr<DocumentStoreFactory> docstore_factory;
    ConnectToService(shell(), "mojo:document_store",
                     GetSynchronousProxy(&docstore_factory));
    MOJO_LOG(INFO) << "Connected to mojo:document_store";
    MOJO_CHECK(docstore_factory.is_bound());

    MOJO_CHECK(docstore_factory->Initialize(std::move(ledger)));
    MOJO_LOG(INFO) << "Sending ledger over!";

    document_store::Status docstore_status;
    mojo::InterfaceHandle<DocumentStore> docstore_handle;
    docstore_factory->NewDocumentStore(&docstore_status, &docstore_handle);
    MOJO_LOG(INFO) << "NewDocumentStore return status "
                   << statustostr(docstore_status);
    MOJO_CHECK(docstore_handle.is_valid());
    MOJO_CHECK(docstore_status == Status::OK);

    mojo::SynchronousInterfacePtr<DocumentStore> docstore;
    docstore = mojo::SynchronousInterfacePtr<DocumentStore>::Create(
        std::move(docstore_handle));
    auto page_id = mojo::Array<uint8_t>::New(16);
    docstore->GetId(&page_id);
    MOJO_LOG(INFO) << "DocumentStore Page ID " << bytestohex(page_id);

    mojo::InterfaceHandle<DocumentStore> docstore_handle2;
    MOJO_CHECK(docstore_factory->GetDocumentStore(
        page_id.Clone(), &docstore_status, &docstore_handle2));
    MOJO_LOG(INFO) << "GetDocumentStore status "
                   << statustostr(docstore_status);

    MOJO_CHECK(docstore_factory->DeleteDocumentStore(page_id.Clone(),
                                                     &docstore_status));
    MOJO_LOG(INFO) << "DeleteDocumentStore status "
                   << statustostr(docstore_status);
    // TODO(azani): Figure out how to check that the docstore message pipe is
    // closed.

    docstore_factory->GetDocumentStore(page_id.Clone(), &docstore_status,
                                       &docstore_handle2);
    MOJO_LOG(INFO) << "GetDocumentStore status  "
                   << statustostr(docstore_status);
    MOJO_CHECK(docstore_status == Status::PAGE_NOT_FOUND);
    MOJO_CHECK(!docstore_handle2.is_valid());
  }
};

}  // namespace

MojoResult MojoMain(MojoHandle request) {
  MOJO_LOG(INFO) << "document_store_test";
  DocumentStoreTest app;
  return mojo::RunApplication(request, &app);
}
