// Copyright 2019 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SRC_MEDIA_AUDIO_AUDIO_CORE_USAGE_SETTINGS_H_
#define SRC_MEDIA_AUDIO_AUDIO_CORE_USAGE_SETTINGS_H_

#include <fuchsia/media/cpp/fidl.h>

namespace media::audio {

// TODO(35491): Remove when transitioned to xunion; xunions generate these functions.
fuchsia::media::Usage UsageFrom(fuchsia::media::AudioRenderUsage render_usage);

fuchsia::media::Usage UsageFrom(fuchsia::media::AudioCaptureUsage capture_usage);

// Usage loudness settings in gain dbfs units.
// This class is not thread safe.
class UsageGainSettings {
 public:
  UsageGainSettings() = default;

  // Gets the gain that should affect all audio elements of the given usage, taking into account
  // the category gain and adjustment.
  //
  // Since this value includes adjustments, it should only be used for mixing and not reflected to
  // users.
  float GetUsageGain(const fuchsia::media::Usage& usage) const;

  void SetUsageGain(fuchsia::media::Usage usage, float gain_db);

  void SetUsageGainAdjustment(fuchsia::media::Usage usage, float gain_db);

 private:
  // TODO(36289): Determine whether mute must be tracked here

  std::array<float, fuchsia::media::RENDER_USAGE_COUNT> render_usage_gain_ = {};
  std::array<float, fuchsia::media::CAPTURE_USAGE_COUNT> capture_usage_gain_ = {};

  std::array<float, fuchsia::media::RENDER_USAGE_COUNT> render_usage_gain_adjustment_ = {};
  std::array<float, fuchsia::media::CAPTURE_USAGE_COUNT> capture_usage_gain_adjustment_ = {};
};

// Usage loudness settings in volume units.
class UsageVolumeSettings {
 public:
  UsageVolumeSettings();

  // Gets the volume that should affect all audio elements of the given usage.
  float GetUsageVolume(const fuchsia::media::Usage& usage) const;

  void SetUsageVolume(fuchsia::media::Usage usage, float volume);

 private:
  std::array<float, fuchsia::media::RENDER_USAGE_COUNT> render_usage_volume_;
  std::array<float, fuchsia::media::CAPTURE_USAGE_COUNT> capture_usage_volume_;
};

}  // namespace media::audio

#endif  // SRC_MEDIA_AUDIO_AUDIO_CORE_USAGE_SETTINGS_H_
