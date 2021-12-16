#pragma once

#include "pch.h"
#include "winrt\base.h"
#include "winrt\Windows.ApplicationModel.h"
#include "DirectXScheduler.h"

#include "Scheduler.h"

namespace ndtech
{
    namespace test
    {
        class App : public winrt::implements<App, winrt::Windows::ApplicationModel::Core::IFrameworkView>
        {
        public:
            void Initialize(winrt::Windows::ApplicationModel::Core::CoreApplicationView const & appView);

            void Load(winrt::hstring);

            void Uninitialize();

            void Run();

            void SetWindow(winrt::Windows::UI::Core::CoreWindow const & window);

            void InitializeDirectXElements();

            // Application lifecycle event handlers
            void OnViewActivated(winrt::Windows::ApplicationModel::Core::CoreApplicationView sender, winrt::Windows::ApplicationModel::Activation::IActivatedEventArgs args);
            void OnSuspending(IInspectable sender, winrt::Windows::ApplicationModel::ISuspendingEventArgs args);
            void OnResuming(IInspectable sender, IInspectable args);

            // Window Event Handlers
            void OnVisibilityChanged(winrt::Windows::UI::Core::CoreWindow sender, winrt::Windows::UI::Core::VisibilityChangedEventArgs args);
            void OnWindowClosed(winrt::Windows::UI::Core::CoreWindow sender, winrt::Windows::UI::Core::CoreWindowEventArgs args);

            // CoreWindow input event handlers
            void OnKeyPressed(winrt::Windows::UI::Core::CoreWindow sender, winrt::Windows::UI::Core::KeyEventArgs args);

        private:
            bool                                                m_windowClosed = false;
            bool                                                m_windowVisible = true;
            DirectXScheduler                                    m_directXScheduler;
            // The holographic space the app will use for rendering.
            winrt::Windows::Graphics::Holographic::HolographicSpace   m_holographicSpace = nullptr;
            Scheduler m_scheduler;
        };
    }
}