// Copyright 2019 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SRC_UI_INPUT_LIB_HID_INPUT_REPORT_CONSUMER_CONTROL_H_
#define SRC_UI_INPUT_LIB_HID_INPUT_REPORT_CONSUMER_CONTROL_H_

#include "src/ui/input/lib/hid-input-report/device.h"

namespace hid_input_report {

class ConsumerControl : public Device {
 public:
  ParseResult ParseReportDescriptor(const hid::ReportDescriptor& hid_report_descriptor) override;

  ParseResult CreateDescriptor(fidl::AnyAllocator& allocator,
                               fuchsia_input_report::DeviceDescriptor& descriptor) override;

  ParseResult ParseInputReport(const uint8_t* data, size_t len, fidl::AnyAllocator& allocator,
                               fuchsia_input_report::InputReport& inputreport) override;

  uint8_t InputReportId() const override { return input_report_id_; }

  DeviceType GetDeviceType() const override { return DeviceType::kConsumerControl; }

 private:
  ParseResult ParseInputReportDescriptor(const hid::ReportDescriptor& hid_report_descriptor);

  // Fields for the input reports.
  size_t num_buttons_ = 0;
  std::array<hid::ReportField, fuchsia_input_report::CONSUMER_CONTROL_MAX_NUM_BUTTONS>
      button_fields_;
  size_t input_report_size_ = 0;
  uint8_t input_report_id_ = 0;
};

}  // namespace hid_input_report

#endif  // SRC_UI_INPUT_LIB_HID_INPUT_REPORT_CONSUMER_CONTROL_H_
