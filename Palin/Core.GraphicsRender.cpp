#include "Core.GraphicsRender.h"

#include <d3dcompiler.h>
#pragma comment(lib, "d3dcompiler.lib")

namespace Mi::Core
{
    struct VERTEX
    {
        DirectX::XMFLOAT3 Position;
        DirectX::XMFLOAT2 Texcoord;
    };

    GraphicsRender::GraphicsRender(_In_ const winrt::com_ptr<IDXGISwapChain1>& SwapChain)
        : mSwapChain(SwapChain)
    {
        DXGI_SWAP_CHAIN_DESC1 SwapChainDesc{};
        winrt::check_hresult(SwapChain->GetDesc1(&SwapChainDesc));
        winrt::check_hresult(SwapChain->GetDevice(IID_PPV_ARGS(&mDevice)));
        winrt::check_hresult(CreateRenderTargetView());
        winrt::check_hresult(SetViewPort(SwapChainDesc.Width, SwapChainDesc.Height));
        winrt::check_hresult(CreateSamplerState());
        winrt::check_hresult(CreateBlendState());
        winrt::check_hresult(CreateShaders());
    }

    GraphicsRender::~GraphicsRender()
    {
        Close();
    }

    winrt::hresult GraphicsRender::BeginFrame() const
    {
        return S_OK;
    }

    winrt::hresult GraphicsRender::EndFrame(
        _In_ const UINT SyncInterval,
        _In_ const UINT PresentFlags,
        _In_opt_ const DXGI_PRESENT_PARAMETERS* PresentParameters
    ) const
    {
        constexpr DXGI_PRESENT_PARAMETERS Empty{};
        if (PresentParameters == nullptr) {
            PresentParameters = &Empty;
        }

        return mSwapChain->Present1(
            SyncInterval,
            SyncInterval ? PresentFlags : (PresentFlags | DXGI_PRESENT_ALLOW_TEARING),
            PresentParameters);
    }

    winrt::hresult GraphicsRender::Draw(
        _In_ ID3D11Texture2D* Texture,
        _In_opt_ const RECT*  Dirty,
        _In_opt_ const bool   BlendState,
        _In_opt_ const POINT  Offset,
        _In_opt_ const DXGI_MODE_ROTATION RotationMode) const
    {
        winrt::hresult Result;

        do {

            D3D11_TEXTURE2D_DESC TextureDesc{};
            Texture->GetDesc(&TextureDesc);

            VERTEX Vertices[NUMBER_VERTICES]{};
            Result = SetDirtyVertex(Vertices, Dirty, { static_cast<long>(TextureDesc.Width), static_cast<long>(TextureDesc.Height) },
                Offset, RotationMode);
            if (FAILED(Result)) {
                return Result;
            }

            // Create new shader resource view
            D3D11_SHADER_RESOURCE_VIEW_DESC ShaderDesc{};
            ShaderDesc.Format                    = TextureDesc.Format;
            ShaderDesc.ViewDimension             = D3D11_SRV_DIMENSION_TEXTURE2D;
            ShaderDesc.Texture2D.MostDetailedMip = TextureDesc.MipLevels - 1;
            ShaderDesc.Texture2D.MipLevels       = TextureDesc.MipLevels;

            winrt::com_ptr<ID3D11ShaderResourceView> ShaderResource{};
            Result = mDevice->CreateShaderResourceView(Texture, &ShaderDesc, ShaderResource.put());
            if (FAILED(Result)) {
                return Result;
            }

            // Create vertex buffer
            D3D11_BUFFER_DESC BufferDesc{};
            BufferDesc.Usage            = D3D11_USAGE_DEFAULT;
            BufferDesc.ByteWidth        = sizeof(VERTEX) * NUMBER_VERTICES;
            BufferDesc.BindFlags        = D3D11_BIND_VERTEX_BUFFER;
            BufferDesc.CPUAccessFlags   = 0;

            D3D11_SUBRESOURCE_DATA InitData{};
            InitData.pSysMem = Vertices;
            
            winrt::com_ptr<ID3D11Buffer> VertexBuffer{};
            Result = mDevice->CreateBuffer(&BufferDesc, &InitData, VertexBuffer.put());
            if (FAILED(Result)) {
                break;
            }

            // Set draw parameters
            winrt::com_ptr<ID3D11DeviceContext> DeviceContext{};
            mDevice->GetImmediateContext(DeviceContext.put());

            constexpr FLOAT BlendFactor[4] = { 0.f, 0.f, 0.f, 0.f };
            DeviceContext->OMSetBlendState(BlendState ? mBlendState.get() : nullptr, BlendFactor, 0xFFFFFFFF);

            DeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
            DeviceContext->IASetInputLayout(mInputLayout.get());
            DeviceContext->VSSetShader(mVertexShader.get(), nullptr, 0);
            DeviceContext->PSSetShader(mPixelShader.get(), nullptr, 0);

            ID3D11ShaderResourceView* const ShaderResources[] = { ShaderResource.get() };
            DeviceContext->PSSetShaderResources(0, _countof(ShaderResources), ShaderResources);

            ID3D11SamplerState* const Samplers[] = { mSamplerState.get() };
            DeviceContext->PSSetSamplers(0, _countof(Samplers), Samplers);

            UINT Stride   = sizeof(VERTEX);
            UINT VBOffset = 0;
            ID3D11Buffer* const VertexBuffers[] = { VertexBuffer.get() };
            DeviceContext->IASetVertexBuffers(0, _countof(VertexBuffers), VertexBuffers, &Stride, &VBOffset);

            ID3D11RenderTargetView* const RenderTargets[] = { mRenderTargetView.get() };
            DeviceContext->OMSetRenderTargets(_countof(RenderTargets), RenderTargets, nullptr);

            // Draw
            DeviceContext->Draw(NUMBER_VERTICES, 0);

        } while (false);

        return Result;
    }

    winrt::hresult GraphicsRender::GetBackBuffer(ID3D11Texture2D** BackBuffer) const
    {
        return mSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast<void**>(BackBuffer));
    }

    winrt::com_ptr<IDXGISwapChain1> GraphicsRender::GetSwapChain() const
    {
        return mSwapChain;
    }

    winrt::hresult GraphicsRender::Resize(_In_ const UINT Width, _In_ const UINT Height, _In_ const DXGI_FORMAT Format)
    {
        mRenderTargetView = nullptr;

        DXGI_SWAP_CHAIN_DESC1 SwapChainDesc{};
        winrt::hresult Result = mSwapChain->GetDesc1(&SwapChainDesc);
        if ( FAILED(Result)) {
            return Result;
        }

        Result = mSwapChain->ResizeBuffers(SwapChainDesc.BufferCount, Width, Height, Format, SwapChainDesc.Flags);
        if (FAILED(Result)) {
            return Result;
        }

        Result = CreateRenderTargetView();
        if (FAILED(Result)) {
            return Result;
        }

        Result = SetViewPort(Width, Height);
        if (FAILED(Result)) {
            return Result;
        }

        return S_OK;
    }

    void GraphicsRender::Close()
    {
        mVertexShader     = nullptr;
        mPixelShader      = nullptr;
        mInputLayout      = nullptr;
        mRenderTargetView = nullptr;
        mSamplerState     = nullptr;
        mBlendState       = nullptr;
        mSwapChain        = nullptr;
        mDevice           = nullptr;
    }

    winrt::hresult GraphicsRender::CreateShaders()
    {
        winrt::hresult Result;

        do {
            winrt::com_ptr<ID3DBlob> ErrorMsgBlob{};
            winrt::com_ptr<ID3DBlob> VertexShaderBlob{};
            winrt::com_ptr<ID3DBlob> PixelShaderBlob{};

            winrt::com_ptr<ID3D11DeviceContext> DeviceContext{};
            mDevice->GetImmediateContext(DeviceContext.put());

            // Create the vertex shader
            static constexpr char VERTEX_SHADER[] = R"(
                struct VS_INPUT
                {
                    float4 Pos : POSITION;
                    float2 Tex : TEXCOORD;
                };

                struct VS_OUTPUT
                {
                    float4 Pos : SV_POSITION;
                    float2 Tex : TEXCOORD;
                };

                VS_OUTPUT VS(VS_INPUT input)
                {
                    return input;
                }
            )";

            Result = D3DCompile(VERTEX_SHADER, _countof(VERTEX_SHADER), nullptr, nullptr, nullptr,
                "VS", "vs_4_0", 0, 0, VertexShaderBlob.put(), ErrorMsgBlob.put());
            if (FAILED(Result)) {
                break;
            }

            Result = mDevice->CreateVertexShader(VertexShaderBlob->GetBufferPointer(), VertexShaderBlob->GetBufferSize(),
                nullptr, mVertexShader.put());
            if (FAILED(Result)) {
                return Result;
            }

            // Create the pixel shader
            static constexpr char PIXEL_SHADER[] = R"(
                Texture2D tx : register( t0 );
                SamplerState samLinear : register( s0 );

                struct PS_INPUT
                {
                    float4 Pos : SV_POSITION;
                    float2 Tex : TEXCOORD;
                };

                float4 PS(PS_INPUT input) : SV_Target
                {
                    return tx.Sample( samLinear, input.Tex );
                }
            )";

            Result = D3DCompile(PIXEL_SHADER, _countof(PIXEL_SHADER), nullptr, nullptr, nullptr,
                "PS", "ps_4_0", 0, 0, PixelShaderBlob.put(), ErrorMsgBlob.put());
            if (FAILED(Result)) {
                break;
            }

            Result = mDevice->CreatePixelShader(PixelShaderBlob->GetBufferPointer(), PixelShaderBlob->GetBufferSize(),
                nullptr, mPixelShader.put());
            if (FAILED(Result)) {
                return Result;
            }

            static constexpr D3D11_INPUT_ELEMENT_DESC INPUT_LAYOUT[] =
            {
                { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
                { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 }
            };

            Result = mDevice->CreateInputLayout(INPUT_LAYOUT, _countof(INPUT_LAYOUT),
                VertexShaderBlob->GetBufferPointer(),
                VertexShaderBlob->GetBufferSize(), mInputLayout.put());
            if (FAILED(Result)) {
                return Result;
            }

            DeviceContext->PSSetShader(mPixelShader.get(), nullptr, 0);
            DeviceContext->VSSetShader(mVertexShader.get(), nullptr, 0);
            DeviceContext->IASetInputLayout(mInputLayout.get());

        } while (false);

        return Result;
    }

    winrt::hresult GraphicsRender::CreateRenderTargetView()
    {
        winrt::com_ptr<ID3D11DeviceContext> DeviceContext{};
        mDevice->GetImmediateContext(DeviceContext.put());
        
        winrt::com_ptr<ID3D11Texture2D> BackBuffer{};
        winrt::hresult Result = mSwapChain->GetBuffer(0, IID_PPV_ARGS(&BackBuffer));
        if (FAILED(Result)) {
            return Result;
        }

        Result = mDevice->CreateRenderTargetView(BackBuffer.get(), nullptr, mRenderTargetView.put());
        if (FAILED(Result)) {
            return Result;
        }

        ID3D11RenderTargetView* const RenderTargets[] =
        {
            mRenderTargetView.get()
        };

        DeviceContext->OMSetRenderTargets(_countof(RenderTargets), RenderTargets, nullptr);
        return S_OK;
    }

    winrt::hresult GraphicsRender::CreateSamplerState()
    {
        winrt::com_ptr<ID3D11DeviceContext> DeviceContext{};
        mDevice->GetImmediateContext(DeviceContext.put());

        D3D11_SAMPLER_DESC SamplerDesc{};
        SamplerDesc.Filter          = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
        SamplerDesc.AddressU        = D3D11_TEXTURE_ADDRESS_CLAMP;
        SamplerDesc.AddressV        = D3D11_TEXTURE_ADDRESS_CLAMP;
        SamplerDesc.AddressW        = D3D11_TEXTURE_ADDRESS_CLAMP;
        SamplerDesc.ComparisonFunc  = D3D11_COMPARISON_NEVER;
        SamplerDesc.MinLOD          = 0;
        SamplerDesc.MaxLOD          = D3D11_FLOAT32_MAX;

        if (const winrt::hresult Result = mDevice->CreateSamplerState(
            &SamplerDesc, mSamplerState.put()); FAILED(Result)) {
            return Result;
        }

        ID3D11SamplerState* const Samplers[] =
        {
            mSamplerState.get()
        };

        DeviceContext->PSSetSamplers(0, _countof(Samplers), Samplers);
        return S_OK;
    }

    winrt::hresult GraphicsRender::CreateBlendState()
    {
        D3D11_BLEND_DESC BlendStateDesc{};
        BlendStateDesc.AlphaToCoverageEnable                    = FALSE;
        BlendStateDesc.IndependentBlendEnable                   = FALSE;
        BlendStateDesc.RenderTarget[0].BlendEnable              = TRUE;
        BlendStateDesc.RenderTarget[0].SrcBlend                 = D3D11_BLEND_SRC_ALPHA;
        BlendStateDesc.RenderTarget[0].DestBlend                = D3D11_BLEND_INV_SRC_ALPHA;
        BlendStateDesc.RenderTarget[0].BlendOp                  = D3D11_BLEND_OP_ADD;
        BlendStateDesc.RenderTarget[0].SrcBlendAlpha            = D3D11_BLEND_ONE;
        BlendStateDesc.RenderTarget[0].DestBlendAlpha           = D3D11_BLEND_ZERO;
        BlendStateDesc.RenderTarget[0].BlendOpAlpha             = D3D11_BLEND_OP_ADD;
        BlendStateDesc.RenderTarget[0].RenderTargetWriteMask    = D3D11_COLOR_WRITE_ENABLE_ALL;

        return mDevice->CreateBlendState(&BlendStateDesc, mBlendState.put());
    }

    winrt::hresult GraphicsRender::SetViewPort(_In_ const UINT Width, _In_ const UINT Height) const
    {
        D3D11_VIEWPORT ViewPort;
        ViewPort.Width      = static_cast<FLOAT>(Width);
        ViewPort.Height     = static_cast<FLOAT>(Height);
        ViewPort.MinDepth   = 0.0f;
        ViewPort.MaxDepth   = 1.0f;
        ViewPort.TopLeftX   = 0.0f;
        ViewPort.TopLeftY   = 0.0f;

        winrt::com_ptr<ID3D11DeviceContext> DeviceContext{};
        mDevice->GetImmediateContext(DeviceContext.put());

        DeviceContext->RSSetViewports(1, &ViewPort);
        return S_OK;
    }

    winrt::hresult GraphicsRender::SetDirtyVertex(
        _Out_writes_(NUMBER_VERTICES) struct VERTEX* Vertices,
        _In_     const RECT* Dirty,
        _In_     const SIZE  ThisSize,
        _In_opt_ const POINT Offset,
        _In_opt_ const DXGI_MODE_ROTATION RotationMode) const
    {
        DXGI_SWAP_CHAIN_DESC1 SwapChainDesc{};
        if (const winrt::hresult Result = mSwapChain->GetDesc1(&SwapChainDesc); FAILED(Result)) {
            return Result;
        }

        const INT CenterX = static_cast<INT>(SwapChainDesc.Width ) / 2;
        const INT CenterY = static_cast<INT>(SwapChainDesc.Height) / 2;

        const INT Width   = ThisSize.cx;
        const INT Height  = ThisSize.cy;

        const RECT FullDirty{ 0, 0, Width, Height };

        if (Dirty == nullptr) {
            Dirty = &FullDirty;
        }

        RECT DestDirty = *Dirty;

        switch (RotationMode)
        {
            default: WINRT_ASSERT(false); // drop through
            case DXGI_MODE_ROTATION_UNSPECIFIED:
            case DXGI_MODE_ROTATION_IDENTITY:
            {
                const RECT TexcoordRect{0, 0, Dirty->right - Dirty->left, Dirty->bottom - Dirty->top};

                Vertices[0].Texcoord = DirectX::XMFLOAT2(static_cast<FLOAT>(TexcoordRect.left ) / static_cast<FLOAT>(Width), static_cast<FLOAT>(TexcoordRect.bottom) / static_cast<FLOAT>(Height));
                Vertices[1].Texcoord = DirectX::XMFLOAT2(static_cast<FLOAT>(TexcoordRect.left ) / static_cast<FLOAT>(Width), static_cast<FLOAT>(TexcoordRect.top   ) / static_cast<FLOAT>(Height));
                Vertices[2].Texcoord = DirectX::XMFLOAT2(static_cast<FLOAT>(TexcoordRect.right) / static_cast<FLOAT>(Width), static_cast<FLOAT>(TexcoordRect.bottom) / static_cast<FLOAT>(Height));
                Vertices[5].Texcoord = DirectX::XMFLOAT2(static_cast<FLOAT>(TexcoordRect.right) / static_cast<FLOAT>(Width), static_cast<FLOAT>(TexcoordRect.top   ) / static_cast<FLOAT>(Height));
                break;
            }
            case DXGI_MODE_ROTATION_ROTATE90:
            {
                const RECT TexcoordRect{ 0, 0, Dirty->right - Dirty->left, Dirty->bottom - Dirty->top };

                DestDirty.left       = Width - Dirty->bottom;
                DestDirty.top        = Dirty->left;
                DestDirty.right      = Width - Dirty->top;
                DestDirty.bottom     = Dirty->right;

                Vertices[0].Texcoord = DirectX::XMFLOAT2(static_cast<FLOAT>(TexcoordRect.right) / static_cast<FLOAT>(Width), static_cast<FLOAT>(TexcoordRect.bottom) / static_cast<FLOAT>(Height));
                Vertices[1].Texcoord = DirectX::XMFLOAT2(static_cast<FLOAT>(TexcoordRect.left ) / static_cast<FLOAT>(Width), static_cast<FLOAT>(TexcoordRect.bottom) / static_cast<FLOAT>(Height));
                Vertices[2].Texcoord = DirectX::XMFLOAT2(static_cast<FLOAT>(TexcoordRect.right) / static_cast<FLOAT>(Width), static_cast<FLOAT>(TexcoordRect.top   ) / static_cast<FLOAT>(Height));
                Vertices[5].Texcoord = DirectX::XMFLOAT2(static_cast<FLOAT>(TexcoordRect.left ) / static_cast<FLOAT>(Width), static_cast<FLOAT>(TexcoordRect.top   ) / static_cast<FLOAT>(Height));
                break;
            }
            case DXGI_MODE_ROTATION_ROTATE180:
            {
                const RECT TexcoordRect{ 0, 0, Dirty->right - Dirty->left, Dirty->bottom - Dirty->top };

                DestDirty.left       = Width  - Dirty->right;
                DestDirty.top        = Height - Dirty->bottom;
                DestDirty.right      = Width  - Dirty->left;
                DestDirty.bottom     = Height - Dirty->top;

                Vertices[0].Texcoord = DirectX::XMFLOAT2(static_cast<FLOAT>(TexcoordRect.right) / static_cast<FLOAT>(Width), static_cast<FLOAT>(TexcoordRect.top   ) / static_cast<FLOAT>(Height));
                Vertices[1].Texcoord = DirectX::XMFLOAT2(static_cast<FLOAT>(TexcoordRect.right) / static_cast<FLOAT>(Width), static_cast<FLOAT>(TexcoordRect.bottom) / static_cast<FLOAT>(Height));
                Vertices[2].Texcoord = DirectX::XMFLOAT2(static_cast<FLOAT>(TexcoordRect.left ) / static_cast<FLOAT>(Width), static_cast<FLOAT>(TexcoordRect.top   ) / static_cast<FLOAT>(Height));
                Vertices[5].Texcoord = DirectX::XMFLOAT2(static_cast<FLOAT>(TexcoordRect.left ) / static_cast<FLOAT>(Width), static_cast<FLOAT>(TexcoordRect.bottom) / static_cast<FLOAT>(Height));
                break;
            }
            case DXGI_MODE_ROTATION_ROTATE270:
            {
                const RECT TexcoordRect{ 0, 0, Dirty->right - Dirty->left, Dirty->bottom - Dirty->top };

                DestDirty.left       = Dirty->top;
                DestDirty.top        = Height - Dirty->right;
                DestDirty.right      = Dirty->bottom;
                DestDirty.bottom     = Height - Dirty->left;

                Vertices[0].Texcoord = DirectX::XMFLOAT2(static_cast<FLOAT>(TexcoordRect.left ) / static_cast<FLOAT>(Width), static_cast<FLOAT>(TexcoordRect.top   ) / static_cast<FLOAT>(Height));
                Vertices[1].Texcoord = DirectX::XMFLOAT2(static_cast<FLOAT>(TexcoordRect.right) / static_cast<FLOAT>(Width), static_cast<FLOAT>(TexcoordRect.top   ) / static_cast<FLOAT>(Height));
                Vertices[2].Texcoord = DirectX::XMFLOAT2(static_cast<FLOAT>(TexcoordRect.left ) / static_cast<FLOAT>(Width), static_cast<FLOAT>(TexcoordRect.bottom) / static_cast<FLOAT>(Height));
                Vertices[5].Texcoord = DirectX::XMFLOAT2(static_cast<FLOAT>(TexcoordRect.right) / static_cast<FLOAT>(Width), static_cast<FLOAT>(TexcoordRect.bottom) / static_cast<FLOAT>(Height));
                break;
            }
        }

        Vertices[3].Texcoord = Vertices[2].Texcoord;
        Vertices[4].Texcoord = Vertices[1].Texcoord;

        // Set positions
        Vertices[0].Position = DirectX::XMFLOAT3(static_cast<FLOAT>(DestDirty.left   - Offset.x - CenterX) / static_cast<FLOAT>(CenterX),
                                            -1 * static_cast<FLOAT>(DestDirty.bottom - Offset.y - CenterY) / static_cast<FLOAT>(CenterY),
                                            0.0f);
        Vertices[1].Position = DirectX::XMFLOAT3(static_cast<FLOAT>(DestDirty.left   - Offset.x - CenterX) / static_cast<FLOAT>(CenterX),
                                            -1 * static_cast<FLOAT>(DestDirty.top    - Offset.y - CenterY) / static_cast<FLOAT>(CenterY),
                                            0.0f);
        Vertices[2].Position = DirectX::XMFLOAT3(static_cast<FLOAT>(DestDirty.right  - Offset.x - CenterX) / static_cast<FLOAT>(CenterX),
                                            -1 * static_cast<FLOAT>(DestDirty.bottom - Offset.y - CenterY) / static_cast<FLOAT>(CenterY),
                                            0.0f);
        Vertices[3].Position = Vertices[2].Position;
        Vertices[4].Position = Vertices[1].Position;
        Vertices[5].Position = DirectX::XMFLOAT3(static_cast<FLOAT>(DestDirty.right  - Offset.x - CenterX) / static_cast<FLOAT>(CenterX),
                                            -1 * static_cast<FLOAT>(DestDirty.top    - Offset.y - CenterY) / static_cast<FLOAT>(CenterY),
                                            0.0f);

        return S_OK;
    }
}
