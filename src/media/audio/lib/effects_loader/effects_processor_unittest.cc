// Copyright 2019 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "src/media/audio/lib/effects_loader/effects_processor.h"

#include <gtest/gtest.h>

#include "src/media/audio/effects/test_effects/test_effects.h"
#include "src/media/audio/lib/effects_loader/testing/effects_loader_test_base.h"

namespace media::audio {
namespace {

class EffectsProcessorTest : public testing::EffectsLoaderTestBase {};

// The following tests validates the EffectsProcessor class itself.
//
// Verify the creation, uniqueness, quantity and deletion of effect instances.
TEST_F(EffectsProcessorTest, CreateDelete) {
  test_effects().AddEffect("assign_to_1.0").WithAction(TEST_EFFECTS_ACTION_ASSIGN, 1.0);

  Effect effect3 = effects_loader()->CreateEffect(0, 1, 1, 1, {});
  Effect effect1 = effects_loader()->CreateEffect(0, 1, 1, 1, {});
  Effect effect2 = effects_loader()->CreateEffect(0, 1, 1, 1, {});
  Effect effect4 = effects_loader()->CreateEffect(0, 1, 1, 1, {});

  ASSERT_TRUE(effect1);
  ASSERT_TRUE(effect2);
  ASSERT_TRUE(effect3);
  ASSERT_TRUE(effect4);

  EXPECT_TRUE(effect1.get() != effect2.get() && effect1.get() != effect3.get() &&
              effect1.get() != effect4.get() && effect2.get() != effect3.get() &&
              effect2.get() != effect4.get() && effect3.get() != effect4.get());

  fuchsia_audio_effects_handle_t effects_handle1 = effect1.get();
  fuchsia_audio_effects_handle_t effects_handle2 = effect2.get();
  fuchsia_audio_effects_handle_t effects_handle3 = effect3.get();
  fuchsia_audio_effects_handle_t effects_handle4 = effect4.get();

  // Create processor
  {
    EffectsProcessor processor;
    EXPECT_EQ(processor.AddEffect(std::move(effect3)), ZX_OK);
    EXPECT_EQ(processor.AddEffect(std::move(effect1)), ZX_OK);
    EXPECT_EQ(processor.AddEffect(std::move(effect2)), ZX_OK);
    EXPECT_EQ(processor.AddEffect(std::move(effect4)), ZX_OK);
    EXPECT_EQ(processor.size(), 4);

    EXPECT_EQ(effects_handle3, processor.GetEffectAt(0).get());
    EXPECT_EQ(effects_handle1, processor.GetEffectAt(1).get());
    EXPECT_EQ(effects_handle2, processor.GetEffectAt(2).get());
    EXPECT_EQ(effects_handle4, processor.GetEffectAt(3).get());

    EXPECT_EQ(4u, test_effects().InstanceCount());
  }

  // All instances should be deleted when the processor is destructed.
  EXPECT_EQ(0u, test_effects().InstanceCount());
}

TEST_F(EffectsProcessorTest, AddEffectWithMismatchedChannelConfig) {
  test_effects().AddEffect("assign_to_1.0").WithAction(TEST_EFFECTS_ACTION_ASSIGN, 1.0);
  Effect single_channel_effect1 = effects_loader()->CreateEffect(0, 1, 1, 1, {});
  Effect single_channel_effect2 = effects_loader()->CreateEffect(0, 1, 1, 1, {});
  Effect two_channel_effect = effects_loader()->CreateEffect(0, 1, 2, 2, {});

  EffectsProcessor processor;
  EXPECT_EQ(processor.channels_in(), 0u);
  EXPECT_EQ(processor.channels_out(), 0u);

  // Add a single channel effect (chans in == chans out == 1).
  EXPECT_EQ(processor.AddEffect(std::move(single_channel_effect1)), ZX_OK);
  EXPECT_EQ(processor.channels_in(), 1u);
  EXPECT_EQ(processor.channels_out(), 1u);

  // Add a second single channel effect.
  EXPECT_EQ(processor.AddEffect(std::move(single_channel_effect2)), ZX_OK);
  EXPECT_EQ(processor.channels_in(), 1u);
  EXPECT_EQ(processor.channels_out(), 1u);

  // Add a two channel effect. This should fail as the processor is currently producing single
  // channel audio out of the last effect.
  EXPECT_EQ(processor.AddEffect(std::move(two_channel_effect)), ZX_ERR_INVALID_ARGS);
}

// Verify (at a VERY Basic level) the methods that handle data flow.
TEST_F(EffectsProcessorTest, ProcessInPlaceFlush) {
  test_effects().AddEffect("increment_by_1.0").WithAction(TEST_EFFECTS_ACTION_ADD, 1.0);
  test_effects().AddEffect("increment_by_2.0").WithAction(TEST_EFFECTS_ACTION_ADD, 2.0);
  test_effects().AddEffect("assign_to_12.0").WithAction(TEST_EFFECTS_ACTION_ASSIGN, 12.0);
  test_effects().AddEffect("increment_by_4.0").WithAction(TEST_EFFECTS_ACTION_ADD, 4.0);

  float buff[4] = {0, 1.0, 2.0, 3.0};

  // Before instances added, ProcessInPlace and Flush should succeed.
  EffectsProcessor processor;
  EXPECT_EQ(processor.ProcessInPlace(4, buff), ZX_OK);
  EXPECT_EQ(processor.Flush(), ZX_OK);
  EXPECT_EQ(0.0, buff[0]);
  EXPECT_EQ(1.0, buff[1]);
  EXPECT_EQ(2.0, buff[2]);
  EXPECT_EQ(3.0, buff[3]);

  // Chaining four instances together, ProcessInPlace and flush should succeed.
  Effect effect1 = effects_loader()->CreateEffect(0, 1, 1, 1, {});
  Effect effect2 = effects_loader()->CreateEffect(1, 1, 1, 1, {});
  Effect effect3 = effects_loader()->CreateEffect(2, 1, 1, 1, {});
  Effect effect4 = effects_loader()->CreateEffect(3, 1, 1, 1, {});
  ASSERT_TRUE(effect1 && effect2 && effect3 && effect4);

  EXPECT_EQ(processor.AddEffect(std::move(effect1)), ZX_OK);
  EXPECT_EQ(processor.AddEffect(std::move(effect2)), ZX_OK);
  EXPECT_EQ(processor.AddEffect(std::move(effect3)), ZX_OK);
  EXPECT_EQ(processor.AddEffect(std::move(effect4)), ZX_OK);
  EXPECT_EQ(4u, test_effects().InstanceCount());

  // The first 2 processors will mutate data, but this will be clobbered by the 3rd processor which
  // just sets every sample to 12.0. The final processor will increment by 4.0 resulting in the
  // expected 16.0 values.
  EXPECT_EQ(processor.ProcessInPlace(4, buff), ZX_OK);
  EXPECT_EQ(buff[0], 16.0);
  EXPECT_EQ(buff[1], 16.0);
  EXPECT_EQ(buff[2], 16.0);
  EXPECT_EQ(buff[3], 16.0);

  // All effects should have initial flush count 0.
  test_effects_inspect_state inspect1 = {};
  test_effects_inspect_state inspect2 = {};
  test_effects_inspect_state inspect3 = {};
  test_effects_inspect_state inspect4 = {};
  EXPECT_EQ(ZX_OK, test_effects().InspectInstance(processor.GetEffectAt(0).get(), &inspect1));
  EXPECT_EQ(ZX_OK, test_effects().InspectInstance(processor.GetEffectAt(1).get(), &inspect2));
  EXPECT_EQ(ZX_OK, test_effects().InspectInstance(processor.GetEffectAt(2).get(), &inspect3));
  EXPECT_EQ(ZX_OK, test_effects().InspectInstance(processor.GetEffectAt(3).get(), &inspect4));
  EXPECT_EQ(0u, inspect1.flush_count);
  EXPECT_EQ(0u, inspect2.flush_count);
  EXPECT_EQ(0u, inspect3.flush_count);
  EXPECT_EQ(0u, inspect4.flush_count);

  // Flush, just sanity test the test_effects library has observed the flush call on each effect.
  EXPECT_EQ(processor.Flush(), ZX_OK);
  EXPECT_EQ(ZX_OK, test_effects().InspectInstance(processor.GetEffectAt(0).get(), &inspect1));
  EXPECT_EQ(ZX_OK, test_effects().InspectInstance(processor.GetEffectAt(1).get(), &inspect2));
  EXPECT_EQ(ZX_OK, test_effects().InspectInstance(processor.GetEffectAt(2).get(), &inspect3));
  EXPECT_EQ(ZX_OK, test_effects().InspectInstance(processor.GetEffectAt(3).get(), &inspect4));
  EXPECT_EQ(1u, inspect1.flush_count);
  EXPECT_EQ(1u, inspect2.flush_count);
  EXPECT_EQ(1u, inspect3.flush_count);
  EXPECT_EQ(1u, inspect4.flush_count);

  // Zero num_frames is valid and should succeed. We assign the buff to some random values here
  // to ensure the processor does not clobber them.
  buff[0] = 20.0;
  buff[1] = 21.0;
  buff[2] = 22.0;
  buff[3] = 23.0;
  EXPECT_EQ(processor.ProcessInPlace(0, buff), ZX_OK);
  EXPECT_EQ(buff[0], 20.0);
  EXPECT_EQ(buff[1], 21.0);
  EXPECT_EQ(buff[2], 22.0);
  EXPECT_EQ(buff[3], 23.0);

  // If no buffer provided, ProcessInPlace should fail (even if 0 num_frames).
  EXPECT_NE(processor.ProcessInPlace(0, nullptr), ZX_OK);
}

TEST_F(EffectsProcessorTest, ReportBlockSize) {
  test_effects().AddEffect("block_size_3").WithBlockSize(3);
  test_effects().AddEffect("block_size_5").WithBlockSize(5);
  test_effects().AddEffect("block_size_any").WithBlockSize(FUCHSIA_AUDIO_EFFECTS_BLOCK_SIZE_ANY);
  test_effects().AddEffect("block_size_1").WithBlockSize(1);

  // Needed to use |CreateEffectByName| since the effect names are cached at loader creation time.
  RecreateLoader();

  // Create processor and verify default block_size.
  EffectsProcessor processor;
  EXPECT_EQ(1u, processor.block_size());

  // Add an effect and observe a change in block_size.
  Effect effect1 = effects_loader()->CreateEffectByName("block_size_3", 1, 1, 1, {});
  ASSERT_TRUE(effect1);
  processor.AddEffect(std::move(effect1));
  EXPECT_EQ(3u, processor.block_size());

  // Add another effect and observe a change in block_size (lcm(3,5)
  Effect effect2 = effects_loader()->CreateEffectByName("block_size_5", 1, 1, 1, {});
  ASSERT_TRUE(effect2);
  processor.AddEffect(std::move(effect2));
  EXPECT_EQ(15u, processor.block_size());

  // Add some final effects that should not change block_size.
  Effect effect3 = effects_loader()->CreateEffectByName("block_size_any", 1, 1, 1, {});
  ASSERT_TRUE(effect3);
  processor.AddEffect(std::move(effect3));
  EXPECT_EQ(15u, processor.block_size());

  Effect effect4 = effects_loader()->CreateEffectByName("block_size_1", 1, 1, 1, {});
  ASSERT_TRUE(effect4);
  processor.AddEffect(std::move(effect4));
  EXPECT_EQ(15u, processor.block_size());
}

TEST_F(EffectsProcessorTest, ReportMaxBufferSize) {
  test_effects().AddEffect("max_buffer_1024").WithMaxFramesPerBuffer(1024);
  test_effects().AddEffect("max_buffer_512").WithMaxFramesPerBuffer(512);
  test_effects().AddEffect("max_buffer_256").WithMaxFramesPerBuffer(256);
  test_effects().AddEffect("max_buffer_128").WithMaxFramesPerBuffer(128);

  // Needed to use |CreateEffectByName| since the effect names are cached at loader creation time.
  RecreateLoader();

  // Create processor and verify default block_size.
  EffectsProcessor processor;
  EXPECT_EQ(0u, processor.max_batch_size());

  // Add an effect and observe a change in buffer size.
  {
    Effect effect = effects_loader()->CreateEffectByName("max_buffer_1024", 1, 1, 1, {});
    ASSERT_TRUE(effect);
    processor.AddEffect(std::move(effect));
    EXPECT_EQ(1024u, processor.max_batch_size());
  }
  {
    Effect effect = effects_loader()->CreateEffectByName("max_buffer_512", 1, 1, 1, {});
    ASSERT_TRUE(effect);
    processor.AddEffect(std::move(effect));
    EXPECT_EQ(512u, processor.max_batch_size());
  }
  {
    Effect effect = effects_loader()->CreateEffectByName("max_buffer_256", 1, 1, 1, {});
    ASSERT_TRUE(effect);
    processor.AddEffect(std::move(effect));
    EXPECT_EQ(256u, processor.max_batch_size());
  }
  {
    Effect effect = effects_loader()->CreateEffectByName("max_buffer_128", 1, 1, 1, {});
    ASSERT_TRUE(effect);
    processor.AddEffect(std::move(effect));
    EXPECT_EQ(128u, processor.max_batch_size());
  }

  // Add a final effect with an increasing max block size to verify we don't increase the reported
  // buffer size.
  {
    Effect effect = effects_loader()->CreateEffectByName("max_buffer_1024", 1, 1, 1, {});
    ASSERT_TRUE(effect);
    processor.AddEffect(std::move(effect));
    EXPECT_EQ(128u, processor.max_batch_size());
  }
}

TEST_F(EffectsProcessorTest, AlignBufferWithBlockSize) {
  test_effects()
      .AddEffect("max_buffer_1024_any_align")
      .WithMaxFramesPerBuffer(1024)
      .WithBlockSize(FUCHSIA_AUDIO_EFFECTS_BLOCK_SIZE_ANY);

  test_effects()
      .AddEffect("any_buffer_300_align")
      .WithMaxFramesPerBuffer(FUCHSIA_AUDIO_EFFECTS_FRAMES_PER_BUFFER_ANY)
      .WithBlockSize(300);

  test_effects()
      .AddEffect("max_buffer_800_any_align")
      .WithMaxFramesPerBuffer(800)
      .WithBlockSize(FUCHSIA_AUDIO_EFFECTS_BLOCK_SIZE_ANY);

  // Needed to use |CreateEffectByName| since the effect names are cached at loader creation time.
  RecreateLoader();

  // Create processor and verify default block_size.
  EffectsProcessor processor;
  EXPECT_EQ(0u, processor.max_batch_size());
  EXPECT_EQ(1u, processor.block_size());

  {
    Effect effect = effects_loader()->CreateEffectByName("max_buffer_1024_any_align", 1, 1, 1, {});
    ASSERT_TRUE(effect);
    processor.AddEffect(std::move(effect));
    EXPECT_EQ(1024u, processor.max_batch_size());
    EXPECT_EQ(1u, processor.block_size());
  }

  // Adding an effect with 300 alignment should drop our max buffer size from 1024 -> 900.
  {
    Effect effect = effects_loader()->CreateEffectByName("any_buffer_300_align", 1, 1, 1, {});
    ASSERT_TRUE(effect);
    processor.AddEffect(std::move(effect));
    EXPECT_EQ(900u, processor.max_batch_size());
    EXPECT_EQ(300u, processor.block_size());
  }
  // Adding an effect with max buffer of 800 should drop aggregate max buffer to 600.
  {
    Effect effect = effects_loader()->CreateEffectByName("max_buffer_800_any_align", 1, 1, 1, {});
    ASSERT_TRUE(effect);
    processor.AddEffect(std::move(effect));
    EXPECT_EQ(600u, processor.max_batch_size());
    EXPECT_EQ(300u, processor.block_size());
  }
}

}  // namespace
}  // namespace media::audio
