#pragma once

#define WINDOWS_LEAN_AND_MEAN
#include <Windows.h>

#if defined(CreateWindow)
#undef CreateWindow
#endif

#if defined(min)
#undef min
#endif

#if defined(max)
#undef max
#endif

#include <shellapi.h>
#include <wrl.h>
using namespace Microsoft::WRL;

#include <d3d12.h>
#include <dxgi1_6.h>
#include <d3dcompiler.h>
#include <dxgidebug.h>

#include "D3DX/d3dx12.h"
