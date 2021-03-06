// Copyright 2019 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "src/media/audio/audio_core/effects_stage.h"

#include <gmock/gmock.h>

#include "src/media/audio/audio_core/packet_queue.h"
#include "src/media/audio/audio_core/process_config.h"
#include "src/media/audio/audio_core/testing/fake_stream.h"
#include "src/media/audio/audio_core/testing/packet_factory.h"
#include "src/media/audio/audio_core/testing/threading_model_fixture.h"
#include "src/media/audio/lib/effects_loader/testing/test_effects.h"

using testing::Each;
using testing::FloatEq;

namespace media::audio {
namespace {

const Format kDefaultFormat = Format(fuchsia::media::AudioStreamType{
    .sample_format = fuchsia::media::AudioSampleFormat::FLOAT,
    .channels = 2,
    .frames_per_second = 48000,
});

class EffectsStageTest : public testing::ThreadingModelFixture {
 protected:
  // Views the memory at |ptr| as a std::array of |N| elements of |T|. If |offset| is provided, it
  // is the number of |T| sized elements to skip at the beginning of |ptr|.
  //
  // It is entirely up to the caller to ensure that values of |T|, |N|, and |offset| are chosen to
  // not overflow |ptr|.
  template <typename T, size_t N>
  std::array<T, N>& as_array(void* ptr, size_t offset = 0) {
    return reinterpret_cast<std::array<T, N>&>(static_cast<T*>(ptr)[offset]);
  }

  testing::TestEffectsModule test_effects_ = testing::TestEffectsModule::Open();
};

TEST_F(EffectsStageTest, ApplyEffectsToSourceStream) {
  // Create a packet queue to use as our source stream.
  auto timeline_function = fbl::MakeRefCounted<VersionedTimelineFunction>(TimelineFunction(
      TimelineRate(FractionalFrames<uint32_t>(kDefaultFormat.frames_per_second()).raw_value(),
                   zx::sec(1).to_nsecs())));
  auto stream = std::make_shared<PacketQueue>(kDefaultFormat, timeline_function);

  // Create an effect we can load.
  test_effects_.AddEffect("add_1.0").WithAction(TEST_EFFECTS_ACTION_ADD, 1.0);

  // Create the effects stage.
  std::vector<PipelineConfig::Effect> effects;
  effects.push_back(PipelineConfig::Effect{
      .lib_name = testing::kTestEffectsModuleName,
      .effect_name = "add_1.0",
      .effect_config = "",
  });
  auto effects_stage = EffectsStage::Create(effects, stream);

  // Enqueue 10ms of frames in the packet queue.
  testing::PacketFactory packet_factory(dispatcher(), kDefaultFormat, PAGE_SIZE);
  stream->PushPacket(packet_factory.CreatePacket(1.0, zx::msec(10)));

  // Read from the effects stage. Since our effect adds 1.0 to each sample, and we populated the
  // packet with 1.0 samples, we expect to see only 2.0 samples in the result.
  auto buf = effects_stage->LockBuffer(zx::time(0) + zx::msec(10), 0, 480);
  ASSERT_TRUE(buf);
  ASSERT_EQ(0u, buf->start().Floor());
  ASSERT_EQ(480u, buf->length().Floor());

  auto& arr = as_array<float, 480>(buf->payload());
  EXPECT_THAT(arr, Each(FloatEq(2.0f)));
}

TEST_F(EffectsStageTest, BlockAlignRequests) {
  // Create a source stream.
  auto stream = std::make_shared<testing::FakeStream>(kDefaultFormat);

  // Create an effect we can load.
  const uint32_t kBlockSize = 128;
  test_effects_.AddEffect("add_1.0")
      .WithAction(TEST_EFFECTS_ACTION_ADD, 1.0)
      .WithBlockSize(kBlockSize);

  // Create the effects stage.
  std::vector<PipelineConfig::Effect> effects;
  effects.push_back(PipelineConfig::Effect{
      .lib_name = testing::kTestEffectsModuleName,
      .effect_name = "add_1.0",
      .effect_config = "",
  });
  auto effects_stage = EffectsStage::Create(effects, stream);

  EXPECT_EQ(effects_stage->block_size(), kBlockSize);

  {
    // Ask for 1 frame; expect to get a full block.
    auto buffer = effects_stage->LockBuffer(zx::time(0), 0, 1);
    EXPECT_EQ(buffer->start().Floor(), 0u);
    EXPECT_EQ(buffer->length().Floor(), kBlockSize);
  }

  {
    // Ask for subsequent frames; expect the same block still.
    auto buffer = effects_stage->LockBuffer(zx::time(0), kBlockSize / 2, kBlockSize / 2);
    EXPECT_EQ(buffer->start().Floor(), 0u);
    EXPECT_EQ(buffer->length().Floor(), kBlockSize);
  }

  {
    // Ask for the second block
    auto buffer = effects_stage->LockBuffer(zx::time(0), kBlockSize, kBlockSize);
    EXPECT_EQ(buffer->start().Floor(), kBlockSize);
    EXPECT_EQ(buffer->length().Floor(), kBlockSize);
  }
}

TEST_F(EffectsStageTest, TruncateToMaxBufferSize) {
  // Create a source stream.
  auto stream = std::make_shared<testing::FakeStream>(kDefaultFormat);

  const uint32_t kBlockSize = 128;
  const uint32_t kMaxBufferSize = 300;
  test_effects_.AddEffect("test_effect")
      .WithBlockSize(kBlockSize)
      .WithMaxFramesPerBuffer(kMaxBufferSize);

  // Create the effects stage.
  std::vector<PipelineConfig::Effect> effects;
  effects.push_back(PipelineConfig::Effect{
      .lib_name = testing::kTestEffectsModuleName,
      .effect_name = "test_effect",
      .effect_config = "",
  });
  auto effects_stage = EffectsStage::Create(effects, stream);

  EXPECT_EQ(effects_stage->block_size(), kBlockSize);

  {
    auto buffer = effects_stage->LockBuffer(zx::time(0), 0, 512);
    EXPECT_EQ(buffer->start().Floor(), 0u);
    // Length is 2 full blocks since 3 blocks would be > 300 frames.
    EXPECT_EQ(buffer->length().Floor(), 256u);
  }
}

}  // namespace
}  // namespace media::audio
