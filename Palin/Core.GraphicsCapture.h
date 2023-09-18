#pragma once


namespace Mi::Core
{
    class IGraphicsCapture
    {
    public:
        virtual HANDLE GetSurfaceHandle() const = 0;
        virtual winrt::com_ptr<ID3D11Texture2D> GetSurface() const = 0;
        virtual winrt::hresult GetDirtyRect(RECT& DirtyRect) const = 0;

        virtual bool IsValid() const = 0;

        virtual bool IsCursorCaptureEnabled() const = 0;
        virtual void IsCursorCaptureEnabled(_In_ bool Enabled) = 0;

        virtual bool IsBorderRequired() const = 0;
        virtual void IsBorderRequired(_In_ bool Enabled) = 0;

        virtual void SubscribeClosedEvent(_In_ const std::function<void(_In_ HWND Window)>& Handler) noexcept = 0;
        virtual void SubscribeResizeEvent(_In_ const std::function<void(_In_ HWND Window)>& Handler) noexcept = 0;
    };
}

#include "Core.GraphicsCapture.Window.h"
#include "Core.GraphicsCapture.Texture.h"
