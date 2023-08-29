#pragma once
#include "Window.StackPanel.h"
#include "Window.DesktopWindow.h"
#include "Core.GraphicsRender.h"
#include "Core.WindowList.h"


namespace Mi::Palin
{
    class App final
    {
        std::thread mRenderThread;
        std::unique_ptr<Core::GraphicsRender>   mRender       { nullptr };
        winrt::com_ptr<ID3D11Device>            mDevice       { nullptr };
        winrt::com_ptr<ID3D11Texture2D>         mSharedTexture{ nullptr };
        winrt::com_ptr<IDXGIKeyedMutex>         mSharedMutex  { nullptr };

        UINT32 mAcquireKey = 1;
        UINT32 mReleaseKey = 0;
        UINT32 mTimeout    = INFINITE;
        DXGI_MODE_ROTATION mRotationMode = DXGI_MODE_ROTATION_IDENTITY;

        std::atomic_bool mStarted = false;

    public:
        ~App();

        App();
        App(      App&&) = delete;
        App(const App& ) = delete;
        App& operator=(      App&&) = delete;
        App& operator=(const App& ) = delete;

        void Close();

        winrt::hresult StartPlayingFromSharedName(
            _In_     DXGI_MODE_ROTATION Mode,
            _In_     std::wstring_view Name,
            _In_     bool   IsUseKeyedMutex,
            _In_opt_ UINT32 AcquireKey = 0,
            _In_opt_ UINT32 ReleaseKey = 0,
            _In_opt_ UINT32 Timeout    = 0
        );

        winrt::hresult StartPlayingFromSharedHandle(
            _In_     DXGI_MODE_ROTATION Mode,
            _In_opt_ HANDLE Process,
            _In_     HANDLE Handle,
            _In_     bool   IsUseKeyedMutex,
            _In_opt_ UINT32 AcquireKey = 0,
            _In_opt_ UINT32 ReleaseKey = 0,
            _In_opt_ UINT32 Timeout    = 0
        );

        winrt::hresult StopPlay();

        [[nodiscard]] winrt::com_ptr<IDXGISwapChain1> GetSwapChain() const;

    private:
        winrt::hresult StartPlaying();
    };

}
