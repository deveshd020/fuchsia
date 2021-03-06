// Copyright 2019 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SRC_DEVELOPER_FEEDBACK_UTILS_COBALT_H_
#define SRC_DEVELOPER_FEEDBACK_UTILS_COBALT_H_

#include <fuchsia/cobalt/cpp/fidl.h>
#include <lib/fit/function.h>
#include <lib/sys/cpp/service_directory.h>
#include <lib/zx/time.h>

#include <map>
#include <memory>
#include <set>
#include <utility>

#include "src/developer/feedback/utils/cobalt_event.h"
#include "src/lib/backoff/exponential_backoff.h"
#include "src/lib/fxl/functional/cancelable_callback.h"

namespace feedback {

// Log events to cobalt.
class Cobalt {
 public:
  // We expect fuchsia.cobalt.LoggerFactory to be in |services|.
  Cobalt(async_dispatcher_t* dispatcher, std::shared_ptr<sys::ServiceDirectory> services);

  // Log an occurrence event with fuchsia.cobalt.Logger with the provided parameters. If the service
  // is not accessible, keep the parameters to try again later.
  template <typename EventCodeType>
  void LogOccurrence(EventCodeType event_code) {
    LogEvent(CobaltEvent(event_code));
  }

  // Log a count event with fuchsia.cobalt.Logger with the provided parameters. If the service is
  // not accessible, keep the parameters to try again later.
  template <typename EventCodeType>
  void LogCount(EventCodeType event_code, uint64_t count) {
    LogEvent(CobaltEvent(event_code, count));
  }

 private:
  void ConnectToLogger(fidl::InterfaceRequest<fuchsia::cobalt::Logger> logger_request);
  void RetryConnectingToLogger();
  void LogEvent(CobaltEvent event);
  void SendEvent(uint64_t event_id);
  void SendAllPendingEvents();

  async_dispatcher_t* dispatcher_;
  std::shared_ptr<sys::ServiceDirectory> services_;

  fuchsia::cobalt::LoggerFactoryPtr logger_factory_;
  fuchsia::cobalt::LoggerPtr logger_;

  // An event is pending if it has been written into a channel, but has not been acknowledged by
  // the recipient.
  std::map<uint64_t, CobaltEvent> pending_events_;
  backoff::ExponentialBackoff logger_reconnection_backoff_;

  // We need to be able to cancel a posted reconnection task when |Cobalt| is destroyed.
  fxl::CancelableClosure reconnect_task_;

  uint64_t next_event_id_ = 0;
};

}  // namespace feedback

#endif  // SRC_DEVELOPER_FEEDBACK_UTILS_COBALT_H_
