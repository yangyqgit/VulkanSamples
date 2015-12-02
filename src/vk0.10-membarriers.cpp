/*
 * Vulkan Samples Kit
 *
 * Copyright (C) 2015 Valve Corporation
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

/*
VULKAN_SAMPLE_SHORT_DESCRIPTION
Use memory barriers to update texture
*/

/* Set up a vertex buffer and a texture and use them to draw a textured
 * quad.  Update the vertex buffer to draw to a different location and
 * use memory barriers to clear the texture to green.  Then use the updated
 * vertex buffer and texture to draw a second quad. The image should be
 * LunarG logo quad on the left and green quad on the right.
 */

#include <util_init.hpp>
#include <assert.h>
#include <string.h>
#include <cstdlib>
#include <cube_data.h>

static const VertexUV left_offset_vb_texture_Data[] =
{
    { XYZ1(  1,  0.5,  1 ), UV( 0.f, 1.f ) },
    { XYZ1(  2,  0.5,  1 ), UV( 1.f, 1.f ) },
    { XYZ1(  1, -0.5,  1 ), UV( 0.f, 0.f ) },
    { XYZ1(  1, -0.5,  1 ), UV( 0.f, 0.f ) },
    { XYZ1(  2,  0.5,  1 ), UV( 1.f, 1.f ) },
    { XYZ1(  2, -0.5,  1 ), UV( 1.f, 0.f ) },
};

static const VertexUV right_offset_vb_texture_Data[] =
{
    { XYZ1(  -2,  0.5,  1 ), UV( 0.f, 1.f ) },
    { XYZ1(  -1,  0.5,  1 ), UV( 1.f, 1.f ) },
    { XYZ1(  -2, -0.5,  1 ), UV( 0.f, 0.f ) },
    { XYZ1(  -2, -0.5,  1 ), UV( 0.f, 0.f ) },
    { XYZ1(  -1,  0.5,  1 ), UV( 1.f, 1.f ) },
    { XYZ1(  -1, -0.5,  1 ), UV( 1.f, 0.f ) },
};

#define DEPTH_PRESENT false

/* For this sample, we'll start with GLSL so the shader function is plain */
/* and then use the glslang GLSLtoSPV utility to convert it to SPIR-V for */
/* the driver.  We do this for clarity rather than using pre-compiled     */
/* SPIR-V                                                                 */

const char *vertShaderText =
        "#version 140\n"
        "#extension GL_ARB_separate_shader_objects : enable\n"
        "#extension GL_ARB_shading_language_420pack : enable\n"
        "layout (std140, binding = 0) uniform buf {\n"
        "        mat4 mvp;\n"
        "} ubuf;\n"
        "layout (location = 0) in vec4 pos;\n"
        "layout (location = 1) in vec2 inTexCoords;\n"
        "layout (location = 0) out vec2 texcoord;\n"
        "void main() {\n"
        "   texcoord = inTexCoords;\n"
        "   gl_Position = ubuf.mvp * pos;\n"
        "\n"
        "   // GL->VK conventions\n"
        "   gl_Position.y = -gl_Position.y;\n"
        "   gl_Position.z = (gl_Position.z + gl_Position.w) / 2.0;\n"
        "}\n";

const char *fragShaderText=
    "#version 140\n"
    "#extension GL_ARB_separate_shader_objects : enable\n"
    "#extension GL_ARB_shading_language_420pack : enable\n"
    "layout (binding = 1) uniform sampler2D tex;\n"
    "layout (location = 0) in vec2 texcoord;\n"
    "layout (location = 0) out vec4 outColor;\n"
    "void main() {\n"
    "   outColor = textureLod(tex, texcoord, 0.0);\n"
    "}\n";

int main(int argc, char **argv)
{
    VkResult U_ASSERT_ONLY res;
    struct sample_info info = {};
    char sample_title[] = "Draw Textured Cube";

    init_global_layer_properties(info);
    info.instance_extension_names.push_back(VK_KHR_SURFACE_EXTENSION_NAME);
#ifdef _WIN32
    info.instance_extension_names.push_back(VK_KHR_WIN32_SURFACE_EXTENSION_NAME);
#else
    info.instance_extension_names.push_back(VK_KHR_XCB_SURFACE_EXTENSION_NAME);
#endif
    info.device_extension_names.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
    init_instance(info, sample_title);
    init_enumerate_device(info);
    init_device(info);
    info.width = info.height = 500;
    init_connection(info);
    init_window(info);
    init_swapchain_extension(info);
    init_command_pool(info);
    init_command_buffer(info);
    execute_begin_command_buffer(info);
    init_device_queue(info);
    init_swap_chain(info);
    init_texture(info);
    init_uniform_buffer(info);
    init_descriptor_and_pipeline_layouts(info, true);
    init_renderpass(info, DEPTH_PRESENT, false);
    init_shaders(info, vertShaderText, fragShaderText);
    init_framebuffers(info, DEPTH_PRESENT);
    init_vertex_buffer(info, left_offset_vb_texture_Data, sizeof(left_offset_vb_texture_Data),
                       sizeof(left_offset_vb_texture_Data[0]), true);
    init_descriptor_pool(info, true);
    init_descriptor_set(info, true);
    init_pipeline_cache(info);
    init_pipeline(info, DEPTH_PRESENT);

    /* VULKAN_KEY_START */

    VkImageSubresourceRange srRange = {};
    srRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    srRange.baseMipLevel = 0;
    srRange.levelCount = VK_REMAINING_MIP_LEVELS;
    srRange.baseArrayLayer = 0;
    srRange.layerCount = VK_REMAINING_ARRAY_LAYERS;

    VkClearColorValue clear_color[1];
    clear_color[0].float32[0] = 0.2f;
    clear_color[0].float32[1] = 0.2f;
    clear_color[0].float32[2] = 0.2f;
    clear_color[0].float32[3] = 0.2f;

    /* We need to do the clear here instead of as a load op since we will use the same    */
    /* pipeline / renderpass multiple times in the frame                                  */
    vkCmdClearColorImage(info.cmd,
           info.buffers[info.current_buffer].image, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
           clear_color, 1, &srRange );

    VkSemaphore presentCompleteSemaphore;
    VkSemaphoreCreateInfo presentCompleteSemaphoreCreateInfo;
    presentCompleteSemaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    presentCompleteSemaphoreCreateInfo.pNext = NULL;
    presentCompleteSemaphoreCreateInfo.flags = 0;

    res = vkCreateSemaphore(info.device,
                            &presentCompleteSemaphoreCreateInfo,
                            NULL,
                            &presentCompleteSemaphore);
    assert(res == VK_SUCCESS);

    // Get the index of the next available swapchain image:
    res = info.fpAcquireNextImageKHR(info.device, info.swap_chain,
                                      UINT64_MAX,
                                      presentCompleteSemaphore,
                                      NULL,
                                      &info.current_buffer);
    // TODO: Deal with the VK_SUBOPTIMAL_KHR and VK_ERROR_OUT_OF_DATE_KHR
    // return codes
    assert(res == VK_SUCCESS);

    VkRenderPassBeginInfo rp_begin;
    rp_begin.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    rp_begin.pNext = NULL;
    rp_begin.renderPass = info.render_pass;
    rp_begin.framebuffer = info.framebuffers[info.current_buffer];
    rp_begin.renderArea.offset.x = 0;
    rp_begin.renderArea.offset.y = 0;
    rp_begin.renderArea.extent.width = info.width;
    rp_begin.renderArea.extent.height = info.height;
    rp_begin.clearValueCount = 0;
    rp_begin.pClearValues = NULL;

    /* Draw a textured quad on the left side of the window */
    vkCmdBeginRenderPass(info.cmd, &rp_begin, VK_SUBPASS_CONTENTS_INLINE);

    vkCmdBindPipeline(info.cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
                                  info.pipeline);
    vkCmdBindDescriptorSets(info.cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, info.pipeline_layout,
            0, NUM_DESCRIPTOR_SETS, info.desc_set.data(), 0, NULL);

    const VkDeviceSize offsets[1] = {0};
    vkCmdBindVertexBuffers(info.cmd, 0, 1, &info.vertex_buffer.buf, offsets);
    VkViewport viewport;
    viewport.height = (float) info.height;
    viewport.width = (float) info.width;
    viewport.minDepth = (float) 0.0f;
    viewport.maxDepth = (float) 1.0f;
    viewport.x = 0;
    viewport.y = 0;
    vkCmdSetViewport(info.cmd, NUM_VIEWPORTS, &viewport);

    VkRect2D scissor;
    scissor.extent.width = info.width;
    scissor.extent.height = info.height;
    scissor.offset.x = 0;
    scissor.offset.y = 0;
    vkCmdSetScissor(info.cmd, NUM_SCISSORS, &scissor);

    vkCmdDraw(info.cmd, 2 * 3, 1, 0, 0);
    vkCmdEndRenderPass(info.cmd);

    res = vkEndCommandBuffer(info.cmd);
    assert(res == VK_SUCCESS);

    const VkCommandBuffer cmd_bufs[] = { info.cmd };
    VkFenceCreateInfo fence_info;
    fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fence_info.pNext = NULL;
    fence_info.flags = 0;
    VkFence drawFence = {NULL};
    vkCreateFence(info.device, &fence_info, NULL, &drawFence);

    VkSubmitInfo submit_info[1] = {};
    submit_info[0].pNext = NULL;
    submit_info[0].sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info[0].waitSemaphoreCount = 1;
    /* Use the present complete semaphore to make sure our present buffer is ready */
    /* to render to                                                                */
    submit_info[0].pWaitSemaphores = &presentCompleteSemaphore;
    submit_info[0].commandBufferCount = 1;
    submit_info[0].pCommandBuffers = cmd_bufs;
    submit_info[0].signalSemaphoreCount = 0;
    submit_info[0].pSignalSemaphores = NULL;

    /* Queue the command buffer for execution */
    res = vkQueueSubmit(info.queue, 1, submit_info, drawFence);
    assert(res == VK_SUCCESS);

    /* Waiting for the fence should mean the shaders are done reading the texture */
    res = vkWaitForFences(info.device, 1, &drawFence, VK_TRUE, FENCE_TIMEOUT);
    assert(res == VK_SUCCESS);

    /* Now change the vertex buffer contents with vertices to draw to the right */
    uint8_t *pData;
    res = vkMapMemory(info.device, info.vertex_buffer.mem, 0, info.vertex_buffer.buffer_info.range, 0, (void **) &pData);
    assert(res == VK_SUCCESS);

    memcpy(pData, right_offset_vb_texture_Data, sizeof(right_offset_vb_texture_Data));

    vkUnmapMemory(info.device, info.vertex_buffer.mem);

    VkCommandBufferBeginInfo cmd_buf_info = {};
    cmd_buf_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    cmd_buf_info.pNext = NULL;
    cmd_buf_info.renderPass = VK_NULL_HANDLE;  /* May only set renderPass and framebuffer */
    cmd_buf_info.framebuffer = VK_NULL_HANDLE; /* for secondary command buffers           */
    cmd_buf_info.flags = 0;
    cmd_buf_info.subpass = 0;
    cmd_buf_info.occlusionQueryEnable = VK_FALSE;
    cmd_buf_info.queryFlags = 0;
    cmd_buf_info.pipelineStatistics = 0;
    res = vkBeginCommandBuffer(info.cmd, &cmd_buf_info);
    assert(res == VK_SUCCESS);

    /* Send a barrier to change the texture image's layout from SHADER_READ_ONLY */
    /* to COLOR_ATTACHMENT_OPTIMAL because we're going to clear it               */
    VkImageMemoryBarrier textureBarrier;
    textureBarrier = {};
    textureBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    textureBarrier.pNext = NULL;
    textureBarrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
    textureBarrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    textureBarrier.oldLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    textureBarrier.newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    textureBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    textureBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    textureBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    textureBarrier.subresourceRange.baseMipLevel = 0;
    textureBarrier.subresourceRange.levelCount = 1;
    textureBarrier.subresourceRange.baseArrayLayer = 0;
    textureBarrier.subresourceRange.layerCount = 1;
    textureBarrier.image = info.textures[0].image;
    VkImageMemoryBarrier *pmemory_barrier = &textureBarrier;
    vkCmdPipelineBarrier(info.cmd, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                         0, 1, (const void * const*)&pmemory_barrier);

    /* Clear the texture to all green */
    clear_color[0].float32[0] = 0.0f;
    clear_color[0].float32[1] = 1.0f;
    clear_color[0].float32[2] = 0.0f;
    clear_color[0].float32[3] = 1.0f;
    /* Clear texture to green */
    vkCmdClearColorImage(info.cmd,
           info.textures[0].image, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
           clear_color, 1, &srRange );

    /* Send a barrier to change the texture image's layout back to SHADER_READ_ONLY */
    /* because we're going to use it as a texture again                             */
    textureBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    textureBarrier.pNext = NULL;
    textureBarrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    textureBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    textureBarrier.oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    textureBarrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    textureBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    textureBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    textureBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    textureBarrier.subresourceRange.baseMipLevel = 0;
    textureBarrier.subresourceRange.levelCount = 1;
    textureBarrier.subresourceRange.baseArrayLayer = 0;
    textureBarrier.subresourceRange.layerCount = 1;
    textureBarrier.image = info.textures[0].image;
    vkCmdPipelineBarrier(info.cmd, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                         0, 1, (const void * const*)&pmemory_barrier);

    /* Draw the second quad to the right using the (now) green texture */
    vkCmdBeginRenderPass(info.cmd, &rp_begin, VK_SUBPASS_CONTENTS_INLINE);

    vkCmdBindPipeline(info.cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
                                  info.pipeline);
    vkCmdBindDescriptorSets(info.cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, info.pipeline_layout,
            0, NUM_DESCRIPTOR_SETS, info.desc_set.data(), 0, NULL);
    vkCmdBindVertexBuffers(info.cmd, 0, 1, &info.vertex_buffer.buf, offsets);
    vkCmdSetViewport(info.cmd, NUM_VIEWPORTS, &viewport);
    vkCmdSetScissor(info.cmd, NUM_SCISSORS, &scissor);

    vkCmdDraw(info.cmd, 2 * 3, 1, 0, 0);
    vkCmdEndRenderPass(info.cmd);

    /* Change the present buffer from COLOR_ATTACHMENT_OPTIMAL to PRESENT_SOURCE_KHR */
    /* so it can be presented                                                        */
    VkImageMemoryBarrier prePresentBarrier = {};
    prePresentBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    prePresentBarrier.pNext = NULL;
    prePresentBarrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    prePresentBarrier.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
    prePresentBarrier.oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    prePresentBarrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    prePresentBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    prePresentBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    prePresentBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    prePresentBarrier.subresourceRange.baseMipLevel = 0;
    prePresentBarrier.subresourceRange.levelCount = 1;
    prePresentBarrier.subresourceRange.baseArrayLayer = 0;
    prePresentBarrier.subresourceRange.layerCount = 1;
    prePresentBarrier.image = info.buffers[info.current_buffer].image;
    pmemory_barrier = &prePresentBarrier;
    vkCmdPipelineBarrier(info.cmd, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                         0, 1, (const void * const*)&pmemory_barrier);

    res = vkEndCommandBuffer(info.cmd);
    assert(res == VK_SUCCESS);

    submit_info[0].pNext = NULL;
    submit_info[0].sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info[0].waitSemaphoreCount = 0;
    submit_info[0].pWaitSemaphores = NULL;
    submit_info[0].commandBufferCount = 1;
    submit_info[0].pCommandBuffers = cmd_bufs;
    submit_info[0].signalSemaphoreCount = 0;
    submit_info[0].pSignalSemaphores = NULL;

    /* Use the same fence, so we need to reset */
    vkResetFences(info.device, 1, &drawFence);
    /* Queue the command buffer for execution */
    res = vkQueueSubmit(info.queue, 1, submit_info, drawFence);
    assert(res == VK_SUCCESS);

    /* Now present the image in the window */
    VkPresentInfoKHR present;
    present.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    present.pNext = NULL;
    present.swapchainCount = 1;
    present.pSwapchains = &info.swap_chain;
    present.pImageIndices = &info.current_buffer;
    present.pWaitSemaphores = NULL;
    present.waitSemaphoreCount = 0;
    present.pResults = NULL;

    /* Make sure command buffer is finished before presenting */
    res = vkWaitForFences(info.device, 1, &drawFence, VK_TRUE, FENCE_TIMEOUT);
    assert(res == VK_SUCCESS);
    res = info.fpQueuePresentKHR(info.queue, &present);
    assert(res == VK_SUCCESS);

    wait_seconds(1);
    /* VULKAN_KEY_END */

    vkDestroySemaphore(info.device, presentCompleteSemaphore, NULL);
    vkDestroyFence(info.device, drawFence, NULL);
    destroy_pipeline(info);
    destroy_pipeline_cache(info);
    destroy_texture(info);
    destroy_descriptor_pool(info);
    destroy_vertex_buffer(info);
    destroy_framebuffers(info);
    destroy_shaders(info);
    destroy_renderpass(info);
    destroy_descriptor_and_pipeline_layouts(info);
    destroy_uniform_buffer(info);
    destroy_swap_chain(info);
    destroy_command_buffer(info);
    destroy_command_pool(info);
    destroy_window(info);
    destroy_device(info);
    destroy_instance(info);
}

