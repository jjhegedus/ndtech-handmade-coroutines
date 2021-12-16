#include "pch.h"
#include "App.h"
#include "winrt/Windows.UI.Input.Spatial.h"
//#include "screengrab.h"


namespace ndtech
{
    using namespace winrt::Windows::Graphics::Holographic;
    using namespace winrt::Windows::Perception::Spatial;
    using namespace winrt::Windows::UI::Input::Spatial;
    using namespace ndtech::components;

    namespace test
    {
        App::~App()
        {
            m_deviceResources->RegisterDeviceNotify(nullptr);

            UnregisterHolographicEventHandlers();
        }


        void App::InitializeComponents()
        {
            m_spinningCube = static_cast<SpinningCube*>(this->AddComponent(std::make_unique<SpinningCube>(*m_deviceResources.get())));
            m_textBillboard = static_cast<TextBillboard*>(this->AddComponent(std::make_unique<TextBillboard>(*m_deviceResources.get())));

            // Initialize the text renderer.
            //constexpr unsigned int offscreenRenderTargetWidth = 2048;
            //m_textRenderer = std::make_unique<TextRenderer>(m_deviceResources.get(), offscreenRenderTargetWidth, offscreenRenderTargetWidth);

            // Initialize the distance renderer.
            // The algorithm seems to break down if the target width is any smaller than 16 times the source width.
            // This is sufficient to reduce the memory requirement for the text message texture by a factor of 768, given the 
            // reduction in channels:
            //   * Memory required to store the offscreen DirectWrite source texture: 2048 * 2048 * 3 = 12,582,912 bytes
            //   * Memory required to store the offscreen distance field texture:      128 *  128 * 1 =     16,384 bytes
            //constexpr unsigned int blurTargetWidth = 256;
            //m_distanceFieldRenderer = std::make_unique<DistanceFieldRenderer>(m_deviceResources.get(), blurTargetWidth, blurTargetWidth);

            // Use DirectWrite to create a high-resolution, antialiased image of vector-based text.
         /*   RenderOffscreenTexture();*/
        }

        winrt::Windows::Graphics::Holographic::HolographicFrame App::Update()
        {
            // Before doing the timer update, there is some work to do per-frame
            // to maintain holographic rendering. First, we will get information
            // about the current frame.

            // The HolographicFrame has information that the app needs in order
            // to update and render the current frame. The app begins each new
            // frame by calling CreateNextFrame.
            HolographicFrame holographicFrame = m_holographicSpace.CreateNextFrame();

            // Get a prediction of where holographic cameras will be when this frame
            // is presented.
            HolographicFramePrediction prediction = holographicFrame.CurrentPrediction();

            // Back buffers can change from frame to frame. Validate each buffer, and recreate
            // resource views and depth buffers as needed.
            m_deviceResources->EnsureCameraResources(holographicFrame, prediction);

            // Next, we get a coordinate system from the attached frame of reference that is
            // associated with the current frame. Later, this coordinate system is used for
            // for creating the stereo view matrices when rendering the sample content.
            SpatialCoordinateSystem currentCoordinateSystem = m_referenceFrame.CoordinateSystem();

#ifdef DRAW_SAMPLE_CONTENT
            // Check for new input state since the last frame.
            SpatialInteractionSourceState pointerState = m_spatialInputHandler->CheckForInput();
            if (pointerState != nullptr)
            {
                // When a Pressed gesture is detected, the sample hologram will be repositioned
                // two meters in front of the user.
                m_spinningCubeRenderer->PositionHologram(
                    pointerState.TryGetPointerPose(currentCoordinateSystem)
                );
            }
#endif

            m_timer->Tick([&]()
            {
                //
                // TODO: Update scene objects.
                //
                // Put time-based updates here. By default this code will run once per frame,
                // but if you change the StepTimer to use a fixed time step this code will
                // run as many times as needed to get to the current step.
                //

#ifdef DRAW_SAMPLE_CONTENT
                //m_spinningCubeRenderer->Update(m_timer);
#endif
            });

            // We complete the frame update by using information about our content positioning
            // to set the focus point.

            uint32_t index = 0;
            for (auto cameraPose = prediction.CameraPoses().GetAt(index); index < prediction.CameraPoses().Size(); index++)
            {
#ifdef DRAW_SAMPLE_CONTENT
                // The HolographicCameraRenderingParameters class provides access to set
                // the image stabilization parameters.
                HolographicCameraRenderingParameters renderingParameters = holographicFrame.GetRenderingParameters(cameraPose);

                // SetFocusPoint informs the system about a specific point in your scene to
                // prioritize for image stabilization. The focus point is set independently
                // for each holographic camera.
                // You should set the focus point near the content that the user is looking at.
                // In this example, we put the focus point at the center of the sample hologram,
                // since that is the only hologram available for the user to focus on.
                // You can also set the relative velocity and facing of that content; the sample
                // hologram is at a fixed point so we only need to indicate its position.
                renderingParameters.SetFocusPoint(
                    currentCoordinateSystem,
                    m_spinningCubeRenderer->GetPosition()
                );
#endif
            }

            // The holographic frame will be used to get up-to-date view and projection matrices and
            // to present the swap chain.
            return holographicFrame;
        }

        void App::SaveAppState()
        {
            //
            // TODO: Insert code here to save your app state.
            //       This method is called when the app is about to suspend.
            //
            //       For example, store information in the SpatialAnchorStore.
            //
        }

        void App::LoadAppState()
        {
            //
            // TODO: Insert code here to load your app state.
            //       This method is called when the app resumes.
            //
            //       For example, load information from the SpatialAnchorStore.
            //
        }


        bool App::Render(HolographicFrame holographicFrame)
        {
            //// Off-screen drawing used by all cameras.
            //// The distance function texture only needs to be created once, for a given text string.
            //if (m_distanceFieldRenderer->GetRenderCount() == 0)
            //{
            //    m_distanceFieldRenderer->RenderDistanceField(m_textRenderer->GetTextureView());
            //    //m_deviceResources->SaveWICTextureToFile(m_distanceFieldRenderer->GetTextureView(), GUID_ContainerFormatJpeg, L"ms-appdata:///local/test.jpg", nullptr, nullptr);
            //    //m_deviceResources->SaveDDSTextureToFile(m_distanceFieldRenderer->GetTextureView(), L"test.dds");

            //    //std::wstring fullpath(winrt::Windows::Storage::ApplicationData::Current().LocalFolder().Path().data());
            //    //fullpath.append(L"\\test.dds");

            //    //DirectX::SaveDDSTextureToFile(m_deviceResources->GetD3DDeviceContext(), m_distanceFieldRenderer->GetTexture(), fullpath.c_str());
            //    //DirectX::SaveDDSTextureToFile(m_deviceResources->GetD3DDeviceContext(), m_textRenderer->GetTexture(), fullpath.c_str());

            //    m_textBillboard->SetQuadTextureView(m_distanceFieldRenderer->GetTextureView());

            //    // The 2048x2048 texture will not be needed again, unless we get hit DeviceLost scenario.
            //    m_textRenderer->ReleaseDeviceDependentResources();
            //}

            return ndtech::App<ndtech::test::App>::Render(holographicFrame);
            //m_textBillboard->Render(m_distanceFieldRenderer->GetTextureView());
            //return true;
        }
    }
}