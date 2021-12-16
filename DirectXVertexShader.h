#pragma once

#include "pch.h"

#include <string>
#include <d3d11.h>
#include "DataProducingCoroutine.h"

namespace ndtech {

    enum class DirectXVertexShaderState {
        initial, loading, loaded
    };

    struct DirectXVertexShader {
        int id;
        std::wstring fileName;
        winrt::com_ptr<ID3D11VertexShader> shader;
        std::vector<::byte> vertexShaderFileData;
        DirectXVertexShaderState state = DirectXVertexShaderState::initial;
    };

}