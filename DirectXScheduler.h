#pragma once

#include <thread>
#include <mutex>
#include <atomic>
#include <sstream>
#include <functional>
#include <queue>

#include "SynchronousCoroutine.h"
#include "ResumableCoroutine.h"
#include "ShaderStructures.h"
#include "DataProducingCoroutine.h"
#include "VoidCoroutine.h"

#include <d3d11.h>
#include <d2d1.h>
#include <dwrite.h>
#include <d2d1_2.h>
#include <dwrite_2.h>
#include <wincodec.h>
#include <dxgi1_3.h>
#include <dxgi1_4.h>
#include <d3d11_4.h>

#include "DirectXHelper.h"
#include "Utilities.h"
#include "ShaderStructures.h"
#include "BaserCoroutine.h"
#include "DirectXVertexShader.h"
#include "ResumableDataProducingCoroutine.h"

namespace ndtech {

    struct D2DTextWriter {
        winrt::com_ptr<ID2D1SolidColorBrush>        brush;
        winrt::com_ptr<IDWriteTextFormat>           textFormat = nullptr;
    };

    struct DirectXTexture {
        winrt::com_ptr<ID3D11SamplerState>          pointSampler;
        winrt::com_ptr<ID3D11Texture2D>             texture;
        winrt::com_ptr<ID3D11ShaderResourceView>    shaderResourceView;
        winrt::com_ptr<ID3D11RenderTargetView>      renderTargetView;
        winrt::com_ptr<ID2D1RenderTarget>           d2dRenderTarget;
        winrt::com_ptr<ID3D11DepthStencilView>      m_d3dDepthStencilView = nullptr;
        const unsigned int                          textureWidth;
        const unsigned int                          textureHeight;
    };

    struct DirectXBillboard {
        winrt::com_ptr<ID3D11VertexShader>                  vertexShader = nullptr;
        winrt::com_ptr<ID3D11InputLayout>                   inputLayout = nullptr;
        winrt::com_ptr<ID3D11PixelShader>                   pixelShader = nullptr;
        winrt::com_ptr<ID3D11Buffer>                        modelConstantBuffer = nullptr;
        winrt::com_ptr<ID3D11GeometryShader>                geometryShader = nullptr;
        winrt::com_ptr<ID3D11Buffer>                        vertexBuffer = nullptr;
        winrt::com_ptr<ID3D11Buffer>                        indexBuffer = nullptr;            
        // Direct3D resources for the default textureView.
        winrt::com_ptr<ID3D11Resource>                      texture = nullptr;
        winrt::com_ptr<ID3D11ShaderResourceView>            placeholderTextureView = nullptr;
        winrt::com_ptr<ID3D11SamplerState>                  quadTextureSamplerState = nullptr;
        winrt::com_ptr<ID3D11ShaderResourceView>            quadTextureView = nullptr;
        // System resources for quad geometry.
        ModelConstantBuffer                                 modelConstantBufferData;
        uint32_t                                            indexCount = 0;
        uint32_t                                            vertexStride = 0;
        float                                               degreesPerSecond = 45.f;
        winrt::Windows::Foundation::Numerics::float3        position = { 0.f, 0.f, -2.f };
        winrt::Windows::Foundation::Numerics::float3        lastPosition = { 0.f, 0.f, -2.f };
        winrt::Windows::Foundation::Numerics::float3        velocity = { 0.f, 0.f,  0.f };
        // This is the rate at which the hologram position is interpolated (LERPed) to the current location.
        const float                                         lerpRate = 4.0f;
        // Number of seconds it takes to fade the hologram in, or out.
        const float                                         maxFadeTime = 1.f;
        // Timer used to fade the hologram in, or out.
        float                                               fadeTime = 0.f;
        // Whether or not the hologram is fading in, or out.
        bool                                                fadingIn = false;
    };

    struct DirectXScheduler {
        DirectXScheduler();
        DirectXScheduler(const DirectXScheduler&) = delete;
        DirectXScheduler& operator=(const DirectXScheduler&) = delete;
        DirectXScheduler(DirectXScheduler&&) = delete;
        ~DirectXScheduler();


        SynchronousCoroutine<int> RunDirectXWorker();
        ResumableCoroutine<int> DoWork();
        void PrepareHolographicSpace(winrt::Windows::Graphics::Holographic::HolographicSpace holographicSpace);
        void CreateDeviceIndependentResources();
        void InitializeUsingHolographicSpace();
        void CreateDeviceResources();
        //winrt::Windows::Foundation::IAsyncAction CreateDeviceDependentResources();

        void AddImmediateDirectXTask(std::function<void(void)> task);

        void AddVertexShader(std::wstring shaderFileName);
        VoidCoroutine InitializeVertexShader(DirectXVertexShader& vertexShader);
        void AddTextWriter(D2DTextWriter textWriter);
        void AddTexture(DirectXTexture texture);
        void AddBillboard(DirectXBillboard billboard);

        // If the current D3D Device supports VPRT, we can avoid using a geometry
        // shader just to set the render target array index.
        bool                                                usingVprtShaders = false;

    private:
        size_t                                                      m_cache_line_size;
        std::thread                                                 m_directXThread;
        // The holographic space the app will use for rendering.
        winrt::Windows::Graphics::Holographic::HolographicSpace     m_holographicSpace = nullptr;

        std::mutex                                                  m_uninitializedVertexShadersMutex;
        std::queue<DirectXVertexShader>                             m_uninitializedVertexShaders;
        std::queue<std::wstring>                                    m_uninitializedVertexShaderNames;
        std::mutex                                                  m_vertexShadersMutex;
        std::vector<DirectXVertexShader>                            m_vertexShaders;

        std::mutex                                                  m_directXImmediateMutex;
        std::queue<std::function<void(void)>>                       m_directXImmediateTasks;
        std::mutex                                                  m_textWritersMutex;
        std::mutex                                                  m_texturesMutex;
        std::mutex                                                  m_billboardsMutex;
        std::mutex                                                  m_uninitializedTextWritersMutex;
        std::mutex                                                  m_uninitializedTexturesMutex;
        std::mutex                                                  m_uninitializedBillboardsMutex;
        std::atomic_bool                                            m_stopDirectXWorker = false;
        std::queue<D2DTextWriter>                                   m_uninitializedTextWriters;
        std::vector<D2DTextWriter>                                  m_textWriters;
        std::queue<DirectXTexture>                                  m_uninitializedTextures;
        std::vector<DirectXTexture>                                 m_textures;
        std::queue<DirectXBillboard>                                m_uninitializedBillboards;
        std::vector<DirectXBillboard>                               m_billboards;

        // Whether or not the current Direct3D device supports the optional feature 
        // for setting the render target array index from the vertex shader stage.
        bool                                                    m_supportsVprt = false;


        // Direct2D factories.
        winrt::com_ptr<ID2D1Factory2>                   m_d2dFactory;
        winrt::com_ptr<IDWriteFactory2>                 m_dwriteFactory;
        winrt::com_ptr<IWICImagingFactory2>             m_wicFactory;
        winrt::com_ptr<IDXGIAdapter3>                   m_dxgiAdapter;
        winrt::com_ptr<IDXGIDevice>                     m_dxgiDeviceBase;
        winrt::Windows::Graphics::DirectX::Direct3D11::IDirect3DDevice  m_device;
        winrt::com_ptr<ID3D11Device4>                   m_d3dDevice;
        winrt::com_ptr<ID3D11DeviceContext3>            m_d3dContext;

        // Properties of the Direct3D device currently in use.
        D3D_FEATURE_LEVEL                                       m_d3dFeatureLevel = D3D_FEATURE_LEVEL_10_0;


        winrt::com_ptr<ID3D11InputLayout>       m_inputLayout;
        winrt::com_ptr<ID3D11GeometryShader>    m_geometryShader;
        winrt::com_ptr<ID3D11PixelShader>       m_pixelShader;
        winrt::com_ptr<ID3D11Buffer>            m_modelConstantBuffer;
    };
}