/*
 * GLRenderSystemProfiler.h
 * 
 * This file is part of the "LLGL" project (Copyright (c) 2015 by Lukas Hermanns)
 * See "LICENSE.txt" for license information.
 */

#ifndef __LLGL_GL_RENDER_SYSTEM_PROFILER_H__
#define __LLGL_GL_RENDER_SYSTEM_PROFILER_H__


#include "GLRenderSystem.h"
#include <LLGL/RenderingProfiler.h>


namespace LLGL
{


class GLRenderSystemProfiler : public GLRenderSystem
{

    public:

        GLRenderSystemProfiler(RenderingProfiler& profiler);

        /* ----- Render system ----- */

        RenderContext* CreateRenderContext(const RenderContextDescriptor& desc, const std::shared_ptr<Window>& window = nullptr) override;

        /* ----- Hardware buffers ------ */

        void UpdateVertexBuffer(VertexBuffer& vertexBuffer, const void* data, std::size_t dataSize, std::size_t offset) override;
        void UpdateIndexBuffer(IndexBuffer& indexBuffer, const void* data, std::size_t dataSize, std::size_t offset) override;
        void UpdateConstantBuffer(ConstantBuffer& constantBuffer, const void* data, std::size_t dataSize, std::size_t offset) override;
        void UpdateStorageBuffer(StorageBuffer& storageBuffer, const void* data, std::size_t dataSize, std::size_t offset) override;

    private:

        RenderingProfiler& profiler_;

};


} // /namespace LLGL


#endif



// ================================================================================
