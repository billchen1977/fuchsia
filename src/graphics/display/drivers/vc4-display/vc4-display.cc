// Copyright 2021 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "vc4-display.h"
#include "vc_image_types.h"

#include <lib/image-format/image_format.h>
#include <lib/zircon-internal/align.h>
#include <lib/zx/pmt.h>
#include <lib/zx/vmo.h>
#include <zircon/pixelformat.h>

#include "src/graphics/display/drivers/vc4-display/vc4-display-bind.h"

namespace vc4_display {

#define DISP_ERROR(fmt, ...) zxlogf(ERROR, "[%s %d]" fmt, __func__, __LINE__, ##__VA_ARGS__)

// List of supported pixel formats
static const zx_pixel_format_t kSupportedPixelFormats[] = {
  ZX_PIXEL_FORMAT_ARGB_8888, ZX_PIXEL_FORMAT_RGB_x888, ZX_PIXEL_FORMAT_BGR_888x, ZX_PIXEL_FORMAT_RGB_565, ZX_PIXEL_FORMAT_RGB_888,
  ZX_PIXEL_FORMAT_NV12,
};
static const uint8_t kSupportedVcImageFormats[] = {
  VC_IMAGE_ARGB8888, VC_IMAGE_XRGB8888, VC_IMAGE_BGRX8888, VC_IMAGE_RGB565, VC_IMAGE_BGR888, 
  VC_IMAGE_YUV420SP,
};

uint8_t GetVcImageFormat(zx_pixel_format_t format)
{
  for (uint32_t i = 0; i < countof(kSupportedPixelFormats); i++) {
    if (kSupportedPixelFormats[i] == format)
      return kSupportedVcImageFormats[i];
  }
  return VC_IMAGE_MIN;
}

uint32_t GetRefreshRate(const struct display_mode* mode)
{
  float refresh_rate =
    (float)mode->pixel_clock_10khz * 10000 /
    (mode->h_front_porch + mode->h_sync_pulse + mode->h_addressable + mode->h_blanking) / 
    (mode->v_front_porch + mode->v_sync_pulse + mode->v_addressable + mode->v_blanking);
  if (mode->flags & MODE_FLAG_INTERLACED)
    refresh_rate *= 2;
  return (uint32_t)(refresh_rate * 100);
}

zx_status_t Vc4Display::raspberrypi_property(uint32_t tag, void *property, uint32_t wsize, uint32_t rsize)
{
  mailbox_data_buf_t mdata;
  mdata.cmd = tag;
  mdata.tx_buffer = reinterpret_cast<const uint8_t*>(property);
  mdata.tx_size = wsize;

  mailbox_channel_t channel;
  channel.mailbox = 0;
  channel.rx_buffer = reinterpret_cast<const uint8_t*>(property);
  channel.rx_size = rsize;

  zx_status_t status = mailbox_.SendCommand(&channel, &mdata);
  if (status != ZX_OK) {
    DISP_ERROR("mailbox_send_command failed - error status %d", status);
    return status;
  }

  return ZX_OK;
}

zx_status_t Vc4Display::GetDisplayMode(struct DisplayInfo* display) {
  struct set_timings timings = {
    .display = static_cast<uint8_t>(display->display_number),
  };
  zx_status_t status = raspberrypi_property(RPI_FIRMWARE_GET_DISPLAY_TIMING, &timings, sizeof(timings), sizeof(timings));
  if (status != ZX_OK)
    return status;

  display->mode.pixel_clock_10khz = timings.clock / 10;
  display->mode.h_addressable = timings.hdisplay;
  display->mode.h_front_porch = timings.hsync_start - timings.hdisplay;
  display->mode.h_sync_pulse = timings.hsync_end - timings.hsync_start;
  display->mode.h_blanking = timings.htotal - timings.hsync_end;
  display->mode.v_addressable = timings.vdisplay;
  display->mode.v_front_porch = timings.vsync_start - timings.vdisplay;
  display->mode.v_sync_pulse = timings.vsync_end - timings.vsync_start;
  display->mode.v_blanking = timings.vtotal - timings.vsync_end;
  display->mode.flags = 0;
  if (timings.flags & TIMINGS_FLAGS_H_SYNC_POS)
    display->mode.flags |= MODE_FLAG_HSYNC_POSITIVE;
  if (timings.flags & TIMINGS_FLAGS_V_SYNC_POS)
    display->mode.flags |= MODE_FLAG_VSYNC_POSITIVE;
  if (timings.flags & TIMINGS_FLAGS_INTERLACE)
    display->mode.flags |= MODE_FLAG_INTERLACED;
  return ZX_OK;
}

zx_status_t Vc4Display::SetDisplayMode(const struct DisplayInfo* display) {
  struct set_timings timings = {
    .display = static_cast<uint8_t>(display->display_number),
    .clock = display->mode.pixel_clock_10khz * 10,
    .hdisplay = static_cast<uint16_t>(display->mode.h_addressable),
    .hsync_start = static_cast<uint16_t>(display->mode.h_addressable + display->mode.h_front_porch),
    .hsync_end = static_cast<uint16_t>(display->mode.h_addressable + display->mode.h_front_porch + display->mode.h_sync_pulse),
    .htotal = static_cast<uint16_t>(display->mode.h_addressable + display->mode.h_front_porch + display->mode.h_sync_pulse + display->mode.h_blanking),
    .vdisplay = static_cast<uint16_t>(display->mode.v_addressable),
    .vsync_start = static_cast<uint16_t>(display->mode.v_addressable + display->mode.v_front_porch),
    .vsync_end = static_cast<uint16_t>(display->mode.v_addressable + display->mode.v_front_porch + display->mode.v_sync_pulse),
    .vtotal = static_cast<uint16_t>(display->mode.v_addressable + display->mode.v_front_porch + display->mode.v_sync_pulse + display->mode.v_blanking),
    .flags = 0,
  };
  if (display->mode.flags & MODE_FLAG_HSYNC_POSITIVE)
    timings.flags |= TIMINGS_FLAGS_H_SYNC_POS;
  if (display->mode.flags & MODE_FLAG_VSYNC_POSITIVE)
    timings.flags |= TIMINGS_FLAGS_V_SYNC_POS;
  if (display->mode.flags & MODE_FLAG_INTERLACED)
    timings.flags |= TIMINGS_FLAGS_INTERLACE;
  timings.vrefresh = GetRefreshRate(&display->mode) / 100;

  return raspberrypi_property(RPI_FIRMWARE_SET_TIMING, &timings, sizeof(timings), 0);
}

zx_status_t Vc4Display::SetDisplayPower(const struct DisplayInfo* display) {
  zx_status_t status;

  struct mailbox_display_pwr pwr = {
    .display = static_cast<uint8_t>(display->display_number),
    .state = 0,
  };
  status = raspberrypi_property(RPI_FIRMWARE_SET_DISPLAY_POWER, &pwr, sizeof(pwr), sizeof(pwr.state));
  if (status != ZX_OK) {
    return status;
  }

  if (!display->power)
    return ZX_OK;

  pwr.state = 1;
  status = raspberrypi_property(RPI_FIRMWARE_SET_DISPLAY_POWER, &pwr, sizeof(pwr), sizeof(pwr.state));
  if (status != ZX_OK) {
    return status;
  }

  for (int i = 0; i < PLANES_PER_CRTC; i++) {
    struct set_plane plane = {
      .display = static_cast<uint8_t>(display->display_number),
      .plane_id = static_cast<uint8_t>(i),
    };
    status = raspberrypi_property(RPI_FIRMWARE_SET_PLANE, &plane, sizeof(plane), 0);
    if (status != ZX_OK) {
      return status;
    }
  }

  struct mailbox_blank_display blank = {
    .tag1 = { RPI_FIRMWARE_FRAMEBUFFER_SET_DISPLAY_NUM, 4, 0, },
    .display = static_cast<uint8_t>(display->display_number),
    .tag2 = { RPI_FIRMWARE_FRAMEBUFFER_BLANK, 4, 0, },
    .blank = 0,
  };
  status = raspberrypi_property(0, &blank, sizeof(blank), sizeof(blank));
  if (status != ZX_OK) {
    return status;
  }

  return ZX_OK;
}

void Vc4Display::PopulateAddedDisplayArgs(const struct DisplayInfo* display, added_display_args_t* arg) {
  arg->display_id = display->display_number;
  arg->edid_present = display->edid_present;
  if (display->edid_present) {
    arg->panel.i2c_bus_id = display->display_number;
  } else {
    arg->panel.params.height = display->mode.v_addressable;
    arg->panel.params.width = display->mode.h_addressable;
    arg->panel.params.refresh_rate_e2 = GetRefreshRate(&display->mode);
  }
  arg->pixel_format_list = kSupportedPixelFormats;
  arg->pixel_format_count = countof(kSupportedPixelFormats);
  arg->cursor_info_count = 0;
}

void Vc4Display::DisplayControllerImplSetDisplayControllerInterface(
    const display_controller_interface_protocol_t* intf) {
  std::vector<added_display_args_t> args;

  {
    fbl::AutoLock lock(&display_lock_);
    dc_intf_ = ddk::DisplayControllerInterfaceProtocolClient(intf);
    for (const auto& [display_number, display] : display_map_) {
      added_display_args_t arg;
      PopulateAddedDisplayArgs(&display, &arg);
      args.push_back(arg);
    }
  }

  if (args.size()) {
    dc_intf_.OnDisplaysChanged(&args[0], args.size(), NULL, 0, NULL, 0, NULL);
  }
}

// part of ZX_PROTOCOL_DISPLAY_CONTROLLER_IMPL ops
zx_status_t Vc4Display::DisplayControllerImplImportVmoImage(image_t* image, zx::vmo vmo,
                                                                size_t offset) {
  return ZX_ERR_NOT_SUPPORTED;
}

// part of ZX_PROTOCOL_DISPLAY_CONTROLLER_IMPL ops
zx_status_t Vc4Display::DisplayControllerImplImportImage(image_t* image,
                                                         zx_unowned_handle_t handle,
                                                         uint32_t index) {
  if (image->type != IMAGE_TYPE_SIMPLE) {
    return ZX_ERR_INVALID_ARGS;
  }

  uint8_t vc_image_type = GetVcImageFormat(image->pixel_format);
  if (vc_image_type == VC_IMAGE_MIN) {
    return ZX_ERR_INVALID_ARGS;
  }
  
  auto import_info = std::make_unique<ImageInfo>();
  if (import_info == nullptr) {
    return ZX_ERR_NO_MEMORY;
  }

  fbl::AutoLock lock(&image_lock_);
  zx_status_t status, status2;
  fuchsia_sysmem_BufferCollectionInfo_2 collection_info;
  status =
      fuchsia_sysmem_BufferCollectionWaitForBuffersAllocated(handle, &status2, &collection_info);
  if (status != ZX_OK) {
    return status;
  }
  if (status2 != ZX_OK) {
    return status2;
  }

  if (!collection_info.settings.has_image_format_constraints ||
      index >= collection_info.buffer_count) {
    return ZX_ERR_OUT_OF_RANGE;
  }

  ZX_DEBUG_ASSERT(
      collection_info.settings.image_format_constraints.pixel_format.has_format_modifier);
  ZX_DEBUG_ASSERT(
      collection_info.settings.image_format_constraints.pixel_format.format_modifier.value ==
      fuchsia_sysmem_FORMAT_MODIFIER_LINEAR);

  uint32_t minimum_row_bytes;
  if (!ImageFormatMinimumRowBytes(&collection_info.settings.image_format_constraints,
                                  image->width, &minimum_row_bytes)) {
    DISP_ERROR("Invalid image width %d for collection", image->width);
    return ZX_ERR_INVALID_ARGS;
  }
  uint64_t offset = collection_info.buffers[index].vmo_usable_start;

  size_t size;
  if (image->pixel_format == ZX_PIXEL_FORMAT_NV12)
      size = ZX_ROUNDUP((minimum_row_bytes * image->height * 3  / 2) + (offset & (PAGE_SIZE - 1)), PAGE_SIZE);
  else
      size = ZX_ROUNDUP((minimum_row_bytes * image->height) + (offset & (PAGE_SIZE - 1)), PAGE_SIZE);

  zx_paddr_t paddr;
  status =
      bti_.pin(ZX_BTI_PERM_READ | ZX_BTI_CONTIGUOUS, zx::vmo(collection_info.buffers[index].vmo),
               offset & ~(PAGE_SIZE - 1), size, &paddr, 1, &import_info->pmt);
  if (status != ZX_OK) {
    DISP_ERROR("Could not pin bit");
    return status;
  }
  // Make sure paddr is allocated in the lower 4GB.
  ZX_ASSERT((paddr + size) <= UINT32_MAX);
  import_info->paddr = paddr;
  import_info->pitch = minimum_row_bytes;
  import_info->vc_image_type = vc_image_type;
  image->handle = reinterpret_cast<uint64_t>(import_info.get());
  imported_images_.push_back(std::move(import_info));
  return status;
}

void Vc4Display::DisplayControllerImplReleaseImage(image_t* image) {
  fbl::AutoLock lock(&image_lock_);
  auto info = reinterpret_cast<ImageInfo*>(image->handle);
  imported_images_.erase(*info);
}

uint32_t Vc4Display::DisplayControllerImplCheckConfiguration(
    const display_config_t** display_configs, size_t display_count, uint32_t** layer_cfg_results,
    size_t* layer_cfg_result_count) {
  fbl::AutoLock lock(&display_lock_);
  size_t i, j;

  for (i = 0; i < display_count; i++) {
    layer_cfg_result_count[i] = 0;
    for (auto& [display_number, display] : display_map_) {
      if (display_number == display_configs[i]->display_id) {
        if (display.edid_present && display.mode.pixel_clock_10khz == 0) {
          display.mode = display_configs[i]->mode;
          if (SetDisplayMode(&display) != ZX_OK) {
            DISP_ERROR("Could not set display mode");
            return CONFIG_DISPLAY_UNSUPPORTED_MODES;
          }
        }

        layer_cfg_result_count[i] = display_configs[i]->layer_count;
        if (display_configs[i]->layer_count > PLANES_PER_CRTC) {
          layer_cfg_results[i][0] = CLIENT_MERGE_BASE;
          for (j = 1; j < display_configs[i]->layer_count; j++) {
            layer_cfg_results[i][j] = CLIENT_MERGE_SRC;
          }
          break;
        }

        for (j = 0; j < display_configs[i]->layer_count; j++) {
          layer_cfg_results[i][j] = 0;
          if (display_configs[i]->layer_list[j]->type == LAYER_TYPE_PRIMARY) {
            const primary_layer_t& layer = display_configs[0]->layer_list[j]->cfg.primary;
            if (layer.transform_mode != FRAME_TRANSFORM_IDENTITY &&
                layer.transform_mode != FRAME_TRANSFORM_REFLECT_X &&
                layer.transform_mode != FRAME_TRANSFORM_REFLECT_Y &&
                layer.transform_mode != FRAME_TRANSFORM_ROT_180) {
              layer_cfg_results[i][j] |= CLIENT_TRANSFORM;
            }
            if (layer.alpha_mode == ALPHA_PREMULTIPLIED) {
              layer_cfg_results[i][j] |= CLIENT_ALPHA;
            }
          } else if (display_configs[i]->layer_list[j]->type == LAYER_TYPE_CURSOR) {
            ;
          } else {
            layer_cfg_results[i][j] |= CLIENT_USE_PRIMARY;
          }       
        }
        break;
      }
    }
    if (layer_cfg_result_count[i] == 0) {
      DISP_ERROR("Unknown display");
      return CONFIG_DISPLAY_TOO_MANY;
    }
  }

  return CONFIG_DISPLAY_OK;
}

void Vc4Display::DisplayControllerImplApplyConfiguration(
    const display_config_t** display_configs, size_t display_count) {
  fbl::AutoLock lock(&display_lock_);
  size_t i, j;

  for (i = 0; i < display_count; i++) {
    for (auto& [display_number, display] : display_map_) {
      if (display_number == display_configs[i]->display_id) {
        if (display.edid_present && memcmp(&display.mode, &display_configs[i]->mode, sizeof(display.mode))) {
          display.mode = display_configs[i]->mode;
          if (SetDisplayMode(&display) != ZX_OK) {
            DISP_ERROR("Could not set display mode");
            break;
          }
        }

        display.handles.clear();
        for (j = 0; j < display_configs[i]->layer_count; j++) {
          struct set_plane plane = {
            .display = static_cast<uint8_t>(display_configs[i]->display_id),
            .plane_id = static_cast<uint8_t>(j),
            .layer = static_cast<int8_t>(display_configs[i]->layer_list[j]->z_index ? 1 : -127),
          };
          if (display_configs[i]->layer_list[j]->type == LAYER_TYPE_PRIMARY) {
            const primary_layer_t& layer = display_configs[0]->layer_list[j]->cfg.primary;
            plane.vc_image_type = reinterpret_cast<struct ImageInfo *>(layer.image.handle)->vc_image_type;
            plane.width = layer.image.width;
            plane.height = layer.image.height;
            plane.src_x = layer.src_frame.x_pos << 16;
            plane.src_y = layer.src_frame.y_pos << 16;
            plane.src_w = layer.src_frame.width << 16;
            plane.src_h = layer.src_frame.height << 16;
            plane.dst_x = layer.dest_frame.x_pos << 16;
            plane.dst_y = layer.dest_frame.y_pos << 16;
            plane.dst_w = layer.dest_frame.width << 16;
            plane.dst_h = layer.dest_frame.height << 16;
            plane.alpha = layer.alpha_mode == ALPHA_DISABLE ? 255 : (uint8_t)(layer.alpha_layer_val * 255);
            switch (layer.transform_mode) {
            case FRAME_TRANSFORM_REFLECT_X:
              plane.transform = TRANSFORM_FLIP_HRIZ;
              break;
            case FRAME_TRANSFORM_REFLECT_Y:
              plane.transform = TRANSFORM_FLIP_VERT;
              break;
            case FRAME_TRANSFORM_ROT_180:
              plane.transform = TRANSFORM_ROTATE_180;
              break;
            default:
              plane.transform = TRANSFORM_NO_ROTATE;
              break;
            }
            if (plane.vc_image_type == VC_IMAGE_YUV420SP) {
              plane.num_planes = 2;
              plane.is_vu = 1;
              plane.color_encoding = VC_IMAGE_YUVINFO_CSC_ITUR_BT709;
              plane.pitch = reinterpret_cast<struct ImageInfo *>(layer.image.handle)->pitch;
              plane.planes[0] = reinterpret_cast<struct ImageInfo *>(layer.image.handle)->paddr;
              plane.planes[1] = plane.planes[0] + plane.pitch * plane.height;
            } else {
              plane.num_planes = 1;
              plane.is_vu = 0;
              plane.color_encoding = VC_IMAGE_YUVINFO_UNSPECIFIED;
              plane.pitch = reinterpret_cast<struct ImageInfo *>(layer.image.handle)->pitch;
              plane.planes[0] = reinterpret_cast<struct ImageInfo *>(layer.image.handle)->paddr;
            }
            display.handles.push_back(layer.image.handle);
          } else if (display_configs[i]->layer_list[j]->type == LAYER_TYPE_CURSOR) {
            const cursor_layer& layer = display_configs[0]->layer_list[j]->cfg.cursor;
            plane.layer = 2;
            plane.vc_image_type = reinterpret_cast<struct ImageInfo *>(layer.image.handle)->vc_image_type;
            plane.width = layer.image.width;
            plane.height = layer.image.height;
            plane.src_x = 0;
            plane.src_y = 0;
            plane.src_w = layer.image.width << 16;
            plane.src_h = layer.image.height << 16;
            plane.dst_x = layer.x_pos << 16;
            plane.dst_y = layer.y_pos << 16;
            plane.dst_w = layer.image.width << 16;
            plane.dst_h = layer.image.height << 16;
            plane.alpha = 255;
            plane.num_planes = 1;
            plane.is_vu = 0;
            plane.color_encoding = VC_IMAGE_YUVINFO_UNSPECIFIED;
            plane.transform = TRANSFORM_NO_ROTATE;
            plane.pitch = reinterpret_cast<struct ImageInfo *>(layer.image.handle)->pitch;
            plane.planes[0] = reinterpret_cast<struct ImageInfo *>(layer.image.handle)->paddr;
            display.handles.push_back(layer.image.handle);
          }
          zx_status_t status = raspberrypi_property(RPI_FIRMWARE_SET_PLANE, &plane, sizeof(plane), 0);
          if (status != ZX_OK) {
            DISP_ERROR("Could not set plane");
          }
        }

        for (; j < PLANES_PER_CRTC; j++) {
          struct set_plane plane = {
            .display = static_cast<uint8_t>(display_configs[i]->display_id),
            .plane_id = static_cast<uint8_t>(j),
          };
          zx_status_t status = raspberrypi_property(RPI_FIRMWARE_SET_PLANE, &plane, sizeof(plane), 0);
          if (status != ZX_OK) {
            DISP_ERROR("Could not set blank plane");
          }
        }
        break;
      }
    }
  }
}

zx_status_t Vc4Display::DisplayControllerImplGetSysmemConnection(zx::channel connection) {
  zx_status_t status = sysmem_.Connect(std::move(connection));
  if (status != ZX_OK) {
    DISP_ERROR("Could not connect to sysmem");
    return status;
  }

  return ZX_OK;
}

zx_status_t Vc4Display::DisplayControllerImplSetBufferCollectionConstraints(
    const image_t* config, zx_unowned_handle_t collection) {
  fuchsia_sysmem_BufferCollectionConstraints constraints = {};
  constraints.usage.display = fuchsia_sysmem_displayUsageLayer;
  constraints.has_buffer_memory_constraints = true;
  fuchsia_sysmem_BufferMemoryConstraints& buffer_constraints =
      constraints.buffer_memory_constraints;
  buffer_constraints.min_size_bytes = 0;
  buffer_constraints.max_size_bytes = 0xffffffff;
  buffer_constraints.physically_contiguous_required = true;
  buffer_constraints.secure_required = false;
  buffer_constraints.ram_domain_supported = true;
  buffer_constraints.cpu_domain_supported = false;
  buffer_constraints.heap_permitted_count = 1;
  buffer_constraints.heap_permitted[0] = fuchsia_sysmem_HeapType_SYSTEM_RAM;
  constraints.image_format_constraints_count = 1;
  fuchsia_sysmem_ImageFormatConstraints& image_constraints =
      constraints.image_format_constraints[0];
  if (config->pixel_format == ZX_PIXEL_FORMAT_NV12) {
    image_constraints.pixel_format.type = fuchsia_sysmem_PixelFormatType_NV12;
    image_constraints.pixel_format.has_format_modifier = true;
    image_constraints.pixel_format.format_modifier.value = fuchsia_sysmem_FORMAT_MODIFIER_LINEAR;
    image_constraints.color_spaces_count = 1;
    image_constraints.color_space[0].type = fuchsia_sysmem_ColorSpaceType_REC709;
  } else {
    image_constraints.pixel_format.type = fuchsia_sysmem_PixelFormatType_BGRA32;
    image_constraints.pixel_format.has_format_modifier = true;
    image_constraints.pixel_format.format_modifier.value = fuchsia_sysmem_FORMAT_MODIFIER_LINEAR;
    image_constraints.color_spaces_count = 1;
    image_constraints.color_space[0].type = fuchsia_sysmem_ColorSpaceType_SRGB;
  }
  image_constraints.min_coded_width = 0;
  image_constraints.max_coded_width = 0xffffffff;
  image_constraints.min_coded_height = 0;
  image_constraints.max_coded_height = 0xffffffff;
  image_constraints.min_bytes_per_row = 0;
  image_constraints.max_bytes_per_row = 0xffffffff;
  image_constraints.max_coded_width_times_coded_height = 0xffffffff;
  image_constraints.layers = 1;
  image_constraints.coded_width_divisor = 1;
  image_constraints.coded_height_divisor = 1;
  image_constraints.bytes_per_row_divisor = 32;
  image_constraints.start_offset_divisor = 32;
  image_constraints.display_width_divisor = 1;
  image_constraints.display_height_divisor = 1;

  zx_status_t status =
      fuchsia_sysmem_BufferCollectionSetConstraints(collection, true, &constraints);

  if (status != ZX_OK) {
    DISP_ERROR("Failed to set constraints");
    return status;
  }

  return ZX_OK;
}

int Vc4Display::VSyncThread() {
  zx_status_t status;
  while (1) {
    // clear interrupt source
    smi_mmio_->Write32(0, SMICS);

    zx::time timestamp;
    status = vsync_irq_.wait(&timestamp);
    if (status != ZX_OK) {
      DISP_ERROR("VSync Interrupt wait failed");
      break;
    }

    fbl::AutoLock lock(&display_lock_);
    uint32_t stat = smi_mmio_->Read32(SMICS);
    if (stat & SMICS_INTERRUPTS) {
      smi_mmio_->Write32(0, SMICS);

      uint32_t chan = smi_mmio_->Read32(SMIDSW0);
      if ((chan & 0xFFFF0000) != SMI_NEW) {
        /* Older firmware. Treat the one interrupt as vblank/
         * complete for all crtcs.
         */
        if (dc_intf_.is_valid()) {
          for (const auto& [display_number, display] : display_map_) {
            std::vector<uint64_t> handles(display.handles);
            display_lock_.Release();
            dc_intf_.OnDisplayVsync(display_number, timestamp.get(), &handles[0], handles.size());
            display_lock_.Acquire();
          }
        }
      } else {
        if (display_map_.count(2) > 0 && chan & 1) {
          smi_mmio_->Write32(SMI_NEW, SMIDSW0);
          if (dc_intf_.is_valid()) {
            std::vector<uint64_t> handles(display_map_[2].handles);
            display_lock_.Release();
            dc_intf_.OnDisplayVsync(2, timestamp.get(), &handles[0], handles.size());
            display_lock_.Acquire();
          }
        }

        if (display_map_.count(7) > 0) {
          /* Check for the secondary display too */
	  chan = smi_mmio_->Read32(SMIDSW1);
          if (chan & 1) {
            smi_mmio_->Write32(SMI_NEW, SMIDSW1);
            if (dc_intf_.is_valid()) {
              std::vector<uint64_t> handles(display_map_[7].handles);
              display_lock_.Release();
              dc_intf_.OnDisplayVsync(7, timestamp.get(), &handles[0], handles.size());
              display_lock_.Acquire();
            }
          }
	}
      }
    }
  }
  return ZX_OK;
}

zx_status_t Vc4Display::ShutdownDisplaySubsytem() {
  fbl::AutoLock lock(&display_lock_);
  for (auto& [display_number, display] : display_map_) {
    if (display.power) {
      display.power = false;
      SetDisplayPower(&display);
    }
  }

  return ZX_OK;
}

zx_status_t Vc4Display::InitDisplaySubsystems() {
  zx_status_t status;
  uint32_t num_displays;

  status = raspberrypi_property(RPI_FIRMWARE_FRAMEBUFFER_GET_NUM_DISPLAYS, &num_displays, sizeof(num_displays), sizeof(num_displays));
  if (status != ZX_OK) {
    DISP_ERROR("Could not get number of display");
    return status;
  }

  status = raspberrypi_property(RPI_FIRMWARE_GET_DISPLAY_CFG, &display_cfg_, sizeof(display_cfg_), sizeof(display_cfg_));
  if (status != ZX_OK) {
    DISP_ERROR("Could not get display config");
    return status;
  }

  for (uint32_t i = 0; i < num_displays; i++) {
    uint32_t display_number;

    status = raspberrypi_property(RPI_FIRMWARE_FRAMEBUFFER_GET_DISPLAY_ID, &display_number, sizeof(display_number), sizeof(display_number));
    if (status != ZX_OK) {
      DISP_ERROR("Could not get display id");
      return status;
    }

    if (display_number == 2 || display_number == 7) {
      struct DisplayInfo display = {
        .display_number = display_number,
        .power = true,
      };
      status = SetDisplayPower(&display);
      if (status != ZX_OK) {
        DISP_ERROR("Could not set display power");
        return status;
      }

      status = GetDisplayMode(&display);
      if (status != ZX_OK) {
        DISP_ERROR("Could not get display mode");
        return status;
      }
      if (display.mode.pixel_clock_10khz == 0) {
        display.edid_present = true;
      }

      display_map_.emplace(display_number, display);
    }
  }

  // Map VSync Interrupt
  status = pdev_.GetInterrupt(0, 0, &vsync_irq_);
  if (status != ZX_OK) {
    DISP_ERROR("Could not map vsync Interruptn");
    return status;
  }

  auto start_thread = [](void* arg) { return static_cast<Vc4Display*>(arg)->VSyncThread(); };
  status = thrd_create_with_name(&vsync_thread_, start_thread, this, "vsync_thread");
  if (status != ZX_OK) {
    DISP_ERROR("Could not create vsync_thread");
    return status;
  }

  return ZX_OK;
}

void Vc4Display::Shutdown() {
  vsync_irq_.destroy();
  thrd_join(vsync_thread_, nullptr);
}

zx_status_t Vc4Display::I2cImplTransact(uint32_t bus_id, const i2c_impl_op_t* op_list,
                                        size_t op_count) {
  if (!op_list)
    return ZX_ERR_INVALID_ARGS;

  fbl::AutoLock lock(&display_lock_);
  if (!display_map_.count(bus_id))
    return ZX_ERR_INVALID_ARGS;

  uint8_t segment_num = 0;
  uint8_t segment_offset = 0;
  for (unsigned i = 0; i < op_count; i++) {
    auto op = op_list[i];

    if (op.address == edid::kDdcSegmentI2cAddress && !op.is_read && op.data_size == 1) {
      segment_num = *((const uint8_t*)op.data_buffer);
    } else if (op.address == edid::kDdcDataI2cAddress && !op.is_read && op.data_size == 1) {
      segment_offset = *((const uint8_t*)op.data_buffer);
    } else if (op.address == edid::kDdcDataI2cAddress && op.is_read) {
      uint32_t block = segment_num * 2 + segment_offset / edid::kBlockSize;
      uint32_t offset = segment_offset % edid::kBlockSize;
      uint8_t* buffer = op.data_buffer;
      size_t size = op.data_size;

      while (size > 0) {
        struct mailbox_get_edid edid = {
          .block = block,
          .display_number = bus_id,
        };
        zx_status_t status = raspberrypi_property(RPI_FIRMWARE_GET_EDID_BLOCK_DISPLAY, &edid, sizeof(edid), sizeof(edid));
        if (status != ZX_OK)
          return status;

        size_t n = edid::kBlockSize - offset;
        if (n > size)
          n = size;
        memcpy(buffer, &edid.edid[offset], n);
        
        block++;
        offset = 0;
        buffer += n;
        size -= n;
      }
    } else {
      return ZX_ERR_NOT_SUPPORTED;
    }

    if (op.stop) {
      segment_num = 0;
      segment_offset = 0;
    }
  }

  return ZX_OK;
}

void Vc4Display::DdkUnbind(ddk::UnbindTxn txn) {
  Shutdown();
  txn.Reply();
}
void Vc4Display::DdkRelease() { delete this; }

zx_status_t Vc4Display::DdkGetProtocol(uint32_t proto_id, void* out_protocol) {
  auto* proto = static_cast<ddk::AnyProtocol*>(out_protocol);
  proto->ctx = this;
  switch (proto_id) {
    case ZX_PROTOCOL_DISPLAY_CONTROLLER_IMPL:
      proto->ops = &display_controller_impl_protocol_ops_;
      return ZX_OK;
    case ZX_PROTOCOL_I2C_IMPL:
      proto->ops = &i2c_impl_protocol_ops_;
      return ZX_OK;
    default:
      return ZX_ERR_NOT_SUPPORTED;
  }
}

zx_status_t Vc4Display::Bind() {
  zx_status_t status = ddk::PDev::FromFragment(parent_, &pdev_);
  if (status != ZX_OK) {
    DISP_ERROR("Could not get pdev protocol");
    return status;
  }

  status = ddk::MailboxProtocolClient::CreateFromDevice(parent_, "display-mailbox", &mailbox_);
  if (status != ZX_OK) {
    DISP_ERROR("could not get mailbox fragment");
    return ZX_ERR_NO_RESOURCES;
  }

  status = ddk::SysmemProtocolClient::CreateFromDevice(parent_, "display-sysmem", &sysmem_);
  if (status != ZX_OK) {
    DISP_ERROR("Could not get sysmem fragment");
    return status;
  }

  status = pdev_.GetBti(0, &bti_);
  if (status != ZX_OK) {
    DISP_ERROR("Could not get BTI handle");
    return status;
  }

  status = pdev_.MapMmio(0, &smi_mmio_);
  if (status != ZX_OK) {
    DISP_ERROR("Could not map SMI mmio");
    return status;
  }

  status = InitDisplaySubsystems();
  if (status != ZX_OK) {
    return status;
  }

  status = DdkAdd("vc4-display");
  if (status != ZX_OK) {
    DISP_ERROR("Could not add device");
    Shutdown();
    return status;
  }

  return ZX_OK;
}

// main bind function called from dev manager
zx_status_t display_bind(void* ctx, zx_device_t* parent) {
  fbl::AllocChecker ac;
  auto dev = fbl::make_unique_checked<vc4_display::Vc4Display>(&ac, parent);
  if (!ac.check()) {
    DISP_ERROR("no bind");
    return ZX_ERR_NO_MEMORY;
  }
  zx_status_t status = dev->Bind();
  if (status == ZX_OK) {
    __UNUSED auto ptr = dev.release();
  }
  return status;
}

static constexpr zx_driver_ops_t display_ops = []() {
  zx_driver_ops_t ops = {};
  ops.version = DRIVER_OPS_VERSION;
  ops.bind = display_bind;
  return ops;
}();

}  // namespace vc4_display

ZIRCON_DRIVER(vc4_display, vc4_display::display_ops, "zircon", "0.1");
