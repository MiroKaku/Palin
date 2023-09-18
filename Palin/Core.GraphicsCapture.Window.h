#pragma once

// WinRT/Windows.Graphics.Capture
#include <winrt/Windows.Graphics.Capture.h>
#include <Windows.Graphics.Capture.Interop.h>


namespace Mi::Core
{
    class GraphicsCaptureForWindow final : public IGraphicsCapture
    {
        HWND        mWindow = nullptr;
        DXGI_FORMAT mFormat = DXGI_FORMAT_UNKNOWN;

        winrt::com_ptr<ID3D11Device>    mDevice { nullptr };
        winrt::com_ptr<ID3D11Texture2D> mSurface{ nullptr };

        winrt::Windows::Graphics::SizeInt32                            mSize          { 0 };
        winrt::Windows::Graphics::DirectX::Direct3D11::IDirect3DDevice mDirect3DDevice{ nullptr };
        winrt::Windows::Graphics::Capture::GraphicsCaptureItem         mCapture       { nullptr };
        winrt::Windows::Graphics::Capture::Direct3D11CaptureFramePool  mFramePool     { nullptr };
        winrt::Windows::Graphics::Capture::GraphicsCaptureSession      mSession       { nullptr };

        winrt::Windows::Graphics::Capture::IGraphicsCaptureItem::Closed_revoker             mCaptureClosedRevoker{};
        winrt::Windows::Graphics::Capture::Direct3D11CaptureFramePool::FrameArrived_revoker mCaptureUpdateRevoker{};

        std::function<void(HWND)> mClosedHandler;
        std::function<void(HWND)> mResizeHandler;

    public:
        virtual ~GraphicsCaptureForWindow() = default;

        GraphicsCaptureForWindow(      GraphicsCaptureForWindow&&) noexcept = default;
        GraphicsCaptureForWindow(const GraphicsCaptureForWindow& ) = delete;
        GraphicsCaptureForWindow& operator=(      GraphicsCaptureForWindow&&) noexcept = default;
        GraphicsCaptureForWindow& operator=(const GraphicsCaptureForWindow& ) = delete;

        explicit GraphicsCaptureForWindow(
            _In_ const winrt::com_ptr<ID3D11Device>& Device,
            _In_ DXGI_FORMAT Format);

        /* method */
        winrt::hresult StartCapture(_In_ HWND Window);
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
        winrt::hresult CreateSharedSurface();

        /* event */
        void OnUpdate(
            _In_ const winrt::Windows::Graphics::Capture::Direct3D11CaptureFramePool& Sender,
            _In_ const winrt::Windows::Foundation::IInspectable&);

        void OnResize(
            _In_ const winrt::Windows::Graphics::Capture::Direct3D11CaptureFramePool& Sender,
            _In_ const winrt::Windows::Foundation::IInspectable&);

        void OnClosed(
            _In_ const winrt::Windows::Graphics::Capture::GraphicsCaptureItem& Sender,
            _In_ const winrt::Windows::Foundation::IInspectable&);
    };
}
