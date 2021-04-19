// Copyright 2021 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SRC_GRAPHICS_DISPLAY_DRIVERS_VC4_DISPLAY_VC4_DISPLAY_H_
#define SRC_GRAPHICS_DISPLAY_DRIVERS_VC4_DISPLAY_VC4_DISPLAY_H_

#include <fuchsia/hardware/display/controller/cpp/banjo.h>
#include <fuchsia/hardware/i2cimpl/cpp/banjo.h>
#include <fuchsia/hardware/mailbox/cpp/banjo.h>
#include <fuchsia/hardware/platform/device/c/banjo.h>
#include <fuchsia/hardware/sysmem/cpp/banjo.h>
#include <lib/device-protocol/pdev.h>
#include <lib/edid/edid.h>
#include <lib/mmio/mmio.h>
#include <lib/zx/bti.h>
#include <lib/zx/interrupt.h>

#include <map>
#include <vector>

#include <ddk/debug.h>
#include <ddk/driver.h>
#include <ddktl/device.h>
#include <fbl/auto_lock.h>
#include <fbl/intrusive_double_list.h>
#include <fbl/mutex.h>

#include <soc/broadcom/rpi-display.h>

namespace vc4_display {

class Vc4Display;

struct DisplayInfo {
  uint32_t display_number;
  bool power;
  bool edid_present;
  display_mode_t mode;
  std::vector<uint64_t> handles;
};

struct ImageInfo : public fbl::DoublyLinkedListable<std::unique_ptr<ImageInfo>> {
  ~ImageInfo() {
    if (pmt)
      pmt.unpin();
  }

  zx::pmt pmt;
  zx_paddr_t paddr;
  uint32_t pitch;
  uint8_t vc_image_type;
};

// Vc4Display will implement only a few subset of Device
using DeviceType = ddk::Device<Vc4Display, ddk::GetProtocolable, ddk::Unbindable>;

class Vc4Display
    : public DeviceType,
      public ddk::DisplayControllerImplProtocol<Vc4Display, ddk::base_protocol>,
      public ddk::I2cImplProtocol<Vc4Display> {
 public:
  Vc4Display(zx_device_t* parent) : DeviceType(parent) {}

  // This function is called from the c-bind function upon driver matching
  zx_status_t Bind();

  // Required functions needed to implement Display Controller Protocol
  void DisplayControllerImplSetDisplayControllerInterface(
      const display_controller_interface_protocol_t* intf);
  zx_status_t DisplayControllerImplImportVmoImage(image_t* image, zx::vmo vmo, size_t offset);
  zx_status_t DisplayControllerImplImportImage(image_t* image, zx_unowned_handle_t handle,
                                               uint32_t index);
  void DisplayControllerImplReleaseImage(image_t* image);
  uint32_t DisplayControllerImplCheckConfiguration(const display_config_t** display_config,
                                                   size_t display_count,
                                                   uint32_t** layer_cfg_results,
                                                   size_t* layer_cfg_result_count);
  void DisplayControllerImplApplyConfiguration(const display_config_t** display_config,
                                               size_t display_count);
  void DisplayControllerImplSetEld(uint64_t display_id, const uint8_t* raw_eld_list,
                                   size_t raw_eld_count) {}  // No ELD required for non-HDA systems.
  zx_status_t DisplayControllerImplGetSysmemConnection(zx::channel connection);
  zx_status_t DisplayControllerImplSetBufferCollectionConstraints(const image_t* config,
                                                                  uint32_t collection);
  zx_status_t DisplayControllerImplGetSingleBufferFramebuffer(zx::vmo* out_vmo,
                                                              uint32_t* out_stride) {
    return ZX_ERR_NOT_SUPPORTED;
  }

  // Required functions for I2cImpl
  uint32_t I2cImplGetBusCount() { return 2; }
  zx_status_t I2cImplGetMaxTransferSize(uint32_t bus_id, size_t* out_size) {
    *out_size = UINT32_MAX;
    return ZX_OK;
  }
  zx_status_t I2cImplSetBitrate(uint32_t bus_id, uint32_t bitrate) {
    // no-op
    return ZX_OK;
  }
  zx_status_t I2cImplTransact(uint32_t bus_id, const i2c_impl_op_t* op_list, size_t op_count);

  // Required functions for DeviceType
  void DdkUnbind(ddk::UnbindTxn txn);
  void DdkRelease();
  zx_status_t DdkGetProtocol(uint32_t proto_id, void* out_protocol);

  int VSyncThread();
  zx_status_t raspberrypi_property(uint32_t tag, void *property, uint32_t wsize, uint32_t rsize);

 private:
  zx_status_t GetDisplayMode(struct DisplayInfo* display);
  zx_status_t SetDisplayMode(const struct DisplayInfo* display);
  zx_status_t SetDisplayPower(const struct DisplayInfo* display);
  void PopulateAddedDisplayArgs(const struct DisplayInfo* display, added_display_args_t* arg);
  void Shutdown();

  zx_status_t InitDisplaySubsystems();
  zx_status_t ShutdownDisplaySubsytem();

  // Zircon handles
  zx::bti bti_;

  // Protocol handles
  ddk::PDev pdev_;
  ddk::SysmemProtocolClient sysmem_;
  ddk::MailboxProtocolClient mailbox_;

  // SMI
  std::optional<ddk::MmioBuffer> smi_mmio_;

  // Interrupts
  zx::interrupt vsync_irq_;

  // Thread handles
  thrd_t vsync_thread_;

  // Locks used by the display driver
  fbl::Mutex image_lock_;    // used for accessing imported_images_
  fbl::Mutex display_lock_;  // general display state (i.e. display_id)

  // Imported Images
  fbl::SizedDoublyLinkedList<std::unique_ptr<ImageInfo>> imported_images_;

  // Display list
  std::map<uint32_t, struct DisplayInfo> display_map_;

  // Display controller related data
  ddk::DisplayControllerInterfaceProtocolClient dc_intf_;
  
  struct get_display_cfg display_cfg_;
};

}  // namespace mt8167s_display

#endif  // SRC_GRAPHICS_DISPLAY_DRIVERS_VC4_DISPLAY_VC4_DISPLAY_H_
