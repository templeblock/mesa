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

#include <assert.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>

#include "anv_private.h"

#include "gen7_pack.h"
#include "gen75_pack.h"

static const uint8_t
anv_surftype(const struct anv_image *image, VkImageViewType view_type,
             bool storage)
{
   switch (view_type) {
   default:
      unreachable("bad VkImageViewType");
   case VK_IMAGE_VIEW_TYPE_1D:
   case VK_IMAGE_VIEW_TYPE_1D_ARRAY:
      assert(image->type == VK_IMAGE_TYPE_1D);
      return SURFTYPE_1D;
   case VK_IMAGE_VIEW_TYPE_CUBE:
   case VK_IMAGE_VIEW_TYPE_CUBE_ARRAY:
      assert(image->type == VK_IMAGE_TYPE_2D);
      return storage ? SURFTYPE_2D : SURFTYPE_CUBE;
   case VK_IMAGE_VIEW_TYPE_2D:
   case VK_IMAGE_VIEW_TYPE_2D_ARRAY:
      assert(image->type == VK_IMAGE_TYPE_2D);
      return SURFTYPE_2D;
   case VK_IMAGE_VIEW_TYPE_3D:
      assert(image->type == VK_IMAGE_TYPE_3D);
      return SURFTYPE_3D;
   }
}

GENX_FUNC(GEN7, GEN75) void
genX(fill_buffer_surface_state)(void *state, enum isl_format format,
                                uint32_t offset, uint32_t range,
                                uint32_t stride)
{
   uint32_t num_elements = range / stride;

   struct GENX(RENDER_SURFACE_STATE) surface_state = {
      .SurfaceType                              = SURFTYPE_BUFFER,
      .SurfaceFormat                            = format,
      .SurfaceVerticalAlignment                 = VALIGN_4,
      .SurfaceHorizontalAlignment               = HALIGN_4,
      .TiledSurface                             = false,
      .RenderCacheReadWriteMode                 = false,
      .SurfaceObjectControlState                = GENX(MOCS),
      .Height                                   = (num_elements >> 7) & 0x3fff,
      .Width                                    = num_elements & 0x7f,
      .Depth                                    = (num_elements >> 21) & 0x3f,
      .SurfacePitch                             = stride - 1,
#  if (ANV_IS_HASWELL)
      .ShaderChannelSelectR                     = SCS_RED,
      .ShaderChannelSelectG                     = SCS_GREEN,
      .ShaderChannelSelectB                     = SCS_BLUE,
      .ShaderChannelSelectA                     = SCS_ALPHA,
#  endif
      .SurfaceBaseAddress                       = { NULL, offset },
   };

   GENX(RENDER_SURFACE_STATE_pack)(NULL, state, &surface_state);
}

static const uint32_t vk_to_gen_tex_filter[] = {
   [VK_FILTER_NEAREST]                          = MAPFILTER_NEAREST,
   [VK_FILTER_LINEAR]                           = MAPFILTER_LINEAR
};

static const uint32_t vk_to_gen_mipmap_mode[] = {
   [VK_SAMPLER_MIPMAP_MODE_BASE]                = MIPFILTER_NONE,
   [VK_SAMPLER_MIPMAP_MODE_NEAREST]             = MIPFILTER_NEAREST,
   [VK_SAMPLER_MIPMAP_MODE_LINEAR]              = MIPFILTER_LINEAR
};

static const uint32_t vk_to_gen_tex_address[] = {
   [VK_SAMPLER_ADDRESS_MODE_REPEAT]             = TCM_WRAP,
   [VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT]    = TCM_MIRROR,
   [VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE]      = TCM_CLAMP,
   [VK_SAMPLER_ADDRESS_MODE_MIRROR_CLAMP_TO_EDGE] = TCM_MIRROR_ONCE,
   [VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER]    = TCM_CLAMP_BORDER,
};

static const uint32_t vk_to_gen_compare_op[] = {
   [VK_COMPARE_OP_NEVER]                        = PREFILTEROPNEVER,
   [VK_COMPARE_OP_LESS]                         = PREFILTEROPLESS,
   [VK_COMPARE_OP_EQUAL]                        = PREFILTEROPEQUAL,
   [VK_COMPARE_OP_LESS_OR_EQUAL]                = PREFILTEROPLEQUAL,
   [VK_COMPARE_OP_GREATER]                      = PREFILTEROPGREATER,
   [VK_COMPARE_OP_NOT_EQUAL]                    = PREFILTEROPNOTEQUAL,
   [VK_COMPARE_OP_GREATER_OR_EQUAL]             = PREFILTEROPGEQUAL,
   [VK_COMPARE_OP_ALWAYS]                       = PREFILTEROPALWAYS,
};

static struct anv_state
alloc_surface_state(struct anv_device *device,
                    struct anv_cmd_buffer *cmd_buffer)
{
      if (cmd_buffer) {
         return anv_cmd_buffer_alloc_surface_state(cmd_buffer);
      } else {
         return anv_state_pool_alloc(&device->surface_state_pool, 64, 64);
      }
}

VkResult genX(CreateSampler)(
    VkDevice                                    _device,
    const VkSamplerCreateInfo*                  pCreateInfo,
    const VkAllocationCallbacks*                pAllocator,
    VkSampler*                                  pSampler)
{
   ANV_FROM_HANDLE(anv_device, device, _device);
   struct anv_sampler *sampler;
   uint32_t mag_filter, min_filter, max_anisotropy;

   assert(pCreateInfo->sType == VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO);

   sampler = anv_alloc2(&device->alloc, pAllocator, sizeof(*sampler), 8,
                        VK_SYSTEM_ALLOCATION_SCOPE_OBJECT);
   if (!sampler)
      return vk_error(VK_ERROR_OUT_OF_HOST_MEMORY);

   if (pCreateInfo->maxAnisotropy > 1) {
      mag_filter = MAPFILTER_ANISOTROPIC;
      min_filter = MAPFILTER_ANISOTROPIC;
      max_anisotropy = (pCreateInfo->maxAnisotropy - 2) / 2;
   } else {
      mag_filter = vk_to_gen_tex_filter[pCreateInfo->magFilter];
      min_filter = vk_to_gen_tex_filter[pCreateInfo->minFilter];
      max_anisotropy = RATIO21;
   }

   struct GEN7_SAMPLER_STATE sampler_state = {
      .SamplerDisable = false,
      .TextureBorderColorMode = DX10OGL,
      .BaseMipLevel = 0.0,
      .MipModeFilter = vk_to_gen_mipmap_mode[pCreateInfo->mipmapMode],
      .MagModeFilter = mag_filter,
      .MinModeFilter = min_filter,
      .TextureLODBias = pCreateInfo->mipLodBias * 256,
      .AnisotropicAlgorithm = EWAApproximation,
      .MinLOD = pCreateInfo->minLod,
      .MaxLOD = pCreateInfo->maxLod,
      .ChromaKeyEnable = 0,
      .ChromaKeyIndex = 0,
      .ChromaKeyMode = 0,
      .ShadowFunction = vk_to_gen_compare_op[pCreateInfo->compareOp],
      .CubeSurfaceControlMode = 0,

      .BorderColorPointer =
         device->border_colors.offset +
         pCreateInfo->borderColor * sizeof(float) * 4,

      .MaximumAnisotropy = max_anisotropy,
      .RAddressMinFilterRoundingEnable = 0,
      .RAddressMagFilterRoundingEnable = 0,
      .VAddressMinFilterRoundingEnable = 0,
      .VAddressMagFilterRoundingEnable = 0,
      .UAddressMinFilterRoundingEnable = 0,
      .UAddressMagFilterRoundingEnable = 0,
      .TrilinearFilterQuality = 0,
      .NonnormalizedCoordinateEnable = pCreateInfo->unnormalizedCoordinates,
      .TCXAddressControlMode = vk_to_gen_tex_address[pCreateInfo->addressModeU],
      .TCYAddressControlMode = vk_to_gen_tex_address[pCreateInfo->addressModeV],
      .TCZAddressControlMode = vk_to_gen_tex_address[pCreateInfo->addressModeW],
   };

   GEN7_SAMPLER_STATE_pack(NULL, sampler->state, &sampler_state);

   *pSampler = anv_sampler_to_handle(sampler);

   return VK_SUCCESS;
}

static const uint8_t anv_halign[] = {
    [4] = HALIGN_4,
    [8] = HALIGN_8,
};

static const uint8_t anv_valign[] = {
    [2] = VALIGN_2,
    [4] = VALIGN_4,
};

static const uint32_t vk_to_gen_swizzle_map[] = {
   [VK_COMPONENT_SWIZZLE_ZERO]                 = SCS_ZERO,
   [VK_COMPONENT_SWIZZLE_ONE]                  = SCS_ONE,
   [VK_COMPONENT_SWIZZLE_R]                    = SCS_RED,
   [VK_COMPONENT_SWIZZLE_G]                    = SCS_GREEN,
   [VK_COMPONENT_SWIZZLE_B]                    = SCS_BLUE,
   [VK_COMPONENT_SWIZZLE_A]                    = SCS_ALPHA
};

static inline uint32_t
vk_to_gen_swizzle(VkComponentSwizzle swizzle, VkComponentSwizzle component)
{
   if (swizzle == VK_COMPONENT_SWIZZLE_IDENTITY)
      return vk_to_gen_swizzle_map[component];
   else
      return vk_to_gen_swizzle_map[swizzle];
}

GENX_FUNC(GEN7, GEN75) void
genX(image_view_init)(struct anv_image_view *iview,
                      struct anv_device *device,
                      const VkImageViewCreateInfo* pCreateInfo,
                      struct anv_cmd_buffer *cmd_buffer)
{
   ANV_FROM_HANDLE(anv_image, image, pCreateInfo->image);

   const VkImageSubresourceRange *range = &pCreateInfo->subresourceRange;

   struct anv_surface *surface =
      anv_image_get_surface_for_aspect_mask(image, range->aspectMask);

   const struct anv_format *format =
      anv_format_for_vk_format(pCreateInfo->format);

   if (pCreateInfo->viewType != VK_IMAGE_VIEW_TYPE_2D)
      anv_finishme("non-2D image views");

   iview->image = image;
   iview->bo = image->bo;
   iview->offset = image->offset + surface->offset;
   iview->format = anv_format_for_vk_format(pCreateInfo->format);

   iview->extent = (VkExtent3D) {
      .width = anv_minify(image->extent.width, range->baseMipLevel),
      .height = anv_minify(image->extent.height, range->baseMipLevel),
      .depth = anv_minify(image->extent.depth, range->baseMipLevel),
   };

   uint32_t depth = 1;
   if (range->layerCount > 1) {
      depth = range->layerCount;
   } else if (image->extent.depth > 1) {
      depth = image->extent.depth;
   }

   const struct isl_extent3d image_align_sa =
      isl_surf_get_image_alignment_sa(&surface->isl);

   struct GENX(RENDER_SURFACE_STATE) surface_state = {
      .SurfaceType = anv_surftype(image, pCreateInfo->viewType, false),
      .SurfaceArray = image->array_size > 1,
      .SurfaceFormat = format->surface_format,
      .SurfaceVerticalAlignment = anv_valign[image_align_sa.height],
      .SurfaceHorizontalAlignment = anv_halign[image_align_sa.width],

      /* From bspec (DevSNB, DevIVB): "Set Tile Walk to TILEWALK_XMAJOR if
       * Tiled Surface is False."
       */
      .TiledSurface = surface->isl.tiling != ISL_TILING_LINEAR,
      .TileWalk = surface->isl.tiling == ISL_TILING_Y0 ?
                  TILEWALK_YMAJOR : TILEWALK_XMAJOR,

      .VerticalLineStride = 0,
      .VerticalLineStrideOffset = 0,

      .RenderCacheReadWriteMode = 0, /* TEMPLATE */

      .Height = image->extent.height - 1,
      .Width = image->extent.width - 1,
      .Depth = depth - 1,
      .SurfacePitch = surface->isl.row_pitch - 1,
      .MinimumArrayElement = range->baseArrayLayer,
      .NumberofMultisamples = MULTISAMPLECOUNT_1,
      .XOffset = 0,
      .YOffset = 0,

      .SurfaceObjectControlState = GENX(MOCS),

      .MIPCountLOD = 0, /* TEMPLATE */
      .SurfaceMinLOD = 0, /* TEMPLATE */

      .MCSEnable = false,
#  if (ANV_IS_HASWELL)
      .ShaderChannelSelectR = vk_to_gen_swizzle(pCreateInfo->components.r,
                                                VK_COMPONENT_SWIZZLE_R),
      .ShaderChannelSelectG = vk_to_gen_swizzle(pCreateInfo->components.g,
                                                VK_COMPONENT_SWIZZLE_G),
      .ShaderChannelSelectB = vk_to_gen_swizzle(pCreateInfo->components.b,
                                                VK_COMPONENT_SWIZZLE_B),
      .ShaderChannelSelectA = vk_to_gen_swizzle(pCreateInfo->components.a,
                                                VK_COMPONENT_SWIZZLE_A),
#  else /* XXX: Seriously? */
      .RedClearColor = 0,
      .GreenClearColor = 0,
      .BlueClearColor = 0,
      .AlphaClearColor = 0,
#  endif
      .ResourceMinLOD = 0.0,
      .SurfaceBaseAddress = { NULL, iview->offset },
   };

   if (image->needs_nonrt_surface_state) {
      iview->nonrt_surface_state = alloc_surface_state(device, cmd_buffer);

      surface_state.RenderCacheReadWriteMode = false;

      /* For non render target surfaces, the hardware interprets field
       * MIPCount/LOD as MIPCount.  The range of levels accessible by the
       * sampler engine is [SurfaceMinLOD, SurfaceMinLOD + MIPCountLOD].
       */
      surface_state.SurfaceMinLOD = range->baseMipLevel;
      surface_state.MIPCountLOD = MAX2(range->levelCount, 1) - 1;

      GENX(RENDER_SURFACE_STATE_pack)(NULL, iview->nonrt_surface_state.map,
                                      &surface_state);

      if (!device->info.has_llc)
         anv_state_clflush(iview->nonrt_surface_state);
   }

   if (image->needs_color_rt_surface_state) {
      iview->color_rt_surface_state = alloc_surface_state(device, cmd_buffer);

      surface_state.RenderCacheReadWriteMode = 0; /* Write only */

      /* For render target surfaces, the hardware interprets field MIPCount/LOD as
       * LOD. The Broadwell PRM says:
       *
       *    MIPCountLOD defines the LOD that will be rendered into.
       *    SurfaceMinLOD is ignored.
       */
      surface_state.MIPCountLOD = range->baseMipLevel;
      surface_state.SurfaceMinLOD = 0;

      GENX(RENDER_SURFACE_STATE_pack)(NULL, iview->color_rt_surface_state.map,
                                      &surface_state);
      if (!device->info.has_llc)
         anv_state_clflush(iview->color_rt_surface_state);
   }

   if (image->needs_storage_surface_state) {
      iview->storage_surface_state = alloc_surface_state(device, cmd_buffer);

      surface_state.SurfaceType =
         anv_surftype(image, pCreateInfo->viewType, true),

      surface_state.SurfaceFormat =
         isl_lower_storage_image_format(&device->isl_dev,
                                        format->surface_format);

      surface_state.SurfaceMinLOD = range->baseMipLevel;
      surface_state.MIPCountLOD = MAX2(range->levelCount, 1) - 1;

      GENX(RENDER_SURFACE_STATE_pack)(NULL, iview->storage_surface_state.map,
                                      &surface_state);
   }
}