#include "Core.GraphicsCapture.h"


namespace Mi::Core
{
    GraphicsCaptureForTexture::~GraphicsCaptureForTexture()
    {
        StopCapture();
    }

    GraphicsCaptureForTexture::GraphicsCaptureForTexture(_In_ const winrt::com_ptr<ID3D11Device>& Device, _In_ DXGI_FORMAT Format)
        : mFormat(Format)
        , mDevice(Device)
    {
    }

    winrt::hresult GraphicsCaptureForTexture::StartCapture(_In_ HWND Window, _In_ HANDLE  SharedHandle, _In_opt_ bool NtHandle)
    {
        if (IsValid()) {
            return DXGI_ERROR_INVALID_CALL;
        }

        mWindow = Window;

        winrt::hresult Result;
        if (NtHandle) {
            DWORD TargetProcessId = 0;
            GetWindowThreadProcessId(Window, &TargetProcessId);

            const auto TargetProcess = std::unique_ptr<std::remove_pointer_t<HANDLE>, decltype(::CloseHandle)*>(
                OpenProcess(PROCESS_DUP_HANDLE, FALSE, TargetProcessId), ::CloseHandle);
            if (TargetProcess == nullptr) {
                return HRESULT_FROM_WIN32(GetLastError());
            }

            if (!DuplicateHandle(TargetProcess.get(), SharedHandle, GetCurrentProcess(), &SharedHandle,
                0, FALSE, DUPLICATE_SAME_ACCESS)) {
                return HRESULT_FROM_WIN32(GetLastError());
            }
            const auto NewHandle = std::unique_ptr<std::remove_pointer_t<HANDLE>, decltype(::CloseHandle)*>(
                SharedHandle, ::CloseHandle);

            Result = mDevice.as<ID3D11Device1>()->OpenSharedResource1(SharedHandle, IID_PPV_ARGS(&mSurface));
        }
        else {
            Result = mDevice->OpenSharedResource(SharedHandle, IID_PPV_ARGS(&mSurface));
        }

        if (FAILED(Result)) {
            return Result;
        }

        D3D11_TEXTURE2D_DESC TexDesc{};
        mSurface->GetDesc(&TexDesc);

        LOG(INFO, "GraphicsCaptureForTexture::StartCapture(), target:"
            "\n\t Width  = %u"
            "\n\t Height = %u"
            "\n\t Format = %d",
            TexDesc.Width, TexDesc.Height, TexDesc.Format);

        try {
            mWatchDog = std::thread(&GraphicsCaptureForTexture::WatchDog, this);
        }
        catch(const std::system_error& Exception) {
            Result = HRESULT_FROM_WIN32(Exception.code().value());
        }

        return Result;
    }

    winrt::hresult GraphicsCaptureForTexture::StartCapture(_In_ HWND Window, _In_ LPCWSTR SharedName)
    {
        if (IsValid()) {
            return DXGI_ERROR_INVALID_CALL;
        }

        mWindow = Window;

        winrt::hresult Result = mDevice.as<ID3D11Device1>()->OpenSharedResourceByName(SharedName,
            DXGI_SHARED_RESOURCE_READ, IID_PPV_ARGS(&mSurface));
        if (FAILED(Result)) {
            return Result;
        }

        try {
            mWatchDog = std::thread(&GraphicsCaptureForTexture::WatchDog, this);
        }
        catch (const std::system_error& Exception) {
            Result = HRESULT_FROM_WIN32(Exception.code().value());
        }

        return Result;
    }

    winrt::hresult GraphicsCaptureForTexture::StopCapture()
    {
        mSurface = nullptr;

        if (mWatchDog.joinable()) {
            if (mWatchDog.get_id() != std::this_thread::get_id()) {
                mWatchDog.join();
            }
        }

        return S_OK;
    }

    winrt::hresult GraphicsCaptureForTexture::GetDirtyRect(RECT& DirtyRect) const
    {
        if(!GetClientRect(mWindow, &DirtyRect)) {
            return HRESULT_FROM_WIN32(GetLastError());
        }

        if (MapWindowPoints(mWindow, HWND_DESKTOP,
            reinterpret_cast<POINT*>(&DirtyRect), 2) == 0) {
            return HRESULT_FROM_WIN32(GetLastError());
        }

        return S_OK;
    }

    HANDLE GraphicsCaptureForTexture::GetSurfaceHandle() const
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

    winrt::com_ptr<ID3D11Texture2D> GraphicsCaptureForTexture::GetSurface() const
    {
        return mSurface;
    }

    bool GraphicsCaptureForTexture::IsValid() const
    {
        return !!mSurface;
    }

    bool GraphicsCaptureForTexture::IsCursorCaptureEnabled() const
    {
        return false;
    }

    void GraphicsCaptureForTexture::IsCursorCaptureEnabled(_In_ bool Enabled)
    {
        UNREFERENCED_PARAMETER(Enabled);
    }

    bool GraphicsCaptureForTexture::IsBorderRequired() const
    {
        return false;
    }

    void GraphicsCaptureForTexture::IsBorderRequired(_In_ bool Enabled)
    {
        UNREFERENCED_PARAMETER(Enabled);
    }

    void GraphicsCaptureForTexture::SubscribeClosedEvent(_In_ const std::function<void(_In_ HWND Window)>& Handler) noexcept
    {
        mClosedHandler = Handler;
    }

    void GraphicsCaptureForTexture::SubscribeResizeEvent(_In_ const std::function<void(_In_ HWND Window)>& Handler) noexcept
    {
        mResizeHandler = Handler;
    }

    winrt::hresult GraphicsCaptureForTexture::WatchDog()
    {
        RECT ClientArea{};
        if (!GetClientRect(mWindow, &ClientArea)) {
            return HRESULT_FROM_WIN32(GetLastError());
        }

        while (IsValid()) {
            if (!IsWindow(mWindow)) {
                break;
            }

            if (IsMinimized(mWindow)) {
                break;
            }

            RECT NewArea{};
            if (!GetClientRect(mWindow, &NewArea)) {
                break;
            }

            // Resize
            if ((ClientArea.right  - ClientArea.left) != (NewArea.right  - NewArea.left) ||
                (ClientArea.bottom - ClientArea.top ) != (NewArea.bottom - NewArea.top )) {

                OnResize(mWindow, {
                    (NewArea.right  - NewArea.left),
                    (NewArea.bottom - NewArea.top)
                });
            }

            std::this_thread::yield();
        }

        if (IsValid()) {
            OnClosed(mWindow);
        }

        return S_OK;
    }

    void GraphicsCaptureForTexture::OnResize(
        _In_ HWND Sender,
        _In_ winrt::Windows::Graphics::SizeInt32 Size)
    {
        UNREFERENCED_PARAMETER(Sender);
        UNREFERENCED_PARAMETER(Size);

        if (mResizeHandler) {
            mResizeHandler(mWindow);
        }
    }

    void GraphicsCaptureForTexture::OnClosed(
        _In_ HWND Sender)
    {
        UNREFERENCED_PARAMETER(Sender);

        StopCapture();

        if (mClosedHandler) {
            mClosedHandler(mWindow);
        }
    }
}
