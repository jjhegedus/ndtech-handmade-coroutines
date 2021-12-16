#include "pch.h"
#include "App.h"

#include <TraceLoggingProvider.h>

TRACELOGGING_DEFINE_PROVIDER(
    traceProvider,
    "ndtech",
    // {38984087-9bc9-57a9-9689-605ce8a6ecdd}
    (0x38984087, 0x9bc9, 0x57a9, 0x96, 0x89, 0x60, 0x5c, 0xe8, 0xa6, 0xec, 0xdd));
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
    TraceLoggingRegister(traceProvider);
    winrt::Windows::ApplicationModel::Core::CoreApplication::Run(AppViewSource());
    TraceLoggingUnregister(traceProvider);
}
