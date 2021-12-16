#pragma once
#include <vector>
#include "winrt/Windows.Foundation.h"

// This may not be good so its not being used.  Using std::future<std::vector<byte>> at the moment instead
namespace winrt::ABI::Windows::Foundation {
    template <> struct __declspec(uuid("3F2F76B5-7CF9-4535-8B91-EC7F74882B81")) __declspec(novtable) IAsyncOperation<std::vector<byte>> : impl_IAsyncOperation<std::vector<byte>> {};

    template <> struct __declspec(uuid("3F2F76B5-7CF9-4535-8B91-EC7F74882B81")) __declspec(novtable) AsyncOperationCompletedHandler<std::vector<byte>> : impl_AsyncOperationCompletedHandler<std::vector<byte>> {};
}