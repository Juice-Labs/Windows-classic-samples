// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//
// Copyright (c) Microsoft Corporation. All rights reserved
//

#pragma once

#define WIN32_LEAN_AND_MEAN     // Exclude rarely-used stuff from Windows headers
// Windows Header Files:
#include <windows.h>

// C RunTime Header Files
#include <stdlib.h>
#include <malloc.h>
#include <memory.h>
#include <wchar.h>
#include <math.h>

#include <d2d1.h>
#include <d2d1helper.h>
#include <dwrite.h>
#include <wincodec.h>

#include <wrl.h>
#include <d3d12.h>
#include <dxgi.h>
#include <dxgi1_4.h>
#include <d3d11on12.h>
#include <d2d1_3.h>
#include <stdint.h>
#include <array>

#include "d3dx12.h"

/******************************************************************
*                                                                 *
*  Macros                                                         *
*                                                                 *
******************************************************************/

template<class Interface>
inline void
SafeRelease(
    Interface **ppInterfaceToRelease
    )
{
    if (*ppInterfaceToRelease != NULL)
    {
        (*ppInterfaceToRelease)->Release();

        (*ppInterfaceToRelease) = NULL;
    }
}

#ifndef Assert
#if defined( DEBUG ) || defined( _DEBUG )
#define Assert(b) if (!(b)) {OutputDebugStringA("Assert: " #b "\n");}
#else
#define Assert(b)
#endif //DEBUG || _DEBUG
#endif


#ifndef HINST_THISCOMPONENT
EXTERN_C IMAGE_DOS_HEADER __ImageBase;
#define HINST_THISCOMPONENT ((HINSTANCE)&__ImageBase)
#endif

/******************************************************************
*                                                                 *
*  DemoApp                                                        *
*                                                                 *
******************************************************************/

class DemoApp
{
public:
    DemoApp();
    ~DemoApp();

    HRESULT Initialize();

    void RunMessageLoop();

private:
    HRESULT CreateDeviceIndependentResources();
    HRESULT CreateDeviceResources();
    void DiscardDeviceResources();

    HRESULT OnRender();

    void OnResize(
        UINT width,
        UINT height
        );

    static LRESULT CALLBACK WndProc(
        HWND hWnd,
        UINT message,
        WPARAM wParam,
        LPARAM lParam
        );

private:
    HWND m_hwnd;
    ID2D1Factory3 *m_pD2DFactory;
    IWICImagingFactory *m_pWICFactory;
    IDWriteFactory *m_pDWriteFactory;
    std::array<Microsoft::WRL::ComPtr<ID2D1Bitmap1>, 2> m_pRenderTarget;
    IDWriteTextFormat *m_pTextFormat;
    ID2D1SolidColorBrush *m_pBlackBrush;
    uint64_t frameIndex = 0;

    Microsoft::WRL::ComPtr<IDXGIFactory> m_dxgiFactory;
    Microsoft::WRL::ComPtr<ID3D12Device> m_d12device;
    Microsoft::WRL::ComPtr<ID3D11On12Device> m_d3d11On12Device;
    Microsoft::WRL::ComPtr<ID2D1Device> m_d2dDevice;
    Microsoft::WRL::ComPtr<ID2D1DeviceContext> m_d2dDeviceContext;
    Microsoft::WRL::ComPtr<ID3D12CommandQueue> m_commandQueue;
    Microsoft::WRL::ComPtr<ID3D11Device> m_d3d11Device;
    Microsoft::WRL::ComPtr<ID3D11DeviceContext> m_d3d11DeviceContext;
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_descHeap;
    Microsoft::WRL::ComPtr<IDXGISwapChain> m_swapChain;

    std::array<Microsoft::WRL::ComPtr<ID3D12Resource>, 2> m_renderTargets;
    std::array<Microsoft::WRL::ComPtr<ID3D11Resource>, 2> m_wrappedBackBuffers;
    std::array<Microsoft::WRL::ComPtr<ID3D12CommandAllocator>, 2> m_commandAllocators;
};


