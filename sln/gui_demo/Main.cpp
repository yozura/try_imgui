#include <d3d11.h>
#include <DirectXColors.h>
#include <dxgi.h>
#include <string>
#include "imgui.h"
#include "backends/imgui_impl_dx11.h"
#include "backends/imgui_impl_win32.h"

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

HWND ghMainWnd = NULL;
HINSTANCE ghAppInstance = NULL;
ID3D11Device* gd3dDevice = NULL;
ID3D11DeviceContext* gd3dDeviceContext = NULL;
IDXGISwapChain* gSwapChain = NULL;
ID3D11RenderTargetView* gRTV = NULL;
ID3D11DepthStencilView* gDSV = NULL;
ID3D11Texture2D* gBackBuffer = NULL;
std::wstring gMainWndCaption;
float gSliderValue = 0.0f;
float gClearColor[3] = { 0.0f, 0.0f, 0.0f };

LRESULT CALLBACK MainWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
bool CreateCustomWindow(HINSTANCE hInstance, int width, int height);
bool CreateCustomDirect3D(int width, int height);
bool CreateCustomImGui();
void Update();
void Draw();
void Cleanup();
int RunWindow();

LRESULT CALLBACK MainWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if(ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
        return true;

    switch (msg)
    {
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }

    return DefWindowProc(hWnd, msg, wParam, lParam);
}

bool CreateCustomWindow(HINSTANCE hInstance, int width, int height)
{
    ghAppInstance = hInstance;

    WNDCLASS wc;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.lpfnWndProc = MainWndProc;
    wc.hInstance = ghAppInstance;
    wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)GetStockObject(NULL_BRUSH);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpszClassName = L"ImGui Example";
    wc.lpszMenuName = 0;

    if (!RegisterClass(&wc))
    {
        MessageBox(NULL, L"RegisterClass Failed.", NULL, 0);
        return false;
    }

    RECT R = { 0, 0, width, height };
    AdjustWindowRect(&R, WS_OVERLAPPEDWINDOW, false);
    int cWidth = R.right - R.left;
    int cHeight = R.bottom - R.top;

    gMainWndCaption = L"ImGui Example";

    ghMainWnd = CreateWindow(L"ImGui Example",
        gMainWndCaption.c_str(),
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        cWidth, cHeight,
        0, 0, ghAppInstance, 0);

    if (!ghMainWnd)
    {
        MessageBox(NULL, L"CreateWindow Failed.", NULL, 0);
        return false;
    }

    ShowWindow(ghMainWnd, SW_SHOW);
    UpdateWindow(ghMainWnd);

    return true;
}

bool CreateCustomDirect3D(int width, int height)
{
    UINT createDeviceFlags = 0;
#if defined(DEBUG) | defined(_DEBUG)
    createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

    D3D_FEATURE_LEVEL featureLevel;
    HRESULT hr = D3D11CreateDevice(0,
                                   D3D_DRIVER_TYPE_HARDWARE,
                                   0,
                                   createDeviceFlags,
                                   0,
                                   0,
                                   D3D11_SDK_VERSION,
                                   &gd3dDevice,
                                   &featureLevel,
                                   &gd3dDeviceContext);
    if (FAILED(hr))
    {
        MessageBox(0, L"D3D11CreateDevice Failed.", 0, 0);
        return false;
    }

    if (featureLevel != D3D_FEATURE_LEVEL_11_0)
    {
        MessageBox(0, L"Direct3D Feature Level 11 unsupported.", 0, 0);
        return false;
    }

    DXGI_SWAP_CHAIN_DESC swapChainDesc;
    swapChainDesc.BufferDesc.Width = width;
    swapChainDesc.BufferDesc.Height = height;
    swapChainDesc.BufferDesc.RefreshRate.Numerator = 60;
    swapChainDesc.BufferDesc.RefreshRate.Denominator = 1;
    swapChainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    swapChainDesc.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
    swapChainDesc.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
    swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapChainDesc.BufferCount = 2;
    swapChainDesc.OutputWindow = ghMainWnd;
    swapChainDesc.Windowed = true;
    swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_SEQUENTIAL;
    swapChainDesc.SampleDesc.Count = 1;
    swapChainDesc.SampleDesc.Quality = 0;
    swapChainDesc.Flags = 0;

    IDXGIDevice* dxgiDevice = 0;
    hr = gd3dDevice->QueryInterface(__uuidof(IDXGIDevice), reinterpret_cast<void**>(&dxgiDevice));
    if (FAILED(hr))
    {
        MessageBox(0, L"QueryInterface IDXGIDevice Failed.", 0, 0);
        return false;
    }

    IDXGIAdapter* dxgiAdapter = 0;
    hr = dxgiDevice->GetParent(__uuidof(IDXGIAdapter), reinterpret_cast<void**>(&dxgiAdapter));
    if (FAILED(hr))
    {
        MessageBox(0, L"GetParent IDXGIAdapter Failed.", 0, 0);
        return false;
    }

    IDXGIFactory* dxgiFactory = 0;
    hr = dxgiAdapter->GetParent(__uuidof(IDXGIFactory), reinterpret_cast<void**>(&dxgiFactory));
    if (FAILED(hr))
    {
        MessageBox(0, L"GetParent IDXGIFactory Failed.", 0, 0);
        return false;
    }

    hr = dxgiFactory->CreateSwapChain(gd3dDevice, &swapChainDesc, &gSwapChain);
    if (FAILED(hr))
    {
        MessageBox(0, L"CreateSwapChain Failed.", 0, 0);
        return false;
    }

    dxgiDevice->Release();
    dxgiAdapter->Release();
    dxgiFactory->Release();

    assert(gd3dDevice);
    assert(gd3dDeviceContext);
    assert(gSwapChain);

    hr = gSwapChain->ResizeBuffers(1, width, height, DXGI_FORMAT_R8G8B8A8_UNORM, 0);
    if (FAILED(hr))
    {
        MessageBox(0, L"ResizeBuffers Failed.", 0, 0);
        return false;
    }

    ID3D11Texture2D* backBuffer;
    hr = gSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&backBuffer));
    if (FAILED(hr))
    {
        MessageBox(0, L"GetBuffer Failed.", 0, 0);
        return false;
    }

    hr = gd3dDevice->CreateRenderTargetView(backBuffer, 0, &gRTV);
    if (FAILED(hr))
    {
        MessageBox(0, L"CreateRenderTargetView Failed.", 0, 0);
        return false;
    }

    backBuffer->Release();

    D3D11_TEXTURE2D_DESC depthStencilDesc;
    depthStencilDesc.Width = width;
    depthStencilDesc.Height = height;
    depthStencilDesc.MipLevels = 1;
    depthStencilDesc.ArraySize = 1;
    depthStencilDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
    depthStencilDesc.SampleDesc.Count = 1;
    depthStencilDesc.SampleDesc.Quality = 0;
    depthStencilDesc.Usage = D3D11_USAGE_DEFAULT;
    depthStencilDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
    depthStencilDesc.CPUAccessFlags = 0;
    depthStencilDesc.MiscFlags = 0;

    hr = gd3dDevice->CreateTexture2D(&depthStencilDesc, 0, &gBackBuffer);
    if (FAILED(hr))
    {
        MessageBox(0, L"CreateTexture2D Failed.", 0, 0);
        return false;
    }

    hr = gd3dDevice->CreateDepthStencilView(gBackBuffer, 0, &gDSV);
    if (FAILED(hr))
    {
        MessageBox(0, L"CreateDepthStencilView Failed.", 0, 0);
        return false;
    }

    gd3dDeviceContext->OMSetRenderTargets(1, &gRTV, gDSV);

    D3D11_VIEWPORT vp;
    vp.TopLeftX = 0;
    vp.TopLeftY = 0;
    vp.Width = static_cast<float>(width);
    vp.Height = static_cast<float>(height);
    vp.MinDepth = 0.0f;
    vp.MaxDepth = 1.0f;

    gd3dDeviceContext->RSSetViewports(1, &vp);

    return true;
}

bool CreateCustomImGui()
{
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;

    ImGui::StyleColorsClassic();

    if (!ImGui_ImplWin32_Init(ghMainWnd))
        return false;

    if (!ImGui_ImplDX11_Init(gd3dDevice, gd3dDeviceContext))
        return false;

    return true;
}

void Update()
{
    ImGui_ImplDX11_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();

    ImGui::Begin("Hello, ImGui");
    ImGui::Text("This is some useful text.");
    ImGui::SliderFloat("float", &gSliderValue, 0.0f, 1.0f);
    ImGui::ColorEdit3("clear color", gClearColor);
    if (ImGui::Button("Button"))
        gSliderValue = 1.0f;
    ImGui::End();

    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(ImVec2(0, 0));

    ImGui::Begin("TextOverlay", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
                                         ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar |
                                         ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoInputs |
                                         ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoBackground);
    ImGui::Text("This text can't move.");
    ImGui::Text("sliderValue: %.1f", gSliderValue);
    ImGui::End();
}

void Draw()
{
    ImGui::Render();
    ImVec4 clear_color = ImVec4(gClearColor[0], gClearColor[1], gClearColor[2], 1.00f);

    gd3dDeviceContext->ClearRenderTargetView(gRTV, reinterpret_cast<float*>(&clear_color));
    gd3dDeviceContext->ClearDepthStencilView(gDSV, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

    gSwapChain->Present(0, 0);
}

int RunWindow()
{
    MSG msg = { 0 };

    while (msg.message != WM_QUIT)
    {
        if (PeekMessage(&msg, 0, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        else
        {
            Update();
            Draw();
        }
    }

    return (int)msg.wParam;
}

void Cleanup()
{
    if (gRTV) gRTV->Release();
    if (gDSV) gDSV->Release();
    if (gBackBuffer) gBackBuffer->Release();
    if (gSwapChain) gSwapChain->Release();
    if (gd3dDeviceContext) gd3dDeviceContext->Release();
    if (gd3dDevice) gd3dDevice->Release();

    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int showCmd)
{
    int width = 1280;
    int height = 720;

    if (!CreateCustomWindow(hInstance, width, height))
        return -1;

    if (!CreateCustomDirect3D(width, height))
        return -1;

    if (!CreateCustomImGui())
        return -1;

    return RunWindow();
}

