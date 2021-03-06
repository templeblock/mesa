/**********************************************************
 * Copyright 2008-2009 VMware, Inc.  All rights reserved.
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use, copy,
 * modify, merge, publish, distribute, sublicense, and/or sell copies
 * of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 **********************************************************/

#include "util/u_inlines.h"
#include "util/u_prim.h"
#include "util/u_upload_mgr.h"
#include "indices/u_indices.h"

#include "svga_cmd.h"
#include "svga_draw.h"
#include "svga_draw_private.h"
#include "svga_resource_buffer.h"
#include "svga_winsys.h"
#include "svga_context.h"
#include "svga_hw_reg.h"


static enum pipe_error
translate_indices(struct svga_hwtnl *hwtnl, struct pipe_resource *src,
                  unsigned offset, enum pipe_prim_type prim, unsigned nr,
                  unsigned index_size,
                  u_translate_func translate, struct pipe_resource **out_buf)
{
   struct pipe_context *pipe = &hwtnl->svga->pipe;
   struct pipe_transfer *src_transfer = NULL;
   struct pipe_transfer *dst_transfer = NULL;
   unsigned size = index_size * nr;
   const void *src_map = NULL;
   struct pipe_resource *dst = NULL;
   void *dst_map = NULL;

   /* Need to trim vertex count to make sure we don't write too much data
    * to the dst buffer in the translate() call.
    */
   u_trim_pipe_prim(prim, &nr);

   size = index_size * nr;

   dst = pipe_buffer_create(pipe->screen,
                            PIPE_BIND_INDEX_BUFFER, PIPE_USAGE_DEFAULT, size);
   if (!dst)
      goto fail;

   src_map = pipe_buffer_map(pipe, src, PIPE_TRANSFER_READ, &src_transfer);
   if (!src_map)
      goto fail;

   dst_map = pipe_buffer_map(pipe, dst, PIPE_TRANSFER_WRITE, &dst_transfer);
   if (!dst_map)
      goto fail;

   translate((const char *) src_map + offset, 0, 0, nr, 0, dst_map);

   pipe_buffer_unmap(pipe, src_transfer);
   pipe_buffer_unmap(pipe, dst_transfer);

   *out_buf = dst;
   return PIPE_OK;

 fail:
   if (src_map)
      pipe_buffer_unmap(pipe, src_transfer);

   if (dst_map)
      pipe_buffer_unmap(pipe, dst_transfer);

   if (dst)
      pipe->screen->resource_destroy(pipe->screen, dst);

   return PIPE_ERROR_OUT_OF_MEMORY;
}


enum pipe_error
svga_hwtnl_simple_draw_range_elements(struct svga_hwtnl *hwtnl,
                                      struct pipe_resource *index_buffer,
                                      unsigned index_size, int index_bias,
                                      unsigned min_index, unsigned max_index,
                                      enum pipe_prim_type prim, unsigned start,
                                      unsigned count,
                                      unsigned start_instance,
                                      unsigned instance_count)
{
   SVGA3dPrimitiveRange range;
   unsigned hw_prim;
   unsigned hw_count;
   unsigned index_offset = start * index_size;

   hw_prim = svga_translate_prim(prim, count, &hw_count);
   if (hw_count == 0)
      return PIPE_OK; /* nothing to draw */

   range.primType = hw_prim;
   range.primitiveCount = hw_count;
   range.indexArray.offset = index_offset;
   range.indexArray.stride = index_size;
   range.indexWidth = index_size;
   range.indexBias = index_bias;

   return svga_hwtnl_prim(hwtnl, &range, count,
                          min_index, max_index, index_buffer,
                          start_instance, instance_count);
}


enum pipe_error
svga_hwtnl_draw_range_elements(struct svga_hwtnl *hwtnl,
                               struct pipe_resource *index_buffer,
                               unsigned index_size, int index_bias,
                               unsigned min_index, unsigned max_index,
                               enum pipe_prim_type prim, unsigned start, unsigned count,
                               unsigned start_instance, unsigned instance_count)
{
   enum pipe_prim_type gen_prim;
   unsigned gen_size, gen_nr;
   enum indices_mode gen_type;
   u_translate_func gen_func;
   enum pipe_error ret = PIPE_OK;

   SVGA_STATS_TIME_PUSH(svga_sws(hwtnl->svga),
                        SVGA_STATS_TIME_HWTNLDRAWELEMENTS);

   if (svga_need_unfilled_fallback(hwtnl, prim)) {
      gen_type = u_unfilled_translator(prim,
                                       index_size,
                                       count,
                                       hwtnl->api_fillmode,
                                       &gen_prim,
                                       &gen_size, &gen_nr, &gen_func);
   }
   else {
      gen_type = u_index_translator(svga_hw_prims,
                                    prim,
                                    index_size,
                                    count,
                                    hwtnl->api_pv,
                                    hwtnl->hw_pv,
                                    PR_DISABLE,
                                    &gen_prim, &gen_size, &gen_nr, &gen_func);
   }

   if (gen_type == U_TRANSLATE_MEMCPY) {
      /* No need for translation, just pass through to hardware:
       */
      ret = svga_hwtnl_simple_draw_range_elements(hwtnl, index_buffer,
                                                   index_size,
                                                   index_bias,
                                                   min_index,
                                                   max_index,
                                                   gen_prim, start, count,
                                                   start_instance,
                                                   instance_count);
   }
   else {
      struct pipe_resource *gen_buf = NULL;

      /* Need to allocate a new index buffer and run the translate
       * func to populate it.  Could potentially cache this translated
       * index buffer with the original to avoid future
       * re-translations.  Not much point if we're just accelerating
       * GL though, as index buffers are typically used only once
       * there.
       */
      ret = translate_indices(hwtnl,
                              index_buffer,
                              start * index_size,
                              gen_prim, gen_nr, gen_size, gen_func, &gen_buf);
      if (ret != PIPE_OK)
         goto done;

      ret = svga_hwtnl_simple_draw_range_elements(hwtnl,
                                                  gen_buf,
                                                  gen_size,
                                                  index_bias,
                                                  min_index,
                                                  max_index,
                                                  gen_prim, 0, gen_nr,
                                                  start_instance,
                                                  instance_count);
done:
      if (gen_buf)
         pipe_resource_reference(&gen_buf, NULL);
   }

   SVGA_STATS_TIME_POP(svga_sws(hwtnl->svga));
   return ret;
}
