#include "pch.h"
#include "App.h"

using namespace winrt;

struct AppViewSource : winrt::implements<AppViewSource, winrt::Windows::ApplicationModel::Core::IFrameworkViewSource>
{
    ndtech::test::App* app;
    winrt::Windows::ApplicationModel::Core::IFrameworkView CreateView()
    {
        if (app == nullptr)
        {
            app = new ndtech::test::App();
        }
        return *app;
    }
};

int __stdcall wWinMain(HINSTANCE, HINSTANCE, PWSTR, int)
{
    winrt::Windows::ApplicationModel::Core::CoreApplication::Run(AppViewSource());
}
