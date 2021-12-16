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

        void App::OnDeviceLost()
        {
#ifdef DRAW_SAMPLE_CONTENT
            m_spinningCubeRenderer->ReleaseDeviceDependentResources();
#endif
        }
        void App::OnDeviceRestored()
        {
#ifdef DRAW_SAMPLE_CONTENT
            m_spinningCubeRenderer->CreateDeviceDependentResources();
#endif
        }
        void App::SetHolographicSpace(winrt::Windows::Graphics::Holographic::HolographicSpace holographicSpace)
        {
            UnregisterHolographicEventHandlers();

            m_holographicSpace = holographicSpace;

            m_spatialInputHandler = std::make_unique<SpatialInputHandler>();

            // Use the default SpatialLocator to track the motion of the device.
            m_locator = winrt::Windows::Perception::Spatial::SpatialLocator::GetDefault();

            // Be able to respond to changes in the positional tracking state.
            m_locatabilityChangedToken = m_locator.LocatabilityChanged({ this, &App::OnLocatabilityChanged });

            // Respond to camera added events by creating any resources that are specific
            // to that camera, such as the back buffer render target view.
            // When we add an event handler for CameraAdded, the API layer will avoid putting
            // the new camera in new HolographicFrames until we complete the deferral we created
            // for that handler, or return from the handler without creating a deferral. This
            // allows the app to take more than one frame to finish creating resources and
            // loading assets for the new holographic camera.
            // This function should be registered before the app creates any HolographicFrames.

            m_cameraAddedToken = m_holographicSpace.CameraAdded({ this, &App::OnCameraAdded });

            // Respond to camera removed events by releasing resources that were created for that
            // camera.
            // When the app receives a CameraRemoved event, it releases all references to the back
            // buffer right away. This includes render target views, Direct2D target bitmaps, and so on.
            // The app must also ensure that the back buffer is not attached as a render target, as
            // shown in DeviceResources::ReleaseResourcesForBackBuffer.
            m_cameraRemovedToken = m_holographicSpace.CameraRemoved({ this, &App::OnCameraRemoved });

            // The simplest way to render world-locked holograms is to create a stationary reference frame
            // when the app is launched. This is roughly analogous to creating a "world" coordinate system
            // with the origin placed at the device's position as the app is launched.
            m_referenceFrame = m_locator.CreateStationaryFrameOfReferenceAtCurrentLocation();

            // Notes on spatial tracking APIs:
            // * Stationary reference frames are designed to provide a best-fit position relative to the
            //   overall space. Individual positions within that reference frame are allowed to drift slightly
            //   as the device learns more about the environment.
            // * When precise placement of individual holograms is required, a SpatialAnchor should be used to
            //   anchor the individual hologram to a position in the real world - for example, a point the user
            //   indicates to be of special interest. Anchor positions do not drift, but can be corrected; the
            //   anchor will use the corrected position starting in the next frame after the correction has
            //   occurred.
        }

        void App::InitializeComponents()
        {
            m_spinningCube = static_cast<SpinningCube*>(this->AddComponent(std::make_unique<SpinningCube>(*m_deviceResources.get())));
            m_textBillboard = static_cast<TextBillboard*>(this->AddComponent(std::make_unique<TextBillboard>(*m_deviceResources.get())));

            // Initialize the text renderer.
            constexpr unsigned int offscreenRenderTargetWidth = 2048;
            m_textRenderer = std::make_unique<TextRenderer>(m_deviceResources.get(), offscreenRenderTargetWidth, offscreenRenderTargetWidth);

            // Initialize the distance renderer.
            // The algorithm seems to break down if the target width is any smaller than 16 times the source width.
            // This is sufficient to reduce the memory requirement for the text message texture by a factor of 768, given the 
            // reduction in channels:
            //   * Memory required to store the offscreen DirectWrite source texture: 2048 * 2048 * 3 = 12,582,912 bytes
            //   * Memory required to store the offscreen distance field texture:      128 *  128 * 1 =     16,384 bytes
            constexpr unsigned int blurTargetWidth = 256;
            m_distanceFieldRenderer = std::make_unique<DistanceFieldRenderer>(m_deviceResources.get(), blurTargetWidth, blurTargetWidth);

            // Use DirectWrite to create a high-resolution, antialiased image of vector-based text.
            RenderOffscreenTexture();
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
                m_spinningCubeRenderer->Update(m_timer);
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


        void App::OnLocatabilityChanged(winrt::Windows::Perception::Spatial::SpatialLocator sender, winrt::Windows::IInspectable args)
        {
            switch (sender.Locatability())
            {
                case winrt::Windows::Perception::Spatial::SpatialLocatability::Unavailable:
                    // Holograms cannot be rendered.
                {
                    winrt::hstring message = L"Warning! Positional tracking is Unavailable.\n";
                    OutputDebugStringW(message.data());
                }
                break;

                // In the following three cases, it is still possible to place holograms using a
                // SpatialLocatorAttachedFrameOfReference.
                case winrt::Windows::Perception::Spatial::SpatialLocatability::PositionalTrackingActivating:
                    // The system is preparing to use positional tracking.

                case winrt::Windows::Perception::Spatial::SpatialLocatability::OrientationOnly:
                    // Positional tracking has not been activated.

                case winrt::Windows::Perception::Spatial::SpatialLocatability::PositionalTrackingInhibited:
                    // Positional tracking is temporarily inhibited. User action may be required
                    // in order to restore positional tracking.
                    break;

                case winrt::Windows::Perception::Spatial::SpatialLocatability::PositionalTrackingActive:
                    // Positional tracking is active. World-locked content can be rendered.
                    break;
            }
        }

        void App::OnCameraAdded(winrt::Windows::Graphics::Holographic::HolographicSpace sender, winrt::Windows::Graphics::Holographic::HolographicSpaceCameraAddedEventArgs args)
        {
            winrt::Windows::Foundation::Deferral deferral = args.GetDeferral();
            winrt::Windows::Graphics::Holographic::HolographicCamera holographicCamera = args.Camera();

            AddHolographicCameraAsync(*(this->m_deviceResources.get()), deferral, holographicCamera);
        }

        winrt::Windows::Foundation::IAsyncAction App::AddHolographicCameraAsync(
            DeviceResources& deviceResources,
            winrt::Windows::Foundation::Deferral deferral,
            winrt::Windows::Graphics::Holographic::HolographicCamera camera
        )
        {
            co_await winrt::resume_background();
            //
            // TODO: Allocate resources for the new camera and load any content specific to
            //       that camera. Note that the render target size (in pixels) is a property
            //       of the HolographicCamera object, and can be used to create off-screen
            //       render targets that match the resolution of the HolographicCamera.
            //

            // Create device-based resources for the holographic camera and add it to the list of
            // cameras used for updates and rendering. Notes:
            //   * Since this function may be called at any time, the AddHolographicCamera function
            //     waits until it can get a lock on the set of holographic camera resources before
            //     adding the new camera. At 60 frames per second this wait should not take long.
            //   * A subsequent Update will take the back buffer from the RenderingParameters of this
            //     camera's CameraPose and use it to create the ID3D11RenderTargetView for this camera.
            //     Content can then be rendered for the HolographicCamera.
            deviceResources.AddHolographicCamera(camera);

            // Holographic frame predictions will not include any information about this camera until
            // the deferral is completed.
            deferral.Complete();
        }

        void App::OnCameraRemoved(winrt::Windows::Graphics::Holographic::HolographicSpace sender, winrt::Windows::Graphics::Holographic::HolographicSpaceCameraRemovedEventArgs args)
        {
            winrt::Windows::Graphics::Holographic::HolographicCamera holographicCamera = args.Camera();

            RemoveHolographicCameraAsync(*(this->m_deviceResources.get()), holographicCamera);
        }

        winrt::Windows::Foundation::IAsyncAction App::RemoveHolographicCameraAsync(
            DeviceResources& deviceResources,
            winrt::Windows::Graphics::Holographic::HolographicCamera camera)
        {
            co_await winrt::resume_background();

            // TODO: Allocate resources for the new camera and load any content specific to
            //       that camera. Note that the render target size (in pixels) is a property
            //       of the HolographicCamera object, and can be used to create off-screen
            //       render targets that match the resolution of the HolographicCamera.
            //

            // Create device-based resources for the holographic camera and add it to the list of
            // cameras used for updates and rendering. Notes:
            //   * Since this function may be called at any time, the AddHolographicCamera function
            //     waits until it can get a lock on the set of holographic camera resources before
            //     adding the new camera. At 60 frames per second this wait should not take long.
            //   * A subsequent Update will take the back buffer from the RenderingParameters of this
            //     camera's CameraPose and use it to create the ID3D11RenderTargetView for this camera.
            //     Content can then be rendered for the HolographicCamera.
            m_deviceResources->RemoveHolographicCamera(camera);
        }

        void App::UnregisterHolographicEventHandlers()
        {
            try
            {
                if (m_holographicSpace != nullptr)
                {
                    // Clear previous event registrations.

                    if (m_cameraAddedToken.value != 0)
                    {
                        m_holographicSpace.CameraAdded(m_cameraAddedToken);
                        m_cameraAddedToken.value = 0;
                    }

                    if (m_cameraRemovedToken.value != 0)
                    {
                        m_holographicSpace.CameraRemoved(m_cameraRemovedToken);
                        m_cameraRemovedToken.value = 0;
                    }
                }

                if (m_locator != nullptr)
                {
                    m_locator.LocatabilityChanged(m_locatabilityChangedToken);
                }
            }
            catch (...)
            {
                // don't do anything
            }
        }

        void App::RenderOffscreenTexture()
        {
            m_textRenderer->RenderTextOffscreen(L"Test1\n");
        }


        bool App::Render(HolographicFrame holographicFrame)
        {
            // Off-screen drawing used by all cameras.
            // The distance function texture only needs to be created once, for a given text string.
            if (m_distanceFieldRenderer->GetRenderCount() == 0)
            {
                m_distanceFieldRenderer->RenderDistanceField(m_textRenderer->GetTextureView());
                //m_deviceResources->SaveWICTextureToFile(m_distanceFieldRenderer->GetTextureView(), GUID_ContainerFormatJpeg, L"ms-appdata:///local/test.jpg", nullptr, nullptr);
                //m_deviceResources->SaveDDSTextureToFile(m_distanceFieldRenderer->GetTextureView(), L"test.dds");

                //std::wstring fullpath(winrt::Windows::Storage::ApplicationData::Current().LocalFolder().Path().data());
                //fullpath.append(L"\\test.dds");

                //DirectX::SaveDDSTextureToFile(m_deviceResources->GetD3DDeviceContext(), m_distanceFieldRenderer->GetTexture(), fullpath.c_str());
                //DirectX::SaveDDSTextureToFile(m_deviceResources->GetD3DDeviceContext(), m_textRenderer->GetTexture(), fullpath.c_str());

                m_textBillboard->SetQuadTextureView(m_distanceFieldRenderer->GetTextureView());

                // The 2048x2048 texture will not be needed again, unless we get hit DeviceLost scenario.
                m_textRenderer->ReleaseDeviceDependentResources();
            }

            return ndtech::App<ndtech::test::App>::Render(holographicFrame);
            //m_textBillboard->Render(m_distanceFieldRenderer->GetTextureView());
            //return true;
        }
    }
}