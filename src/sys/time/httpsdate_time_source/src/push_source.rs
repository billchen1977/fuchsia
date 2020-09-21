// Copyright 2020 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

//! The `push_source` mod defines an implementation of the `PushSource` API and traits to hook in
//! an algorithm that produces time updates. In the future this module may be moved to a common
//! library.

use anyhow::Error;
use async_trait::async_trait;
use fidl_fuchsia_time_external::{
    Properties, PushSourceRequest, PushSourceRequestStream, PushSourceWatchSampleResponder,
    PushSourceWatchStatusResponder, Status, TimeSample,
};
use fuchsia_zircon as zx;
use futures::{
    channel::mpsc::{channel, Sender},
    lock::Mutex,
    StreamExt, TryStreamExt,
};
use log::warn;
use std::sync::{Arc, Weak};
use watch_handler::{Sender as WatchSender, WatchHandler};

/// A time update generated by an |UpdateAlgorithm|.
#[derive(Clone, PartialEq, Debug)]
pub enum Update {
    /// A new TimeSample. The Arc may be removed once fidl tables support clone.
    Sample(Arc<TimeSample>),
    /// A new Status.
    Status(Status),
}

impl From<Status> for Update {
    fn from(status: Status) -> Self {
        Update::Status(status)
    }
}

impl From<TimeSample> for Update {
    fn from(sample: TimeSample) -> Self {
        Update::Sample(Arc::new(sample))
    }
}

#[cfg(test)]
impl Update {
    /// Returns true iff the update contained is a status.
    pub fn is_status(&self) -> bool {
        match self {
            Update::Sample(_) => false,
            Update::Status(_) => true,
        }
    }
}

/// An |UpdateAlgorithm| asynchronously produces Updates.
#[async_trait]
pub trait UpdateAlgorithm {
    /// Update the algorithm's knowledge of device properties.
    async fn update_device_properties(&self, properties: Properties);

    /// Generate updates asynchronously and push them to |sink|. This method may run
    /// indefinitely. This method may generate duplicate updates.
    // TODO(satsukiu) - use a generator library instead once one is available
    async fn generate_updates(&self, sink: Sender<Update>) -> Result<(), Error>;
}

/// An implementation of |fuchsia.time.external.PushSource| that routes time updates from an
/// |UpdateAlgorithm| to clients of the fidl protocol and routes device property updates from fidl
/// clients to the |UpdateAlgorithm|.
pub struct PushSource<UA: UpdateAlgorithm> {
    /// Internal state of the push source.
    internal: Mutex<PushSourceInternal>,
    /// The algorithm used to obtain new updates.
    update_algorithm: UA,
}

impl<UA: UpdateAlgorithm> PushSource<UA> {
    /// Create a new |PushSource| that polls |update_algorithm| for time updates and starts in the
    /// |initial_status| status.
    pub fn new(update_algorithm: UA, initial_status: Status) -> Self {
        Self { internal: Mutex::new(PushSourceInternal::new(initial_status)), update_algorithm }
    }

    /// Polls updates received on |update_algorithm| and pushes them to bound clients.
    pub async fn poll_updates(&self) -> Result<(), Error> {
        // Updates should be processed immediately so add no extra buffer space.
        let (sender, mut receiver) = channel(0);
        let updater_fut = self.update_algorithm.generate_updates(sender);
        let consumer_fut = async move {
            while let Some(update) = receiver.next().await {
                self.internal.lock().await.push_update(update).await;
            }
        };
        let (update_res, _) = futures::future::join(updater_fut, consumer_fut).await;
        update_res
    }

    /// Handle a single client's requests received on the given |request_stream|.
    pub async fn handle_requests_for_stream(
        &self,
        mut request_stream: PushSourceRequestStream,
    ) -> Result<(), Error> {
        let client_context = self.internal.lock().await.register_client();
        while let Some(request) = request_stream.try_next().await? {
            client_context.lock().await.handle_request(request, &self.update_algorithm).await?;
        }
        Ok(())
    }
}

/// Contains internal state for |PushSource| that must be updated atomically.
struct PushSourceInternal {
    /// A set of weak pointers to registered clients.
    clients: Vec<Weak<Mutex<PushSourceClientHandler>>>,
    /// The last known sample.
    latest_sample: Option<Arc<TimeSample>>,
    /// The last known status.
    latest_status: Status,
}

impl PushSourceInternal {
    /// Create a new |PushSourceInternal|.
    pub fn new(initial_status: Status) -> Self {
        PushSourceInternal { clients: vec![], latest_sample: None, latest_status: initial_status }
    }

    /// Create a new client handler registered to receive asynchonous updates
    /// for the duration of its lifetime.
    pub fn register_client(&mut self) -> Arc<Mutex<PushSourceClientHandler>> {
        let client = Arc::new(Mutex::new(PushSourceClientHandler {
            sample_watcher: WatchHandler::create(self.latest_sample.clone()),
            status_watcher: WatchHandler::create(Some(self.latest_status)),
        }));
        self.clients.push(Arc::downgrade(&client));
        client
    }

    /// Push a new update to all existing clients.
    pub async fn push_update(&mut self, update: Update) {
        match &update {
            Update::Sample(sample) => self.latest_sample = Some(Arc::clone(&sample)),
            Update::Status(status) => self.latest_status = *status,
        }
        // Discard any references to clients that no longer exist.
        let mut client_arcs = vec![];
        self.clients.retain(|client_weak| match client_weak.upgrade() {
            Some(client_arc) => {
                client_arcs.push(client_arc);
                true
            }
            None => false,
        });
        for client in client_arcs {
            client.lock().await.handle_update(update.clone());
        }
    }
}

/// Per-client state for handling requests.
struct PushSourceClientHandler {
    /// Watcher for parking `WatchSample` requests.
    sample_watcher: WatchHandler<Arc<TimeSample>, WatchSampleResponder>,
    /// Watcher for parking `WatchStatus` requests.
    status_watcher: WatchHandler<Status, WatchStatusResponder>,
}

impl PushSourceClientHandler {
    /// Handle a fidl request received from the client.
    async fn handle_request(
        &mut self,
        request: PushSourceRequest,
        update_algorithm: &impl UpdateAlgorithm,
    ) -> Result<(), Error> {
        match request {
            PushSourceRequest::WatchSample { responder } => {
                self.sample_watcher.watch(WatchSampleResponder(responder)).map_err(|e| {
                    e.responder.0.control_handle().shutdown_with_epitaph(zx::Status::BAD_STATE);
                    e
                })?;
            }
            PushSourceRequest::WatchStatus { responder } => {
                self.status_watcher.watch(WatchStatusResponder(responder)).map_err(|e| {
                    e.responder.0.control_handle().shutdown_with_epitaph(zx::Status::BAD_STATE);
                    e
                })?;
            }
            PushSourceRequest::UpdateDeviceProperties { properties, .. } => {
                update_algorithm.update_device_properties(properties).await;
            }
        }
        Ok(())
    }

    /// Push an internal update to any hanging gets parked by the client.
    fn handle_update(&mut self, update: Update) {
        match update {
            Update::Sample(sample) => self.sample_watcher.set_value(sample),
            Update::Status(status) => self.status_watcher.set_value(status),
        }
    }
}

struct WatchSampleResponder(PushSourceWatchSampleResponder);
struct WatchStatusResponder(PushSourceWatchStatusResponder);

impl WatchSender<Arc<TimeSample>> for WatchSampleResponder {
    fn send_response(self, data: Arc<TimeSample>) {
        let time_sample = TimeSample { utc: data.utc.clone(), monotonic: data.monotonic.clone() };
        self.0.send(time_sample).unwrap_or_else(|e| warn!("Error sending response: {:?}", e));
    }
}

impl WatchSender<Status> for WatchStatusResponder {
    fn send_response(self, data: Status) {
        self.0.send(data).unwrap_or_else(|e| warn!("Error sending response: {:?}", e));
    }
}

#[cfg(test)]
mod test {
    use super::*;
    use fidl::{endpoints::create_proxy_and_stream, Error as FidlError};
    use fidl_fuchsia_time_external::{PushSourceMarker, PushSourceProxy};
    use fuchsia_async as fasync;
    use futures::{channel::mpsc::Receiver, FutureExt, SinkExt};
    use matches::assert_matches;

    /// An UpdateAlgorithm that forwards updates produced by a test.
    struct TestUpdateAlgorithm {
        /// Receiver that accepts updates pushed by a test.
        receiver: Mutex<Option<Receiver<Update>>>,
        /// List of received device property updates
        device_property_updates: Mutex<Vec<Properties>>,
    }

    impl TestUpdateAlgorithm {
        fn new() -> (Self, Sender<Update>) {
            let (sender, receiver) = channel(0);
            (
                TestUpdateAlgorithm {
                    receiver: Mutex::new(Some(receiver)),
                    device_property_updates: Mutex::new(vec![]),
                },
                sender,
            )
        }
    }

    #[async_trait]
    impl UpdateAlgorithm for TestUpdateAlgorithm {
        async fn update_device_properties(&self, properties: Properties) {
            self.device_property_updates.lock().await.push(properties);
        }

        async fn generate_updates(&self, sink: Sender<Update>) -> Result<(), Error> {
            let receiver = self.receiver.lock().await.take().unwrap();
            receiver.map(Ok).forward(sink).await?;
            Ok(())
        }
    }

    struct TestHarness {
        /// The PushSource under test.
        test_source: Arc<PushSource<TestUpdateAlgorithm>>,
        /// Tasks spawned for the test.
        tasks: Vec<fasync::Task<Result<(), Error>>>,
        /// Sender that injects updates to test_source.
        update_sender: Sender<Update>,
    }

    impl TestHarness {
        /// Create a new TestHarness.
        fn new() -> Self {
            let (update_algorithm, update_sender) = TestUpdateAlgorithm::new();
            let test_source = Arc::new(PushSource::new(update_algorithm, Status::Ok));
            let source_clone = Arc::clone(&test_source);
            let update_task = fasync::Task::spawn(async move { source_clone.poll_updates().await });

            TestHarness { test_source, tasks: vec![update_task], update_sender }
        }

        /// Return a new proxy connected to the test PushSource.
        fn new_proxy(&mut self) -> PushSourceProxy {
            let source_clone = Arc::clone(&self.test_source);
            let (proxy, stream) = create_proxy_and_stream::<PushSourceMarker>().unwrap();
            let server_task = fasync::Task::spawn(async move {
                source_clone.handle_requests_for_stream(stream).await
            });
            self.tasks.push(server_task);
            proxy
        }

        /// Push a new update to the PushSource.
        async fn push_update(&mut self, update: Update) {
            self.update_sender.send(update).await.unwrap();
        }

        /// Assert that the TestUpdateAlgorithm received the property updates.
        async fn assert_device_properties(&self, properties: &[Properties]) {
            assert_eq!(
                self.test_source.update_algorithm.device_property_updates.lock().await.as_slice(),
                properties
            );
        }
    }

    // Since we rely on WatchHandler to achieve most of the hanging get behavior, these tests
    // focus primarily on behavior specific to PushSource and ensuring multiple clients are
    // supported.

    #[fasync::run_until_stalled(test)]
    async fn test_watch_sample_closes_on_multiple_watches() {
        let mut harness = TestHarness::new();
        let proxy = harness.new_proxy();

        // Since there aren't any samples yet the first call should hang
        let first_watch_fut = proxy.watch_sample();
        // Calling again while second watch is active should close the channel.
        assert_matches!(
            proxy.watch_sample().await.unwrap_err(),
            FidlError::ClientChannelClosed{status: zx::Status::BAD_STATE, .. }
        );
        assert_matches!(
            first_watch_fut.await.unwrap_err(),
            FidlError::ClientChannelClosed{status: zx::Status::BAD_STATE, .. }
        );
    }

    #[fasync::run_until_stalled(test)]
    async fn test_watch_status_closes_on_multiple_watches() {
        let mut harness = TestHarness::new();
        let proxy = harness.new_proxy();

        // First watch always immediately returns Ok
        assert_eq!(proxy.watch_status().await.unwrap(), Status::Ok);
        // In absence of updates second watch does not finish
        let second_watch_fut = proxy.watch_status();
        // Calling again while second watch is active should close the channel.
        assert_matches!(
            proxy.watch_status().await.unwrap_err(),
            FidlError::ClientChannelClosed{status: zx::Status::BAD_STATE, .. }
        );
        assert_matches!(
            second_watch_fut.await.unwrap_err(),
            FidlError::ClientChannelClosed{status: zx::Status::BAD_STATE, .. }
        );
    }

    #[fasync::run_until_stalled(test)]
    async fn test_watch_sample() {
        let mut harness = TestHarness::new();
        let proxy = harness.new_proxy();

        // The first watch completes only after update is produced.
        let sample_fut = proxy.watch_sample();
        harness
            .push_update(Update::Sample(Arc::new(TimeSample {
                monotonic: Some(23),
                utc: Some(24),
            })))
            .await;
        assert_eq!(sample_fut.await.unwrap(), TimeSample { monotonic: Some(23), utc: Some(24) });

        // Subsequent watches complete only after a new update is produced.
        let sample_fut = proxy.watch_sample();
        harness
            .push_update(Update::Sample(Arc::new(TimeSample {
                monotonic: Some(25),
                utc: Some(26),
            })))
            .await;
        assert_eq!(sample_fut.await.unwrap(), TimeSample { monotonic: Some(25), utc: Some(26) });

        // Watches hangs in absence of new update.
        assert!(proxy.watch_sample().now_or_never().is_none());
    }

    #[fasync::run_until_stalled(test)]
    async fn test_watch_sample_sent_to_all_clients() {
        let mut harness = TestHarness::new();
        let proxy = harness.new_proxy();
        let proxy_2 = harness.new_proxy();

        // The first watch completes only after update is produced.
        let sample_fut = proxy.watch_sample();
        let sample_fut_2 = proxy_2.watch_sample();
        harness
            .push_update(Update::Sample(Arc::new(TimeSample {
                monotonic: Some(23),
                utc: Some(24),
            })))
            .await;
        assert_eq!(sample_fut.await.unwrap(), TimeSample { monotonic: Some(23), utc: Some(24) });
        assert_eq!(sample_fut_2.await.unwrap(), TimeSample { monotonic: Some(23), utc: Some(24) });

        // Subsequent watches complete only after a new update is produced.
        let sample_fut = proxy.watch_sample();
        let sample_fut_2 = proxy_2.watch_sample();
        harness
            .push_update(Update::Sample(Arc::new(TimeSample {
                monotonic: Some(25),
                utc: Some(26),
            })))
            .await;
        assert_eq!(sample_fut.await.unwrap(), TimeSample { monotonic: Some(25), utc: Some(26) });
        assert_eq!(sample_fut_2.await.unwrap(), TimeSample { monotonic: Some(25), utc: Some(26) });

        // A client that connects later gets the latest update.
        let proxy_3 = harness.new_proxy();
        assert_eq!(
            proxy_3.watch_sample().await.unwrap(),
            TimeSample { monotonic: Some(25), utc: Some(26) }
        );
    }

    #[fasync::run_until_stalled(test)]
    async fn test_watch_status() {
        let mut harness = TestHarness::new();
        let proxy = harness.new_proxy();

        // The first watch completes immediately.
        assert_eq!(proxy.watch_status().await.unwrap(), Status::Ok);

        // Subsequent watches complete only after an update is produced.
        let status_fut = proxy.watch_status();
        harness.push_update(Update::Status(Status::Hardware)).await;
        assert_eq!(status_fut.await.unwrap(), Status::Hardware);

        // Watches hang in absence of a new update.
        assert!(proxy.watch_status().now_or_never().is_none());
    }

    #[fasync::run_until_stalled(test)]
    async fn test_watch_status_sent_to_all_clients() {
        let mut harness = TestHarness::new();
        let proxy = harness.new_proxy();
        let proxy_2 = harness.new_proxy();

        // The first watch completes immediately.
        assert_eq!(proxy.watch_status().await.unwrap(), Status::Ok);
        assert_eq!(proxy_2.watch_status().await.unwrap(), Status::Ok);

        // Subsequent watches complete only after an update is produced.
        let status_fut = proxy.watch_status();
        let status_fut_2 = proxy_2.watch_status();
        harness.push_update(Update::Status(Status::Hardware)).await;
        assert_eq!(status_fut.await.unwrap(), Status::Hardware);
        assert_eq!(status_fut_2.await.unwrap(), Status::Hardware);

        // A client that connects later gets the latest update.
        let proxy_3 = harness.new_proxy();
        assert_eq!(proxy_3.watch_status().await.unwrap(), Status::Hardware);
    }

    #[fasync::run_singlethreaded(test)]
    async fn test_property_updates_sent_to_update_algorithm() {
        let mut harness = TestHarness::new();
        let proxy = harness.new_proxy();
        let proxy_2 = harness.new_proxy();

        proxy.update_device_properties(Properties {}).unwrap();
        proxy_2.update_device_properties(Properties {}).unwrap();
        // Sleep here to allow the executor to run the tasks servicing these requests.
        fasync::Timer::new(fasync::Time::after(zx::Duration::from_nanos(1000))).await;
        harness.assert_device_properties(&vec![Properties {}, Properties {}]).await;
    }
}
