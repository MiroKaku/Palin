#pragma once

namespace Mi::Core
{
    constexpr size_t NUMBER_VERTICES = 6;

    class GraphicsRender
    {
    public:
        /* method */
        ~GraphicsRender();
        GraphicsRender(      GraphicsRender&&) noexcept = default;
        GraphicsRender(const GraphicsRender& ) = default;
        GraphicsRender& operator=(      GraphicsRender&&) noexcept = default;
        GraphicsRender& operator=(const GraphicsRender& ) = default;

        explicit GraphicsRender(_In_ const winrt::com_ptr<IDXGISwapChain1>& SwapChain);

        [[nodiscard]] winrt::hresult BeginFrame() const;
        winrt::hresult EndFrame(
            _In_ UINT SyncInterval,
            _In_ UINT PresentFlags,
            _In_opt_ const DXGI_PRESENT_PARAMETERS* PresentParameters = nullptr) const;

        winrt::hresult Draw(
            _In_ ID3D11Texture2D* Texture,
            _In_opt_ const RECT*  Dirty = nullptr,
            _In_opt_ bool   BlendState  = true,
            _In_opt_ POINT  Offset      = {},
            _In_opt_ DXGI_MODE_ROTATION RotationMode = DXGI_MODE_ROTATION::DXGI_MODE_ROTATION_IDENTITY) const;

        winrt::hresult GetBackBuffer(_Out_ ID3D11Texture2D** BackBuffer) const;
        [[nodiscard]] winrt::com_ptr<IDXGISwapChain1> GetSwapChain() const;

        winrt::hresult Resize(
            _In_ UINT Width,
            _In_ UINT Height,
            _In_ DXGI_FORMAT Format
        );

        void Close();

    private:
        winrt::hresult CreateShaders();
        winrt::hresult CreateRenderTargetView();
        winrt::hresult CreateSamplerState();
        winrt::hresult CreateBlendState();
        [[nodiscard]] winrt::hresult SetViewPort(_In_ UINT Width, _In_ UINT Height) const;

        winrt::hresult SetDirtyVertex(
            _Out_writes_(NUMBER_VERTICES) struct VERTEX* Vertices,
            _In_ const RECT* Dirty,
            _In_       SIZE  ThisSize,
            _In_opt_   POINT Offset,
            _In_opt_   DXGI_MODE_ROTATION RotationMode) const;

    private:
        winrt::com_ptr<ID3D11Device>            mDevice{};
        winrt::com_ptr<IDXGISwapChain1>         mSwapChain{};

        winrt::com_ptr<ID3D11RenderTargetView>  mRenderTargetView{};
        winrt::com_ptr<ID3D11SamplerState>      mSamplerState{};
        winrt::com_ptr<ID3D11BlendState>        mBlendState{};

        winrt::com_ptr<ID3D11VertexShader>      mVertexShader{};
        winrt::com_ptr<ID3D11PixelShader>       mPixelShader{};
        winrt::com_ptr<ID3D11InputLayout>       mInputLayout{};
    };

}
