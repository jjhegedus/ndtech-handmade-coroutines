#pragma once
#include "ndtech/App.h"
#include "ndtech.components/SpinningCube.h"
#include "ndtech.components/TextBillboard.h"


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

            // Initialize the components that are specific to this app
            void InitializeComponents();

            // Starts the holographic frame and updates the content.
            winrt::Windows::Graphics::Holographic::HolographicFrame Update();

            // Handle saving and loading of app state owned by AppMain.
            void SaveAppState();
            void LoadAppState();

            virtual bool Render(winrt::Windows::Graphics::Holographic::HolographicFrame holographicFrame);

        private:
            //void RenderOffscreenTexture();

            // Components
            ndtech::components::SpinningCube*               m_spinningCube;
            ndtech::components::TextBillboard*              m_textBillboard;

            //// Component support types
            //// Renders text off-screen. Used to create a texture to render on the quad.
            //std::unique_ptr<TextRenderer>                                           m_textRenderer;

            //// Performs a gather operation to create a 2D pixel distance map.
            //std::unique_ptr<DistanceFieldRenderer>                                  m_distanceFieldRenderer;
        };
    }
}