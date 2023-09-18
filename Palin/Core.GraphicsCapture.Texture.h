#pragma once
#include <winrt/windows.graphics.h>


namespace Mi::Core
{
    class GraphicsCaptureForTexture final : public IGraphicsCapture
    {
        HWND        mWindow = nullptr;
        DXGI_FORMAT mFormat = DXGI_FORMAT_UNKNOWN;

        winrt::com_ptr<ID3D11Device>    mDevice { nullptr };
        winrt::com_ptr<ID3D11Texture2D> mSurface{ nullptr };

        std::thread mWatchDog;
        std::function<void(HWND)> mClosedHandler;
        std::function<void(HWND)> mResizeHandler;

    public:
        virtual ~GraphicsCaptureForTexture();

        GraphicsCaptureForTexture(      GraphicsCaptureForTexture&&) noexcept = default;
        GraphicsCaptureForTexture(const GraphicsCaptureForTexture& ) = delete;
        GraphicsCaptureForTexture& operator=(      GraphicsCaptureForTexture&&) noexcept = default;
        GraphicsCaptureForTexture& operator=(const GraphicsCaptureForTexture& ) = delete;

        explicit GraphicsCaptureForTexture(
            _In_ const winrt::com_ptr<ID3D11Device>& Device,
            _In_ DXGI_FORMAT Format);

        /* method */
        winrt::hresult StartCapture(_In_ HWND Window, _In_ HANDLE  SharedHandle, _In_opt_ bool NtHandle = false);
        winrt::hresult StartCapture(_In_ HWND Window, _In_ LPCWSTR SharedName);
        winrt::hresult StopCapture ();

        /* interface */
        HANDLE GetSurfaceHandle() const override;
        winrt::com_ptr<ID3D11Texture2D> GetSurface() const override;
        winrt::hresult GetDirtyRect(RECT& DirtyRect) const override;

        bool IsValid() const override;

        bool IsCursorCaptureEnabled() const override;
        void IsCursorCaptureEnabled(_In_ bool Enabled) override;

        bool IsBorderRequired() const override;
        void IsBorderRequired(_In_ bool Enabled) override;

        void SubscribeClosedEvent(_In_ const std::function<void(_In_ HWND Window)>& Handler) noexcept override;
        void SubscribeResizeEvent(_In_ const std::function<void(_In_ HWND Window)>& Handler) noexcept override;

    private:
        winrt::hresult WatchDog();

        /* event */
        void OnResize(
            _In_ HWND Sender,
            _In_ winrt::Windows::Graphics::SizeInt32 Size);

        void OnClosed(
            _In_ HWND Sender);
    };

}
