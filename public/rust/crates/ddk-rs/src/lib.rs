// Copyright 2017 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

extern crate ddk_sys;
extern crate fuchsia_zircon as zircon;
extern crate fuchsia_zircon_sys as zircon_sys;

use std::ffi::{CStr, CString};
use zircon::Status;
use zircon_sys as sys;
use std::slice;
use std::os::raw::c_char;

pub struct Driver {
    driver: *mut ddk_sys::zx_driver_t,
}

impl Driver {
    pub fn wrap(driver: *mut ddk_sys::zx_driver_t) -> Driver {
        Driver { driver: driver }
    }
}

pub struct Device {
    device: *mut ddk_sys::zx_device_t,
}

impl Device {
    pub fn wrap(device: *mut ddk_sys::zx_device_t) -> Device {
        Device { device: device }
    }
}

pub trait DeviceOps {
    fn name(&self) -> String;

    fn open(&mut self, _flags: u32) -> Result<Option<Device>, Status> {
        Ok(None)
    }

    fn open_at(&mut self, _path: &str, _flags: u32) -> Result<Option<Device>, Status> {
        Err(Status::ErrNotSupported)
    }

    fn close(&mut self, _flags: u32) -> Status {
        Status::NoError
    }

    fn unbind(&mut self) -> Status {
        Status::NoError
    }

    fn release(&mut self) -> Status {
        Status::ErrNotSupported
    }

    fn read(&mut self, _buf: &mut [u8], _offset: u64) -> Result<usize, Status> {
        Err(Status::ErrNotSupported)
    }

    fn write(&mut self, _buf: &[u8], _offset: u64) -> Result<usize, Status> {
        Err(Status::ErrNotSupported)
    }

}

pub type AddDeviceFlags = ddk_sys::device_add_flags_t;

pub fn add_device(device_ops: Box<DeviceOps>, flags: AddDeviceFlags) -> Result<Device, Status> {
    let device_name = device_ops.name();
    if device_name.len() > ddk_sys::ZX_DEVICE_NAME_MAX {
        return Err(Status::ErrInvalidArgs)
    }

    let mut device_add_args: ddk_sys::device_add_args_t = ddk_sys::device_add_args_t::new();
    // TODO(stange): See if it's necessary to double Box device_ops.
    device_add_args.ctx = Box::into_raw(Box::new(device_ops)) as *mut u8;
    device_add_args.name = CString::new(device_name).unwrap().as_ptr();
    device_add_args.flags = flags;
    unsafe {
        device_add_args.ops = &mut DEVICE_OPS;
        let mut ddk_device: *mut ddk_sys::zx_device_t = std::ptr::null_mut();
        // TODO(stange): Look into passing in parent or getting it from somewhere.
        let ret = ddk_sys::device_add(std::ptr::null_mut(), &mut device_add_args, &mut ddk_device);
        match ret {
            sys::ZX_OK => Ok(Device::wrap(ddk_device)),
            _ => Err(Status::from_raw(ret)),
        }
    }
}

// Should be called after unbind() has been called on the device and the driver is
// ready remove the device.
pub fn remove_device(device: Device) -> Status {
    unsafe {
        Status::from_raw(ddk_sys::device_remove(device.device))
    }
}

extern fn ddk_get_protocol(ctx: *mut u8, proto_id: u32, protocol: *mut u8) -> sys::zx_status_t {
    sys::ZX_ERR_NOT_SUPPORTED
}

fn open_result(ret: Result<Option<Device>, Status>, dev_out: *mut *mut ddk_sys::zx_device_t) -> sys::zx_status_t {
    match ret {
      Err(status) => status as sys::zx_status_t,
      Ok(None) => sys::ZX_OK,
      Ok(Some(new_device)) => {
        // TODO(qwandor): This assumes that the implementor of DeviceOps.open has already called
        // add_device with the DEVICE_ADD_INSTANCE flag. Would it be better to call it here instead?
        unsafe { *dev_out = new_device.device };
        sys::ZX_OK
      }
    }
}

extern fn ddk_open(ctx: *mut u8, dev_out: *mut *mut ddk_sys::zx_device_t, flags: u32) -> sys::zx_status_t {
    let mut device: Box<Box<DeviceOps>> = unsafe { Box::from_raw(ctx as *mut Box<DeviceOps>) };
    let ret = device.open(flags);
    let _ = Box::into_raw(device);
    open_result(ret, dev_out)
}

extern fn ddk_open_at(ctx: *mut u8, dev_out: *mut *mut ddk_sys::zx_device_t, path: *const c_char, flags: u32) -> sys::zx_status_t {
    let mut device: Box<Box<DeviceOps>> = unsafe { Box::from_raw(ctx as *mut Box<DeviceOps>) };
    let path = unsafe { CStr::from_ptr(path).to_str().unwrap() };
    let ret = device.open_at(path, flags);
    let _ = Box::into_raw(device);
    open_result(ret, dev_out)
}

extern fn ddk_close(ctx: *mut u8, flags: u32) -> sys::zx_status_t {
    let mut device: Box<Box<DeviceOps>> = unsafe { Box::from_raw(ctx as *mut Box<DeviceOps>) };
    let ret = device.close(flags);
    let _ = Box::into_raw(device);
    ret as sys::zx_status_t
}

extern fn ddk_unbind(ctx: *mut u8) {
    let mut device: Box<Box<DeviceOps>> = unsafe { Box::from_raw(ctx as *mut Box<DeviceOps>) };
    device.unbind();
    let _ = Box::into_raw(device);
}

extern fn ddk_release(ctx: *mut u8) {
    let mut device: Box<Box<DeviceOps>> = unsafe { Box::from_raw(ctx as *mut Box<DeviceOps>) };
    device.release();
    // Don't Box::into_raw the device so it can be freed by Rust
}

extern fn ddk_read(ctx: *mut u8, buf: *mut u8, count: usize, off: sys::zx_off_t, actual: *mut usize) -> sys::zx_status_t {
    let mut device: Box<Box<DeviceOps>> = unsafe { Box::from_raw(ctx as *mut Box<DeviceOps>) };
    let rust_buf = unsafe { slice::from_raw_parts_mut(buf, count) };
    let ret = match device.read(rust_buf, off) {
        Ok(r) => {
                unsafe { *actual = r };
                sys::ZX_OK
            },
        // TODO(qwandor): Investigate adding a to_raw method for Status.
        Err(status) => status as sys::zx_status_t
    };
    let _ = Box::into_raw(device);
    ret
}

extern fn ddk_write(ctx: *mut u8, buf: *const u8, count: usize, off: sys::zx_off_t, actual: *mut usize) -> sys::zx_status_t {
    let mut device: Box<Box<DeviceOps>> = unsafe { Box::from_raw(ctx as *mut Box<DeviceOps>) };
    let rust_buf = unsafe { slice::from_raw_parts(buf, count) };
    let ret = match device.write(rust_buf, off) {
        Ok(r) => {
                unsafe { *actual = r };
                sys::ZX_OK
            },
        Err(status) => status as sys::zx_status_t
    };
    let _ = Box::into_raw(device);
    ret
}

extern fn ddk_iotxn_queue(ctx: *mut u8, txn: *mut ddk_sys::iotxn_t) {
}

extern fn ddk_get_size(ctx: *mut u8) -> u64 {
    0
}

extern fn ddk_ioctl(ctx: *mut u8, op: u32, in_buf: *const u8, in_len: usize, out_buf: *mut u8, out_len: usize, out_actual: *mut usize) -> sys::zx_status_t {
    sys::ZX_ERR_NOT_SUPPORTED
}

extern fn ddk_suspend(ctx: *mut u8, flags: u32) -> sys::zx_status_t {
    sys::ZX_ERR_NOT_SUPPORTED
}

extern fn ddk_resume(ctx: *mut u8, flags: u32) -> sys::zx_status_t {
    sys::ZX_ERR_NOT_SUPPORTED
}

static mut DEVICE_OPS: ddk_sys::zx_protocol_device_t = ddk_sys::zx_protocol_device_t {
        version: ddk_sys::DEVICE_OPS_VERSION,
        get_protocol: Some(ddk_get_protocol),
        open: Some(ddk_open),
        open_at: Some(ddk_open_at),
        close: Some(ddk_close),
        unbind: Some(ddk_unbind),
        release: Some(ddk_release),
        read: Some(ddk_read),
        write: Some(ddk_write),
        iotxn_queue: Some(ddk_iotxn_queue),
        get_size: Some(ddk_get_size),
        ioctl: Some(ddk_ioctl),
        suspend: Some(ddk_suspend),
        resume: Some(ddk_resume),
};

#[cfg(test)]
mod tests {
    #[test]
    fn it_works() {
    }
}
