// Copyright 2020 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

use crate::log_if_err;
use crate::message::{Message, MessageReturn};
use crate::node::Node;
use crate::utils::connect_proxy;
use anyhow::{format_err, Error};
use async_trait::async_trait;
use fidl_fuchsia_device as fdev;
use fuchsia_inspect::{self as inspect, NumericProperty, Property};
use fuchsia_syslog::fx_log_err;
use fuchsia_zircon as zx;
use std::rc::Rc;

/// Node: DeviceControlHandler
///
/// Summary: Provides an interface to control the power, performance, and sleep states of a devhost
///          device
///
/// Handles Messages:
///     - GetPerformanceState
///     - SetPerformanceState
///
/// Sends Messages: N/A
///
/// FIDL dependencies:
///     - fuchsia.device.Controller: the node uses this protocol to control the power, performance,
///       and sleep states of a devhost device

pub const MAX_PERF_STATES: u32 = fdev::MAX_DEVICE_PERFORMANCE_STATES;

/// A builder for constructing the DeviceControlhandler node
pub struct DeviceControlHandlerBuilder<'a> {
    driver_path: String,
    driver_proxy: Option<fdev::ControllerProxy>,
    inspect_root: Option<&'a inspect::Node>,
}

impl<'a> DeviceControlHandlerBuilder<'a> {
    pub fn new_with_driver_path(driver_path: String) -> Self {
        Self { driver_path, driver_proxy: None, inspect_root: None }
    }

    #[cfg(test)]
    pub fn new_with_proxy(driver_path: String, proxy: fdev::ControllerProxy) -> Self {
        Self { driver_path, driver_proxy: Some(proxy), inspect_root: None }
    }

    #[cfg(test)]
    pub fn with_inspect_root(mut self, root: &'a inspect::Node) -> Self {
        self.inspect_root = Some(root);
        self
    }

    pub fn build(self) -> Result<Rc<DeviceControlHandler>, Error> {
        // Optionally use the default proxy
        let proxy = if self.driver_proxy.is_none() {
            connect_proxy::<fdev::ControllerMarker>(&self.driver_path)?
        } else {
            self.driver_proxy.unwrap()
        };

        // Optionally use the default inspect root node
        let inspect_root = self.inspect_root.unwrap_or(inspect::component::inspector().root());

        Ok(Rc::new(DeviceControlHandler {
            driver_path: self.driver_path.clone(),
            driver_proxy: proxy,
            inspect: InspectData::new(
                inspect_root,
                format!("DeviceControlHandler ({})", self.driver_path),
            ),
        }))
    }
}

pub struct DeviceControlHandler {
    driver_path: String,
    driver_proxy: fdev::ControllerProxy,

    /// A struct for managing Component Inspection data
    inspect: InspectData,
}

impl DeviceControlHandler {
    async fn handle_get_performance_state(&self) -> Result<MessageReturn, Error> {
        fuchsia_trace::duration!(
            "power_manager",
            "DeviceControlHandler::handle_get_performance_state",
            "driver" => self.driver_path.as_str()
        );
        // TODO(fxb/43744): The controller API doesn't exist yet (fxb/43743)
        Ok(MessageReturn::GetPerformanceState(0))
    }

    async fn handle_set_performance_state(&self, in_state: u32) -> Result<MessageReturn, Error> {
        fuchsia_trace::duration!(
            "power_manager",
            "DeviceControlHandler::handle_set_performance_state",
            "driver" => self.driver_path.as_str(),
            "state" => in_state
        );

        let result = self.set_performance_state(in_state).await;
        log_if_err!(result, "Failed to set performance state");
        fuchsia_trace::instant!(
            "power_manager",
            "DeviceControlHandler::set_performance_state_result",
            fuchsia_trace::Scope::Thread,
            "driver" => self.driver_path.as_str(),
            "result" => format!("{:?}", result).as_str()
        );

        match result.as_ref() {
            Ok(_) => self.inspect.perf_state.set(in_state.into()),
            Err(e) => {
                self.inspect.set_performance_state_errors.add(1);
                self.inspect.last_set_performance_state_error.set(format!("{}", e).as_str())
            }
        }

        result
    }

    async fn set_performance_state(&self, in_state: u32) -> Result<MessageReturn, Error> {
        // Make the FIDL call
        let (status, out_state) =
            self.driver_proxy.set_performance_state(in_state).await.map_err(|e| {
                format_err!(
                    "{} ({}): set_performance_state IPC failed: {}",
                    self.name(),
                    self.driver_path,
                    e
                )
            })?;

        // Check the status code
        zx::Status::ok(status).map_err(|e| {
            format_err!(
                "{} ({}): set_performance_state driver returned error: {}",
                self.name(),
                self.driver_path,
                e
            )
        })?;

        // On success, in_state will equal out_state
        if in_state == out_state {
            Ok(MessageReturn::SetPerformanceState)
        } else {
            Err(format_err!(
                "{} ({}): expected in_state == out_state (in_state={}; out_state={})",
                self.name(),
                self.driver_path,
                in_state,
                out_state
            ))
        }
    }
}

#[async_trait(?Send)]
impl Node for DeviceControlHandler {
    fn name(&self) -> &'static str {
        "DeviceControlHandler"
    }

    async fn handle_message(&self, msg: &Message) -> Result<MessageReturn, Error> {
        match msg {
            Message::GetPerformanceState => self.handle_get_performance_state().await,
            Message::SetPerformanceState(state) => self.handle_set_performance_state(*state).await,
            _ => Err(format_err!("Unsupported message: {:?}", msg)),
        }
    }
}

struct InspectData {
    perf_state: inspect::UintProperty,
    set_performance_state_errors: inspect::UintProperty,
    last_set_performance_state_error: inspect::StringProperty,
}

impl InspectData {
    fn new(parent: &inspect::Node, name: String) -> Self {
        // Create a local root node and properties
        let root = parent.create_child(name);
        let perf_state = root.create_uint("performance_state", 0);
        let set_performance_state_errors = root.create_uint("set_performance_state_errors", 0);
        let last_set_performance_state_error =
            root.create_string("last_set_performance_state_error", "");

        // Pass ownership of the new node to the parent node, otherwise it'll be dropped
        parent.record(root);

        InspectData { perf_state, set_performance_state_errors, last_set_performance_state_error }
    }
}

#[cfg(test)]
pub mod tests {
    use super::*;
    use fuchsia_async as fasync;
    use futures::TryStreamExt;
    use inspect::assert_inspect_tree;
    use std::cell::Cell;

    fn setup_fake_driver(
        mut set_performance_state: impl FnMut(u32) + 'static,
    ) -> fdev::ControllerProxy {
        let (proxy, mut stream) =
            fidl::endpoints::create_proxy_and_stream::<fdev::ControllerMarker>().unwrap();
        fasync::spawn_local(async move {
            while let Ok(req) = stream.try_next().await {
                match req {
                    Some(fdev::ControllerRequest::SetPerformanceState {
                        requested_state,
                        responder,
                    }) => {
                        set_performance_state(requested_state as u32);
                        let _ = responder.send(zx::Status::OK.into_raw(), requested_state);
                    }
                    _ => assert!(false),
                }
            }
        });

        proxy
    }

    pub fn setup_test_node(
        set_performance_state: impl FnMut(u32) + 'static,
    ) -> Rc<DeviceControlHandler> {
        DeviceControlHandlerBuilder::new_with_proxy(
            "Fake".to_string(),
            setup_fake_driver(set_performance_state),
        )
        .build()
        .unwrap()
    }

    #[fasync::run_singlethreaded(test)]
    async fn test_set_performance_state() {
        let recvd_perf_state = Rc::new(Cell::new(0));
        let recvd_perf_state_clone = recvd_perf_state.clone();
        let set_performance_state = move |state| {
            recvd_perf_state_clone.set(state);
        };
        let node = setup_test_node(set_performance_state);

        let new_perf_state = 1;
        match node.handle_message(&Message::SetPerformanceState(new_perf_state)).await.unwrap() {
            MessageReturn::SetPerformanceState => {}
            _ => panic!(),
        }
        assert_eq!(recvd_perf_state.get(), new_perf_state);

        let new_perf_state = 2;
        match node.handle_message(&Message::SetPerformanceState(new_perf_state)).await.unwrap() {
            MessageReturn::SetPerformanceState => {}
            _ => panic!(),
        }
        assert_eq!(recvd_perf_state.get(), new_perf_state);
    }

    /// Tests for the presence and correctness of dynamically-added inspect data
    #[fasync::run_singlethreaded(test)]
    async fn test_inspect_data() {
        let inspector = inspect::Inspector::new();
        let _node = DeviceControlHandlerBuilder::new_with_proxy(
            "Fake".to_string(),
            fidl::endpoints::create_proxy::<fdev::ControllerMarker>().unwrap().0,
        )
        .with_inspect_root(inspector.root())
        .build()
        .unwrap();

        assert_inspect_tree!(
            inspector,
            root: {
                "DeviceControlHandler (Fake)": contains {}
            }
        );
    }
}
