#pragma once
#include <Windows.Graphics.DirectX.Direct3D11.interop.h>


inline auto CreateDirect3DDevice(IDXGIDevice* Device)
{
    winrt::com_ptr<::IInspectable> D3dDevice;
    winrt::check_hresult(CreateDirect3D11DeviceFromDXGIDevice(Device, D3dDevice.put()));
    return D3dDevice.as<winrt::Windows::Graphics::DirectX::Direct3D11::IDirect3DDevice>();
}

inline auto CreateDirect3DSurface(IDXGISurface* Surface)
{
    winrt::com_ptr<::IInspectable> D3dSurface;
    winrt::check_hresult(CreateDirect3D11SurfaceFromDXGISurface(Surface, D3dSurface.put()));
    return D3dSurface.as<winrt::Windows::Graphics::DirectX::Direct3D11::IDirect3DSurface>();
}

template <typename T>
auto GetDXGIInterfaceFromObject(winrt::Windows::Foundation::IInspectable const& Object)
{
    const auto Access = Object.as<::Windows::Graphics::DirectX::Direct3D11::IDirect3DDxgiInterfaceAccess>();

    winrt::com_ptr<T> Result;
    winrt::check_hresult(Access->GetInterface(winrt::guid_of<T>(), Result.put_void()));
    return Result;
}
