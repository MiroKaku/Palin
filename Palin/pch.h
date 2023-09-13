#pragma once

// Windows SDK support
#include <Unknwn.h>
#include <inspectable.h>
#include <dispatcherqueue.h>
#include <Windowsx.h>

// WinRT
#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Foundation.Collections.h>
#include <winrt/Windows.Foundation.Metadata.h>
#include <winrt/Windows.Graphics.DirectX.h>
#include <winrt/Windows.Graphics.DirectX.Direct3d11.h>
#include <winrt/Windows.Graphics.Imaging.h>
#include <winrt/Windows.Security.Authorization.AppCapabilityAccess.h>
#include <winrt/Windows.Storage.h>
#include <winrt/Windows.Storage.Pickers.h>
#include <winrt/Windows.Storage.Streams.h>
#include <winrt/Windows.System.h>
#include <winrt/Windows.UI.h>
#include <winrt/Windows.UI.Composition.h>
#include <winrt/Windows.UI.Composition.Desktop.h>
#include <winrt/Windows.UI.Popups.h>

// STL
#include <atomic>
#include <memory>
#include <algorithm>
#include <unordered_set>
#include <vector>
#include <optional>
#include <future>
#include <mutex>

// D3D
#include <d3d11_4.h>
#include <dxgi1_6.h>

// DWM
#include <dwmapi.h>

// Interop
#include "Interop.Direct3D11.h"
#include "Interop.Composition.h"

// Global Data
EXTERN_C IMAGE_DOS_HEADER __ImageBase;
#define HINST_THISCOMPONENT ((HINSTANCE)&__ImageBase)

#define LOG_ERROR 0
#define LOG_INFO  1
#define LOG(Tag, fmt, ...) \
    if constexpr (LOG_##Tag == LOG_ERROR){ \
        fprintf(stderr, "[Mi.Palin][!] " fmt "\n", ## __VA_ARGS__); \
    } \
    else if constexpr(LOG_##Tag == LOG_INFO) { \
        fprintf(stdout, "[Mi.Palin][+] " fmt "\n", ## __VA_ARGS__); \
    }
