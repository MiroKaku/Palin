#include "Core.GraphicsCapture.h"


namespace Mi::Core
{
    GraphicsCaptureForWindow::GraphicsCaptureForWindow(
        _In_ const winrt::com_ptr<ID3D11Device>& Device,
        _In_ DXGI_FORMAT Format)
        : mFormat(Format)
        , mDevice(Device)
    {
    }

    winrt::hresult GraphicsCaptureForWindow::StartCapture(_In_ HWND Window)
    {
        if (IsValid()) {
            return DXGI_ERROR_INVALID_CALL;
        }

        winrt::hresult Result;

        try {
            // Capture
            const auto CaptureInterop = winrt::get_activation_factory<
                winrt::Windows::Graphics::Capture::GraphicsCaptureItem, IGraphicsCaptureItemInterop>();

            winrt::check_hresult(CaptureInterop->CreateForWindow(
                Window,
                winrt::guid_of<ABI::Windows::Graphics::Capture::IGraphicsCaptureItem>(),
                winrt::put_abi(mCapture)));
            mCaptureClosedRevoker = mCapture.Closed(winrt::auto_revoke, { this, &GraphicsCaptureForWindow::OnClosed });

            // FramePool
            const auto DXGIDevice = mDevice.as<IDXGIDevice>();
            mDirect3DDevice = CreateDirect3DDevice(DXGIDevice.get());

            mFramePool = winrt::Windows::Graphics::Capture::Direct3D11CaptureFramePool::Create(
                mDirect3DDevice,
                static_cast<winrt::Windows::Graphics::DirectX::DirectXPixelFormat>(mFormat),
                2,
                mSize);
            mCaptureUpdateRevoker = mFramePool.FrameArrived(winrt::auto_revoke, { this, &GraphicsCaptureForWindow::OnUpdate });

            // Session
            mSession = mFramePool.CreateCaptureSession(mCapture);
            mWindow  = Window;
            mSize    = mCapture.Size();

            // Surface
            winrt::check_hresult(CreateSharedSurface());

            mSession.StartCapture();
        }
        catch (const winrt::hresult_error& Exception) {
            Result = Exception.code();
        }

        return Result;
    }

    winrt::hresult GraphicsCaptureForWindow::StopCapture()
    {
        mCaptureUpdateRevoker = {};
        mCaptureClosedRevoker = {};

        mSession        = nullptr;
        mFramePool      = nullptr;
        mCapture        = nullptr;
        mDirect3DDevice = nullptr;
        mSurface        = nullptr;

        return S_OK;
    }

    winrt::hresult GraphicsCaptureForWindow::GetDirtyRect(RECT& DirtyRect) const
    {
        return DwmGetWindowAttribute(mWindow, DWMWA_EXTENDED_FRAME_BOUNDS,
            &DirtyRect, sizeof(DirtyRect));
    }

    [[nodiscard]] HANDLE GraphicsCaptureForWindow::GetSurfaceHandle() const
    {
        HANDLE Handle = nullptr;
        if (mSurface) {
            winrt::com_ptr<IDXGIResource> Resource;
            if (SUCCEEDED(mSurface->QueryInterface(IID_PPV_ARGS(&Resource)))) {
                (void)Resource->GetSharedHandle(&Handle);
            }
        }
        return Handle;
    }

    [[nodiscard]] winrt::com_ptr<ID3D11Texture2D> GraphicsCaptureForWindow::GetSurface() const
    {
        return mSurface;
    }

    [[nodiscard]] bool GraphicsCaptureForWindow::IsValid() const
    {
        return !!mSurface;
    }

    [[nodiscard]] bool GraphicsCaptureForWindow::IsCursorCaptureEnabled() const
    {
        if (mSession) {
            if (winrt::Windows::Foundation::Metadata::ApiInformation::IsPropertyPresent(
                winrt::name_of<winrt::Windows::Graphics::Capture::GraphicsCaptureSession>(), L"IsCursorCaptureEnabled")) {
                // Windows 10, version 2004 (introduced in 10.0.19041.0)
                return mSession.IsCursorCaptureEnabled();
            }
        }
        return true;
    }

    void GraphicsCaptureForWindow::IsCursorCaptureEnabled(_In_ bool Enabled)
    {
        if (mSession) {
            if (winrt::Windows::Foundation::Metadata::ApiInformation::IsPropertyPresent(
                winrt::name_of<winrt::Windows::Graphics::Capture::GraphicsCaptureSession>(), L"IsCursorCaptureEnabled")) {
                // Windows 10, version 2004 (introduced in 10.0.19041.0)
                return mSession.IsCursorCaptureEnabled(Enabled);
            }
        }
    }

    [[nodiscard]] bool GraphicsCaptureForWindow::IsBorderRequired() const
    {
        if (mSession) {
            if (winrt::Windows::Foundation::Metadata::ApiInformation::IsPropertyPresent(
                winrt::name_of<winrt::Windows::Graphics::Capture::GraphicsCaptureSession>(), L"IsBorderRequired")) {
                // Windows 10, version 2004 (introduced in 10.0.19041.0)
                return mSession.IsBorderRequired();
            }
        }
        return true;
    }

    void GraphicsCaptureForWindow::IsBorderRequired(_In_ bool Enabled)
    {
        if (mSession) {
            if (winrt::Windows::Foundation::Metadata::ApiInformation::IsPropertyPresent(
                winrt::name_of<winrt::Windows::Graphics::Capture::GraphicsCaptureSession>(), L"IsBorderRequired")) {
                // Windows 10, version 2004 (introduced in 10.0.19041.0)
                return mSession.IsBorderRequired(Enabled);
            }
        }
    }

    void GraphicsCaptureForWindow::SubscribeClosedEvent(_In_ const std::function<void(_In_ HWND Window)>& Handler) noexcept
    {
        mClosedHandler = Handler;
    }

    winrt::hresult GraphicsCaptureForWindow::CreateSharedSurface()
    {
        D3D11_TEXTURE2D_DESC Texture2DDesc{};
        Texture2DDesc.Format             = mFormat;
        Texture2DDesc.Width              = mSize.Width;
        Texture2DDesc.Height             = mSize.Height;
        Texture2DDesc.MipLevels          = 1;
        Texture2DDesc.ArraySize          = 1;
        Texture2DDesc.SampleDesc.Count   = 1;
        Texture2DDesc.SampleDesc.Quality = 0;
        Texture2DDesc.BindFlags          = D3D11_BIND_SHADER_RESOURCE;
        Texture2DDesc.Usage              = D3D11_USAGE_DEFAULT;
        Texture2DDesc.MiscFlags          = D3D11_RESOURCE_MISC_SHARED;
        return mDevice->CreateTexture2D(&Texture2DDesc, nullptr, mSurface.put());
    }

    void GraphicsCaptureForWindow::OnUpdate(
        _In_ const winrt::Windows::Graphics::Capture::Direct3D11CaptureFramePool& Sender,
        _In_ const winrt::Windows::Foundation::IInspectable& Object)
    {
        const auto Frame = Sender.TryGetNextFrame();

        if (const auto FrameSize = Frame.ContentSize(); FrameSize != mSize) {
            mSize = FrameSize;
            return OnResize(Sender, Object);
        }

        winrt::com_ptr<ID3D11DeviceContext> D3D11Context;
        mDevice->GetImmediateContext(D3D11Context.put());

        const auto WithFrame = GetDXGIInterfaceFromObject<ID3D11Texture2D>(Frame.Surface());
        D3D11Context->CopyResource(mSurface.get(), WithFrame.get());
    }

    void GraphicsCaptureForWindow::OnResize(
        _In_ const winrt::Windows::Graphics::Capture::Direct3D11CaptureFramePool& Sender,
        _In_ const winrt::Windows::Foundation::IInspectable&)
    {
        mSurface = nullptr;

        Sender.Recreate(
            mDirect3DDevice,
            static_cast<winrt::Windows::Graphics::DirectX::DirectXPixelFormat>(mFormat),
            2,
            mSize);

        winrt::check_hresult(CreateSharedSurface());
    }

    void GraphicsCaptureForWindow::OnClosed(
        _In_ const winrt::Windows::Graphics::Capture::GraphicsCaptureItem& Sender,
        _In_ const winrt::Windows::Foundation::IInspectable&)
    {
        UNREFERENCED_PARAMETER(Sender);

        StopCapture();
    }
}
