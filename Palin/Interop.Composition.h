#pragma once
#include <winrt/Windows.UI.Composition.Desktop.h>
#include <Windows.UI.Composition.Interop.h>
#include <d2d1_1.h>


inline auto CreateDispatcherQueueController(
    _In_ DISPATCHERQUEUE_THREAD_TYPE            const ThreadMode,
    _In_ DISPATCHERQUEUE_THREAD_APARTMENTTYPE   const ComMode
)
{
    const DispatcherQueueOptions Options
    {
        sizeof(DispatcherQueueOptions),
        ThreadMode,
        ComMode
    };

    winrt::Windows::System::DispatcherQueueController Controller{ nullptr };
    winrt::check_hresult(::CreateDispatcherQueueController(Options,
        reinterpret_cast<ABI::Windows::System::IDispatcherQueueController**>(winrt::put_abi(Controller))));
    return Controller;
}

inline auto CreateDesktopWindowTarget(
    _In_ winrt::Windows::UI::Composition::Compositor const& Compositor,
    _In_ const HWND Window,
    _In_ const bool IsTopMost
)
{
    const auto Interop = Compositor.as<ABI::Windows::UI::Composition::Desktop::ICompositorDesktopInterop>();

    winrt::Windows::UI::Composition::Desktop::DesktopWindowTarget Target{ nullptr };
    winrt::check_hresult(Interop->CreateDesktopWindowTarget(Window, IsTopMost,
        reinterpret_cast<ABI::Windows::UI::Composition::Desktop::IDesktopWindowTarget**>(winrt::put_abi(Target))));
    return Target;
}

inline auto CreateCompositionGraphicsDevice(
    _In_ winrt::Windows::UI::Composition::Compositor const& Compositor,
    _In_ ::IUnknown* Device
)
{
    const auto CompositorInterop = Compositor.as<ABI::Windows::UI::Composition::ICompositorInterop>();

    winrt::com_ptr<ABI::Windows::UI::Composition::ICompositionGraphicsDevice> GraphicsInterop{};
    winrt::check_hresult(CompositorInterop->CreateGraphicsDevice(Device, GraphicsInterop.put()));

    winrt::Windows::UI::Composition::CompositionGraphicsDevice GraphicsDevice{ nullptr };
    winrt::check_hresult(GraphicsInterop->QueryInterface(
        winrt::guid_of<winrt::Windows::UI::Composition::CompositionGraphicsDevice>(), 
        winrt::put_abi(GraphicsDevice)));

    return GraphicsDevice;
}

inline void ResizeSurface(
    _In_ winrt::Windows::UI::Composition::CompositionDrawingSurface const& Surface,
    _In_ winrt::Windows::Foundation::Size const& Size
)
{
    const auto SurfaceInterop = Surface.as<ABI::Windows::UI::Composition::ICompositionDrawingSurfaceInterop>();
    const SIZE NewSize =
    {
        static_cast<LONG>(std::round(Size.Width)),
        static_cast<LONG>(std::round(Size.Height))
    };

    winrt::check_hresult(SurfaceInterop->Resize(NewSize));
}

inline auto SurfaceBeginDraw(
    _In_ winrt::Windows::UI::Composition::CompositionDrawingSurface const& Surface
)
{
    const auto SurfaceInterop = Surface.as<ABI::Windows::UI::Composition::ICompositionDrawingSurfaceInterop>();
    
    POINT Offset = {};
    winrt::com_ptr<ID2D1DeviceContext> Context{};

    winrt::check_hresult(SurfaceInterop->BeginDraw(nullptr, IID_PPV_ARGS(&Context), &Offset));
    Context->SetTransform(D2D1::Matrix3x2F::Translation(static_cast<FLOAT>(Offset.x), static_cast<FLOAT>(Offset.y)));

    return Context;
}

inline void SurfaceEndDraw(
    _In_ winrt::Windows::UI::Composition::CompositionDrawingSurface const& Surface
)
{
    const auto SurfaceInterop = Surface.as<ABI::Windows::UI::Composition::ICompositionDrawingSurfaceInterop>();
    winrt::check_hresult(SurfaceInterop->EndDraw());
}

inline auto CreateCompositionSurfaceForSwapChain(
    _In_ winrt::Windows::UI::Composition::Compositor const& Compositor,
    _In_ ::IUnknown* SwapChain
)
{
    const auto CompositorInterop = Compositor.as<ABI::Windows::UI::Composition::ICompositorInterop>();

    winrt::com_ptr<ABI::Windows::UI::Composition::ICompositionSurface> SurfaceInterop{};
    winrt::check_hresult(CompositorInterop->CreateCompositionSurfaceForSwapChain(SwapChain, SurfaceInterop.put()));

    winrt::Windows::UI::Composition::ICompositionSurface Surface{ nullptr };
    winrt::check_hresult(SurfaceInterop->QueryInterface(
        winrt::guid_of<winrt::Windows::UI::Composition::ICompositionSurface>(),
        winrt::put_abi(Surface)));

    return Surface;
}
