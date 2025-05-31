// DirectX12_Proj.cpp : Defines the entry point for the application.
//

#include "framework.h"
#include "DirectX12_Proj.h"
#include <d3d12.h>
#include <dxgi1_6.h>
#include <comdef.h>
#include <Windows.h>
#include <wrl/client.h>
#include <d3dcompiler.h>
#include "d3dx12.h"
#include <DirectXMath.h>
#include "DDSTextureLoader.h"
#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3dcompiler.lib")
using namespace DirectX;

#define MAX_LOADSTRING 100

// step 10: define our vertices
struct Vertex
{
    XMFLOAT3 Position;  // offset 0
    XMFLOAT3 Color;     // offset 12
    XMFLOAT2 UV;        // offset 24 (12 + 12)
    XMFLOAT3 Normal;    // offset 32 (12 + 12 + 8)
};

struct Light
{
    XMFLOAT3 Position;
    float FallOffStart;
    XMFLOAT3 Direction;
    float FallOffEnd;
    XMFLOAT3 Strength;
    int type;	// 1 for directional, 2 for point light, 3 for spot light
};

struct ConstantBuffer
{
    XMMATRIX Model;
    XMMATRIX World;
    XMMATRIX LightViewProj;
    XMMATRIX LightViewProjTextureSpace;
    XMMATRIX PerspectiveViewProj;
    Light directionalLight;
    XMFLOAT4 CameraPosition;
};
const int g_iWidth = 1024;
const int g_iHeight = 768;


XMVECTOR eyePos;
XMMATRIX view, model, proj, modelNormal;

// Sun light
ConstantBuffer cBuffer;

// Global Variables:
HINSTANCE hInst;                                // current instance
WCHAR szTitle[MAX_LOADSTRING];                  // The title bar text
WCHAR szWindowClass[MAX_LOADSTRING];            // the main window class name
HWND hWnd;
using Microsoft::WRL::ComPtr;
#include <wrl/client.h>;

// Forward declarations of functions included in this code module:
ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);

//#define ThrowIfFailed(hr) if (FAILED(hr)) { __debugbreak(); }
#define ThrowIfFailed(hr)                                                \
    do {                                                                \
        HRESULT _hr = (hr);                                             \
        if (FAILED(_hr)) {                                              \
            _com_error err(_hr);                                        \
            MessageBoxW(nullptr, err.ErrorMessage(), L"Error", MB_OK | MB_ICONERROR); \
            __debugbreak();                                             \
        }                                                               \
    } while (0)


LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_KEYDOWN:

        XMFLOAT4 eyeP;
        XMStoreFloat4(&eyeP, eyePos);

        if (wParam == VK_ESCAPE)
        {
            PostQuitMessage(0);
            return 0;
        }
        if (wParam == 'W')
        {
            eyeP.z += 0.1f;
        }
        if (wParam == 'S')
        {
            eyeP.z -= 0.1f;
        }
        if (wParam == 'A')
        {
            eyeP.x -= 0.1f;
        }
        if (wParam == 'D')
        {
            eyeP.x += 0.1f;
        }
        if (wParam == 'Q')
        {
            eyeP.y -= 0.1f;
        }
        if (wParam == 'E')
        {
            eyeP.y += 0.1f;
        }

        // Light
        if (wParam == 'I')
        {
            cBuffer.directionalLight.Position.z += 0.1f;
        }
        if (wParam == 'K')
        {
            cBuffer.directionalLight.Position.z -= 0.1f;
        }
        if (wParam == 'J')
        {
            cBuffer.directionalLight.Position.x -= 0.1f;
        }
        if (wParam == 'L')
        {
            cBuffer.directionalLight.Position.x += 0.1f;
        }
        if (wParam == 'U')
        {
            cBuffer.directionalLight.Position.y -= 0.1f;
        }
        if (wParam == 'O')
        {
            cBuffer.directionalLight.Position.y += 0.1f;
        }

        eyePos = XMLoadFloat4(&eyeP);
        break;

    case WM_COMMAND:
    {
        int wmId = LOWORD(wParam);
        // Parse the menu selections:
        switch (wmId)
        {
        case IDM_ABOUT:
            DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
            break;
        case IDM_EXIT:
            DestroyWindow(hWnd);
            break;
        default:
            return DefWindowProc(hWnd, message, wParam, lParam);
        }
    }
    break;
    case WM_PAINT:
    {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hWnd, &ps);
        // TODO: Add any drawing code that uses hdc here...
        EndPaint(hWnd, &ps);
    }
    break;
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
        break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_ LPWSTR    lpCmdLine,
    _In_ int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    // TODO: Place code here.

    // Initialize global strings
    LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    LoadStringW(hInstance, IDC_DIRECTX12PROJ, szWindowClass, MAX_LOADSTRING);
    MyRegisterClass(hInstance);

    // Perform application initialization:
    if (!InitInstance(hInstance, nCmdShow))
    {
        return FALSE;
    }

    HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_DIRECTX12PROJ));

    MSG msg = { 0 };


    // DirectX 12 init:
    ID3D12Device* device = nullptr;
    IDXGIFactory2* factory = nullptr;
    ID3D12CommandQueue* commandQueue = nullptr;
    ID3D12CommandAllocator* commandAllocator = nullptr;
    ID3D12GraphicsCommandList* commandList = nullptr;
    ID3D12Fence* fence = nullptr;
    HANDLE fenceEvt;
    IDXGISwapChain1* swapChain = nullptr;
    ID3D12Debug* debugController;
    ID3D12DescriptorHeap* rtvHeap;
    ID3D12Resource* rtvResources[3];

    // optional but recommended: enable the debug layer
    ThrowIfFailed(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController)));
    debugController->EnableDebugLayer();

    // step 1: init the device
    ThrowIfFailed(D3D12CreateDevice(nullptr, D3D_FEATURE_LEVEL_12_2, IID_PPV_ARGS(&device)));

    // step 2: create a factory
    ThrowIfFailed(CreateDXGIFactory2(DXGI_CREATE_FACTORY_DEBUG, IID_PPV_ARGS(&factory)));

    // step 3: create command queue
    D3D12_COMMAND_QUEUE_DESC cmdQueueDesc;
    ZeroMemory(&cmdQueueDesc, sizeof(D3D12_COMMAND_QUEUE_DESC));

    cmdQueueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
    cmdQueueDesc.Priority = 0;
    cmdQueueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;

    ThrowIfFailed(device->CreateCommandQueue(&cmdQueueDesc, IID_PPV_ARGS(&commandQueue)));

    // step 4: create command allocator
    ThrowIfFailed(device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&commandAllocator)));

    // step 5: create the command list
    ThrowIfFailed(device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, commandAllocator, nullptr, IID_PPV_ARGS(&commandList)));
    ThrowIfFailed(commandList->Close());

    // step 6: fence
    ThrowIfFailed(device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence)));
    fenceEvt = CreateEvent(nullptr, FALSE, FALSE, nullptr);

    // step 7: swap chain descriptor
    DXGI_SWAP_CHAIN_DESC1 swapchaindesc;
    ZeroMemory(&swapchaindesc, sizeof(DXGI_SWAP_CHAIN_DESC1));

    swapchaindesc.Width = g_iWidth;
    swapchaindesc.Height = g_iHeight;
    swapchaindesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    swapchaindesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapchaindesc.BufferCount = 3;
    swapchaindesc.Flags = 0;
    swapchaindesc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
    swapchaindesc.Scaling = DXGI_SCALING_STRETCH;
    swapchaindesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    swapchaindesc.SampleDesc.Count = 1;

    ThrowIfFailed(factory->CreateSwapChainForHwnd(commandQueue, hWnd, &swapchaindesc, nullptr, nullptr, &swapChain));

    // step 8: Render Target View Descriptor Heap
    D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc;
    ZeroMemory(&rtvHeapDesc, sizeof(D3D12_DESCRIPTOR_HEAP_DESC));

    rtvHeapDesc.NumDescriptors = 3;
    rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
    rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

    ThrowIfFailed(device->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&rtvHeap)));

    CD3DX12_CPU_DESCRIPTOR_HANDLE rtvCPUHandle(rtvHeap->GetCPUDescriptorHandleForHeapStart());
    // step 9: create render target view for each frame
    for (UINT i = 0; i < 3; i++)
    {
        // get buffer
        ThrowIfFailed(swapChain->GetBuffer(i, IID_PPV_ARGS(&rtvResources[i])));


        // create rtv
        device->CreateRenderTargetView(rtvResources[i], nullptr, rtvCPUHandle);

        // offset
        rtvCPUHandle.Offset(1, device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV));
    }
    assert(IsWindow(hWnd));


    Vertex cube_verticies[] = {

        // vertex.x, vertex.y, color.r, color.g, color.b
        /*0*/XMFLOAT3({-0.5f, -0.5f, -0.5f}), XMFLOAT3({ 1.0f, 0.0f, 1.0f }), XMFLOAT2(0.0f, 0.0f), XMFLOAT3(0.0f, 0.0f, -1.0f), // Max Depth
        /*1*/XMFLOAT3({-0.5f, +0.5f, -0.5f}), XMFLOAT3({ 1.0f, 0.0f, 1.0f }), XMFLOAT2(1.0f, 0.0f), XMFLOAT3(0.0f, 0.0f, -1.0f), // Max Depth
        /*2*/XMFLOAT3({+0.5f, +0.5f, -0.5f}), XMFLOAT3({ 1.0f, 0.0f, 1.0f }), XMFLOAT2(1.0f, 1.0f), XMFLOAT3(0.0f, 0.0f, -1.0f), // Max Depth
        /*2*/XMFLOAT3({+0.5f, +0.5f, -0.5f}), XMFLOAT3({ 1.0f, 0.0f, 1.0f }), XMFLOAT2(1.0f, 1.0f), XMFLOAT3(0.0f, 0.0f, -1.0f), // Max Depth
        /*3*/XMFLOAT3({+0.5f, -0.5f, -0.5f}), XMFLOAT3({ 1.0f, 0.0f, 1.0f }), XMFLOAT2(0.0f, 1.0f), XMFLOAT3(0.0f, 0.0f, -1.0f), // Max Depth
        /*0*/XMFLOAT3({-0.5f, -0.5f, -0.5f}), XMFLOAT3({ 1.0f, 0.0f, 1.0f }), XMFLOAT2(0.0f, 0.0f), XMFLOAT3(0.0f, 0.0f, -1.0f), // Max Depth

        /*0*/XMFLOAT3({-0.5f, -0.5f, +0.5f}), XMFLOAT3({ 1.0f, 0.0f, 1.0f }), XMFLOAT2(0.0f, 0.0f), XMFLOAT3(0.0f, 0.0f, +1.0f), // Least Depth
        /*2*/XMFLOAT3({+0.5f, +0.5f, +0.5f}), XMFLOAT3({ 1.0f, 0.0f, 1.0f }), XMFLOAT2(1.0f, 1.0f), XMFLOAT3(0.0f, 0.0f, +1.0f), // Least Depth
        /*1*/XMFLOAT3({-0.5f, +0.5f, +0.5f}), XMFLOAT3({ 1.0f, 0.0f, 1.0f }), XMFLOAT2(1.0f, 0.0f), XMFLOAT3(0.0f, 0.0f, +1.0f), // Least Depth
        /*2*/XMFLOAT3({+0.5f, +0.5f, +0.5f}), XMFLOAT3({ 1.0f, 0.0f, 1.0f }), XMFLOAT2(1.0f, 1.0f), XMFLOAT3(0.0f, 0.0f, +1.0f), // Least Depth
        /*0*/XMFLOAT3({-0.5f, -0.5f, +0.5f}), XMFLOAT3({ 1.0f, 0.0f, 1.0f }), XMFLOAT2(0.0f, 0.0f), XMFLOAT3(0.0f, 0.0f, +1.0f), // Least Depth
        /*3*/XMFLOAT3({+0.5f, -0.5f, +0.5f}), XMFLOAT3({ 1.0f, 0.0f, 1.0f }), XMFLOAT2(0.0f, 1.0f), XMFLOAT3(0.0f, 0.0f, +1.0f), // Least Depth


        /*3*/XMFLOAT3({-0.5f, +0.5f, -0.5f}), XMFLOAT3({ 1.0f, 1.0f, 0.0f }), XMFLOAT2(0.0f, 0.0f), XMFLOAT3(0.0f, +1.0f, 0.0f),// bottom left--------------------------------------
        /*3*/XMFLOAT3({-0.5f, +0.5f, +0.5f}), XMFLOAT3({ 1.0f, 1.0f, 0.0f }), XMFLOAT2(1.0f, 0.0f), XMFLOAT3(0.0f, +1.0f, 0.0f),// bottom left
        /*3*/XMFLOAT3({+0.5f, +0.5f, +0.5f}), XMFLOAT3({ 1.0f, 1.0f, 0.0f }), XMFLOAT2(1.0f, 1.0f), XMFLOAT3(0.0f, +1.0f, 0.0f),// bottom left
        /*3*/XMFLOAT3({+0.5f, +0.5f, +0.5f}), XMFLOAT3({ 1.0f, 1.0f, 0.0f }), XMFLOAT2(1.0f, 1.0f), XMFLOAT3(0.0f, +1.0f, 0.0f),// bottom left
        /*3*/XMFLOAT3({+0.5f, +0.5f, -0.5f}), XMFLOAT3({ 1.0f, 1.0f, 0.0f }), XMFLOAT2(0.0f, 1.0f), XMFLOAT3(0.0f, +1.0f, 0.0f),// bottom left
        /*3*/XMFLOAT3({-0.5f, +0.5f, -0.5f}), XMFLOAT3({ 1.0f, 1.0f, 0.0f }), XMFLOAT2(0.0f, 0.0f), XMFLOAT3(0.0f, +1.0f, 0.0f),// bottom left--------------------------------------


        /*3*/XMFLOAT3({-0.5f, -0.5f, -0.5f}), XMFLOAT3({ 1.0f, 1.0f, 0.0f }), XMFLOAT2(0.0f, 0.0f), XMFLOAT3(0.0f, -1.0f, 0.0f), // bottom left--------------------------------------
        /*3*/XMFLOAT3({+0.5f, -0.5f, -0.5f}), XMFLOAT3({ 1.0f, 1.0f, 0.0f }), XMFLOAT2(1.0f, 0.0f), XMFLOAT3(0.0f, -1.0f, 0.0f), // bottom left
        /*3*/XMFLOAT3({+0.5f, -0.5f, +0.5f}), XMFLOAT3({ 1.0f, 1.0f, 0.0f }), XMFLOAT2(1.0f, 1.0f), XMFLOAT3(0.0f, -1.0f, 0.0f), // bottom left
        /*3*/XMFLOAT3({+0.5f, -0.5f, +0.5f}), XMFLOAT3({ 1.0f, 1.0f, 0.0f }), XMFLOAT2(1.0f, 1.0f), XMFLOAT3(0.0f, -1.0f, 0.0f), // bottom left
        /*3*/XMFLOAT3({-0.5f, -0.5f, +0.5f}), XMFLOAT3({ 1.0f, 1.0f, 0.0f }), XMFLOAT2(0.0f, 1.0f), XMFLOAT3(0.0f, -1.0f, 0.0f), // bottom left
        /*3*/XMFLOAT3({-0.5f, -0.5f, -0.5f}), XMFLOAT3({ 1.0f, 1.0f, 0.0f }), XMFLOAT2(0.0f, 0.0f), XMFLOAT3(0.0f, -1.0f, 0.0f), // bottom left--------------------------------------


        /*3*/XMFLOAT3({+0.5f, -0.5f, -0.5f}), XMFLOAT3({ 0.0f, 1.0f, 1.0f }), XMFLOAT2(0.0f, 0.0f), XMFLOAT3(+1.0f, 0.0f, 0.0f), // bottom left--------------------------------------
        /*3*/XMFLOAT3({+0.5f, +0.5f, -0.5f}), XMFLOAT3({ 0.0f, 1.0f, 1.0f }), XMFLOAT2(1.0f, 0.0f), XMFLOAT3(+1.0f, 0.0f, 0.0f), // bottom left
        /*3*/XMFLOAT3({+0.5f, +0.5f, +0.5f}), XMFLOAT3({ 0.0f, 1.0f, 1.0f }), XMFLOAT2(1.0f, 1.0f), XMFLOAT3(+1.0f, 0.0f, 0.0f), // bottom left
        /*3*/XMFLOAT3({+0.5f, +0.5f, +0.5f}), XMFLOAT3({ 0.0f, 1.0f, 1.0f }), XMFLOAT2(1.0f, 1.0f), XMFLOAT3(+1.0f, 0.0f, 0.0f), // bottom left
        /*3*/XMFLOAT3({+0.5f, -0.5f, +0.5f}), XMFLOAT3({ 0.0f, 1.0f, 1.0f }), XMFLOAT2(0.0f, 1.0f), XMFLOAT3(+1.0f, 0.0f, 0.0f), // bottom left
        /*3*/XMFLOAT3({+0.5f, -0.5f, -0.5f}), XMFLOAT3({ 0.0f, 1.0f, 1.0f }), XMFLOAT2(0.0f, 0.0f), XMFLOAT3(+1.0f, 0.0f, 0.0f), // bottom left--------------------------------------

        /*3*/XMFLOAT3({-0.5f, -0.5f, -0.5f}), XMFLOAT3({ 0.0f, 1.0f, 1.0f }), XMFLOAT2(0.0f, 0.0f), XMFLOAT3(-1.0f, 0.0f, 0.0f), // bottom left--------------------------------------
        /*3*/XMFLOAT3({-0.5f, -0.5f, +0.5f}), XMFLOAT3({ 0.0f, 1.0f, 1.0f }), XMFLOAT2(1.0f, 0.0f), XMFLOAT3(-1.0f, 0.0f, 0.0f), // bottom left
        /*3*/XMFLOAT3({-0.5f, +0.5f, +0.5f}), XMFLOAT3({ 0.0f, 1.0f, 1.0f }), XMFLOAT2(1.0f, 1.0f), XMFLOAT3(-1.0f, 0.0f, 0.0f), // bottom left
        /*3*/XMFLOAT3({-0.5f, +0.5f, +0.5f}), XMFLOAT3({ 0.0f, 1.0f, 1.0f }), XMFLOAT2(1.0f, 1.0f), XMFLOAT3(-1.0f, 0.0f, 0.0f), // bottom left
        /*3*/XMFLOAT3({-0.5f, +0.5f, -0.5f}), XMFLOAT3({ 0.0f, 1.0f, 1.0f }), XMFLOAT2(0.0f, 1.0f), XMFLOAT3(-1.0f, 0.0f, 0.0f), // bottom left
        /*3*/XMFLOAT3({-0.5f, -0.5f, -0.5f}), XMFLOAT3({ 0.0f, 1.0f, 1.0f }), XMFLOAT2(0.0f, 0.0f), XMFLOAT3(-1.0f, 0.0f, 0.0f), // bottom left


    };


    float uv_wrap = 7.0f;
    Vertex quad_verticies_y[] = {
        { XMFLOAT3(-0.5f, 0.0f, -0.5f), XMFLOAT3(1.0f, 0.0f, 1.0f), XMFLOAT2(0.0f, 0.0f), XMFLOAT3(0.0f, 1.0f, 0.0f) },
        { XMFLOAT3(-0.5f, 0.0f, +0.5f), XMFLOAT3(1.0f, 1.0f, 0.0f), XMFLOAT2(uv_wrap, 0.0f), XMFLOAT3(0.0f, 1.0f, 0.0f) },
        { XMFLOAT3(+0.5f, 0.0f, +0.5f), XMFLOAT3(1.0f, 1.0f, 1.0f), XMFLOAT2(uv_wrap, uv_wrap), XMFLOAT3(0.0f, 1.0f, 0.0f) },
        { XMFLOAT3(+0.5f, 0.0f, -0.5f), XMFLOAT3(0.0f, 1.0f, 1.0f), XMFLOAT2(0.0f, uv_wrap), XMFLOAT3(0.0f, 1.0f, 0.0f) },
    };

    UINT16 quad_indicies_y[] = {
        0, 1, 2,
        2, 3, 0
    };

    Vertex quad_verticies_z[] = {
        { XMFLOAT3(0.0f,  0.0f, 0.0f), XMFLOAT3(1.0f, 0.0f, 1.0f), XMFLOAT2(0.0f, 0.0f), XMFLOAT3(0.0f, 1.0f, 0.0f) },
        { XMFLOAT3(1.0f,  0.0f, 0.0f), XMFLOAT3(1.0f, 1.0f, 0.0f), XMFLOAT2(1.0f, 0.0f), XMFLOAT3(0.0f, 1.0f, 0.0f) },
        { XMFLOAT3(1.0f, -1.0f, 0.0f), XMFLOAT3(1.0f, 1.0f, 1.0f), XMFLOAT2(1.0f, 1.0f), XMFLOAT3(0.0f, 1.0f, 0.0f) },
        { XMFLOAT3(0.0f, -1.0f, 0.0f), XMFLOAT3(1.0f, 0.0f, 1.0f), XMFLOAT2(0.0f, 1.0f), XMFLOAT3(0.0f, 1.0f, 0.0f) },
        { XMFLOAT3(1.0f, -1.0f, 0.0f), XMFLOAT3(1.0f, 1.0f, 1.0f), XMFLOAT2(1.0f, 1.0f), XMFLOAT3(0.0f, 1.0f, 0.0f) },
        { XMFLOAT3(0.0f, -1.0f, 0.0f), XMFLOAT3(1.0f, 0.0f, 1.0f), XMFLOAT2(0.0f, 1.0f), XMFLOAT3(0.0f, 1.0f, 0.0f) },
    };
    UINT16 quad_indicies_z[] = {
        0, 1, 2,
        2, 3, 0
    };

    cBuffer.directionalLight.Position = XMFLOAT3(-1.0f, 1.0f, -1.0f);
    cBuffer.directionalLight.Direction = XMFLOAT3(0.0f, -1.0f, -1.0f);
    cBuffer.directionalLight.Strength = XMFLOAT3(1.0f, 1.0f, 1.0f);
    cBuffer.directionalLight.FallOffStart = 0.1f;
    cBuffer.directionalLight.FallOffEnd = 3.0f;

    // Constant buffer steps
    // step 1: create 2 resources

    const int CBV_Views = 15;
    ID3D12Resource* pCBVResource[CBV_Views];
    void* pCBVBytes[CBV_Views];

    for (UINT resource_index = 0; resource_index < CBV_Views; resource_index++)
    {
        // step 1:
        //auto cbuffer_desc = ;
        //auto cbuffer_heap_properties = ;

        ThrowIfFailed(device->CreateCommittedResource(
            &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
            D3D12_HEAP_FLAG_NONE,
            &CD3DX12_RESOURCE_DESC::Buffer(512U),
            D3D12_RESOURCE_STATE_GENERIC_READ,
            nullptr,
            IID_PPV_ARGS(&pCBVResource[resource_index])
        ));

        ThrowIfFailed(pCBVResource[resource_index]->Map(0, nullptr, reinterpret_cast<void**>(&pCBVBytes[resource_index])));
        CopyMemory(pCBVBytes[resource_index], &cBuffer, 512U);
        pCBVResource[resource_index]->Unmap(0, nullptr);
    }

    auto create_vertex_buffer_view = [&device](Vertex* verticies, std::size_t vertexSize) -> D3D12_VERTEX_BUFFER_VIEW
    {
        void* pData;
        ID3D12Resource* pVertexBufferViewResource = nullptr;
        D3D12_VERTEX_BUFFER_VIEW vbufferview;
        // Liam's Notes: Using Upload heaps should be done along side default heaps but this will work for the time being.
        // vertex buffer view
        // step 11.1: create a resource to store the vertex buffer

        auto vbuffer_desc = CD3DX12_RESOURCE_DESC::Buffer(vertexSize);
        auto vbuffer_heap_properties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);

        ThrowIfFailed(device->CreateCommittedResource(
            &vbuffer_heap_properties,
            D3D12_HEAP_FLAG_NONE,
            &vbuffer_desc,
            D3D12_RESOURCE_STATE_GENERIC_READ,
            nullptr,
            IID_PPV_ARGS(&pVertexBufferViewResource)
        ));

        // step 11.2 copy the data to the resource
        CD3DX12_RANGE range(0, 0);
        ThrowIfFailed(pVertexBufferViewResource->Map(0, &range, reinterpret_cast<void**>(&pData)));
        CopyMemory(pData, verticies, vertexSize);
        pVertexBufferViewResource->Unmap(0, nullptr);


        ZeroMemory(&vbufferview, sizeof(D3D12_VERTEX_BUFFER_VIEW));

        vbufferview.BufferLocation = pVertexBufferViewResource->GetGPUVirtualAddress();
        vbufferview.StrideInBytes = sizeof(Vertex);
        vbufferview.SizeInBytes = vertexSize;

        return vbufferview;
    };


    auto create_index_buffer_view = [&device](std::uint16_t* indicies, std::size_t indexSize) -> D3D12_INDEX_BUFFER_VIEW
    {
        D3D12_INDEX_BUFFER_VIEW ibufferview;
        // index buffer view
        // step 11.2: create a resource to store the index buffer
        ID3D12Resource* pIndexBufferViewResource;

        auto ibuffer_desc = CD3DX12_RESOURCE_DESC::Buffer(indexSize);
        auto ibuffer_heap_properties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);

        ThrowIfFailed(device->CreateCommittedResource(
            &ibuffer_heap_properties,
            D3D12_HEAP_FLAG_NONE,
            &ibuffer_desc,
            D3D12_RESOURCE_STATE_GENERIC_READ,
            nullptr,
            IID_PPV_ARGS(&pIndexBufferViewResource)
        ));

        // step 11.2 copy the data to the resource
        CD3DX12_RANGE range(0, 0);
        void* pData;
        ThrowIfFailed(pIndexBufferViewResource->Map(0, &range, reinterpret_cast<void**>(&pData)));
        CopyMemory(pData, indicies, indexSize);
        pIndexBufferViewResource->Unmap(0, nullptr);


        ZeroMemory(&ibufferview, sizeof(D3D12_INDEX_BUFFER_VIEW));

        ibufferview.BufferLocation = pIndexBufferViewResource->GetGPUVirtualAddress();
        ibufferview.SizeInBytes = indexSize;
        ibufferview.Format = DXGI_FORMAT_R16_UINT;
        return ibufferview;
    };
    /***********************************************************************************************************************************************************/
    // Depth Buffer



    ComPtr<ID3D12Resource> depthBufferResource;

    // depth buffer
    {
        D3D12_RESOURCE_DESC depthDesc;
        ZeroMemory(&depthDesc, sizeof(D3D12_RESOURCE_DESC));

        depthDesc.Format = DXGI_FORMAT_R24G8_TYPELESS;
        depthDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
        depthDesc.Width = g_iWidth;
        depthDesc.Height = g_iHeight;
        depthDesc.MipLevels = 1;
        depthDesc.DepthOrArraySize = 1;
        depthDesc.SampleDesc.Count = 1;
        depthDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

        D3D12_CLEAR_VALUE clearValue;
        ZeroMemory(&clearValue, sizeof(D3D12_CLEAR_VALUE));

        clearValue.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
        clearValue.DepthStencil.Depth = 1.0f;

        // depth buffer resource
        auto heapProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
        ThrowIfFailed(device->CreateCommittedResource(
            &heapProps,
            D3D12_HEAP_FLAG_NONE,
            &depthDesc,
            D3D12_RESOURCE_STATE_DEPTH_WRITE,
            &clearValue,
            IID_PPV_ARGS(&depthBufferResource)
        ));
    }


    // depth stencil view desc
    D3D12_DEPTH_STENCIL_VIEW_DESC depthStenvilViewDesc;
    ZeroMemory(&depthStenvilViewDesc, sizeof(D3D12_DEPTH_STENCIL_VIEW_DESC));

    depthStenvilViewDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
    depthStenvilViewDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
    depthStenvilViewDesc.Flags = D3D12_DSV_FLAG_NONE;

    ID3D12DescriptorHeap* depthDescriptorHeap;
    D3D12_DESCRIPTOR_HEAP_DESC depthdescHeapDesc;
    ZeroMemory(&depthdescHeapDesc, sizeof(D3D12_DESCRIPTOR_HEAP_DESC));

    depthdescHeapDesc.NumDescriptors = 1;
    depthdescHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;

    ThrowIfFailed(device->CreateDescriptorHeap(&depthdescHeapDesc, IID_PPV_ARGS(&depthDescriptorHeap)));

    device->CreateDepthStencilView(depthBufferResource.Get(), &depthStenvilViewDesc, depthDescriptorHeap->GetCPUDescriptorHandleForHeapStart());



    // Don't need this anymore but leaving the code here.

    //ID3D12DescriptorHeap* depthStencilSRVHeap;

    D3D12_DESCRIPTOR_HEAP_DESC depthSRVHeapDesc = {};
    depthSRVHeapDesc.NumDescriptors = 1;
    depthSRVHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    depthSRVHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;

    //ThrowIfFailed(device->CreateDescriptorHeap(&depthSRVHeapDesc, IID_PPV_ARGS(&depthStencilSRVHeap)));

    ComPtr<ID3D12DescriptorHeap> depthSRVDescriptorHeap;


    D3D12_DESCRIPTOR_HEAP_DESC dsvDesc = { };
    dsvDesc.NumDescriptors = 1;
    dsvDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    dsvDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    ThrowIfFailed(device->CreateDescriptorHeap(&dsvDesc, IID_PPV_ARGS(&depthSRVDescriptorHeap)));


    /*****************************************************/
    // Shadow map resource
    ComPtr<ID3D12Resource> shadowMapResource;
    ID3D12DescriptorHeap* shadowDSVHeap;

    // depth buffer for shadow map
    {
        D3D12_RESOURCE_DESC depthDesc;
        ZeroMemory(&depthDesc, sizeof(D3D12_RESOURCE_DESC));

        depthDesc.Format = DXGI_FORMAT_R24G8_TYPELESS;
        depthDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
        depthDesc.Width = g_iWidth;
        depthDesc.Height = g_iHeight;
        depthDesc.MipLevels = 1;
        depthDesc.DepthOrArraySize = 1;
        depthDesc.SampleDesc.Count = 1;
        depthDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

        D3D12_CLEAR_VALUE clearValue;
        ZeroMemory(&clearValue, sizeof(D3D12_CLEAR_VALUE));

        clearValue.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
        clearValue.DepthStencil.Depth = 1.0f;

        // depth buffer resource
        ThrowIfFailed(device->CreateCommittedResource(
            &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
            D3D12_HEAP_FLAG_NONE,
            &depthDesc,
            D3D12_RESOURCE_STATE_DEPTH_WRITE,
            &clearValue,
            IID_PPV_ARGS(&shadowMapResource)
        ));

        D3D12_DESCRIPTOR_HEAP_DESC dsvDescHeap = { };
        dsvDescHeap.NumDescriptors = 1;
        dsvDescHeap.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
        dsvDescHeap.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
        ThrowIfFailed(device->CreateDescriptorHeap(&dsvDescHeap, IID_PPV_ARGS(&shadowDSVHeap)));

        D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
        dsvDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT; // must match your resource's clear value format
        dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
        dsvDesc.Flags = D3D12_DSV_FLAG_NONE;

        device->CreateDepthStencilView(shadowMapResource.Get(), &dsvDesc, shadowDSVHeap->GetCPUDescriptorHandleForHeapStart());
    }
    // Texture Heap
    // Descriptor Index 0 -> Stone
    // Descriptor Index 1 -> Checkboard
    // Descriptor Index 2 -> Depth Buffer for Shadow Map
    ID3D12DescriptorHeap* textureDescriptorHeap = nullptr;
    D3D12_DESCRIPTOR_HEAP_DESC srvDesc = { };
    srvDesc.NumDescriptors = 3;
    srvDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    srvDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;

    ThrowIfFailed(device->CreateDescriptorHeap(&srvDesc, IID_PPV_ARGS(&textureDescriptorHeap)));

    float dist = 10.0f;
    cBuffer.directionalLight.Position = XMFLOAT3(0.0f, dist, -dist);
    // Light Projection
    auto update_shadow_object = [&](XMFLOAT3 Translate, float Rotation, XMFLOAT3 Scale = XMFLOAT3(1.0f, 1.0f, 1.0f))
    {
        XMVECTOR sceneCenter = XMVectorSet(0.0f, 0.0f, 0.0f, 1.0f);
        float distanceBack = 10.0f; // How far back the light camera is placed
        float sceneRadius = 6.0f;

        
        XMFLOAT4 LightP = XMFLOAT4(
            cBuffer.directionalLight.Position.x, 
            cBuffer.directionalLight.Position.y, 
            cBuffer.directionalLight.Position.z, 
        1.0f);

        //XMFLOAT4 LightP = XMFLOAT4(0.0f, 4.0f, 4.0f, 1.0f);
        
        XMVECTOR lightPos = XMLoadFloat4(&LightP);       
        XMVECTOR lightDirection = XMVector3Normalize(sceneCenter - lightPos);   
        XMStoreFloat3(&cBuffer.directionalLight.Direction, lightDirection);
        
        XMVECTOR up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);

        XMMATRIX lightView = XMMatrixLookAtLH(lightPos, sceneCenter, up);

        // Correct:
        XMMATRIX lightProjection = XMMatrixOrthographicOffCenterLH(-sceneRadius, sceneRadius, -sceneRadius, sceneRadius, 0.01f, 60.0f);

        //XMMATRIX lightViewProjection = lightProjection * lightView;
        
        XMMATRIX scale = XMMatrixScaling(Scale.x, Scale.y, Scale.z);
        XMMATRIX rotation = XMMatrixRotationRollPitchYaw(XMConvertToRadians(0.0f), XMConvertToRadians(Rotation), XMConvertToRadians(0.0f));
        XMMATRIX translate = XMMatrixTranslation(Translate.x, Translate.y, Translate.z);

        //model = translate;
        model = scale * rotation * translate;
        //model = translate * rotation * scale;

        XMStoreFloat4(&cBuffer.CameraPosition, eyePos);

        cBuffer.World = XMMatrixTranspose(model * view * proj);


        
        XMMATRIX T(
            0.5f, 0.0f, 0.0f, 0.0f,
            0.0f, -0.5f, 0.0f, 0.0f,
            0.0f, 0.0f, 1.0f, 0.0f,
            0.5f, 0.5f, 0.0f, 1.0f
        );

        cBuffer.Model = XMMatrixTranspose(model);

        cBuffer.LightViewProj = XMMatrixTranspose(lightView * lightProjection);
        cBuffer.LightViewProjTextureSpace = XMMatrixTranspose(lightView * lightProjection * T);
        
    };
    //float angle = 0.0f;
    auto update_world_matrix = [&](XMFLOAT3 Translate, float Rotation, XMFLOAT3 Scale = XMFLOAT3(1.0f, 1.0f, 1.0f))
    {
        update_shadow_object(Translate, Rotation, Scale);

        // View Matrix
        //eyePos = XMVectorSet(0.0f, +3.0f, -6.0f, 0.0f);  // Camera position
        XMVECTOR focusPos = XMVectorSet(0.0f, 0.0f, 0.0f, 0.0f);   // Where camera looks
        XMVECTOR upDir = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);   // Up direction

        view = XMMatrixLookAtLH(eyePos, focusPos, upDir);

        // Projection Matrix
        float fovAngleY = XMConvertToRadians(45.0f); // 45 degree FOV vertical
        float aspectRatio = g_iWidth / g_iHeight;            // Width / Height of viewport
        float nearZ = 0.1f;
        float farZ = 100.0f;

        proj = XMMatrixPerspectiveFovLH(fovAngleY, aspectRatio, nearZ, farZ);

        XMMATRIX scale = XMMatrixScaling(Scale.x, Scale.y, Scale.z);
        XMMATRIX rotation = XMMatrixRotationRollPitchYaw(XMConvertToRadians(0.0f), XMConvertToRadians(Rotation), XMConvertToRadians(0.0f));
        XMMATRIX translate = XMMatrixTranslation(Translate.x, Translate.y, Translate.z);

        model = scale * rotation * translate;
        modelNormal = rotation;


        XMStoreFloat4(&cBuffer.CameraPosition, eyePos);

        cBuffer.Model = XMMatrixTranspose(model);
        //cBuffer.PerspectiveViewProj = XMMatrixTranspose(XMMatrixMultiply(view, proj));
        cBuffer.PerspectiveViewProj = XMMatrixTranspose(XMMatrixMultiply(view, proj));
        

    };

    

    /***********************************************************************************************************************************************************/

    /***********************************************************************************************************************************************************/
    // Shader Resource View

    // step 1: load the texture into a resource and upload heap

    ComPtr<ID3D12Resource> checkboardResourceView = nullptr;
    ComPtr<ID3D12Resource> checkboardUploadHeap = nullptr;
    ComPtr<ID3D12Resource> stoneResourceView = nullptr;
    ComPtr<ID3D12Resource> stoneUploadHeap = nullptr;

    ThrowIfFailed(commandAllocator->Reset());
    ThrowIfFailed(commandList->Reset(commandAllocator, nullptr));

    ThrowIfFailed(CreateDDSTextureFromFile12(device, commandList, L"checkboard.dds", checkboardResourceView, checkboardUploadHeap));
    ThrowIfFailed(CreateDDSTextureFromFile12(device, commandList, L"Stone.dds", stoneResourceView, stoneUploadHeap));


    ThrowIfFailed(commandList->Close());

    ID3D12CommandList* cmdLists[] = { commandList };
    commandQueue->ExecuteCommandLists(_countof(cmdLists), cmdLists);

    // Sync the CPU/GPU


    // step 2: shader resource view:


    D3D12_SHADER_RESOURCE_VIEW_DESC shaderResourceViewDesc = { };
    shaderResourceViewDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    shaderResourceViewDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    shaderResourceViewDesc.Format = checkboardResourceView->GetDesc().Format;
    shaderResourceViewDesc.Texture2D.MipLevels = checkboardResourceView->GetDesc().MipLevels;


    CD3DX12_CPU_DESCRIPTOR_HANDLE texHeapStartCPU(textureDescriptorHeap->GetCPUDescriptorHandleForHeapStart());
    device->CreateShaderResourceView(checkboardResourceView.Get(), &shaderResourceViewDesc, texHeapStartCPU);


    shaderResourceViewDesc.Format = stoneResourceView->GetDesc().Format;
    shaderResourceViewDesc.Texture2D.MipLevels = stoneUploadHeap->GetDesc().MipLevels;

    // Next descriptor
    texHeapStartCPU.Offset(1, device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV));
    device->CreateShaderResourceView(stoneResourceView.Get(), &shaderResourceViewDesc, texHeapStartCPU);


    D3D12_SHADER_RESOURCE_VIEW_DESC depthSRVDesc = { };
    ZeroMemory(&depthSRVDesc, sizeof(D3D12_SHADER_RESOURCE_VIEW_DESC));

    depthSRVDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    depthSRVDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    depthSRVDesc.Format = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;    // HAS TO BE R32 Format
    depthSRVDesc.Texture2D.MipLevels = 1;

    // Next descriptor
    texHeapStartCPU.Offset(1, device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV));
    CD3DX12_GPU_DESCRIPTOR_HANDLE shadowDepthBufferTextureHandle(textureDescriptorHeap->GetGPUDescriptorHandleForHeapStart());
    shadowDepthBufferTextureHandle.Offset(2, device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV));

    device->CreateShaderResourceView(shadowMapResource.Get(), &depthSRVDesc, texHeapStartCPU);




    // step 3: sampler state:
    CD3DX12_STATIC_SAMPLER_DESC samplerState[2];
    samplerState[0].Init(
        0, // base register
        D3D12_FILTER_MIN_MAG_MIP_POINT,
        D3D12_TEXTURE_ADDRESS_MODE_WRAP,
        D3D12_TEXTURE_ADDRESS_MODE_WRAP,
        D3D12_TEXTURE_ADDRESS_MODE_WRAP
    );
    // shadow sampler
    CD3DX12_STATIC_SAMPLER_DESC shadowSampler;
    samplerState[1].Init(
        1, // shader register
        D3D12_FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT, // comparison filtering
        D3D12_TEXTURE_ADDRESS_MODE_CLAMP, // addressU
        D3D12_TEXTURE_ADDRESS_MODE_CLAMP, // addressV
        D3D12_TEXTURE_ADDRESS_MODE_CLAMP, // addressW
        0.0f, // mipLODBias
        1, // maxAnisotropy
        D3D12_COMPARISON_FUNC_LESS_EQUAL, // comparison function
        D3D12_STATIC_BORDER_COLOR_OPAQUE_BLACK, // border color
        0, // minLOD
        D3D12_FLOAT32_MAX, // maxLOD
        D3D12_SHADER_VISIBILITY_PIXEL // shader visibility.
    );

    /***********************************************************************************************************************************************************/
    // Vertex and Index buffer views

    D3D12_VERTEX_BUFFER_VIEW cube_vertex_buffer_view = create_vertex_buffer_view(cube_verticies, sizeof(cube_verticies));

    D3D12_VERTEX_BUFFER_VIEW quad_vertex_buffer_view = create_vertex_buffer_view(quad_verticies_y, sizeof(quad_verticies_y));
    D3D12_INDEX_BUFFER_VIEW quad_index_buffer_view = create_index_buffer_view(quad_indicies_y, sizeof(quad_indicies_y));

    D3D12_VERTEX_BUFFER_VIEW screenspace_vertex_buffer_view = create_vertex_buffer_view(quad_verticies_z, sizeof(quad_verticies_z));
    D3D12_INDEX_BUFFER_VIEW screenspace_index_buffer_view = create_index_buffer_view(quad_indicies_z, sizeof(quad_indicies_z));

    // step 12 - root signature (empty for now, we dont have any Constant Buffers or Shader Resources)
    ID3DBlob* signature;
    ID3DBlob* error;
    ID3D12RootSignature* rootSignature;
    CD3DX12_ROOT_SIGNATURE_DESC root_sig_desc;
    ZeroMemory(&root_sig_desc, sizeof(CD3DX12_ROOT_SIGNATURE_DESC));

    // srv desciptor range:
    D3D12_DESCRIPTOR_RANGE srvTextureRange = {};
    srvTextureRange.BaseShaderRegister = 0; // base register
    srvTextureRange.NumDescriptors = 1;
    srvTextureRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
    srvTextureRange.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;


    D3D12_DESCRIPTOR_RANGE dsvTextureRange = {};
    dsvTextureRange.BaseShaderRegister = 1; // base register
    dsvTextureRange.NumDescriptors = 1;
    dsvTextureRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
    dsvTextureRange.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

    D3D12_DESCRIPTOR_RANGE shadowTextureRange = {};
    shadowTextureRange.BaseShaderRegister = 2; // base register
    shadowTextureRange.NumDescriptors = 1;
    shadowTextureRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
    shadowTextureRange.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

    CD3DX12_ROOT_PARAMETER rootParameters[4];
    rootParameters[0].InitAsConstantBufferView(0, 0, D3D12_SHADER_VISIBILITY_ALL);
    rootParameters[1].InitAsDescriptorTable(1, &srvTextureRange, D3D12_SHADER_VISIBILITY_ALL);
    rootParameters[2].InitAsDescriptorTable(1, &dsvTextureRange, D3D12_SHADER_VISIBILITY_ALL);
    rootParameters[3].InitAsDescriptorTable(1, &shadowTextureRange, D3D12_SHADER_VISIBILITY_ALL);
    root_sig_desc.Init(_countof(rootParameters), rootParameters, _countof(samplerState), samplerState, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);
    ThrowIfFailed(D3D12SerializeRootSignature(&root_sig_desc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &error));
    ThrowIfFailed(device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&rootSignature)));


    // step 13 - compile shaders
    ComPtr<ID3DBlob> vs_opaque;
    ComPtr<ID3DBlob> ps_opaque;

    ComPtr<ID3DBlob> vs_depth_opaque;
    ComPtr<ID3DBlob> ps_depth_opaque;

    ComPtr<ID3DBlob> vs_shadow_opaque;
    ComPtr<ID3DBlob> ps_shadow_opaque;

    ComPtr<ID3DBlob> vs_outline_opaque;
    ComPtr<ID3DBlob> ps_outline_opaque;

    ComPtr<ID3DBlob> errors;

    HRESULT hr;

    hr = D3DCompileFromFile(L"VS.hlsl", nullptr, nullptr, "VSMainOpaque", "vs_5_0", D3DCOMPILE_DEBUG, 0, &vs_opaque, &errors);
    if (FAILED(hr))
    {
        OutputDebugStringA((char*)errors->GetBufferPointer());
        __debugbreak();
    }

    hr = D3DCompileFromFile(L"PS.hlsl", nullptr, nullptr, "PSMainOpaque", "ps_5_0", D3DCOMPILE_DEBUG, 0, &ps_opaque, &errors);
    if (FAILED(hr))
    {
        OutputDebugStringA((char*)errors->GetBufferPointer());
        __debugbreak();
    }
    hr = D3DCompileFromFile(L"VS.hlsl", nullptr, nullptr, "VSMainDepthCamera", "vs_5_0", D3DCOMPILE_DEBUG, 0, &vs_depth_opaque, &errors);
    if (FAILED(hr))
    {
        OutputDebugStringA((char*)errors->GetBufferPointer());
        __debugbreak();
    }

    hr = D3DCompileFromFile(L"PS.hlsl", nullptr, nullptr, "PSMainDepthCamera", "ps_5_0", D3DCOMPILE_DEBUG, 0, &ps_depth_opaque, &errors);
    if (FAILED(hr))
    {
        OutputDebugStringA((char*)errors->GetBufferPointer());
        __debugbreak();
    }
    hr = D3DCompileFromFile(L"VS.hlsl", nullptr, nullptr, "VSMainShadow", "vs_5_0", D3DCOMPILE_DEBUG, 0, &vs_shadow_opaque, &errors);
    if (FAILED(hr))
    {
        OutputDebugStringA((char*)errors->GetBufferPointer());
        __debugbreak();
    }

    hr = D3DCompileFromFile(L"PS.hlsl", nullptr, nullptr, "PSMainShadow", "ps_5_0", D3DCOMPILE_DEBUG, 0, &ps_shadow_opaque, &errors);
    if (FAILED(hr))
    {
        OutputDebugStringA((char*)errors->GetBufferPointer());
        __debugbreak();
    }
    hr = D3DCompileFromFile(L"VS.hlsl", nullptr, nullptr, "VSMainOutline", "vs_5_0", D3DCOMPILE_DEBUG, 0, &vs_outline_opaque, &errors);
    if (FAILED(hr))
    {
        OutputDebugStringA((char*)errors->GetBufferPointer());
        __debugbreak();
    }

    hr = D3DCompileFromFile(L"PS.hlsl", nullptr, nullptr, "PSMainOutline", "ps_5_0", D3DCOMPILE_DEBUG, 0, &ps_outline_opaque, &errors);
    if (FAILED(hr))
    {
        OutputDebugStringA((char*)errors->GetBufferPointer());
        __debugbreak();
    }


    // step 14 - input layout that describes how our vertex buffer is layed out

    D3D12_INPUT_ELEMENT_DESC inputLayout[] = {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0U, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "COLOR", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12U, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "UVCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24U, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 32U, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
    };

    D3D12_DEPTH_STENCIL_DESC opaqueDepthViewDesc;
    ZeroMemory(&opaqueDepthViewDesc, sizeof(D3D12_DEPTH_STENCIL_DESC));

    opaqueDepthViewDesc.DepthEnable = TRUE;
    opaqueDepthViewDesc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
    opaqueDepthViewDesc.DepthFunc = D3D12_COMPARISON_FUNC_LESS;
    opaqueDepthViewDesc.StencilEnable = FALSE;



    D3D12_DEPTH_STENCIL_DESC opaqueDepthStencilViewDesc;
    ZeroMemory(&opaqueDepthStencilViewDesc, sizeof(D3D12_DEPTH_STENCIL_DESC));

    opaqueDepthStencilViewDesc.DepthFunc = D3D12_COMPARISON_FUNC_LESS; // changed now 2025-05-28 
    opaqueDepthStencilViewDesc.DepthEnable = true;
    opaqueDepthStencilViewDesc.StencilEnable = true;

    opaqueDepthStencilViewDesc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
    opaqueDepthStencilViewDesc.StencilReadMask = 255;
    opaqueDepthStencilViewDesc.StencilWriteMask = 255;
    
    opaqueDepthStencilViewDesc.FrontFace.StencilPassOp = D3D12_STENCIL_OP_REPLACE;  // Replace this value with what we set in cmdList->SetStencilRef()
    opaqueDepthStencilViewDesc.FrontFace.StencilFailOp = D3D12_STENCIL_OP_KEEP;
    opaqueDepthStencilViewDesc.FrontFace.StencilDepthFailOp = D3D12_STENCIL_OP_KEEP;
    opaqueDepthStencilViewDesc.FrontFace.StencilFunc = D3D12_COMPARISON_FUNC_ALWAYS;    // Always passes
    opaqueDepthStencilViewDesc.BackFace = opaqueDepthStencilViewDesc.FrontFace;

    D3D12_DEPTH_STENCIL_DESC outlineDepthStencilViewDesc;   
    ZeroMemory(&outlineDepthStencilViewDesc, sizeof(D3D12_DEPTH_STENCIL_DESC));

    outlineDepthStencilViewDesc.DepthFunc = D3D12_COMPARISON_FUNC_LESS; // changed now 2025-05-28 
    outlineDepthStencilViewDesc.DepthEnable = true;
    outlineDepthStencilViewDesc.StencilEnable = true;

    outlineDepthStencilViewDesc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
    outlineDepthStencilViewDesc.StencilReadMask = 255;
    outlineDepthStencilViewDesc.StencilWriteMask = 255;

    outlineDepthStencilViewDesc.FrontFace.StencilPassOp = D3D12_STENCIL_OP_KEEP;  // Replace this value with what we set in cmdList->SetStencilRef()
    outlineDepthStencilViewDesc.FrontFace.StencilFailOp = D3D12_STENCIL_OP_KEEP;
    outlineDepthStencilViewDesc.FrontFace.StencilDepthFailOp = D3D12_STENCIL_OP_KEEP;
    outlineDepthStencilViewDesc.FrontFace.StencilFunc = D3D12_COMPARISON_FUNC_NOT_EQUAL;    // Always passes
    outlineDepthStencilViewDesc.BackFace = outlineDepthStencilViewDesc.FrontFace;


    // step 15 - pipeline state object
    D3D12_GRAPHICS_PIPELINE_STATE_DESC opaquePsoDesc;
    ZeroMemory(&opaquePsoDesc, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));

    ID3D12PipelineState* opaquePipelineStateStencilOn;
    ID3D12PipelineState* opaquePipelineState;
    ID3D12PipelineState* depthPipelineState;
    ID3D12PipelineState* shadowPipelineState;
    ID3D12PipelineState* outlineStencilPSOState;

    opaquePsoDesc.pRootSignature = rootSignature;
    opaquePsoDesc.InputLayout = { inputLayout, _countof(inputLayout) };
    opaquePsoDesc.VS = { vs_opaque->GetBufferPointer(), vs_opaque->GetBufferSize() };
    opaquePsoDesc.PS = { ps_opaque->GetBufferPointer(), ps_opaque->GetBufferSize() };
    opaquePsoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
    opaquePsoDesc.SampleMask = UINT_MAX;
    opaquePsoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    opaquePsoDesc.SampleDesc.Count = 1;
    opaquePsoDesc.NumRenderTargets = 1;
    opaquePsoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
    opaquePsoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
    opaquePsoDesc.DepthStencilState = opaqueDepthViewDesc;
    opaquePsoDesc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;


    // Stenciling

    ThrowIfFailed(device->CreateGraphicsPipelineState(&opaquePsoDesc, IID_PPV_ARGS(&opaquePipelineState)));


    D3D12_GRAPHICS_PIPELINE_STATE_DESC depthCameraPsoDesc = opaquePsoDesc;
    depthCameraPsoDesc.VS = { vs_depth_opaque->GetBufferPointer(), vs_depth_opaque->GetBufferSize() };
    depthCameraPsoDesc.PS = { ps_depth_opaque->GetBufferPointer(), ps_depth_opaque->GetBufferSize() };
    depthCameraPsoDesc.DepthStencilState.DepthEnable = false;
    depthCameraPsoDesc.DepthStencilState.StencilEnable = false;

    ThrowIfFailed(device->CreateGraphicsPipelineState(&depthCameraPsoDesc, IID_PPV_ARGS(&depthPipelineState)));

    D3D12_GRAPHICS_PIPELINE_STATE_DESC shadowCameraPsoDesc = opaquePsoDesc;

    shadowCameraPsoDesc.VS = { vs_shadow_opaque->GetBufferPointer(), vs_shadow_opaque->GetBufferSize() };
    shadowCameraPsoDesc.PS = { /*ps_shadow_opaque->GetBufferPointer(), ps_shadow_opaque->GetBufferSize()*/ };
    shadowCameraPsoDesc.DepthStencilState = opaqueDepthViewDesc;

    D3D12_RASTERIZER_DESC shadowRasterizerDesc = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
    shadowRasterizerDesc.DepthBias = 10000;           // e.g. 1000
    shadowRasterizerDesc.DepthBiasClamp = 0.0f;
    shadowRasterizerDesc.SlopeScaledDepthBias = 1.0f;             // tweak this to reduce acne but avoid peter-panning
    shadowRasterizerDesc.CullMode = D3D12_CULL_MODE_BACK;
    shadowRasterizerDesc.FrontCounterClockwise = FALSE;
    shadowRasterizerDesc.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;
    shadowCameraPsoDesc.RasterizerState = shadowRasterizerDesc;
    
    shadowCameraPsoDesc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;


    ThrowIfFailed(device->CreateGraphicsPipelineState(&shadowCameraPsoDesc, IID_PPV_ARGS(&shadowPipelineState)));

    D3D12_GRAPHICS_PIPELINE_STATE_DESC outlinePSODesc = opaquePsoDesc;
    outlinePSODesc.VS = { vs_outline_opaque->GetBufferPointer(), vs_outline_opaque->GetBufferSize() };
    outlinePSODesc.PS = { ps_outline_opaque->GetBufferPointer(), ps_outline_opaque->GetBufferSize() };
    outlinePSODesc.DepthStencilState = outlineDepthStencilViewDesc;



    D3D12_GRAPHICS_PIPELINE_STATE_DESC opaquePSOStencilOnDes = opaquePsoDesc;
    opaquePSOStencilOnDes.DepthStencilState = opaqueDepthStencilViewDesc;
    opaquePSOStencilOnDes.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;

    ThrowIfFailed(device->CreateGraphicsPipelineState(&opaquePSOStencilOnDes, IID_PPV_ARGS(&opaquePipelineStateStencilOn)));

    ThrowIfFailed(device->CreateGraphicsPipelineState(&outlinePSODesc, IID_PPV_ARGS(&outlineStencilPSOState)));

    D3D12_VIEWPORT opaqueViewPort;
    ZeroMemory(&opaqueViewPort, sizeof(D3D12_VIEWPORT));

    opaqueViewPort.Width = g_iWidth;
    opaqueViewPort.Height = g_iHeight;
    opaqueViewPort.MinDepth = 0.0f;
    opaqueViewPort.MaxDepth = 1.0f;

    D3D12_VIEWPORT debugViewPort;
    ZeroMemory(&debugViewPort, sizeof(D3D12_VIEWPORT));

    debugViewPort.TopLeftX = g_iWidth * 0.75f ;
    debugViewPort.TopLeftY = g_iHeight * 0.75f;

    debugViewPort.Width = g_iWidth - (g_iWidth * 0.75f);
    debugViewPort.Height = g_iHeight - (g_iHeight * 0.75f);
    debugViewPort.MinDepth = 0.0f;
    debugViewPort.MaxDepth = 1.0f;

    D3D12_RECT scissorsRect;
    ZeroMemory(&scissorsRect, sizeof(D3D12_RECT));

    scissorsRect.right = g_iWidth;
    scissorsRect.bottom = g_iHeight;

    UINT64 back_buffer_index = 0;
    IDXGISwapChain3* swapChain3 = nullptr;
    float angle = 30.f;
    eyePos = XMVectorSet(0.0f, 3.0f, -6.0f, 1.0f);
    UINT64 fenceValue = 0;
    UINT64 frame = 0;
    while (msg.message != WM_QUIT)
    {
    
        angle += 0.2f;
    
        ThrowIfFailed(swapChain->QueryInterface(IID_PPV_ARGS(&swapChain3)));
        back_buffer_index = swapChain3->GetCurrentBackBufferIndex();
    
        while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        // reset the command objects
        //
        ThrowIfFailed(commandList->Reset(commandAllocator, shadowPipelineState));
    
        // indicate that the back buffer will be used as a render target
    
    
        CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(rtvHeap->GetCPUDescriptorHandleForHeapStart());
        rtvHandle.Offset(back_buffer_index, device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV));
    
        commandList->RSSetScissorRects(1, &scissorsRect);
        commandList->RSSetViewports(1, &opaqueViewPort);
        commandList->SetGraphicsRootSignature(rootSignature);
        commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    
        // Draw the shadow map
    
    
    
        float clear_color[4] = { 0.0f, 0.2f, 0.4f, 1.0f };
        
        // Set pipeline state to shadow Pipeline
        commandList->SetPipelineState(shadowPipelineState);
    
        // Bind the Depth Buffer (Not the RTV, we don't want to draw to the Frame Buffer, just store the depth pixels in the depth buffer)
        commandList->OMSetRenderTargets(0, nullptr, FALSE, &shadowDSVHeap->GetCPUDescriptorHandleForHeapStart());
        
        if (frame > 0)
            commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(
                shadowMapResource.Get(),
                D3D12_RESOURCE_STATE_GENERIC_READ,  // from
                D3D12_RESOURCE_STATE_DEPTH_WRITE));

        frame++;

        // Reset all depth values to 1.0f
        commandList->ClearDepthStencilView(shadowDSVHeap->GetCPUDescriptorHandleForHeapStart(), D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);
        commandList->SetDescriptorHeaps(1, &textureDescriptorHeap);
        commandList->SetGraphicsRootDescriptorTable(1, textureDescriptorHeap->GetGPUDescriptorHandleForHeapStart());
        commandList->SetGraphicsRootDescriptorTable(2, shadowDepthBufferTextureHandle);
        commandList->SetGraphicsRootDescriptorTable(3, shadowDepthBufferTextureHandle);

        // MAY NEED THIS
        // -------------------------- Draw Calls --------------------------
        // Draw calls will populate the depth buffer

        UINT16 cbvIndex = 0;

        // Shadow Draw Call #1
        commandList->IASetVertexBuffers(0, 1, &cube_vertex_buffer_view);
        update_shadow_object(XMFLOAT3(-1.0f, 2.f, 0.0f), +angle);
        CopyMemory(pCBVBytes[cbvIndex], &cBuffer, sizeof(cBuffer));
        commandList->SetGraphicsRootConstantBufferView(0, pCBVResource[cbvIndex]->GetGPUVirtualAddress());
        
        commandList->DrawInstanced(_countof(cube_verticies), 1, 0, 0);
    
        // Shadow Draw Call #2
        cbvIndex++;
        commandList->IASetVertexBuffers(0, 1, &cube_vertex_buffer_view);
        update_shadow_object(XMFLOAT3(+1.0f, 2.f, 0.0f), -angle);
        CopyMemory(pCBVBytes[cbvIndex], &cBuffer, sizeof(cBuffer));
        commandList->SetGraphicsRootConstantBufferView(0, pCBVResource[cbvIndex]->GetGPUVirtualAddress());
        commandList->DrawInstanced(_countof(cube_verticies), 1, 0, 0);
    
        // Shadow Draw Call #3
        cbvIndex++;
        commandList->IASetVertexBuffers(0, 1, &quad_vertex_buffer_view);
        commandList->IASetIndexBuffer(&quad_index_buffer_view);
        update_shadow_object(XMFLOAT3(0.0f, 0.0f, 0.0f), 0.0f, XMFLOAT3(7.0f, 1.0f, 7.0f));
        CopyMemory(pCBVBytes[cbvIndex], &cBuffer, sizeof(cBuffer));
        commandList->SetGraphicsRootConstantBufferView(0, pCBVResource[cbvIndex]->GetGPUVirtualAddress());
        commandList->DrawIndexedInstanced(_countof(quad_indicies_y), 1, 0, 0, 0);

        // Shadow Draw Call #4 - Reflective Shadow
        cbvIndex++;
        commandList->IASetVertexBuffers(0, 1, &cube_vertex_buffer_view);
        update_shadow_object(XMFLOAT3(-2.5f, 0.5f, 0.0f), 0.0f, XMFLOAT3(1.0f, 1.0f, 1.0f));
        CopyMemory(pCBVBytes[cbvIndex], &cBuffer, sizeof(cBuffer));
        commandList->SetGraphicsRootConstantBufferView(0, pCBVResource[cbvIndex]->GetGPUVirtualAddress());
        commandList->DrawInstanced(_countof(cube_verticies), 1, 0, 0);
    
        // Shadow Draw Call #3
    
        // Resource Barrier -> Depth Write to Shader Resource
        commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(
            shadowMapResource.Get(),
            D3D12_RESOURCE_STATE_DEPTH_WRITE,            // from
            D3D12_RESOURCE_STATE_GENERIC_READ)); // to
        
        // Resource Barrier
        commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(rtvResources[back_buffer_index], D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));

        // Now we are ready to clear the render target view
        commandList->OMSetRenderTargets(
            1, 
            &rtvHandle, 
            FALSE, 
            &depthDescriptorHeap->GetCPUDescriptorHandleForHeapStart()
        );

        // Clear RRender Target View
        // Make sure to pass in a CPU handle to the current Swap Chain Index
        commandList->ClearRenderTargetView(rtvHandle, clear_color, 0, nullptr);
        commandList->ClearDepthStencilView(depthDescriptorHeap->GetCPUDescriptorHandleForHeapStart(), D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);

        // ------------------------------------------------------ Draw Regular Scene ---------------------------------------------------------------------------------------------------------------------------------------------

        // Bind the Opaque PSO
        commandList->SetPipelineState(opaquePipelineState);
        commandList->IASetVertexBuffers(0, 1, &quad_vertex_buffer_view);
        commandList->IASetIndexBuffer(&quad_index_buffer_view);
        
        cbvIndex++;
        update_world_matrix(XMFLOAT3(0.0f, 0.0f, 0.0f), 0.0f, XMFLOAT3(7.0f, 1.0f, 7.0f));
        CopyMemory(pCBVBytes[cbvIndex], &cBuffer, sizeof(cBuffer));
        commandList->SetGraphicsRootConstantBufferView(0, pCBVResource[cbvIndex]->GetGPUVirtualAddress());
        commandList->DrawIndexedInstanced(_countof(quad_indicies_y), 1, 0, 0, 0);


        commandList->SetPipelineState(opaquePipelineStateStencilOn);


        //commandList->SetGraphicsRootDescriptorTable(2, shadowDepthStencilSRVHeap->GetGPUDescriptorHandleForHeapStart());

        CD3DX12_GPU_DESCRIPTOR_HANDLE stoneTextureGPUHandle(textureDescriptorHeap->GetGPUDescriptorHandleForHeapStart());
        stoneTextureGPUHandle.Offset(1, device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV));
        commandList->SetGraphicsRootDescriptorTable(1, stoneTextureGPUHandle);

        // Draw Call #2
        commandList->OMSetStencilRef(1);
        cbvIndex++;
        commandList->IASetVertexBuffers(0, 1, &cube_vertex_buffer_view);
        update_world_matrix(XMFLOAT3(-1.0f, 2.0f, 0.0f), +angle);
        CopyMemory(pCBVBytes[cbvIndex], &cBuffer, sizeof(cBuffer));
        commandList->SetGraphicsRootConstantBufferView(0, pCBVResource[cbvIndex]->GetGPUVirtualAddress());
        commandList->DrawInstanced(_countof(cube_verticies), 1, 0, 0);

        // Draw Call #3
        cbvIndex++;
        commandList->OMSetStencilRef(2);
        commandList->IASetVertexBuffers(0, 1, &cube_vertex_buffer_view);
        update_world_matrix(XMFLOAT3(+1.0f, 2.0f, 0.0f), -angle);
        CopyMemory(pCBVBytes[cbvIndex], &cBuffer, sizeof(cBuffer));
        commandList->SetGraphicsRootConstantBufferView(0, pCBVResource[cbvIndex]->GetGPUVirtualAddress());
        commandList->DrawInstanced(_countof(cube_verticies), 1, 0, 0);

        // Draw Call #4 - Reflective Object
        cbvIndex++;
        commandList->IASetVertexBuffers(0, 1, &cube_vertex_buffer_view);
        update_shadow_object(XMFLOAT3(-2.5f, 0.5f, 0.0f), 0.0f, XMFLOAT3(1.0f, 1.0f, 1.0f));
        CopyMemory(pCBVBytes[cbvIndex], &cBuffer, sizeof(cBuffer));
        commandList->SetGraphicsRootConstantBufferView(0, pCBVResource[cbvIndex]->GetGPUVirtualAddress());
        commandList->DrawInstanced(_countof(cube_verticies), 1, 0, 0);

        // -----------------------------------------------------------------
        // Draw Outlines
        commandList->SetPipelineState(outlineStencilPSOState);
        commandList->OMSetStencilRef(1);
        
        // Draw Call #4
        cbvIndex++;
        commandList->IASetVertexBuffers(0, 1, &cube_vertex_buffer_view);
        update_world_matrix(XMFLOAT3(-1.0f, 2.f, 0.0f), angle, XMFLOAT3(1.05f, 1.05f, 1.05f));
        CopyMemory(pCBVBytes[cbvIndex], &cBuffer, sizeof(cBuffer));
        commandList->SetGraphicsRootConstantBufferView(0, pCBVResource[cbvIndex]->GetGPUVirtualAddress());
        commandList->DrawInstanced(_countof(cube_verticies), 1, 0, 0);

        commandList->OMSetStencilRef(2);

        // Draw Call #4
        cbvIndex++;
        commandList->IASetVertexBuffers(0, 1, &cube_vertex_buffer_view);
        update_world_matrix(XMFLOAT3(+1.0f, 2.f, 0.0f), -angle, XMFLOAT3(1.05f, 1.05f, 1.05f));
        CopyMemory(pCBVBytes[cbvIndex], &cBuffer, sizeof(cBuffer));
        commandList->SetGraphicsRootConstantBufferView(0, pCBVResource[cbvIndex]->GetGPUVirtualAddress());
        commandList->DrawInstanced(_countof(cube_verticies), 1, 0, 0);

        commandList->SetPipelineState(opaquePipelineState);

        // Draw Call #5 - Debug Light Position
        cbvIndex++;
        commandList->IASetVertexBuffers(0, 1, &cube_vertex_buffer_view);
        update_world_matrix(cBuffer.directionalLight.Position, 0.0f, XMFLOAT3(1.0f, 1.0f, 1.0f));
        CopyMemory(pCBVBytes[cbvIndex], &cBuffer, sizeof(cBuffer));
        commandList->SetGraphicsRootConstantBufferView(0, pCBVResource[cbvIndex]->GetGPUVirtualAddress());
        commandList->DrawInstanced(_countof(cube_verticies), 1, 0, 0);


        // ------------------------------------------------------ Draw Quad --------------------------------------------------------

        // Bind the SRV pointer to the depth buffer
    
        // Switch to our Shadow VS and PS
        commandList->SetPipelineState(depthPipelineState);
    
        // Set the Vertex Buffer & Index Buffer views
        commandList->IASetVertexBuffers(0, 1, &screenspace_vertex_buffer_view);
        commandList->IASetIndexBuffer(&screenspace_index_buffer_view);           
        commandList->RSSetScissorRects(1, &scissorsRect);
        commandList->RSSetViewports(1, &debugViewPort);
    
        // Draw the quad that's used to show the shadow map
        cbvIndex++;
        update_world_matrix(XMFLOAT3(0.0f, 0.0f, 0.0f), 0.0f, XMFLOAT3(1.0f, 1.0f, 1.0f));
        CopyMemory(pCBVBytes[cbvIndex], &cBuffer, sizeof(cBuffer));
        commandList->SetGraphicsRootConstantBufferView(0, pCBVResource[cbvIndex]->GetGPUVirtualAddress());
        commandList->DrawInstanced(6, 1, 0, 0);

        commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(
            rtvResources[back_buffer_index],
            D3D12_RESOURCE_STATE_RENDER_TARGET,  // from
            D3D12_RESOURCE_STATE_PRESENT)
        );
    
        ThrowIfFailed(commandList->Close());
    
        ID3D12CommandList* cmdLists[] = { commandList };
        commandQueue->ExecuteCommandLists(_countof(cmdLists), cmdLists);
    
        // Present the current back buffer to the screen
        ThrowIfFailed(swapChain->Present(1, 0));
        //continue;
    
        // Sync the CPU/GPU
        ThrowIfFailed(commandQueue->Signal(fence, fenceValue));
        if (fence->GetCompletedValue() < fenceValue)
        {
            fence->SetEventOnCompletion(fenceValue, fenceEvt);
            WaitForSingleObject(fenceEvt, INFINITE);
        }
        fenceValue++;
        ThrowIfFailed(commandAllocator->Reset());
    }

    return (int)msg.wParam;
}



//
//  FUNCTION: MyRegisterClass()
//
//  PURPOSE: Registers the window class.
//
ATOM MyRegisterClass(HINSTANCE hInstance)
{
    WNDCLASSEXW wcex;

    wcex.cbSize = sizeof(WNDCLASSEX);

    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = WndProc;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = 0;
    wcex.hInstance = hInstance;
    wcex.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_DIRECTX12PROJ));
    wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wcex.lpszMenuName = MAKEINTRESOURCEW(IDC_DIRECTX12PROJ);
    wcex.lpszClassName = szWindowClass;
    wcex.hIconSm = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));
    //ss
    return RegisterClassExW(&wcex);
}

//
//   FUNCTION: InitInstance(HINSTANCE, int)
//
//   PURPOSE: Saves instance handle and creates main window
//
//   COMMENTS:
//
//        In this function, we save the instance handle in a global variable and
//        create and display the main program window.
//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
    hInst = hInstance; // Store instance handle in our global variable

    hWnd = CreateWindowW(szWindowClass, L"D3D12 Shadow Mapping", WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, 0, g_iWidth, g_iHeight, nullptr, nullptr, hInstance, nullptr);

    ShowWindow(hWnd, nCmdShow);
    //UpdateWindow(hWnd);

    return TRUE;
}

//
//  FUNCTION: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  PURPOSE: Processes messages for the main window.
//
//  WM_COMMAND  - process the application menu
//  WM_PAINT    - Paint the main window
//  WM_DESTROY  - post a quit message and return
//
//


// Message handler for about box.
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    switch (message)
    {
    case WM_INITDIALOG:
        return (INT_PTR)TRUE;

    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
        {
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        }
        break;
    }
    return (INT_PTR)FALSE;
}

/*
// Draw Regular scene
        if (0)
        {
            commandList->SetPipelineState(opaquePipelineState);


            commandList->ClearRenderTargetView(rtvHandle, clear_color, 0, nullptr);

            // get the current back buffer resource heap, and set it to the render target
            commandList->OMSetRenderTargets(1, nullptr, FALSE, &depthDescriptorHeap->GetCPUDescriptorHandleForHeapStart());
            commandList->ClearDepthStencilView(depthDescriptorHeap->GetCPUDescriptorHandleForHeapStart(), D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);



            CD3DX12_GPU_DESCRIPTOR_HANDLE texHeapStartGPU(textureDescriptorHeap->GetGPUDescriptorHandleForHeapStart());
            texHeapStartGPU.Offset(1, device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV));
            commandList->SetGraphicsRootDescriptorTable(1, texHeapStartGPU);

            
            // Shadow Draw Call #1
            commandList->IASetVertexBuffers(0, 1, &cube_vertex_buffer_view);
            update_shadow_object(XMFLOAT3(-1.0f, 3.0f, 0.0f), -angle);
            CopyMemory(pCBVBytes[0], &cBuffer, sizeof(cBuffer));
            commandList->SetGraphicsRootConstantBufferView(0, pCBVResource[0]->GetGPUVirtualAddress());
            commandList->DrawInstanced(_countof(cube_verticies), 1, 0, 0);

            // Shadow Draw Call #2
            commandList->IASetVertexBuffers(0, 1, &cube_vertex_buffer_view);
            update_shadow_object(XMFLOAT3(+1.0f, 3.0f, 0.0f), -angle);
            CopyMemory(pCBVBytes[1], &cBuffer, sizeof(cBuffer));
            commandList->SetGraphicsRootConstantBufferView(0, pCBVResource[1]->GetGPUVirtualAddress());
            commandList->DrawInstanced(_countof(cube_verticies), 1, 0, 0);

            // Shadow Draw Call #3
            commandList->IASetVertexBuffers(0, 1, &quad_vertex_buffer_view);
            commandList->IASetIndexBuffer(&quad_index_buffer_view);
            update_world_matrix(XMFLOAT3(0.0f, 2.0f, 0.0f), 0.0f, XMFLOAT3(7.0f, 1.0f, 7.0f));
            CopyMemory(pCBVBytes[2], &cBuffer, sizeof(cBuffer));
            commandList->SetGraphicsRootConstantBufferView(0, pCBVResource[2]->GetGPUVirtualAddress());
            commandList->DrawIndexedInstanced(_countof(quad_indicies_y), 1, 0, 0, 0);
            

            // ----------------------------------------------------------------------------------------------------------------------------------------------------
            // Draw Regular Scene

            // Draw Call #1
commandList->IASetVertexBuffers(0, 1, &cube_vertex_buffer_view);
update_world_matrix(XMFLOAT3(-1.0f, 1.0f, 0.0f), angle);
CopyMemory(pCBVBytes[3], &cBuffer, sizeof(cBuffer));
commandList->SetGraphicsRootConstantBufferView(0, pCBVResource[3]->GetGPUVirtualAddress());
commandList->DrawInstanced(_countof(cube_verticies), 1, 0, 0);

// Draw Call #2
commandList->IASetVertexBuffers(0, 1, &cube_vertex_buffer_view);
update_world_matrix(XMFLOAT3(+1.0f, 1.0f, 0.0f), -angle);
CopyMemory(pCBVBytes[4], &cBuffer, sizeof(cBuffer));
commandList->SetGraphicsRootConstantBufferView(0, pCBVResource[4]->GetGPUVirtualAddress());
commandList->DrawInstanced(_countof(cube_verticies), 1, 0, 0);

texHeapStartGPU.Offset(-1, device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV));


CD3DX12_GPU_DESCRIPTOR_HANDLE depthHeapStartGPU(textureDescriptorHeap->GetGPUDescriptorHandleForHeapStart());
depthHeapStartGPU.Offset(0, device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV));

commandList->SetGraphicsRootDescriptorTable(1, depthHeapStartGPU);

// Draw Call #3

// Swap Vertex buffers and Index buffers, we are drawing a quad:
commandList->IASetVertexBuffers(0, 1, &quad_vertex_buffer_view);
commandList->IASetIndexBuffer(&quad_index_buffer_view);

// Draw the quad off in center
update_world_matrix(XMFLOAT3(0.0f, 0.0f, 0.0f), 0.0f, XMFLOAT3(7.0f, 1.0f, 7.0f));
CopyMemory(pCBVBytes[5], &cBuffer, sizeof(cBuffer));
commandList->SetGraphicsRootConstantBufferView(0, pCBVResource[5]->GetGPUVirtualAddress());
commandList->DrawIndexedInstanced(_countof(quad_indicies_y), 1, 0, 0, 0);


// Draw the quad off to the right
//update_world_matrix(XMFLOAT3(0.0f, 0.0f, 0.0f), 0.0f, XMFLOAT3(7.0f, 1.0f, 7.0f));
//CopyMemory(pCBVBytes[5], &cBuffer, sizeof(cBuffer));
//commandList->SetGraphicsRootConstantBufferView(0, pCBVResource[5]->GetGPUVirtualAddress());
//commandList->DrawIndexedInstanced(_countof(quad_indicies_y), 1, 0, 0, 0);
        }
*/