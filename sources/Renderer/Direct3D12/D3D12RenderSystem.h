/*
 * D3D12RenderSystem.h
 * 
 * This file is part of the "LLGL" project (Copyright (c) 2015 by Lukas Hermanns)
 * See "LICENSE.txt" for license information.
 */

#ifndef __LLGL_D3D12_RENDER_SYSTEM_H__
#define __LLGL_D3D12_RENDER_SYSTEM_H__


#include <LLGL/RenderSystem.h>
#include <LLGL/VideoAdapter.h>
#include "D3D12RenderContext.h"

#include "RenderState/D3D12GraphicsPipeline.h"
#include "Shader/D3D12Shader.h"
#include "Shader/D3D12ShaderProgram.h"

#include "../ContainerTypes.h"
#include <d3d12.h>
#include <dxgi1_4.h>


namespace LLGL
{


class D3D12RenderSystem : public RenderSystem
{

    public:

        /* ----- Common ----- */

        D3D12RenderSystem();
        ~D3D12RenderSystem();

        std::map<RendererInfo, std::string> QueryRendererInfo() const override;

        RenderingCaps QueryRenderingCaps() const override;

        ShadingLanguage QueryShadingLanguage() const override;

        /* ----- Render Context ------ */

        RenderContext* CreateRenderContext(const RenderContextDescriptor& desc, const std::shared_ptr<Window>& window = nullptr) override;

        void Release(RenderContext& renderContext) override;

        /* ----- Hardware Buffers ------ */

        VertexBuffer* CreateVertexBuffer() override;
        IndexBuffer* CreateIndexBuffer() override;
        ConstantBuffer* CreateConstantBuffer() override;
        StorageBuffer* CreateStorageBuffer() override;

        void Release(VertexBuffer& vertexBuffer) override;
        void Release(IndexBuffer& indexBuffer) override;
        void Release(ConstantBuffer& constantBuffer) override;
        void Release(StorageBuffer& storageBuffer) override;

        void SetupVertexBuffer(VertexBuffer& vertexBuffer, const void* data, std::size_t dataSize, const BufferUsage usage, const VertexFormat& vertexFormat) override;
        void SetupIndexBuffer(IndexBuffer& indexBuffer, const void* data, std::size_t dataSize, const BufferUsage usage, const IndexFormat& indexFormat) override;
        void SetupConstantBuffer(ConstantBuffer& constantBuffer, const void* data, std::size_t dataSize, const BufferUsage usage) override;
        void SetupStorageBuffer(StorageBuffer& storageBuffer, const void* data, std::size_t dataSize, const BufferUsage usage) override;

        void UpdateVertexBuffer(VertexBuffer& vertexBuffer, const void* data, std::size_t dataSize, std::size_t offset) override;
        void UpdateIndexBuffer(IndexBuffer& indexBuffer, const void* data, std::size_t dataSize, std::size_t offset) override;
        void UpdateConstantBuffer(ConstantBuffer& constantBuffer, const void* data, std::size_t dataSize, std::size_t offset) override;
        void UpdateStorageBuffer(StorageBuffer& storageBuffer, const void* data, std::size_t dataSize, std::size_t offset) override;

        /* ----- Textures ----- */

        Texture* CreateTexture() override;

        void Release(Texture& texture) override;

        TextureDescriptor QueryTextureDescriptor(const Texture& texture) override;

        void WriteTexture1D(Texture& texture, const TextureFormat format, int size, const ImageDataDescriptor* imageDesc = nullptr) override;
        void WriteTexture2D(Texture& texture, const TextureFormat format, const Gs::Vector2i& size, const ImageDataDescriptor* imageDesc = nullptr) override;
        void WriteTexture3D(Texture& texture, const TextureFormat format, const Gs::Vector3i& size, const ImageDataDescriptor* imageDesc = nullptr) override;
        void WriteTextureCube(Texture& texture, const TextureFormat format, const Gs::Vector2i& size, const ImageDataDescriptor* imageDesc = nullptr) override;
        void WriteTexture1DArray(Texture& texture, const TextureFormat format, int size, unsigned int layers, const ImageDataDescriptor* imageDesc = nullptr) override;
        void WriteTexture2DArray(Texture& texture, const TextureFormat format, const Gs::Vector2i& size, unsigned int layers, const ImageDataDescriptor* imageDesc = nullptr) override;
        void WriteTextureCubeArray(Texture& texture, const TextureFormat format, const Gs::Vector2i& size, unsigned int layers, const ImageDataDescriptor* imageDesc = nullptr) override;
        
        void WriteTexture1DSub(Texture& texture, int mipLevel, int position, int size, const ImageDataDescriptor& imageDesc) override;
        void WriteTexture2DSub(Texture& texture, int mipLevel, const Gs::Vector2i& position, const Gs::Vector2i& size, const ImageDataDescriptor& imageDesc) override;
        void WriteTexture3DSub(Texture& texture, int mipLevel, const Gs::Vector3i& position, const Gs::Vector3i& size, const ImageDataDescriptor& imageDesc) override;

        void WriteTextureCubeSub(
            Texture& texture, int mipLevel, const Gs::Vector2i& position, const AxisDirection cubeFace,
            const Gs::Vector2i& size, const ImageDataDescriptor& imageDesc
        ) override;
        
        void WriteTexture1DArraySub(
            Texture& texture, int mipLevel, int position, unsigned int layerOffset,
            int size, unsigned int layers, const ImageDataDescriptor& imageDesc
        ) override;
        
        void WriteTexture2DArraySub(
            Texture& texture, int mipLevel, const Gs::Vector2i& position, unsigned int layerOffset,
            const Gs::Vector2i& size, unsigned int layers, const ImageDataDescriptor& imageDesc
        ) override;

        void WriteTextureCubeArraySub(
            Texture& texture, int mipLevel, const Gs::Vector2i& position, unsigned int layerOffset, const AxisDirection cubeFaceOffset,
            const Gs::Vector2i& size, unsigned int cubeFaces, const ImageDataDescriptor& imageDesc
        ) override;

        void ReadTexture(const Texture& texture, int mipLevel, ColorFormat dataFormat, DataType dataType, void* data) override;

        /* ----- Sampler States ---- */

        Sampler* CreateSampler(const SamplerDescriptor& desc) override;

        void Release(Sampler& sampler) override;

        /* ----- Render Targets ----- */

        RenderTarget* CreateRenderTarget(unsigned int multiSamples = 0) override;

        void Release(RenderTarget& renderTarget) override;

        /* ----- Shader ----- */

        Shader* CreateShader(const ShaderType type) override;
        ShaderProgram* CreateShaderProgram() override;

        void Release(Shader& shader) override;
        void Release(ShaderProgram& shaderProgram) override;

        /* ----- Pipeline States ----- */

        GraphicsPipeline* CreateGraphicsPipeline(const GraphicsPipelineDescriptor& desc) override;
        //ComputePipeline* CreateComputePipeline(const ComputePipelineDescriptor& desc) override;
        
        void Release(GraphicsPipeline& graphicsPipeline) override;

        /* ----- Queries ----- */

        Query* CreateQuery(const QueryType type) override;

        void Release(Query& query) override;

        /* ----- Extended internal functions ----- */

        ID3D12CommandQueue* CreateDXCommandQueue();
        ID3D12CommandAllocator* CreateDXCommandAllocator();
        ID3D12DescriptorHeap* CreateDXDescriptorHeap(const D3D12_DESCRIPTOR_HEAP_DESC& desc);
        IDXGISwapChain1* CreateDXSwapChain(const DXGI_SWAP_CHAIN_DESC1& desc, HWND wnd);

        //! Waits until the GPU has done all previous work.
        void SyncGPU(UINT64& fenceValue);

        inline D3D_FEATURE_LEVEL GetFeatureLevel() const
        {
            return featureLevel_;
        }

    private:

        void CreateFactory();
        void QueryVideoAdapters();
        void CreateDevice();
        bool CreateDevice(HRESULT& hr, IDXGIAdapter* adapter, const std::vector<D3D_FEATURE_LEVEL>& featureLevels);
        void CreateGPUSynchObjects();
        void CreateRootSignature();

        /* ----- Common objects ----- */

        IDXGIFactory4*                              factory_        = nullptr;
        ID3D12Device*                               device_         = nullptr;
        ID3D12CommandQueue*                         cmdQueue_       = nullptr;
        ID3D12RootSignature*                        rootSignature_  = nullptr;

        ID3D12Fence*                                fence_          = nullptr;
        HANDLE                                      fenceEvent_     = 0;

        D3D_FEATURE_LEVEL                           featureLevel_   = D3D_FEATURE_LEVEL_9_1;

        /* ----- Hardware object containers ----- */

        HWObjectContainer<D3D12RenderContext>       renderContexts_;
        
        /*HWObjectContainer<D3D12VertexBuffer>        vertexBuffers_;
        HWObjectContainer<D3D12IndexBuffer>         indexBuffers_;
        HWObjectContainer<D3D12ConstantBuffer>      constantBuffers_;

        HWObjectContainer<D3D12Texture>             textures_;
        HWObjectContainer<D3D12RenderTarget>        renderTargets_;*/

        HWObjectContainer<D3D12Shader>              shaders_;
        HWObjectContainer<D3D12ShaderProgram>       shaderPrograms_;

        HWObjectContainer<D3D12GraphicsPipeline>    graphicsPipelines_;
        //HWObjectContainer<D3D12Sampler>             samplers_;

        /* ----- Other members ----- */

        std::vector<VideoAdapterDescriptor>         videoAdatperDescs_;

};


} // /namespace LLGL


#endif



// ================================================================================
