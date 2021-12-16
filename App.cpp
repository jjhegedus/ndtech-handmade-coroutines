#include "pch.h"
#include "App.h"
#include "Utilities.h"
#include <chrono>
#include <iomanip>

using namespace std::chrono;
using namespace std::chrono_literals;

namespace ndtech {
    using namespace winrt::Windows::Graphics::Holographic;

    namespace test {
        //using namespace winrt;

        //using namespace winrt::Windows;
        using namespace winrt::Windows::ApplicationModel::Core;
        //using namespace winrt::Windows::Foundation::Numerics;
        //using namespace winrt::Windows::UI;
        using namespace winrt::Windows::UI::Core;
        using namespace winrt::Windows::ApplicationModel::Activation;
        using namespace winrt::Windows::ApplicationModel;
        //using namespace winrt::Windows::Graphics::Holographic;

        void App::Initialize(winrt::Windows::ApplicationModel::Core::CoreApplicationView const & appView) {
            appView.Activated({ this, &App::OnViewActivated });
            winrt::Windows::ApplicationModel::Core::CoreApplication::Suspending({ this, &App::OnSuspending });
            winrt::Windows::ApplicationModel::Core::CoreApplication::Resuming({ this, &App::OnResuming });

            TraceLogWrite("Initialize");

            m_scheduler.AddTask(std::pair<std::function<void(void)>, time_point<system_clock>>{[]() {
                std::stringstream ss;
                auto now = std::chrono::system_clock::now();
                auto now_c = std::chrono::system_clock::to_time_t(now);
                ss << "Inside Task2: time is " << std::ctime(&now_c) << std::endl;
                DebugOutput(ss.str().c_str());
            }, system_clock::now() + 3s});

            m_scheduler.AddTask(std::pair<std::function<void(void)>, time_point<system_clock>>{[]() {
                std::stringstream ss;
                auto now = std::chrono::system_clock::now();
                auto now_c = std::chrono::system_clock::to_time_t(now);
                ss << "Inside Task1: time is " << std::ctime(&now_c) << std::endl;
                DebugOutput(ss.str().c_str());
            }, system_clock::now()});

            m_scheduler.AddTask(std::pair<std::function<void(void)>, time_point<system_clock>>{[]() {
                std::stringstream ss;
                auto now = std::chrono::system_clock::now();
                auto now_c = std::chrono::system_clock::to_time_t(now);
                ss << "Inside Task3: time is " << std::ctime(&now_c) << std::endl;
                DebugOutput(ss.str().c_str());
            }, system_clock::now() + 1s});

        }


        void App::Load(winrt::hstring) {}

        void App::Uninitialize() {}

        void App::Run() {
            while (!m_windowClosed)
            {
                if (m_windowVisible && (m_holographicSpace != nullptr))
                {
                    winrt::Windows::UI::Core::CoreWindow::GetForCurrentThread().Dispatcher().ProcessEvents(winrt::Windows::UI::Core::CoreProcessEventsOption::ProcessAllIfPresent);


                }
                else
                {
                    winrt::Windows::UI::Core::CoreWindow::GetForCurrentThread().Dispatcher().ProcessEvents(winrt::Windows::UI::Core::CoreProcessEventsOption::ProcessOneAndAllPending);
                }
            }
        }

        void App::SetWindow(winrt::Windows::UI::Core::CoreWindow const & window) {
            window.KeyDown({ this, &App::OnKeyPressed });
            window.Closed({ this, &App::OnWindowClosed });
            window.VisibilityChanged({ this, &App::OnVisibilityChanged });

            // Create a holographic space for the core window for the current view.
            // Presenting holographic frames that are created by this holographic space will put
            // the app into exclusive mode.
            m_holographicSpace = winrt::Windows::Graphics::Holographic::HolographicSpace::CreateForCoreWindow(window);

            m_directXScheduler.PrepareHolographicSpace(m_holographicSpace);

            InitializeDirectXElements();
        }

        void App::InitializeDirectXElements() {
            m_directXScheduler.AddVertexShader(m_directXScheduler.usingVprtShaders ? L"ms-appx:///TextBillboardVprtVertexShader.cso" : L"ms-appx:///TextBillboardVertexShader.cso");
            //m_directXScheduler.AddVertexShader(L"ms-appx:///DistanceFieldPixelShader.cso");
        }


        //winrt::Windows::Foundation::IAsyncAction App::LoadShaderFiles() {

        //}

        void App::OnViewActivated(CoreApplicationView sender, IActivatedEventArgs args) {
            sender.CoreWindow().Activate();
        }

        void App::OnSuspending(IInspectable sender, ISuspendingEventArgs args) {
            ISuspendingDeferral suspendingDeferral = args.SuspendingOperation().GetDeferral();
        }

        void App::OnResuming(IInspectable sender, IInspectable args) {}

        void App::OnVisibilityChanged(CoreWindow sender, VisibilityChangedEventArgs args) {
            m_windowVisible = args.Visible();
        }

        void App::OnWindowClosed(CoreWindow sender, CoreWindowEventArgs args) {
            m_windowClosed = true;
        }

        void App::OnKeyPressed(CoreWindow sender, KeyEventArgs args) {
            //
            // TODO: Bluetooth keyboards are supported by HoloLens. You can use this method for
            //       keyboard input if you want to support it as an optional input method for
            //       your holographic app.
            //
        }
    }
}