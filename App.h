#pragma once
#include "ndtech/App.h"
#include "ndtech.components/SpinningCube.h"
#include "ndtech.components/TextBillboard.h"
#include "ndtech/TextRenderer.h"
#include "ndtech/DistanceFieldRenderer.h"


namespace ndtech
{
    namespace test
    {
        class App :
            public ndtech::App<App>
        {
        public:
            App()
            {
                this->m_holographicSpace = nullptr;
            };

            ~App();

            // Inherited via IDeviceNotify
            virtual void OnDeviceLost() override;
            virtual void OnDeviceRestored() override;

            // Sets the holographic space. This is our closest analogue to setting a new window
            // for the app.
            void SetHolographicSpace(winrt::Windows::Graphics::Holographic::HolographicSpace holographicSpace);

            // Initialize the components that are specific to this app
            void InitializeComponents();

            // Starts the holographic frame and updates the content.
            winrt::Windows::Graphics::Holographic::HolographicFrame Update();

            // Handle saving and loading of app state owned by AppMain.
            void SaveAppState();
            void LoadAppState();

            virtual bool Render(winrt::Windows::Graphics::Holographic::HolographicFrame holographicFrame);

        private:
            // Used to notify the app when the positional tracking state changes.
            void OnLocatabilityChanged(
                winrt::Windows::Perception::Spatial::SpatialLocator sender,
                winrt::Windows::IInspectable args);

            // Asynchronously creates resources for new holographic cameras.
            void OnCameraAdded(
                winrt::Windows::Graphics::Holographic::HolographicSpace sender,
                winrt::Windows::Graphics::Holographic::HolographicSpaceCameraAddedEventArgs args);

            winrt::Windows::Foundation::IAsyncAction App::AddHolographicCameraAsync(
                DeviceResources& deviceResources,
                winrt::Windows::Foundation::Deferral deferral,
                winrt::Windows::Graphics::Holographic::HolographicCamera camera);


            // Synchronously releases resources for holographic cameras that are no longer
            // attached to the system.
            void OnCameraRemoved(
                winrt::Windows::Graphics::Holographic::HolographicSpace sender,
                winrt::Windows::Graphics::Holographic::HolographicSpaceCameraRemovedEventArgs args);

            winrt::Windows::Foundation::IAsyncAction RemoveHolographicCameraAsync(
                DeviceResources& deviceResources,
                winrt::Windows::Graphics::Holographic::HolographicCamera camera);

            void RenderOffscreenTexture();

            // SpatialLocator that is attached to the primary camera.
            winrt::Windows::Perception::Spatial::SpatialLocator                     m_locator = nullptr;

            // Event registration tokens.
            winrt::event_token                                                      m_cameraAddedToken;
            winrt::event_token                                                      m_cameraRemovedToken;
            winrt::event_token                                                      m_locatabilityChangedToken;

            //Clears event registration state. Used when changing to a new HolographicSpace
            // and when tearing down AppMain.
            void UnregisterHolographicEventHandlers();

            // Components
            ndtech::components::SpinningCube*               m_spinningCube;
            ndtech::components::TextBillboard*              m_textBillboard;

            // Component support types
            // Renders text off-screen. Used to create a texture to render on the quad.
            std::unique_ptr<TextRenderer>                                           m_textRenderer;

            // Performs a gather operation to create a 2D pixel distance map.
            std::unique_ptr<DistanceFieldRenderer>                                  m_distanceFieldRenderer;
        };
    }
}