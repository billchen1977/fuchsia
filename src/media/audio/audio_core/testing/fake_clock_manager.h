// Copyright 2020 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SRC_MEDIA_AUDIO_AUDIO_CORE_TESTING_FAKE_CLOCK_MANAGER_H_
#define SRC_MEDIA_AUDIO_AUDIO_CORE_TESTING_FAKE_CLOCK_MANAGER_H_

#include <lib/zx/clock.h>

#include <mutex>
#include <unordered_map>

#include "src/lib/fxl/synchronization/thread_annotations.h"
#include "src/media/audio/audio_core/audio_clock_manager.h"

namespace media::audio::testing {

class FakeClockManager : public AudioClockManager,
                         public std::enable_shared_from_this<FakeClockManager> {
 public:
  FakeClockManager() = default;

  std::unique_ptr<AudioClock> CreateClientAdjustable(zx::clock clock) override;
  std::unique_ptr<AudioClock> CreateClientFixed(zx::clock clock) override;
  std::unique_ptr<AudioClock> CreateDeviceAdjustable(zx::clock clock, uint32_t domain) override;
  std::unique_ptr<AudioClock> CreateDeviceFixed(zx::clock clock, uint32_t domain) override;

  // To create FakeAudioClocks based on custom clock specifications.
  std::unique_ptr<AudioClock> CreateClientAdjustable(zx::time start_time,
                                                     int32_t rate_adjust_ppm) override;
  std::unique_ptr<AudioClock> CreateClientFixed(zx::time start_time,
                                                int32_t rate_adjust_ppm) override;
  std::unique_ptr<AudioClock> CreateDeviceAdjustable(zx::time start_time, int32_t rate_adjust_ppm,
                                                     uint32_t domain) override;
  std::unique_ptr<AudioClock> CreateDeviceFixed(zx::time start_time, int32_t rate_adjust_ppm,
                                                uint32_t domain) override;

  zx::time mono_time() const override {
    std::lock_guard<std::mutex> lock(mutex_);
    return mono_time_;
  }

  void AdvanceMonoTimeBy(zx::duration duration) override {
    std::lock_guard<std::mutex> lock(mutex_);
    mono_time_ += duration;
  }

  TimelineFunction ref_to_mono_time_transform(zx_koid_t clock_id) const {
    std::lock_guard<std::mutex> lock(mutex_);
    return ref_time_to_mono_time_transforms_.at(clock_id);
  }

  void UpdateClockRate(zx_koid_t clock_id, int32_t rate_adjust_ppm);

 private:
  void UpdateRefToMonoTransform(const zx::clock& clock);
  void UpdateRefToMonoTransform(zx_koid_t clock_id, zx::time start_time, int32_t rate_adjust_ppm);

  mutable std::mutex mutex_;
  zx::time mono_time_ FXL_GUARDED_BY(mutex_);
  std::unordered_map<zx_koid_t, TimelineFunction> ref_time_to_mono_time_transforms_
      FXL_GUARDED_BY(mutex_);
};

}  // namespace media::audio::testing

#endif  // SRC_MEDIA_AUDIO_AUDIO_CORE_TESTING_FAKE_CLOCK_MANAGER_H_
