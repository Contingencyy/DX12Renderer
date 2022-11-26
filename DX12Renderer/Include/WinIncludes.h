#pragma once

#define NOMINMAX
#define WINDOWS_LEAN_AND_MEAN
#include <Windows.h>

#include <shellapi.h>
#include <wrl.h>
using namespace Microsoft::WRL;

#include <d3d12.h>
#include <dxgi1_6.h>
#include <dxgidebug.h>

#include "D3DX/d3dx12.h"
#include "DXC/inc/dxcapi.h"

#if defined(CreateWindow)
#undef CreateWindow
#endif

#if defined(near)
#undef near
#endif

#if defined(far)
#undef far
#endif

#if defined(LoadImage)
#undef LoadImage
#endif

#if defined(OPAQUE)
#undef OPAQUE
#endif

#if defined(TRANSPARENT)
#undef TRANSPARENT
#endif
