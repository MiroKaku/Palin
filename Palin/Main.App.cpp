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
            WindowRect.right - WindowRect.left,
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

        mRender = std::make_unique<Core::GraphicsRender>(SwapChain);
    }

    void App::Close()
    {
        StopPlay();

        mRender = nullptr;
        mDevice = nullptr;
    }

    winrt::hresult App::StartPlayingFromSharedName(
        _In_     HWND               Window,
        _In_     std::wstring_view  Name,
        _In_     DXGI_MODE_ROTATION Mode,
        _In_     bool               IsUseKeyedMutex,
        _In_opt_ UINT32             AcquireKey,
        _In_opt_ UINT32             ReleaseKey,
        _In_opt_ UINT32             Timeout
    )
    {
        if (mStarted) {
            return S_FALSE;
        }
        
        mTimeout      = Timeout;
        mAcquireKey   = AcquireKey;
        mReleaseKey   = ReleaseKey;
        mRotationMode = Mode;
        
        DWORD TargetProcessId = 0;
        GetWindowThreadProcessId(Window, &TargetProcessId);

        mWatchTarget = decltype(mWatchTarget)(OpenProcess(SYNCHRONIZE  | PROCESS_DUP_HANDLE, FALSE, TargetProcessId), [this](HANDLE Handle) -> void
        {
            if (Handle) {
                CloseHandle(Handle);

                if (mClosedRevoker) {
                    mClosedRevoker();
                }
            }
        });

        const auto Result = mDevice.as<ID3D11Device1>()->OpenSharedResourceByName(
            Name.data(), DXGI_SHARED_RESOURCE_READ, IID_PPV_ARGS(&mSharedTexture));
        if (FAILED(Result)) {
            return Result;
        }

        if (IsUseKeyedMutex) {
            (void)mSharedTexture->QueryInterface(IID_PPV_ARGS(&mSharedMutex));
        }

        return StartPlaying();
    }

    winrt::hresult App::StartPlayingFromSharedHandle(
        _In_     HWND               Window,
        _In_     HANDLE             Handle,
        _In_     DXGI_MODE_ROTATION Mode,
        _In_     bool               IsNtHandle,
        _In_     bool               IsUseKeyedMutex,
        _In_opt_ UINT32             AcquireKey,
        _In_opt_ UINT32             ReleaseKey,
        _In_opt_ UINT32             Timeout
    )
    {
        if (mStarted) {
            return S_FALSE;
        }

        winrt::hresult Result;
        
        mTimeout      = Timeout;
        mAcquireKey   = AcquireKey;
        mReleaseKey   = ReleaseKey;
        mRotationMode = Mode;

        DWORD TargetProcessId = 0;
        GetWindowThreadProcessId(Window, &TargetProcessId);

        mWatchTarget = decltype(mWatchTarget)(OpenProcess(SYNCHRONIZE  | PROCESS_DUP_HANDLE, FALSE, TargetProcessId), [this](HANDLE Handle) -> void
        {
            if (Handle) {
                CloseHandle(Handle);

                if (mClosedRevoker) {
                    mClosedRevoker();
                }
            }
        });

        if (IsNtHandle) {
            HANDLE NtHandle = nullptr;
            if (DuplicateHandle(mWatchTarget.get(), Handle, GetCurrentProcess(), &NtHandle,
                0, FALSE, DUPLICATE_SAME_ACCESS)) {

                Result = mDevice.as<ID3D11Device1>()->OpenSharedResource1(NtHandle, IID_PPV_ARGS(&mSharedTexture));

                CloseHandle(NtHandle);
            }
            else {
                Result = HRESULT_FROM_WIN32(GetLastError());
            }
        }
        else {
            Result = mDevice.as<ID3D11Device1>()->OpenSharedResource(Handle, IID_PPV_ARGS(&mSharedTexture));
        }

        if (FAILED(Result)) {
            return Result;
        }

        if (IsUseKeyedMutex) {
            (void)mSharedTexture->QueryInterface(IID_PPV_ARGS(&mSharedMutex));
        }

        return StartPlaying();
    }

    winrt::hresult App::StartPlaying()
    {
        D3D11_TEXTURE2D_DESC TextureDesc{};
        mSharedTexture->GetDesc(&TextureDesc);
        winrt::hresult Result = mRender->Resize(TextureDesc.Width, TextureDesc.Height, TextureDesc.Format);
        if (FAILED(Result)) {
            return Result;
        }

        static const auto RenderThread = [this]
        {
            winrt::hresult Result;

            while (mStarted) {
                if (WaitForSingleObject(mWatchTarget.get(), 0) == WAIT_OBJECT_0) {
                    mWatchTarget = nullptr;

                    break;
                }

                try {
                    winrt::check_hresult(mRender->BeginFrame());
                    {
                        if (mSharedMutex) {
                            Result = mSharedMutex->AcquireSync(mAcquireKey, mTimeout);
                            if (SUCCEEDED(Result)) {
                                winrt::check_hresult(mRender->Draw(mSharedTexture.get(),
                                    nullptr, false, {}, mRotationMode));

                                mSharedMutex->ReleaseSync(mReleaseKey);
                            }
                        }
                        else {
                            winrt::check_hresult(mRender->Draw(mSharedTexture.get(),
                                nullptr, false, {}, mRotationMode));
                        }
                    }
                    winrt::check_hresult(mRender->EndFrame(1, 0));

                } catch (const winrt::hresult_error& Exception) {
                    std::this_thread::yield();
                    Result = Exception.code();
                }
            }
        };

        try {
            mRenderThread = std::thread(RenderThread);
            mStarted      = true;
        }
        catch (const std::system_error& Exception) {
            Result = HRESULT_FROM_WIN32(Exception.code().value());
        }

        return Result;
    }

    winrt::hresult App::StopPlay()
    {
        if (mStarted) {
            mStarted = false;

            if (mRenderThread.joinable()) {
                mRenderThread.join();
            }

            mSharedMutex   = nullptr;
            mSharedTexture = nullptr;
        }

        return S_OK;
    }

    void App::RegisterClosedRevoke(const std::function<void()>& Revoker)
    {
        mClosedRevoker = Revoker;
    }

    winrt::com_ptr<IDXGISwapChain1> App::GetSwapChain() const
    {
        if (mRender) {
            return mRender->GetSwapChain();
        }
        return { nullptr };
    }

}
