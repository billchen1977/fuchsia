// Copyright 2020 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
use std::time::Duration;

pub(crate) const DAEMON: &str = "daemon";

pub(crate) const MDNS_BROADCAST_INTERVAL_SECS: u64 = 20;

// How many seconds to give before dropping an MDNS target and marking it
// as disconnected.
pub(crate) const MDNS_TARGET_DROP_GRACE_PERIOD_SECS: u64 = 5;

pub(crate) const FASTBOOT_CHECK_INTERVAL_SECS: u64 = 3;

pub(crate) const FASTBOOT_DROP_GRACE_PERIOD_SECS: u64 = 2;

pub(crate) const DEFAULT_MAX_RETRY_COUNT: u64 = 30;

// Delay between retry attempts to find the RCS.
pub(crate) const RETRY_DELAY: Duration = Duration::from_millis(200);

// Config keys
pub(crate) const SSH_PRIV: &str = "ssh.priv";
pub(crate) const SSH_PORT: &str = "ssh.port";
pub(crate) const OVERNET_MAX_RETRY_COUNT: &str = "overnet.max_retry_count";

pub const LOG_FILE_PREFIX: &str = "ffx.daemon";

#[cfg(not(test))]
pub async fn get_socket() -> String {
    const OVERNET_SOCKET: &str = "overnet.socket";
    const DEFAULT_SOCKET: &str = "/tmp/ascendd";
    ffx_config::get(OVERNET_SOCKET).await.unwrap_or(DEFAULT_SOCKET.to_string())
}

#[cfg(test)]
pub async fn get_socket() -> String {
    std::thread_local! {
        static DEFAULT_SOCKET: String = {
            tempfile::Builder::new()
                .prefix("ascendd_for_test")
                .suffix(".sock")
                .tempfile().unwrap()
                .path()
                .file_name().and_then(std::ffi::OsStr::to_str).unwrap().to_string()
        };
    }

    DEFAULT_SOCKET.with(|k| k.clone())
}

pub(crate) const CURRENT_EXE_HASH: &str = "current.hash";
