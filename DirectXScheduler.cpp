#include "pch.h"
#include "DirectXScheduler.h"

#include "Utilities.h"

//#ifdef _DEBUG
//#ifndef OUTPUT_DEBUG_STRINGS
//#define DebugOutput(cstr)
//#endif
//#endif


namespace ndtech {

    DirectXScheduler::DirectXScheduler() {

        m_cache_line_size = Utilities::cache_line_size();

        AddImmediateDirectXTask([this]() {CreateDeviceIndependentResources(); });

        m_directXThread = std::thread{ &DirectXScheduler::RunDirectXWorker, this };
    }

    DirectXScheduler::~DirectXScheduler() {
        if (m_directXThread.joinable()) {
            m_directXThread.join();
        }
    }


    void DirectXScheduler::CreateDeviceIndependentResources() {
        DebugOutput("App::CreateDeviceIndependentResources()\n");
        // Initialize Direct2D resources.
        D2D1_FACTORY_OPTIONS options{};

#if defined(_DEBUG)
        // If the project is in a debug build, enable Direct2D debugging via SDK Layers.
        options.debugLevel = D2D1_DEBUG_LEVEL_INFORMATION;
#endif

        // Initialize the Direct2D Factory.
        winrt::check_hresult(
            D2D1CreateFactory(
                D2D1_FACTORY_TYPE_SINGLE_THREADED,
                __uuidof(ID2D1Factory2),
                &options,
                reinterpret_cast<void **>(m_d2dFactory.put())
            )
        );

        // Initialize the DirectWrite Factory.
        winrt::check_hresult(
            DWriteCreateFactory(
                DWRITE_FACTORY_TYPE_SHARED,
                __uuidof(IDWriteFactory2),
                reinterpret_cast<::IUnknown **>(m_dwriteFactory.put())
            )
        );

        // Initialize the Windows Imaging Component (WIC) Factory.
        winrt::check_hresult(
            CoCreateInstance(
                CLSID_WICImagingFactory2,
                nullptr,
                CLSCTX_INPROC_SERVER,
                IID_PPV_ARGS(m_wicFactory.put())
            )
        );
    }

    void DirectXScheduler::PrepareHolographicSpace(winrt::Windows::Graphics::Holographic::HolographicSpace holographicSpace) {
        m_holographicSpace = holographicSpace;
        AddImmediateDirectXTask([this]() {InitializeUsingHolographicSpace(); });
    }

    void DirectXScheduler::InitializeUsingHolographicSpace() {
        // The holographic space might need to determine which adapter supports
        // holograms, in which case it will specify a non-zero PrimaryAdapterId.
        LUID id =
        {
            m_holographicSpace.PrimaryAdapterId().LowPart,
            m_holographicSpace.PrimaryAdapterId().HighPart
        };

        // When a primary adapter ID is given to the app, the app should find
        // the corresponding DXGI adapter and use it to create Direct3D devices
        // and device contexts. Otherwise, there is no restriction on the DXGI
        // adapter the app can use.
        if ((id.HighPart != 0) && (id.LowPart != 0))
        {
            UINT createFlags = 0;
#ifdef DEBUG
            if (DX::SdkLayersAvailable())
            {
                createFlags |= DXGI_CREATE_FACTORY_DEBUG;
            }
#endif
            // Create the DXGI factory.
            winrt::com_ptr<IDXGIFactory1> dxgiFactory;
            winrt::check_hresult(
                CreateDXGIFactory2(
                    createFlags,
                    IID_PPV_ARGS(dxgiFactory.put())
                )
            );
            winrt::com_ptr<IDXGIFactory4> dxgiFactory4;
            dxgiFactory4 = dxgiFactory.as<IDXGIFactory4>();

            // Retrieve the adapter specified by the holographic space.
            winrt::check_hresult(
                dxgiFactory4->EnumAdapterByLuid(
                    id,
                    IID_PPV_ARGS(m_dxgiAdapter.put())
                )
            );
        }
        else
        {
            m_dxgiAdapter = nullptr;
        }

        CreateDeviceResources();

        m_holographicSpace.SetDirect3D11Device(m_device);
    }


    void DirectXScheduler::CreateDeviceResources() {
        // This flag adds support for surfaces with a different color channel ordering
        // than the API default. It is required for compatibility with Direct2D.
        UINT creationFlags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;

#if defined(_DEBUG)
        if (SdkLayersAvailable())
        {
            // If the project is in a debug build, enable debugging via SDK Layers with this flag.
            creationFlags |= D3D11_CREATE_DEVICE_DEBUG;
        }
#endif

        // This array defines the set of DirectX hardware feature levels this app will support.
        // Note the ordering should be preserved.
        // Note that HoloLens supports feature level 11.1. The HoloLens emulator is also capable
        // of running on graphics cards starting with feature level 10.0.
        D3D_FEATURE_LEVEL featureLevels[] =
        {
            D3D_FEATURE_LEVEL_12_1,
            D3D_FEATURE_LEVEL_12_0,
            D3D_FEATURE_LEVEL_11_1,
            D3D_FEATURE_LEVEL_11_0,
            D3D_FEATURE_LEVEL_10_1,
            D3D_FEATURE_LEVEL_10_0
        };

        // Create the Direct3D 11 API device object and a corresponding context.
        winrt::com_ptr<ID3D11Device> device;
        winrt::com_ptr<ID3D11DeviceContext> context;

        const HRESULT hr = D3D11CreateDevice(
            (IDXGIAdapter *)&(m_dxgiAdapter.operator*()),        // Either nullptr, or the primary adapter determined by Windows Holographic.
            D3D_DRIVER_TYPE_HARDWARE,   // Create a device using the hardware graphics driver.
            0,                          // Should be 0 unless the driver is D3D_DRIVER_TYPE_SOFTWARE.
            creationFlags,              // Set debug and Direct2D compatibility flags.
            featureLevels,              // List of feature levels this app can support.
            ARRAYSIZE(featureLevels),   // Size of the list above.
            D3D11_SDK_VERSION,          // Always set this to D3D11_SDK_VERSION for Windows Store apps.
            device.put(),                    // Returns the Direct3D device created.
            &m_d3dFeatureLevel,         // Returns feature level of device created.
            context.put()                    // Returns the device immediate context.
        );

        if (FAILED(hr))
        {
            // If the initialization fails, fall back to the WARP device.
            // For more information on WARP, see:
            // http://go.microsoft.com/fwlink/?LinkId=286690
            winrt::check_hresult(
                D3D11CreateDevice(
                    nullptr,              // Use the default DXGI adapter for WARP.
                    D3D_DRIVER_TYPE_WARP, // Create a WARP device instead of a hardware device.
                    0,
                    creationFlags,
                    featureLevels,
                    ARRAYSIZE(featureLevels),
                    D3D11_SDK_VERSION,
                    device.put(),
                    &m_d3dFeatureLevel,
                    context.put()
                )
            );
        }

        // Store pointers to the Direct3D device and immediate context.
        m_d3dDevice = device.as<ID3D11Device4>();


#ifdef _DEBUG
        std::stringstream ss;
        ss << "DirectXScheduler::CreateDeviceResources: m_d3dDevice = " << this->m_d3dDevice.get() << std::endl;
#endif
        DebugOutput(ss.str().c_str());
        //m_device = device.as<winrt::Windows::Graphics::DirectX::Direct3D11::IDirect3DDevice>();
        //winrt::abi<winrt::Windows::Graphics::DirectX::Direct3D11::IDirect3DDevice> d = device.as<winrt::abi<winrt::Windows::Graphics::DirectX::Direct3D11::IDirect3DDevice>>();
        //m_device.copy_from(device);
        //winrt::Windows::Graphics::DirectX::Direct3D11::IDirect3DDevice d = ((winrt::Windows::Graphics::DirectX::Direct3D11::IDirect3DDevice)(*device));

        m_d3dContext = context.as<ID3D11DeviceContext3>();

        // Acquire the DXGI interface for the Direct3D device.
        winrt::com_ptr<IDXGIDevice3> dxgiDevice;
        dxgiDevice = m_d3dDevice.as<IDXGIDevice3>();
        m_dxgiDeviceBase = m_d3dDevice.as<IDXGIDevice>();

        // Create interop device
        winrt::com_ptr<::IInspectable> inspectable{ nullptr };
        winrt::check_hresult(CreateDirect3D11DeviceFromDXGIDevice(m_dxgiDeviceBase.get(), inspectable.put()));
        //auto device_winrt = inspectable.as<winrt::ABI::Windows::Graphics::DirectX::Direct3D11::IDirect3DDevice>();
        m_device = inspectable.as<winrt::Windows::Graphics::DirectX::Direct3D11::IDirect3DDevice>();


        //m_device = m_dxgiDeviceBase.as<winrt::Windows::Graphics::DirectX::Direct3D11::IDirect3DDevice>();
        //winrt::com_ptr<winrt::ABI::Windows::Graphics::DirectX::Direct3D11::IDirect3DDevice> abiDevice;


        // Wrap the native device using a WinRT interop object.
        //m_d3dInteropDevice = CreateDirect3DDevice(dxgiDevice.Get());

        // Cache the DXGI adapter.
        // This is for the case of no preferred DXGI adapter, or fallback to WARP.
        winrt::com_ptr<IDXGIAdapter> dxgiAdapter;
        winrt::check_hresult(dxgiDevice->GetAdapter(dxgiAdapter.put()));
        m_dxgiAdapter = dxgiAdapter.as<IDXGIAdapter3>();

        // Check for device support for the optional feature that allows setting the render target array index from the vertex shader stage.
        D3D11_FEATURE_DATA_D3D11_OPTIONS3 options;
        m_d3dDevice->CheckFeatureSupport(D3D11_FEATURE_D3D11_OPTIONS3, &options, sizeof(options));
        if (options.VPAndRTArrayIndexFromAnyShaderFeedingRasterizer)
        {
            m_supportsVprt = true;
        }
    }

    //winrt::Windows::Foundation::IAsyncAction DirectXScheduler::CreateDeviceDependentResources() {

    //    // On devices that do support the D3D11_FEATURE_D3D11_OPTIONS3::
    //    // VPAndRTArrayIndexFromAnyShaderFeedingRasterizer optional feature
    //    // we can avoid using a pass-through geometry shader to set the render
    //    // target array index, thus avoiding any overhead that would be 
    //    // incurred by setting the geometry shader stage.
    //    std::wstring vertexShaderFileName = m_supportsVprt ? L"ms-appx:///VprtVertexShader.cso" : L"ms-appx:///VertexShader.cso";

    //    // Load shaders asynchronously.
    //    std::vector<byte> vertexFileData = co_await Utilities::ReadDataCoAwait(vertexShaderFileName);
    //    std::vector<byte> pixelFileData = co_await Utilities::ReadDataCoAwait(L"ms-appx:///PixelShader.cso");

    //    std::vector<byte> geometryFileData;
    //    if (!m_supportsVprt)
    //    {
    //        // Load the pass-through geometry shader.
    //        geometryFileData = co_await Utilities::ReadDataCoAwait(L"ms-appx:///GeometryShader.cso");
    //    }

    //    // create the vertex shader and input layout.
    //    winrt::check_hresult(
    //        m_d3dDevice->CreateVertexShader(
    //            vertexFileData.data(),
    //            vertexFileData.size(),
    //            nullptr,
    //            put(m_vertexShader)
    //        )
    //    );

    //    constexpr std::array<D3D11_INPUT_ELEMENT_DESC, 2> vertexDesc =
    //    { {
    //        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0,  0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
    //    { "COLOR",    0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
    //        } };

    //    winrt::check_hresult(
    //        m_d3dDevice->CreateInputLayout(
    //            vertexDesc.data(),
    //            vertexDesc.size(),
    //            vertexFileData.data(),
    //            vertexFileData.size(),
    //            put(m_inputLayout)
    //        )
    //    );


    //    // create the pixel shader and constant buffer.
    //    winrt::check_hresult(
    //        m_d3dDevice->CreatePixelShader(
    //            pixelFileData.data(),
    //            pixelFileData.size(),
    //            nullptr,
    //            put(m_pixelShader)
    //        )
    //    );

    //    const CD3D11_BUFFER_DESC constantBufferDesc(sizeof(ModelConstantBuffer), D3D11_BIND_CONSTANT_BUFFER);
    //    winrt::check_hresult(
    //        m_d3dDevice->CreateBuffer(
    //            &constantBufferDesc,
    //            nullptr,
    //            put(m_modelConstantBuffer)
    //        )
    //    );

    //    if (!m_supportsVprt)
    //    {
    //        winrt::check_hresult(
    //            m_d3dDevice->CreateGeometryShader(
    //                geometryFileData.data(),
    //                geometryFileData.size(),
    //                nullptr,
    //                put(m_geometryShader)
    //            )
    //        );
    //    }
    //}

    SynchronousCoroutine<int> DirectXScheduler::RunDirectXWorker() {

        ResumableCoroutine<int> doWorkCoroutine = DoWork();

        co_await doWorkCoroutine;

        while (!doWorkCoroutine.done()) {
            doWorkCoroutine.resume();
        }

        co_return 0;
    }

    ResumableCoroutine<int> DirectXScheduler::DoWork() {

        while (!m_stopDirectXWorker.load()) {

            {  // run immediate directx tasks: begin
                std::lock_guard<std::mutex> guard(m_directXImmediateMutex);

                while (!m_directXImmediateTasks.empty()) {
                    std::function<void(void)> task(m_directXImmediateTasks.front());

                    task();

                    m_directXImmediateTasks.pop();
                }
            }// run immediate directx tasks: end

            { // initialize vertex shaders: begin
                std::lock_guard<std::mutex> guard(m_uninitializedVertexShadersMutex);

                while (!m_uninitializedVertexShaders.empty()) {
                    DirectXVertexShader &vertexShader = m_uninitializedVertexShaders.front();

                    if (vertexShader.state == DirectXVertexShaderState::initial) {

                        TraceLogWrite(
                            "Processing Vertex Shader",
                            TraceLoggingWideString(vertexShader.fileName.c_str(), "vertexShader.fileName"));

                        TraceLogWrite("Test");

                        vertexShader.state = DirectXVertexShaderState::loading;
                        InitializeVertexShader(vertexShader);
                        DebugOutput("after loading vertex shader;");
                    }

                    if (vertexShader.state == DirectXVertexShaderState::loaded) {
                        m_vertexShaders.push_back(vertexShader);
                        m_uninitializedVertexShaders.pop();
                    }

                }
            } // initialize vertex shaders: end


            { // process vertex shaders: begin
                std::lock_guard<std::mutex> guard(m_vertexShadersMutex);

                for (DirectXVertexShader &vertexShader : m_vertexShaders) {
                    //DebugOutput("After getting the vertex shader\n");
                }
            } // process vertex shaders: end

            //{
            //    std::lock_guard<std::mutex> guard(m_uninitializedTextWritersMutex);

            //    while (!m_uninitializedTextWriters.empty()) {
            //        D2DTextWriter task(m_uninitializedTextWriters.front());

            //        DebugOutput("TextRenderer::CreateDeviceDependentResources():BEGIN\n");

            //        //// Create a default sampler state, which will use point sampling.
            //        //CD3D11_SAMPLER_DESC desc(D3D11_DEFAULT);
            //        //m_deviceResources->GetD3DDevice()->CreateSamplerState(&desc, put(m_pointSampler));

            //        //// Create the texture that will be used as the offscreen render target.
            //        //CD3D11_TEXTURE2D_DESC textureDesc(
            //        //    DXGI_FORMAT_B8G8R8A8_UNORM,
            //        //    m_textureWidth,
            //        //    m_textureHeight,
            //        //    1,
            //        //    1,
            //        //    D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET
            //        //);
            //        //m_deviceResources->GetD3DDevice()->CreateTexture2D(&textureDesc, nullptr, put(m_texture));

            //        //// Create read and write views for the offscreen render target.
            //        //m_deviceResources->GetD3DDevice()->CreateShaderResourceView(get(m_texture), nullptr, put(m_shaderResourceView));
            //        //m_deviceResources->GetD3DDevice()->CreateRenderTargetView(get(m_texture), nullptr, put(m_renderTargetView));

            //        //// In this example, we are using D2D and DirectWrite; so, we need to create a D2D render target as well.
            //        //D2D1_RENDER_TARGET_PROPERTIES props = D2D1::RenderTargetProperties(
            //        //    D2D1_RENDER_TARGET_TYPE_DEFAULT,
            //        //    D2D1::PixelFormat(DXGI_FORMAT_UNKNOWN, D2D1_ALPHA_MODE_PREMULTIPLIED),
            //        //    96,
            //        //    96
            //        //);

            //        //// The DXGI surface is used to create the render target.
            //        //com_ptr<IDXGISurface> dxgiSurface;
            //        //dxgiSurface = m_texture.as<IDXGISurface>();
            //        //check_hresult(m_deviceResources->GetD2DFactory()->CreateDxgiSurfaceRenderTarget(get(dxgiSurface), &props, put(m_d2dRenderTarget)));

            //        //// Create a solid color brush that will be used to render the text.
            //        //check_hresult(m_d2dRenderTarget->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::Cornsilk), put(m_whiteBrush)));

            //        //// This is where we format the text that will be written on the render target.
            //        //check_hresult(
            //        //    m_deviceResources->GetDWriteFactory()->CreateTextFormat(
            //        //        L"Consolas",
            //        //        NULL,
            //        //        DWRITE_FONT_WEIGHT_NORMAL,
            //        //        DWRITE_FONT_STYLE_NORMAL,
            //        //        DWRITE_FONT_STRETCH_NORMAL,
            //        //        400.0f,
            //        //        L"",
            //        //        put(m_textFormat)
            //        //    )
            //        //);
            //        //check_hresult(m_textFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER));
            //        //check_hresult(m_textFormat->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER));

            //        m_uninitializedTextWriters.pop();
            //    }
            //}



        }

        co_return 0;
    }

    void DirectXScheduler::AddImmediateDirectXTask(std::function<void(void)> task) {
        std::lock_guard<std::mutex> guard(m_directXImmediateMutex);

        m_directXImmediateTasks.push(task);
    }

    void DirectXScheduler::AddVertexShader(std::wstring shaderFileName) {
        DirectXVertexShader vertexShader;
        vertexShader.fileName = shaderFileName;

        std::lock_guard<std::mutex> guard(m_uninitializedVertexShadersMutex);
        m_uninitializedVertexShaders.push(vertexShader);
    }

    VoidCoroutine DirectXScheduler::InitializeVertexShader(DirectXVertexShader& vertexShader) {

        // Load vertex shader asynchronously.
        std::vector<::byte> vertexShaderFileData = co_await ndtech::Utilities::ReadDataCoAwait(vertexShader.fileName);

        HRESULT hr = m_d3dDevice->CreateVertexShader(
            vertexShaderFileData.data(),
            vertexShaderFileData.size(),
            nullptr,
            vertexShader.shader.put()
        );

        vertexShader.state = DirectXVertexShaderState::loaded;
        DebugOutput("InitVertexShader complete\n");

    }

    DataProducingCoroutine<int> GetDataFromCompletableDataProducingCoroutine() {

        cout << "Executing GetDataFromCompletableDataProducingCoroutine\n";

        co_return 1;
    }

    void DirectXScheduler::AddTextWriter(D2DTextWriter textWriter) {
        std::lock_guard<std::mutex> guard(m_uninitializedTextWritersMutex);

        m_uninitializedTextWriters.push(textWriter);
    }

    void DirectXScheduler::AddTexture(DirectXTexture texture) {
        std::lock_guard<std::mutex> guard(m_uninitializedTexturesMutex);

        m_uninitializedTextures.push(texture);
    }

    void DirectXScheduler::AddBillboard(DirectXBillboard billboard) {
        std::lock_guard<std::mutex> guard(m_uninitializedBillboardsMutex);

        m_uninitializedBillboards.push(billboard);
    }


}