// Copyright 2019 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SRC_GRAPHICS_LIB_COMPUTE_SPINEL_PLATFORMS_VK_TESTS_SPINEL_VK_TEST_FXT_SPINEL_VK_RENDER_H_
#define SRC_GRAPHICS_LIB_COMPUTE_SPINEL_PLATFORMS_VK_TESTS_SPINEL_VK_TEST_FXT_SPINEL_VK_RENDER_H_

//
// This fixture supports writing explicit Spinel tests.
//
// The SVG fixture subclasses this fixture to enable writing simple
// rendering tests.
//

#include <map>

#include "fxt_spinel_vk.h"
#include "spinel/spinel_opcodes.h"
//
//
//

namespace spinel::vk::test {

struct param_spinel_vk_render
{
  char const * name;

  struct
  {
    uint32_t width;
    uint32_t height;
  } surface;

  struct
  {
    uint32_t composition[4] = { 0, 0, UINT32_MAX, UINT32_MAX };
    uint32_t render[4]      = { 0, 0, UINT32_MAX, UINT32_MAX };
  } clip;

  char const * svg   = nullptr;
  uint32_t     loops = 1;

  //
  // The map pairs define this relationship:
  //
  //   { checksum : { vendorID { deviceID }* }* }+
  //
  //     - each checksum  has zero or more associated vendor IDs
  //     - each vendor ID has zero or more associated device IDs
  //
  // An empty device ID set implies the checksum applies to all physical
  // devices that match the vendor ID.
  //
  // An empty vendor ID map implies the checksum applies to all physical
  // devices.
  //
  std::map<uint32_t, std::map<uint32_t, std::set<uint32_t>>> checksums;

  enum vendors
  {
    INTEL  = 0x8086,
    NVIDIA = 0x10DE,
    AMD    = 0x1002,
    ARM    = 0x13B5
  };

  enum devices
  {
    AMD_V1807B = 0x15DD
  };
};

// Implementing this function is necessary to avoid Valgrind warnings
// when registering tests parameterized with this struct
// (see https://bugs.fuchsia.dev/p/fuchsia/issues/detail/?id=43334)
void
PrintTo(const param_spinel_vk_render & param, std::ostream * os);

//
//
//

struct fxt_spinel_vk_render : public fxt_spinel_vk,
                              public testing::WithParamInterface<param_spinel_vk_render>
{
  //
  //
  //
  struct surface
  {
    struct
    {
      VkImage               image;
      VkDeviceMemory        dm;
      VkDescriptorImageInfo image_info;
    } d;
    struct
    {
      VkDescriptorBufferInfo dbi;
      VkDeviceMemory         dm;
      void *                 map;
    } h;
  } surface;

  //
  //
  //
  void
  SetUp() override;

  void
  TearDown() override;

  //
  // test the surface
  //
  void
  checksum();

  //
  // param name suffix generator
  //
  static std::string
  param_name(testing::TestParamInfo<param_spinel_vk_render> const & info);

  //
  // simplest submitter
  //
  static void
  spn_render_submitter(VkQueue               queue,  //
                       VkFence               fence,  //
                       VkCommandBuffer const cb,     //
                       void *                data);
};

}  // namespace spinel::vk::test

//
//
//

#endif  // SRC_GRAPHICS_LIB_COMPUTE_SPINEL_PLATFORMS_VK_TESTS_SPINEL_VK_TEST_FXT_SPINEL_VK_RENDER_H_
