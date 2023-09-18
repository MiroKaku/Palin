#include "Main.App.h"


namespace Mi::Palin
{
    App::~App()
    {
        Close();
    }

    App::App()
    {
        UINT Flags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;

#if _DEBUG
        Flags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

        winrt::hresult Result = D3D11CreateDevice(
            nullptr,
            D3D_DRIVER_TYPE_HARDWARE,
            nullptr,
            Flags,
            nullptr, 0,
            D3D11_SDK_VERSION,
            mDevice.put(),
            nullptr,
            nullptr);
        if (Result == DXGI_ERROR_UNSUPPORTED) {
            Result = D3D11CreateDevice(
                nullptr,
                D3D_DRIVER_TYPE_WARP,
                nullptr,
                Flags,
                nullptr, 0,
                D3D11_SDK_VERSION,
                mDevice.put(),
                nullptr,
                nullptr);
        }
        winrt::check_hresult(Result);

        const HWND BackgroundWindow = GetShellWindow();

        RECT WindowRect{};
        winrt::check_bool(GetClientRect(BackgroundWindow, &WindowRect));

        const winrt::Windows::Graphics::SizeInt32 WindowSize
        {
            WindowRect.right  - WindowRect.left,
            WindowRect.bottom - WindowRect.top
        };

        const auto DXGIDevice = mDevice.as<IDXGIDevice2>();

        winrt::com_ptr<IDXGIAdapter> Adapter;
        winrt::check_hresult(DXGIDevice->GetParent(IID_PPV_ARGS(&Adapter)));

        winrt::com_ptr<IDXGIFactory2> Factory;
        winrt::check_hresult(Adapter->GetParent(IID_PPV_ARGS(&Factory)));

        DXGI_SWAP_CHAIN_DESC1 SwapChainDesc = {};
        SwapChainDesc.Width              = WindowSize.Width;
        SwapChainDesc.Height             = WindowSize.Height;
        SwapChainDesc.Format             = DXGI_FORMAT_B8G8R8A8_UNORM;
        SwapChainDesc.BufferUsage        = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        SwapChainDesc.SampleDesc.Count   = 1;
        SwapChainDesc.SampleDesc.Quality = 0;
        SwapChainDesc.BufferCount        = 2;
        SwapChainDesc.Scaling            = DXGI_SCALING_STRETCH;
        SwapChainDesc.SwapEffect         = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;
        SwapChainDesc.AlphaMode          = DXGI_ALPHA_MODE_UNSPECIFIED;

        winrt::com_ptr<IDXGISwapChain1> SwapChain{};
        winrt::check_hresult(Factory->CreateSwapChainForComposition(
            mDevice.get(), &SwapChainDesc, nullptr, SwapChain.put()));

        mRender            = std::make_unique<Core::GraphicsRender>(SwapChain);
        mCaptureForTexture = std::make_unique<Core::GraphicsCaptureForTexture>(mDevice, DXGI_FORMAT_B8G8R8A8_UNORM);
        mCaptureForWindow  = std::make_unique<Core::GraphicsCaptureForWindow >(mDevice, DXGI_FORMAT_B8G8R8A8_UNORM);
    }

    void App::Close()
    {
        StopPlay();

        mCaptureForTexture = nullptr;
        mCaptureForWindow  = nullptr;
        mRender            = nullptr;
        mDevice            = nullptr;
    }

    winrt::com_ptr<IDXGISwapChain1> App::GetSwapChain() const
    {
        if (mRender) {
            return mRender->GetSwapChain();
        }
        return { nullptr };
    }

    void App::SetKeyedMutex(_In_ bool Enable, _In_ UINT32 AcquireKey, _In_ UINT32 ReleaseKey, _In_ UINT32 Timeout)
    {
        mKeyedMutex = Enable;
        mAcquireKey = AcquireKey;
        mReleaseKey = ReleaseKey;
        mTimeout    = Timeout;
    }

    void App::SetRotationMode(_In_ DXGI_MODE_ROTATION Mode)
    {
        mRotationMode = Mode;
    }

    winrt::hresult App::StartPlay(_In_ HWND Window)
    {
        const auto Result = mCaptureForWindow->StartCapture(Window);
        if (FAILED(Result)) {
            return Result;
        }

        return StartRenderThread(mCaptureForWindow.get());
    }

    winrt::hresult App::StartPlay(_In_ HWND Window, _In_ LPCWSTR Name)
    {
        const auto Result = mCaptureForTexture->StartCapture(Window, Name);
        if (FAILED(Result)) {
            return Result;
        }

        return StartRenderThread(mCaptureForTexture.get());
    }

    winrt::hresult App::StartPlay(_In_ HWND Window, _In_ HANDLE Handle, _In_ bool NtHandle)
    {
        const auto Result = mCaptureForTexture->StartCapture(Window, Handle, NtHandle);
        if (FAILED(Result)) {
            return Result;
        }

        return StartRenderThread(mCaptureForTexture.get());
    }

    winrt::hresult App::StartRenderThread(Core::IGraphicsCapture* Capture)
    {
        mResizeCount = 1;

        Capture->IsBorderRequired(false);
        Capture->IsCursorCaptureEnabled(false);
        Capture->SubscribeClosedEvent([this](HWND) { if (mClosedRevoker) mClosedRevoker(); });
        Capture->SubscribeResizeEvent([this](HWND)
        {
            ++mResizeCount;
        });

        const auto RenderThread = [this, Capture]
        {
            LOG(INFO, "App::RenderThread() startup.");

            while (mStarted) {
                winrt::hresult Result;

                const auto& Surface = Capture->GetSurface();
                if (Surface == nullptr) {
                    std::this_thread::yield();
                    continue;
                }

                winrt::com_ptr<IDXGIKeyedMutex> SurfaceMutex{ nullptr };
                if (mKeyedMutex) {
                    (void)Surface->QueryInterface(IID_PPV_ARGS(&SurfaceMutex));
                }

                if (mResizeCount) {
                    D3D11_TEXTURE2D_DESC TextureDesc{};
                    Surface->GetDesc(&TextureDesc);

                    Result = mRender->Resize(TextureDesc.Width, TextureDesc.Height, DXGI_FORMAT_B8G8R8A8_UNORM);
                    if (FAILED(Result)) {
                        LOG(ERROR, "App::RenderThread, GraphicsRender::Resize(%ux%u, %d) failed, Result=0x%0*X",
                            TextureDesc.Width, TextureDesc.Height, DXGI_FORMAT_B8G8R8A8_UNORM, 8, Result.value);
                        break;
                    }

                    --mResizeCount; continue;
                }

                try {
                    winrt::check_hresult(mRender->BeginFrame());
                    {
                        if (SurfaceMutex) {
                            Result = SurfaceMutex->AcquireSync(mAcquireKey, mTimeout);
                            if (SUCCEEDED(Result)) {
                                winrt::check_hresult(mRender->Draw(Surface.get(),
                                    nullptr, false, {}, mRotationMode));

                                (void)SurfaceMutex->ReleaseSync(mReleaseKey);
                            }
                        }
                        else {
                            winrt::check_hresult(mRender->Draw(Surface.get(),
                                nullptr, false, {}, mRotationMode));
                        }
                    }
                    winrt::check_hresult(mRender->EndFrame(1, 0));

                } catch (const winrt::hresult_error& Exception) {
                    std::this_thread::yield();
                    Result = Exception.code();
                    LOG(ERROR, "App::RenderThread() has unhandled exception, Result=0x%0*X", 8, Result.value);
                }
            }

            LOG(INFO, "App::RenderThread() quit.");
        };

        winrt::hresult Result;
        try {
            mRenderThread = std::thread(RenderThread);
            mStarted      = true;
        }
        catch (const std::system_error& Exception) {
            Result = HRESULT_FROM_WIN32(Exception.code().value());
            LOG(ERROR, "App::RenderThread() startup failed., Result=0x%0*X", 8, Result.value);
        }

        return Result;
    }

    winrt::hresult App::StopPlay()
    {
        mStarted = false;
        if (mRenderThread.joinable()) {
            mRenderThread.join();
        }

        if (mCaptureForTexture) {
            mCaptureForTexture->StopCapture();
        }
        if (mCaptureForWindow) {
            mCaptureForWindow->StopCapture();
        }

        return S_OK;
    }

    void App::RegisterClosedRevoker(const std::function<void()>& Revoker)
    {
        mClosedRevoker = Revoker;
    }
}
