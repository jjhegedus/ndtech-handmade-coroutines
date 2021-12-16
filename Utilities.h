#pragma once

#include <future>
#include "winrt/Windows.Storage.h"
#include "winrt/Windows.Storage.Streams.h"

#include <inspectable.h>
#include "winrt/Windows.Foundation.h"
#include <string>
#include <thread>
#include <iostream>
#include <sstream>

using namespace winrt::Windows::Foundation;

namespace ndtech {
    namespace Utilities {
        auto getBenchmarker ();
        size_t cache_line_size ();
        std::future<std::vector<byte>> ReadDataCoAwait(const std::wstring& filename);
        //float ConvertDipsToPixels(float dips, float dpi);
        std::string GetLastErrorAsString();
        std::string GetHRAsString(HRESULT hr);
        std::string ThreadIdToString(std::thread::id threadId);
    }
}



#define TraceLogWrite(eventName, ...) \
    _TlgWrite_imp(_TlgWrite, \
    traceProvider, eventName, \
    (NULL, NULL), \
    TraceLoggingString(ndtech::Utilities::ThreadIdToString(std::this_thread::get_id()).c_str(), "threadId"), \
    __VA_ARGS__)