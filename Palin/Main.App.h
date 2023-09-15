#pragma once
#include "Window.StackPanel.h"
#include "Window.DesktopWindow.h"
#include "Core.GraphicsRender.h"
#include "Core.GraphicsCapture.h"
#include "Core.WindowList.h"


namespace Mi::Palin
{
    class App final
    {
        winrt::com_ptr<ID3D11Device>    mDevice { nullptr };
        winrt::com_ptr<ID3D11Texture2D> mSurface{ nullptr };
        winrt::com_ptr<IDXGIKeyedMutex> mSurfaceMutex{ nullptr };

        std::thread mRenderThread;
        std::unique_ptr<Core::GraphicsRender> mRender{ nullptr };
        std::unique_ptr<Core::GraphicsCaptureForTexture> mCaptureForTexture;
        std::unique_ptr<Core::GraphicsCaptureForWindow>  mCaptureForWindow;

        UINT32 mAcquireKey = 1;
        UINT32 mReleaseKey = 0;
        UINT32 mTimeout    = INFINITE;
        DXGI_MODE_ROTATION mRotationMode = DXGI_MODE_ROTATION_IDENTITY;

        std::atomic_bool mStarted = false;
        std::function<void()> mClosedRevoker = nullptr;

    public:
        ~App();

        App();
        App(      App&&) = delete;
        App(const App& ) = delete;
        App& operator=(      App&&) = delete;
        App& operator=(const App& ) = delete;

        void Close();

        [[nodiscard]] winrt::com_ptr<IDXGISwapChain1> GetSwapChain() const;

        void SetKeyedMutex  (_In_ bool Enable, _In_ UINT32 AcquireKey, _In_ UINT32 ReleaseKey, _In_ UINT32 Timeout);
        void SetRotationMode(_In_ DXGI_MODE_ROTATION Mode);

        winrt::hresult StartPlay(_In_ HWND Window);
        winrt::hresult StartPlay(_In_ HWND Window, _In_ LPCWSTR Name);
        winrt::hresult StartPlay(_In_ HWND Window, _In_ HANDLE Handle, _In_ bool NtHandle);
        winrt::hresult StopPlay();

        void RegisterClosedRevoker(const std::function<void()>& Revoker);

    private:
        winrt::hresult StartRenderThread();
    };

}
