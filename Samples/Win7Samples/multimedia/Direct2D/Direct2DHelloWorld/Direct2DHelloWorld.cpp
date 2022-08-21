// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//
// Copyright (c) Microsoft Corporation. All rights reserved


#include "Direct2DHelloWorld.h"

#pragma comment( lib, "d3d12" )
#pragma comment( lib, "d3d11" )

//
// Provides the entry point to the application.
//
int WINAPI WinMain(
    HINSTANCE /*hInstance*/,
    HINSTANCE /*hPrevInstance*/,
    LPSTR /*lpCmdLine*/,
    int /*nCmdShow*/
    )
{
    // Ignore the return value because we want to run the program even in the
    // unlikely event that HeapSetInformation fails.
    HeapSetInformation(NULL, HeapEnableTerminationOnCorruption, NULL, 0);

    if (SUCCEEDED(CoInitialize(NULL)))
    {
        {
            DemoApp app;

            if (SUCCEEDED(app.Initialize()))
            {
                app.RunMessageLoop();
            }
        }
        CoUninitialize();
    }

    return 0;
}


//
// Initialize members.
//
DemoApp::DemoApp() :
    m_hwnd(NULL),
    m_pD2DFactory(NULL),
    m_pDWriteFactory(NULL),
    m_pTextFormat(NULL),
    m_pBlackBrush(NULL)
{
}

//
// Release resources.
//
DemoApp::~DemoApp()
{
    SafeRelease(&m_pD2DFactory);
    SafeRelease(&m_pDWriteFactory);
    SafeRelease(&m_pTextFormat);
    SafeRelease(&m_pBlackBrush);

}

//
// Creates the application window and initializes
// device-independent resources.
//
HRESULT DemoApp::Initialize()
{
    HRESULT hr;

    // Initialize device-indpendent resources, such
    // as the Direct2D factory.
    hr = CreateDeviceIndependentResources();
    if (SUCCEEDED(hr))
    {
        // Register the window class.
        WNDCLASSEXW wcex = { sizeof(WNDCLASSEXW) };
        wcex.style         = CS_HREDRAW | CS_VREDRAW;
        wcex.lpfnWndProc   = DemoApp::WndProc;
        wcex.cbClsExtra    = 0;
        wcex.cbWndExtra    = sizeof(LONG_PTR);
        wcex.hInstance     = HINST_THISCOMPONENT;
        wcex.hbrBackground = NULL;
        wcex.lpszMenuName  = NULL;
        wcex.hCursor       = LoadCursor(NULL, IDC_ARROW);
        wcex.lpszClassName = L"D2DDemoApp";

        RegisterClassExW(&wcex);

        // Create the application window.
        //
        // Because the CreateWindow function takes its size in pixels, we
        // obtain the system DPI and use it to scale the window size.
        FLOAT dpiX, dpiY;
        m_pD2DFactory->GetDesktopDpi(&dpiX, &dpiY);

        m_hwnd = CreateWindowW(
            L"D2DDemoApp",
            L"Direct2D Demo Application",
            WS_OVERLAPPEDWINDOW,
            CW_USEDEFAULT,
            CW_USEDEFAULT,
            static_cast<UINT>(ceil(640.f * dpiX / 96.f)),
            static_cast<UINT>(ceil(480.f * dpiY / 96.f)),
            NULL,
            NULL,
            HINST_THISCOMPONENT,
            this
            );
        hr = m_hwnd ? S_OK : E_FAIL;
        if (SUCCEEDED(hr))
        {
            ShowWindow(m_hwnd, SW_SHOWNORMAL);

            UpdateWindow(m_hwnd);
        }
    }

    return hr;
}


//
// Create resources which are not bound
// to any device. Their lifetime effectively extends for the
// duration of the app. These resources include the Direct2D and
// DirectWrite factories,  and a DirectWrite Text Format object
// (used for identifying particular font characteristics).
//
HRESULT DemoApp::CreateDeviceIndependentResources()
{
    using namespace Microsoft::WRL;

    static const WCHAR msc_fontName[] = L"Verdana";
    static const FLOAT msc_fontSize = 50;
    HRESULT hr;
    ID2D1GeometrySink *pSink = NULL;

    ComPtr<IDXGIAdapter> adapter;
    ComPtr<IDXGIDevice> dxgiDevice;

    hr = CreateDXGIFactory(IID_PPV_ARGS(&m_dxgiFactory));
    if (FAILED(hr))
    {
        return hr;
    }

    hr = m_dxgiFactory->EnumAdapters(0, adapter.GetAddressOf());
    if (FAILED(hr))
    {
        return hr;
    }

    hr = D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_12_0, IID_PPV_ARGS(&m_d12device));
    if (FAILED(hr))
    {
        return hr;
    }

    D3D12_COMMAND_QUEUE_DESC queueDesc = {};
    queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
    queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;

    hr = m_d12device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&m_commandQueue));
    if (FAILED(hr))
    {
        return hr;
    }

    const UINT d3d11DeviceFlags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;

    ComPtr<ID3D11Device> d3d11Device;
    hr = D3D11On12CreateDevice(
        m_d12device.Get(),
        d3d11DeviceFlags,
        nullptr,
        0,
        reinterpret_cast<IUnknown**>(m_commandQueue.GetAddressOf()),
        1,
        0,
        &d3d11Device,
        &m_d3d11DeviceContext,
        nullptr
    );

    d3d11Device.As(&m_d3d11On12Device);
    m_d3d11On12Device.As(&dxgiDevice);

    // Create a Direct2D factory.
    hr = D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &m_pD2DFactory);

    if (SUCCEEDED(hr))
    {
        hr = m_pD2DFactory->CreateDevice(dxgiDevice.Get(), &m_d2dDevice);
        if (FAILED(hr))
        {
            return hr;
        }

        hr = m_d2dDevice->CreateDeviceContext(D2D1_DEVICE_CONTEXT_OPTIONS_NONE, &m_d2dDeviceContext);
        if (FAILED(hr))
        {
            return hr;
        }

        // Create a DirectWrite factory.
        hr = DWriteCreateFactory(
            DWRITE_FACTORY_TYPE_SHARED,
            __uuidof(m_pDWriteFactory),
            reinterpret_cast<IUnknown **>(&m_pDWriteFactory)
            );
    }
    if (SUCCEEDED(hr))
    {
        // Create a DirectWrite text format object.
        hr = m_pDWriteFactory->CreateTextFormat(
            msc_fontName,
            NULL,
            DWRITE_FONT_WEIGHT_NORMAL,
            DWRITE_FONT_STYLE_NORMAL,
            DWRITE_FONT_STRETCH_NORMAL,
            msc_fontSize,
            L"", //locale
            &m_pTextFormat
            );
    }
    if (SUCCEEDED(hr))
    {
        // Center the text horizontally and vertically.
        m_pTextFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);

        m_pTextFormat->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);

    }

    SafeRelease(&pSink);

    return hr;
}

//
//  This method creates resources which are bound to a particular
//  Direct3D device. It's all centralized here, in case the resources
//  need to be recreated in case of Direct3D device loss (eg. display
//  change, remoting, removal of video card, etc).
//
HRESULT DemoApp::CreateDeviceResources()
{
    using namespace Microsoft::WRL;
    HRESULT hr = S_OK;

    if (!m_pRenderTarget[0].Get())
    {
        const UINT FrameCount = 2;

        DXGI_SWAP_CHAIN_DESC swapChainDesc = {};
        swapChainDesc.BufferCount = FrameCount;
        swapChainDesc.BufferDesc.Width = 1920;
        swapChainDesc.BufferDesc.Height = 1080;
        swapChainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
        swapChainDesc.OutputWindow = m_hwnd;
        swapChainDesc.SampleDesc.Count = 1;
        swapChainDesc.Windowed = TRUE;

        m_dxgiFactory->CreateSwapChain(
            m_commandQueue.Get(),        // Swap chain needs the queue so that it can force a flush on it.
            &swapChainDesc,
            &m_swapChain
        );

        // Describe and create a render target view (RTV) descriptor heap.
        D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
        rtvHeapDesc.NumDescriptors = FrameCount;
        rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
        rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
        m_d12device->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&m_descHeap));

        auto m_rtvDescriptorSize = m_d12device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

        float dpi = GetDpiForWindow(m_hwnd);
        D2D1_BITMAP_PROPERTIES1 bitmapProperties = D2D1::BitmapProperties1(
            D2D1_BITMAP_OPTIONS_TARGET | D2D1_BITMAP_OPTIONS_CANNOT_DRAW,
            D2D1::PixelFormat(DXGI_FORMAT_UNKNOWN, D2D1_ALPHA_MODE_PREMULTIPLIED),
            dpi,
            dpi
        );

        // Create frame resources.
        {
            CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(m_descHeap->GetCPUDescriptorHandleForHeapStart());

            // Create a RTV, D2D render target, and a command allocator for each frame.
            for (UINT n = 0; n < FrameCount; n++)
            {
                m_swapChain->GetBuffer(n, IID_PPV_ARGS(&m_renderTargets[n]));
                m_d12device->CreateRenderTargetView(m_renderTargets[n].Get(), nullptr, rtvHandle);

                // Create a wrapped 11On12 resource of this back buffer. Since we are 
                // rendering all Direct3D 12 content first and then all D2D content, we specify 
                // the In resource state as RENDER_TARGET - because Direct3D 12 will have last 
                // used it in this state - and the Out resource state as PRESENT. When 
                // ReleaseWrappedResources() is called on the 11On12 device, the resource 
                // will be transitioned to the PRESENT state.
                D3D11_RESOURCE_FLAGS d3d11Flags = { D3D11_BIND_RENDER_TARGET };
                hr = m_d3d11On12Device->CreateWrappedResource(
                    m_renderTargets[n].Get(),
                    &d3d11Flags,
                    D3D12_RESOURCE_STATE_RENDER_TARGET,
                    D3D12_RESOURCE_STATE_PRESENT,
                    IID_PPV_ARGS(&m_wrappedBackBuffers[n])
                );

                // Create a render target for D2D to draw directly to this back buffer.
                ComPtr<IDXGISurface> surface;
                m_wrappedBackBuffers[n].As(&surface);
                hr = m_d2dDeviceContext->CreateBitmapFromDxgiSurface(
                    surface.Get(),
                    &bitmapProperties,
                    &m_pRenderTarget[n]
                );

                rtvHandle.Offset(1, m_rtvDescriptorSize);

                hr = m_d12device->CreateCommandAllocator(
                    D3D12_COMMAND_LIST_TYPE_DIRECT,
                    IID_PPV_ARGS(&m_commandAllocators[n]));
            }
        }

        if (SUCCEEDED(hr))
        {
            // Create a black brush.
            hr = m_d2dDeviceContext->CreateSolidColorBrush(
                D2D1::ColorF(D2D1::ColorF::Black),
                &m_pBlackBrush
                );
        }

    }

    return hr;
}


//
//  Discard device-specific resources which need to be recreated
//  when a Direct3D device is lost
//
void DemoApp::DiscardDeviceResources()
{
    //SafeRelease(&m_pRenderTarget);
    SafeRelease(&m_pBlackBrush);
}

//
// The main window message loop.
//
void DemoApp::RunMessageLoop()
{
    MSG msg;

    while (GetMessage(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
}


//
//  Called whenever the application needs to display the client
//  window. This method writes "Hello, World"
//
//  Note that this function will not render anything if the window
//  is occluded (e.g. when the screen is locked).
//  Also, this function will automatically discard device-specific
//  resources if the Direct3D device disappears during function
//  invocation, and will recreate the resources the next time it's
//  invoked.
//
HRESULT DemoApp::OnRender()
{
    HRESULT hr;

    hr = CreateDeviceResources();

    if (SUCCEEDED(hr))
    {
        auto index = frameIndex % 2;

        D2D1_SIZE_F rtSize = m_pRenderTarget[index]->GetSize();
        D2D1_RECT_F textRect = D2D1::RectF(0, 0, rtSize.width, rtSize.height);
        static const WCHAR sc_helloWorld[] = L"Hello, World!";

        // Acquire our wrapped render target resource for the current back buffer.
        m_d3d11On12Device->AcquireWrappedResources(m_wrappedBackBuffers[index].GetAddressOf(), 1);

        // Render text directly to the back buffer.
        m_d2dDeviceContext->SetTarget(m_pRenderTarget[index].Get());
        m_d2dDeviceContext->BeginDraw();
        m_d2dDeviceContext->SetTransform(D2D1::Matrix3x2F::Identity());
        m_d2dDeviceContext->Clear(D2D1::ColorF(D2D1::ColorF::White));
        m_d2dDeviceContext->DrawText(
            sc_helloWorld,
            ARRAYSIZE(sc_helloWorld) - 1,
            m_pTextFormat,
            D2D1::RectF(0, 0, rtSize.width, rtSize.height),
            m_pBlackBrush
        );
        m_d2dDeviceContext->EndDraw();

        // Release our wrapped render target resource. Releasing 
        // transitions the back buffer resource to the state specified
        // as the OutState when the wrapped resource was created.
        m_d3d11On12Device->ReleaseWrappedResources(m_wrappedBackBuffers[index].GetAddressOf(), 1);

        // Flush to submit the 11 command list to the shared command queue.
        m_d3d11DeviceContext->Flush();

        m_swapChain->Present(0, 0);

        ++frameIndex;
    }

    return hr;
}

//
//  If the application receives a WM_SIZE message, this method
//  resizes the render target appropriately.
//
void DemoApp::OnResize(UINT width, UINT height)
{
    (width);
    (height);
    // FIXME: Unimplemented
}


//
// The window message handler.
//
LRESULT CALLBACK DemoApp::WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    LRESULT result = 0;

    if (message == WM_CREATE)
    {
        LPCREATESTRUCT pcs = (LPCREATESTRUCT)lParam;
        DemoApp *pDemoApp = (DemoApp *)pcs->lpCreateParams;

        ::SetWindowLongPtrW(
            hwnd,
            GWLP_USERDATA,
            reinterpret_cast<LONG_PTR>(pDemoApp)
            );

        result = 1;
    }
    else
    {
        DemoApp *pDemoApp = reinterpret_cast<DemoApp *>(
            ::GetWindowLongPtrW(
                hwnd,
                GWLP_USERDATA
                ));

        bool wasHandled = false;

        if (pDemoApp)
        {
            switch (message)
            {
            case WM_SIZE:
                {
                    UINT width = LOWORD(lParam);
                    UINT height = HIWORD(lParam);
                    pDemoApp->OnResize(width, height);
                }
                wasHandled = true;
                result = 0;
                break;

            case WM_PAINT:
            case WM_DISPLAYCHANGE:
                {
                    PAINTSTRUCT ps;
                    BeginPaint(hwnd, &ps);

                    pDemoApp->OnRender();
                    EndPaint(hwnd, &ps);
                }
                wasHandled = true;
                result = 0;
                break;

            case WM_DESTROY:
                {
                    PostQuitMessage(0);
                }
                wasHandled = true;
                result = 1;
                break;
            }
        }

        if (!wasHandled)
        {
            result = DefWindowProc(hwnd, message, wParam, lParam);
        }
    }

    return result;
}
