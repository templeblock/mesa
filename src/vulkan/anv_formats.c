/*
 * Copyright © 2015 Intel Corporation
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */

#include "anv_private.h"
#include "brw_surface_formats.h"

#include "gen7_pack.h"

#define fmt(__vk_fmt, __hw_fmt, ...) \
   [__vk_fmt] = { \
      .vk_format = __vk_fmt, \
      .name = #__vk_fmt, \
      .surface_format = __hw_fmt, \
      .isl_layout = &isl_format_layouts[__hw_fmt], \
      __VA_ARGS__ \
   }

static const struct anv_format anv_formats[] = {
   fmt(VK_FORMAT_UNDEFINED,               ISL_FORMAT_RAW),
   fmt(VK_FORMAT_R4G4_UNORM_PACK8,        ISL_FORMAT_UNSUPPORTED),
   fmt(VK_FORMAT_R4G4B4A4_UNORM_PACK16,   ISL_FORMAT_UNSUPPORTED),
   fmt(VK_FORMAT_B4G4R4A4_UNORM_PACK16,   ISL_FORMAT_UNSUPPORTED),
   fmt(VK_FORMAT_R5G6B5_UNORM_PACK16,     ISL_FORMAT_UNSUPPORTED),
   fmt(VK_FORMAT_B5G6R5_UNORM_PACK16,     ISL_FORMAT_UNSUPPORTED),
   fmt(VK_FORMAT_R5G5B5A1_UNORM_PACK16,   ISL_FORMAT_UNSUPPORTED),
   fmt(VK_FORMAT_B5G5R5A1_UNORM_PACK16,   ISL_FORMAT_UNSUPPORTED),
   fmt(VK_FORMAT_A1R5G5B5_UNORM_PACK16,   ISL_FORMAT_UNSUPPORTED),
   fmt(VK_FORMAT_R8_UNORM,                ISL_FORMAT_R8_UNORM),
   fmt(VK_FORMAT_R8_SNORM,                ISL_FORMAT_R8_SNORM),
   fmt(VK_FORMAT_R8_USCALED,              ISL_FORMAT_R8_USCALED),
   fmt(VK_FORMAT_R8_SSCALED,              ISL_FORMAT_R8_SSCALED),
   fmt(VK_FORMAT_R8_UINT,                 ISL_FORMAT_R8_UINT),
   fmt(VK_FORMAT_R8_SINT,                 ISL_FORMAT_R8_SINT),
   fmt(VK_FORMAT_R8_SRGB,                 ISL_FORMAT_UNSUPPORTED),
   fmt(VK_FORMAT_R8G8_UNORM,              ISL_FORMAT_R8G8_UNORM),
   fmt(VK_FORMAT_R8G8_SNORM,              ISL_FORMAT_R8G8_SNORM),
   fmt(VK_FORMAT_R8G8_USCALED,            ISL_FORMAT_R8G8_USCALED),
   fmt(VK_FORMAT_R8G8_SSCALED,            ISL_FORMAT_R8G8_SSCALED),
   fmt(VK_FORMAT_R8G8_UINT,               ISL_FORMAT_R8G8_UINT),
   fmt(VK_FORMAT_R8G8_SINT,               ISL_FORMAT_R8G8_SINT),
   fmt(VK_FORMAT_R8G8_SRGB,               ISL_FORMAT_UNSUPPORTED), /* L8A8_UNORM_SRGB */
   fmt(VK_FORMAT_R8G8B8_UNORM,            ISL_FORMAT_R8G8B8X8_UNORM),
   fmt(VK_FORMAT_R8G8B8_SNORM,            ISL_FORMAT_R8G8B8_SNORM),
   fmt(VK_FORMAT_R8G8B8_USCALED,          ISL_FORMAT_R8G8B8_USCALED),
   fmt(VK_FORMAT_R8G8B8_SSCALED,          ISL_FORMAT_R8G8B8_SSCALED),
   fmt(VK_FORMAT_R8G8B8_UINT,             ISL_FORMAT_R8G8B8_UINT),
   fmt(VK_FORMAT_R8G8B8_SINT,             ISL_FORMAT_R8G8B8_SINT),
   fmt(VK_FORMAT_R8G8B8_SRGB,             ISL_FORMAT_UNSUPPORTED), /* B8G8R8A8_UNORM_SRGB */
   fmt(VK_FORMAT_R8G8B8A8_UNORM,          ISL_FORMAT_R8G8B8A8_UNORM),
   fmt(VK_FORMAT_R8G8B8A8_SNORM,          ISL_FORMAT_R8G8B8A8_SNORM),
   fmt(VK_FORMAT_R8G8B8A8_USCALED,        ISL_FORMAT_R8G8B8A8_USCALED),
   fmt(VK_FORMAT_R8G8B8A8_SSCALED,        ISL_FORMAT_R8G8B8A8_SSCALED),
   fmt(VK_FORMAT_R8G8B8A8_UINT,           ISL_FORMAT_R8G8B8A8_UINT),
   fmt(VK_FORMAT_R8G8B8A8_SINT,           ISL_FORMAT_R8G8B8A8_SINT),
   fmt(VK_FORMAT_R8G8B8A8_SRGB,           ISL_FORMAT_R8G8B8A8_UNORM_SRGB),
   fmt(VK_FORMAT_A8B8G8R8_UNORM_PACK32,   ISL_FORMAT_R8G8B8A8_UNORM),
   fmt(VK_FORMAT_A8B8G8R8_SNORM_PACK32,   ISL_FORMAT_R8G8B8A8_SNORM),
   fmt(VK_FORMAT_A8B8G8R8_USCALED_PACK32, ISL_FORMAT_R8G8B8A8_USCALED),
   fmt(VK_FORMAT_A8B8G8R8_SSCALED_PACK32, ISL_FORMAT_R8G8B8A8_SSCALED),
   fmt(VK_FORMAT_A8B8G8R8_UINT_PACK32,    ISL_FORMAT_R8G8B8A8_UINT),
   fmt(VK_FORMAT_A8B8G8R8_SINT_PACK32,    ISL_FORMAT_R8G8B8A8_SINT),
   fmt(VK_FORMAT_A8B8G8R8_SRGB_PACK32,    ISL_FORMAT_R8G8B8A8_UNORM_SRGB),
   fmt(VK_FORMAT_A2R10G10B10_UNORM_PACK32, ISL_FORMAT_B10G10R10A2_UNORM),
   fmt(VK_FORMAT_A2R10G10B10_SNORM_PACK32, ISL_FORMAT_B10G10R10A2_SNORM),
   fmt(VK_FORMAT_A2R10G10B10_USCALED_PACK32, ISL_FORMAT_B10G10R10A2_USCALED),
   fmt(VK_FORMAT_A2R10G10B10_SSCALED_PACK32, ISL_FORMAT_B10G10R10A2_SSCALED),
   fmt(VK_FORMAT_A2R10G10B10_UINT_PACK32, ISL_FORMAT_B10G10R10A2_UINT),
   fmt(VK_FORMAT_A2R10G10B10_SINT_PACK32, ISL_FORMAT_B10G10R10A2_SINT),
   fmt(VK_FORMAT_A2B10G10R10_UNORM_PACK32, ISL_FORMAT_R10G10B10A2_UNORM),
   fmt(VK_FORMAT_A2B10G10R10_SNORM_PACK32, ISL_FORMAT_R10G10B10A2_SNORM),
   fmt(VK_FORMAT_A2B10G10R10_USCALED_PACK32, ISL_FORMAT_R10G10B10A2_USCALED),
   fmt(VK_FORMAT_A2B10G10R10_SSCALED_PACK32, ISL_FORMAT_R10G10B10A2_SSCALED),
   fmt(VK_FORMAT_A2B10G10R10_UINT_PACK32, ISL_FORMAT_R10G10B10A2_UINT),
   fmt(VK_FORMAT_A2B10G10R10_SINT_PACK32, ISL_FORMAT_R10G10B10A2_SINT),
   fmt(VK_FORMAT_R16_UNORM,               ISL_FORMAT_R16_UNORM),
   fmt(VK_FORMAT_R16_SNORM,               ISL_FORMAT_R16_SNORM),
   fmt(VK_FORMAT_R16_USCALED,             ISL_FORMAT_R16_USCALED),
   fmt(VK_FORMAT_R16_SSCALED,             ISL_FORMAT_R16_SSCALED),
   fmt(VK_FORMAT_R16_UINT,                ISL_FORMAT_R16_UINT),
   fmt(VK_FORMAT_R16_SINT,                ISL_FORMAT_R16_SINT),
   fmt(VK_FORMAT_R16_SFLOAT,              ISL_FORMAT_R16_FLOAT),
   fmt(VK_FORMAT_R16G16_UNORM,            ISL_FORMAT_R16G16_UNORM),
   fmt(VK_FORMAT_R16G16_SNORM,            ISL_FORMAT_R16G16_SNORM),
   fmt(VK_FORMAT_R16G16_USCALED,          ISL_FORMAT_R16G16_USCALED),
   fmt(VK_FORMAT_R16G16_SSCALED,          ISL_FORMAT_R16G16_SSCALED),
   fmt(VK_FORMAT_R16G16_UINT,             ISL_FORMAT_R16G16_UINT),
   fmt(VK_FORMAT_R16G16_SINT,             ISL_FORMAT_R16G16_SINT),
   fmt(VK_FORMAT_R16G16_SFLOAT,           ISL_FORMAT_R16G16_FLOAT),
   fmt(VK_FORMAT_R16G16B16_UNORM,         ISL_FORMAT_R16G16B16_UNORM),
   fmt(VK_FORMAT_R16G16B16_SNORM,         ISL_FORMAT_R16G16B16_SNORM),
   fmt(VK_FORMAT_R16G16B16_USCALED,       ISL_FORMAT_R16G16B16_USCALED),
   fmt(VK_FORMAT_R16G16B16_SSCALED,       ISL_FORMAT_R16G16B16_SSCALED),
   fmt(VK_FORMAT_R16G16B16_UINT,          ISL_FORMAT_R16G16B16_UINT),
   fmt(VK_FORMAT_R16G16B16_SINT,          ISL_FORMAT_R16G16B16_SINT),
   fmt(VK_FORMAT_R16G16B16_SFLOAT,        ISL_FORMAT_R16G16B16_FLOAT),
   fmt(VK_FORMAT_R16G16B16A16_UNORM,      ISL_FORMAT_R16G16B16A16_UNORM),
   fmt(VK_FORMAT_R16G16B16A16_SNORM,      ISL_FORMAT_R16G16B16A16_SNORM),
   fmt(VK_FORMAT_R16G16B16A16_USCALED,    ISL_FORMAT_R16G16B16A16_USCALED),
   fmt(VK_FORMAT_R16G16B16A16_SSCALED,    ISL_FORMAT_R16G16B16A16_SSCALED),
   fmt(VK_FORMAT_R16G16B16A16_UINT,       ISL_FORMAT_R16G16B16A16_UINT),
   fmt(VK_FORMAT_R16G16B16A16_SINT,       ISL_FORMAT_R16G16B16A16_SINT),
   fmt(VK_FORMAT_R16G16B16A16_SFLOAT,     ISL_FORMAT_R16G16B16A16_FLOAT),
   fmt(VK_FORMAT_R32_UINT,                ISL_FORMAT_R32_UINT,),
   fmt(VK_FORMAT_R32_SINT,                ISL_FORMAT_R32_SINT,),
   fmt(VK_FORMAT_R32_SFLOAT,              ISL_FORMAT_R32_FLOAT,),
   fmt(VK_FORMAT_R32G32_UINT,             ISL_FORMAT_R32G32_UINT,),
   fmt(VK_FORMAT_R32G32_SINT,             ISL_FORMAT_R32G32_SINT,),
   fmt(VK_FORMAT_R32G32_SFLOAT,           ISL_FORMAT_R32G32_FLOAT,),
   fmt(VK_FORMAT_R32G32B32_UINT,          ISL_FORMAT_R32G32B32_UINT,),
   fmt(VK_FORMAT_R32G32B32_SINT,          ISL_FORMAT_R32G32B32_SINT,),
   fmt(VK_FORMAT_R32G32B32_SFLOAT,        ISL_FORMAT_R32G32B32_FLOAT,),
   fmt(VK_FORMAT_R32G32B32A32_UINT,       ISL_FORMAT_R32G32B32A32_UINT,),
   fmt(VK_FORMAT_R32G32B32A32_SINT,       ISL_FORMAT_R32G32B32A32_SINT,),
   fmt(VK_FORMAT_R32G32B32A32_SFLOAT,     ISL_FORMAT_R32G32B32A32_FLOAT,),
   fmt(VK_FORMAT_R64_UINT,                ISL_FORMAT_R64_PASSTHRU),
   fmt(VK_FORMAT_R64_SINT,                ISL_FORMAT_R64_PASSTHRU),
   fmt(VK_FORMAT_R64_SFLOAT,              ISL_FORMAT_R64_FLOAT),
   fmt(VK_FORMAT_R64G64_UINT,             ISL_FORMAT_R64G64_PASSTHRU),
   fmt(VK_FORMAT_R64G64_SINT,             ISL_FORMAT_R64G64_PASSTHRU),
   fmt(VK_FORMAT_R64G64_SFLOAT,           ISL_FORMAT_R64G64_FLOAT),
   fmt(VK_FORMAT_R64G64B64_UINT,          ISL_FORMAT_R64G64B64_PASSTHRU),
   fmt(VK_FORMAT_R64G64B64_SINT,          ISL_FORMAT_R64G64B64_PASSTHRU),
   fmt(VK_FORMAT_R64G64B64_SFLOAT,        ISL_FORMAT_R64G64B64_FLOAT),
   fmt(VK_FORMAT_R64G64B64A64_UINT,       ISL_FORMAT_R64G64B64A64_PASSTHRU),
   fmt(VK_FORMAT_R64G64B64A64_SINT,       ISL_FORMAT_R64G64B64A64_PASSTHRU),
   fmt(VK_FORMAT_R64G64B64A64_SFLOAT,     ISL_FORMAT_R64G64B64A64_FLOAT),
   fmt(VK_FORMAT_B10G11R11_UFLOAT_PACK32, ISL_FORMAT_R11G11B10_FLOAT),
   fmt(VK_FORMAT_E5B9G9R9_UFLOAT_PACK32,  ISL_FORMAT_R9G9B9E5_SHAREDEXP),

   fmt(VK_FORMAT_D16_UNORM,               ISL_FORMAT_R16_UNORM,               .depth_format = D16_UNORM),
   fmt(VK_FORMAT_X8_D24_UNORM_PACK32,     ISL_FORMAT_R24_UNORM_X8_TYPELESS,   .depth_format = D24_UNORM_X8_UINT),
   fmt(VK_FORMAT_D32_SFLOAT,              ISL_FORMAT_R32_FLOAT,               .depth_format = D32_FLOAT),
   fmt(VK_FORMAT_S8_UINT,                 ISL_FORMAT_R8_UINT,                                                       .has_stencil = true),
   fmt(VK_FORMAT_D16_UNORM_S8_UINT,       ISL_FORMAT_R16_UNORM,               .depth_format = D16_UNORM,            .has_stencil = true),
   fmt(VK_FORMAT_D24_UNORM_S8_UINT,       ISL_FORMAT_R24_UNORM_X8_TYPELESS,   .depth_format = D24_UNORM_X8_UINT,    .has_stencil = true),
   fmt(VK_FORMAT_D32_SFLOAT_S8_UINT,      ISL_FORMAT_R32_FLOAT,               .depth_format = D32_FLOAT,            .has_stencil = true),

   fmt(VK_FORMAT_BC1_RGB_UNORM_BLOCK,     ISL_FORMAT_UNSUPPORTED),
   fmt(VK_FORMAT_BC1_RGB_SRGB_BLOCK,      ISL_FORMAT_UNSUPPORTED),
   fmt(VK_FORMAT_BC1_RGBA_UNORM_BLOCK,    ISL_FORMAT_UNSUPPORTED),
   fmt(VK_FORMAT_BC1_RGBA_SRGB_BLOCK,     ISL_FORMAT_UNSUPPORTED),
   fmt(VK_FORMAT_BC2_UNORM_BLOCK,         ISL_FORMAT_UNSUPPORTED),
   fmt(VK_FORMAT_BC2_SRGB_BLOCK,          ISL_FORMAT_UNSUPPORTED),
   fmt(VK_FORMAT_BC3_UNORM_BLOCK,         ISL_FORMAT_UNSUPPORTED),
   fmt(VK_FORMAT_BC3_SRGB_BLOCK,          ISL_FORMAT_UNSUPPORTED),
   fmt(VK_FORMAT_BC4_UNORM_BLOCK,         ISL_FORMAT_UNSUPPORTED),
   fmt(VK_FORMAT_BC4_SNORM_BLOCK,         ISL_FORMAT_UNSUPPORTED),
   fmt(VK_FORMAT_BC5_UNORM_BLOCK,         ISL_FORMAT_UNSUPPORTED),
   fmt(VK_FORMAT_BC5_SNORM_BLOCK,         ISL_FORMAT_UNSUPPORTED),
   fmt(VK_FORMAT_BC6H_UFLOAT_BLOCK,       ISL_FORMAT_UNSUPPORTED),
   fmt(VK_FORMAT_BC6H_SFLOAT_BLOCK,       ISL_FORMAT_UNSUPPORTED),
   fmt(VK_FORMAT_BC7_UNORM_BLOCK,         ISL_FORMAT_UNSUPPORTED),
   fmt(VK_FORMAT_BC7_SRGB_BLOCK,          ISL_FORMAT_UNSUPPORTED),
   fmt(VK_FORMAT_ETC2_R8G8B8_UNORM_BLOCK, ISL_FORMAT_ETC2_RGB8),
   fmt(VK_FORMAT_ETC2_R8G8B8_SRGB_BLOCK,  ISL_FORMAT_ETC2_SRGB8),
   fmt(VK_FORMAT_ETC2_R8G8B8A1_UNORM_BLOCK, ISL_FORMAT_ETC2_RGB8_PTA),
   fmt(VK_FORMAT_ETC2_R8G8B8A1_SRGB_BLOCK, ISL_FORMAT_ETC2_SRGB8_PTA),
   fmt(VK_FORMAT_ETC2_R8G8B8A8_UNORM_BLOCK, ISL_FORMAT_ETC2_EAC_RGBA8),
   fmt(VK_FORMAT_ETC2_R8G8B8A8_SRGB_BLOCK, ISL_FORMAT_ETC2_EAC_SRGB8_A8),
   fmt(VK_FORMAT_EAC_R11_UNORM_BLOCK,     ISL_FORMAT_EAC_R11),
   fmt(VK_FORMAT_EAC_R11_SNORM_BLOCK,     ISL_FORMAT_EAC_SIGNED_R11),
   fmt(VK_FORMAT_EAC_R11G11_UNORM_BLOCK,  ISL_FORMAT_EAC_RG11),
   fmt(VK_FORMAT_EAC_R11G11_SNORM_BLOCK,  ISL_FORMAT_EAC_SIGNED_RG11),
   fmt(VK_FORMAT_ASTC_4x4_UNORM_BLOCK,    ISL_FORMAT_UNSUPPORTED),
   fmt(VK_FORMAT_ASTC_4x4_SRGB_BLOCK,     ISL_FORMAT_UNSUPPORTED),
   fmt(VK_FORMAT_ASTC_5x4_UNORM_BLOCK,    ISL_FORMAT_UNSUPPORTED),
   fmt(VK_FORMAT_ASTC_5x4_SRGB_BLOCK,     ISL_FORMAT_UNSUPPORTED),
   fmt(VK_FORMAT_ASTC_5x5_UNORM_BLOCK,    ISL_FORMAT_UNSUPPORTED),
   fmt(VK_FORMAT_ASTC_5x5_SRGB_BLOCK,     ISL_FORMAT_UNSUPPORTED),
   fmt(VK_FORMAT_ASTC_6x5_UNORM_BLOCK,    ISL_FORMAT_UNSUPPORTED),
   fmt(VK_FORMAT_ASTC_6x5_SRGB_BLOCK,     ISL_FORMAT_UNSUPPORTED),
   fmt(VK_FORMAT_ASTC_6x6_UNORM_BLOCK,    ISL_FORMAT_UNSUPPORTED),
   fmt(VK_FORMAT_ASTC_6x6_SRGB_BLOCK,     ISL_FORMAT_UNSUPPORTED),
   fmt(VK_FORMAT_ASTC_8x5_UNORM_BLOCK,    ISL_FORMAT_UNSUPPORTED),
   fmt(VK_FORMAT_ASTC_8x5_SRGB_BLOCK,     ISL_FORMAT_UNSUPPORTED),
   fmt(VK_FORMAT_ASTC_8x6_UNORM_BLOCK,    ISL_FORMAT_UNSUPPORTED),
   fmt(VK_FORMAT_ASTC_8x6_SRGB_BLOCK,     ISL_FORMAT_UNSUPPORTED),
   fmt(VK_FORMAT_ASTC_8x8_UNORM_BLOCK,    ISL_FORMAT_UNSUPPORTED),
   fmt(VK_FORMAT_ASTC_8x8_SRGB_BLOCK,     ISL_FORMAT_UNSUPPORTED),
   fmt(VK_FORMAT_ASTC_10x5_UNORM_BLOCK,   ISL_FORMAT_UNSUPPORTED),
   fmt(VK_FORMAT_ASTC_10x5_SRGB_BLOCK,    ISL_FORMAT_UNSUPPORTED),
   fmt(VK_FORMAT_ASTC_10x6_UNORM_BLOCK,   ISL_FORMAT_UNSUPPORTED),
   fmt(VK_FORMAT_ASTC_10x6_SRGB_BLOCK,    ISL_FORMAT_UNSUPPORTED),
   fmt(VK_FORMAT_ASTC_10x8_UNORM_BLOCK,   ISL_FORMAT_UNSUPPORTED),
   fmt(VK_FORMAT_ASTC_10x8_SRGB_BLOCK,    ISL_FORMAT_UNSUPPORTED),
   fmt(VK_FORMAT_ASTC_10x10_UNORM_BLOCK,  ISL_FORMAT_UNSUPPORTED),
   fmt(VK_FORMAT_ASTC_10x10_SRGB_BLOCK,   ISL_FORMAT_UNSUPPORTED),
   fmt(VK_FORMAT_ASTC_12x10_UNORM_BLOCK,  ISL_FORMAT_UNSUPPORTED),
   fmt(VK_FORMAT_ASTC_12x10_SRGB_BLOCK,   ISL_FORMAT_UNSUPPORTED),
   fmt(VK_FORMAT_ASTC_12x12_UNORM_BLOCK,  ISL_FORMAT_UNSUPPORTED),
   fmt(VK_FORMAT_ASTC_12x12_SRGB_BLOCK,   ISL_FORMAT_UNSUPPORTED),
   fmt(VK_FORMAT_B8G8R8_UNORM,            ISL_FORMAT_UNSUPPORTED),
   fmt(VK_FORMAT_B8G8R8_SNORM,            ISL_FORMAT_UNSUPPORTED),
   fmt(VK_FORMAT_B8G8R8_USCALED,          ISL_FORMAT_UNSUPPORTED),
   fmt(VK_FORMAT_B8G8R8_SSCALED,          ISL_FORMAT_UNSUPPORTED),
   fmt(VK_FORMAT_B8G8R8_UINT,             ISL_FORMAT_UNSUPPORTED),
   fmt(VK_FORMAT_B8G8R8_SINT,             ISL_FORMAT_UNSUPPORTED),
   fmt(VK_FORMAT_B8G8R8_SRGB,             ISL_FORMAT_UNSUPPORTED),
   fmt(VK_FORMAT_B8G8R8A8_UNORM,          ISL_FORMAT_B8G8R8A8_UNORM),
   fmt(VK_FORMAT_B8G8R8A8_SNORM,          ISL_FORMAT_UNSUPPORTED),
   fmt(VK_FORMAT_B8G8R8A8_USCALED,        ISL_FORMAT_UNSUPPORTED),
   fmt(VK_FORMAT_B8G8R8A8_SSCALED,        ISL_FORMAT_UNSUPPORTED),
   fmt(VK_FORMAT_B8G8R8A8_UINT,           ISL_FORMAT_UNSUPPORTED),
   fmt(VK_FORMAT_B8G8R8A8_SINT,           ISL_FORMAT_UNSUPPORTED),
   fmt(VK_FORMAT_B8G8R8A8_SRGB,           ISL_FORMAT_B8G8R8A8_UNORM_SRGB),
};

#undef fmt

const struct anv_format *
anv_format_for_vk_format(VkFormat format)
{
   return &anv_formats[format];
}

/**
 * Exactly one bit must be set in \a aspect.
 */
enum isl_format
anv_get_isl_format(VkFormat format, VkImageAspectFlags aspect)
{
   const struct anv_format *anv_fmt = &anv_formats[format];

   switch (aspect) {
   case VK_IMAGE_ASPECT_COLOR_BIT:
      return anv_fmt->surface_format;
   case VK_IMAGE_ASPECT_DEPTH_BIT:
      assert(anv_fmt->depth_format != 0);
      return anv_fmt->surface_format;
   case VK_IMAGE_ASPECT_STENCIL_BIT:
      assert(anv_fmt->has_stencil);
      return ISL_FORMAT_R8_UINT;
   default:
      unreachable("bad VkImageAspect");
      return ISL_FORMAT_UNSUPPORTED;
   }
}

// Format capabilities

void anv_validate_GetPhysicalDeviceFormatProperties(
    VkPhysicalDevice                            physicalDevice,
    VkFormat                                    _format,
    VkFormatProperties*                         pFormatProperties)
{
   const struct anv_format *format = anv_format_for_vk_format(_format);
   fprintf(stderr, "vkGetFormatProperties(%s)\n", format->name);
   anv_GetPhysicalDeviceFormatProperties(physicalDevice, _format, pFormatProperties);
}

static void
anv_physical_device_get_format_properties(struct anv_physical_device *physical_device,
                                          const struct anv_format *format,
                                          VkFormatProperties *out_properties)
{
   const struct brw_surface_format_info *info;
   int gen;
   VkFormatFeatureFlags flags;

   assert(format != NULL);

   gen = physical_device->info->gen * 10;
   if (physical_device->info->is_haswell)
      gen += 5;

   if (format->surface_format== ISL_FORMAT_UNSUPPORTED)
      goto unsupported;

   uint32_t linear = 0, tiled = 0, buffer = 0;
   if (anv_format_is_depth_or_stencil(format)) {
      tiled |= VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT;
      if (physical_device->info->gen >= 8) {
         tiled |= VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT;
         tiled |= VK_FORMAT_FEATURE_BLIT_SRC_BIT;
      }
      if (format->depth_format) {
         tiled |= VK_FORMAT_FEATURE_BLIT_DST_BIT;
      }
   } else {
      /* The surface_formats table only contains color formats */
      info = &surface_formats[format->surface_format];
      if (!info->exists)
         goto unsupported;

      if (info->sampling <= gen) {
         flags = VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT |
                 VK_FORMAT_FEATURE_BLIT_SRC_BIT;
         linear |= flags;
         tiled |= flags;

         if (!isl_format_is_compressed(format->surface_format))
            buffer |= VK_FORMAT_FEATURE_UNIFORM_TEXEL_BUFFER_BIT;
      }
      if (info->render_target <= gen) {
         flags = VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BIT |
                 VK_FORMAT_FEATURE_BLIT_DST_BIT;
         linear |= flags;
         tiled |= flags;
      }
      if (info->alpha_blend <= gen) {
         linear |= VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BLEND_BIT;
         tiled |= VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BLEND_BIT;
      }
      if (info->input_vb <= gen) {
         buffer |= VK_FORMAT_FEATURE_VERTEX_BUFFER_BIT;
      }

      if (isl_is_storage_image_format(format->surface_format)) {
         tiled |= VK_FORMAT_FEATURE_STORAGE_IMAGE_BIT;
         linear |= VK_FORMAT_FEATURE_STORAGE_IMAGE_BIT;
         buffer |= VK_FORMAT_FEATURE_STORAGE_TEXEL_BUFFER_BIT;
      }

      if (format->surface_format == ISL_FORMAT_R32_SINT &&
          format->surface_format == ISL_FORMAT_R32_UINT) {
         tiled |= VK_FORMAT_FEATURE_STORAGE_IMAGE_ATOMIC_BIT;
         linear |= VK_FORMAT_FEATURE_STORAGE_IMAGE_ATOMIC_BIT;
         buffer |= VK_FORMAT_FEATURE_STORAGE_TEXEL_BUFFER_ATOMIC_BIT;
      }
   }

   out_properties->linearTilingFeatures = linear;
   out_properties->optimalTilingFeatures = tiled;
   out_properties->bufferFeatures = buffer;

   return;

 unsupported:
   out_properties->linearTilingFeatures = 0;
   out_properties->optimalTilingFeatures = 0;
   out_properties->bufferFeatures = 0;
}


void anv_GetPhysicalDeviceFormatProperties(
    VkPhysicalDevice                            physicalDevice,
    VkFormat                                    format,
    VkFormatProperties*                         pFormatProperties)
{
   ANV_FROM_HANDLE(anv_physical_device, physical_device, physicalDevice);

   anv_physical_device_get_format_properties(
               physical_device,
               anv_format_for_vk_format(format),
               pFormatProperties);
}

VkResult anv_GetPhysicalDeviceImageFormatProperties(
    VkPhysicalDevice                            physicalDevice,
    VkFormat                                    _format,
    VkImageType                                 type,
    VkImageTiling                               tiling,
    VkImageUsageFlags                           usage,
    VkImageCreateFlags                          flags,
    VkImageFormatProperties*                    pImageFormatProperties)
{
   ANV_FROM_HANDLE(anv_physical_device, physical_device, physicalDevice);
   const struct anv_format *format = anv_format_for_vk_format(_format);
   VkFormatProperties format_props;
   VkFormatFeatureFlags format_feature_flags;
   VkExtent3D maxExtent;
   uint32_t maxMipLevels;
   uint32_t maxArraySize;

   anv_physical_device_get_format_properties(physical_device, format,
                                             &format_props);

   /* Extract the VkFormatFeatureFlags that are relevant for the queried
    * tiling.
    */
   if (tiling == VK_IMAGE_TILING_LINEAR) {
      format_feature_flags = format_props.linearTilingFeatures;
   } else if (tiling == VK_IMAGE_TILING_OPTIMAL) {
      format_feature_flags = format_props.optimalTilingFeatures;
   } else {
      unreachable("bad VkImageTiling");
   }

   switch (type) {
   default:
      unreachable("bad VkImageType");
   case VK_IMAGE_TYPE_1D:
      maxExtent.width = 16384;
      maxExtent.height = 1;
      maxExtent.depth = 1;
      maxMipLevels = 15; /* log2(maxWidth) + 1 */
      maxArraySize = 2048;
      break;
   case VK_IMAGE_TYPE_2D:
      /* FINISHME: Does this really differ for cube maps? The documentation
       * for RENDER_SURFACE_STATE suggests so.
       */
      maxExtent.width = 16384;
      maxExtent.height = 16384;
      maxExtent.depth = 1;
      maxMipLevels = 15; /* log2(maxWidth) + 1 */
      maxArraySize = 2048;
      break;
   case VK_IMAGE_TYPE_3D:
      maxExtent.width = 2048;
      maxExtent.height = 2048;
      maxExtent.depth = 2048;
      maxMipLevels = 12; /* log2(maxWidth) + 1 */
      maxArraySize = 1;
      break;
   }

   if (usage & VK_IMAGE_USAGE_TRANSFER_SRC_BIT) {
      /* Meta implements transfers by sampling from the source image. */
      if (!(format_feature_flags & VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT)) {
         goto unsupported;
      }
   }

   if (usage & VK_IMAGE_USAGE_TRANSFER_DST_BIT) {
      if (format->has_stencil) {
         /* Not yet implemented because copying to a W-tiled surface is crazy
          * hard.
          */
         anv_finishme("support VK_IMAGE_USAGE_TRANSFER_DST_BIT for "
                      "stencil format");
         goto unsupported;
      }
   }

   if (usage & VK_IMAGE_USAGE_SAMPLED_BIT) {
      if (!(format_feature_flags & VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT)) {
         goto unsupported;
      }
   }

   if (usage & VK_IMAGE_USAGE_STORAGE_BIT) {
      if (!(format_feature_flags & VK_FORMAT_FEATURE_STORAGE_IMAGE_BIT)) {
         goto unsupported;
      }
   }

   if (usage & VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT) {
      if (!(format_feature_flags & VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BIT)) {
         goto unsupported;
      }
   }

   if (usage & VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT) {
      if (!(format_feature_flags & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT)) {
         goto unsupported;
      }
   }

   if (usage & VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT) {
      /* Nothing to check. */
   }

   if (usage & VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT) {
      /* Ignore this flag because it was removed from the
       * provisional_I_20150910 header.
       */
   }

   *pImageFormatProperties = (VkImageFormatProperties) {
      .maxExtent = maxExtent,
      .maxMipLevels = maxMipLevels,
      .maxArrayLayers = maxArraySize,

      /* FINISHME: Support multisampling */
      .sampleCounts = VK_SAMPLE_COUNT_1_BIT,

      /* FINISHME: Accurately calculate
       * VkImageFormatProperties::maxResourceSize.
       */
      .maxResourceSize = UINT32_MAX,
   };

   return VK_SUCCESS;

unsupported:
   *pImageFormatProperties = (VkImageFormatProperties) {
      .maxExtent = { 0, 0, 0 },
      .maxMipLevels = 0,
      .maxArrayLayers = 0,
      .sampleCounts = 0,
      .maxResourceSize = 0,
   };

   return VK_SUCCESS;
}

void anv_GetPhysicalDeviceSparseImageFormatProperties(
    VkPhysicalDevice                            physicalDevice,
    VkFormat                                    format,
    VkImageType                                 type,
    uint32_t                                    samples,
    VkImageUsageFlags                           usage,
    VkImageTiling                               tiling,
    uint32_t*                                   pNumProperties,
    VkSparseImageFormatProperties*              pProperties)
{
   /* Sparse images are not yet supported. */
   *pNumProperties = 0;
}