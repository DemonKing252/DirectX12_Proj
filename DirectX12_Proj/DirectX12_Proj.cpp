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
    if (!InitInstance (hInstance, nCmdShow))
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

    swapchaindesc.Width = 1920;
    swapchaindesc.Height = 1080;
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

    // step 10: define our vertices
    struct Vertex
    {
        XMFLOAT3 Position;
        XMFLOAT3 Color;
        XMFLOAT2 UV;
        XMFLOAT3 Normal;
    };

    /*
        // vertex.x, vertex.y, color.r, color.g, color.b
        XMFLOAT2({-0.25f, -0.25f}), XMFLOAT3({  1.0f, 0.0f, 0.0f }),
        XMFLOAT2({0.0f, +0.25f, }), XMFLOAT3({ 0.0f, 1.0f, 0.0f, }),
        XMFLOAT2({+0.25f, -0.25f}), XMFLOAT3({  0.0f, 0.0f, 1.0f }),
    */

    Vertex cube_verticies[] = {

        // vertex.x, vertex.y, color.r, color.g, color.b
        /*0*/XMFLOAT3({-0.5f, -0.5f, -0.5f}), XMFLOAT3({ 1.0f, 0.0f, 1.0f }), XMFLOAT2(0.0f, 0.0f), XMFLOAT3(0.0f, 0.0f, -1.0f), // top left
        /*1*/XMFLOAT3({-0.5f, +0.5f, -0.5f}), XMFLOAT3({ 1.0f, 0.0f, 1.0f }), XMFLOAT2(1.0f, 0.0f), XMFLOAT3(0.0f, 0.0f, -1.0f), // top right
        /*2*/XMFLOAT3({+0.5f, +0.5f, -0.5f}), XMFLOAT3({ 1.0f, 0.0f, 1.0f }), XMFLOAT2(1.0f, 1.0f), XMFLOAT3(0.0f, 0.0f, -1.0f), // bottom right
        /*2*/XMFLOAT3({+0.5f, +0.5f, -0.5f}), XMFLOAT3({ 1.0f, 0.0f, 1.0f }), XMFLOAT2(1.0f, 1.0f), XMFLOAT3(0.0f, 0.0f, -1.0f), // bottom right
        /*3*/XMFLOAT3({+0.5f, -0.5f, -0.5f}), XMFLOAT3({ 1.0f, 0.0f, 1.0f }), XMFLOAT2(0.0f, 1.0f), XMFLOAT3(0.0f, 0.0f, -1.0f), // bottom left
        /*0*/XMFLOAT3({-0.5f, -0.5f, -0.5f}), XMFLOAT3({ 1.0f, 0.0f, 1.0f }), XMFLOAT2(0.0f, 0.0f), XMFLOAT3(0.0f, 0.0f, -1.0f), // top left    --------------------------------------

        /*0*/XMFLOAT3({-0.5f, -0.5f, +0.5f}), XMFLOAT3({ 1.0f, 0.0f, 1.0f }), XMFLOAT2(0.0f, 0.0f), XMFLOAT3(0.0f, 0.0f, +1.0f), // top left --------------------------------------
        /*2*/XMFLOAT3({+0.5f, +0.5f, +0.5f}), XMFLOAT3({ 1.0f, 0.0f, 1.0f }), XMFLOAT2(1.0f, 1.0f), XMFLOAT3(0.0f, 0.0f, +1.0f), // bottom right
        /*1*/XMFLOAT3({-0.5f, +0.5f, +0.5f}), XMFLOAT3({ 1.0f, 0.0f, 1.0f }), XMFLOAT2(1.0f, 0.0f), XMFLOAT3(0.0f, 0.0f, +1.0f), // top right
        /*2*/XMFLOAT3({+0.5f, +0.5f, +0.5f}), XMFLOAT3({ 1.0f, 0.0f, 1.0f }), XMFLOAT2(1.0f, 1.0f), XMFLOAT3(0.0f, 0.0f, +1.0f), // bottom right
        /*0*/XMFLOAT3({-0.5f, -0.5f, +0.5f}), XMFLOAT3({ 1.0f, 0.0f, 1.0f }), XMFLOAT2(0.0f, 0.0f), XMFLOAT3(0.0f, 0.0f, +1.0f), // top left
        /*3*/XMFLOAT3({+0.5f, -0.5f, +0.5f}), XMFLOAT3({ 1.0f, 0.0f, 1.0f }), XMFLOAT2(0.0f, 1.0f), XMFLOAT3(0.0f, 0.0f, +1.0f), // bottom left --------------------------------------


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



                                                                            
        ///*4*/XMFLOAT3({-0.5f, -0.5f, +0.5f}), XMFLOAT3({ 1.0f, 0.0f, 0.0f }), XMFLOAT2(0.0f, 0.0f), // top left
        ///*5*/XMFLOAT3({-0.5f, +0.5f, +0.5f}), XMFLOAT3({ 0.0f, 1.0f, 0.0f }), XMFLOAT2(0.0f, 0.0f), // top right
        ///*6*/XMFLOAT3({+0.5f, +0.5f, +0.5f}), XMFLOAT3({ 0.0f, 0.0f, 1.0f }), XMFLOAT2(0.0f, 0.0f), // bottom right
        ///*7*/XMFLOAT3({+0.5f, -0.5f, +0.5f}), XMFLOAT3({ 1.0f, 0.0f, 1.0f }), XMFLOAT2(0.0f, 0.0f), // bottom left

    };
    

    Vertex quad_verticies[] = {
        { XMFLOAT3(-0.5f, 0.0f, -0.5f), XMFLOAT3(1.0f, 0.0f, 1.0f), XMFLOAT2(0.0f, 0.0f), XMFLOAT3(0.0f, 1.0f, 0.0f) },
        { XMFLOAT3(-0.5f, 0.0f, +0.5f), XMFLOAT3(1.0f, 1.0f, 0.0f), XMFLOAT2(5.0f, 0.0f), XMFLOAT3(0.0f, 1.0f, 0.0f) },
        { XMFLOAT3(+0.5f, 0.0f, +0.5f), XMFLOAT3(1.0f, 1.0f, 1.0f), XMFLOAT2(5.0f, 6.0f), XMFLOAT3(0.0f, 1.0f, 0.0f) },
        { XMFLOAT3(+0.5f, 0.0f, -0.5f), XMFLOAT3(0.0f, 1.0f, 1.0f), XMFLOAT2(0.0f, 6.0f), XMFLOAT3(0.0f, 1.0f, 0.0f) },
    };
    UINT16 quad_indicies[] = {
        0, 1, 2,
        2, 3, 0
    };

    struct Light
    {
        XMFLOAT3 Direction;
        float pad1;
        XMFLOAT3 Strength;
        float pad2;
    };

    struct ConstantBuffer
    {
        DirectX::XMMATRIX Model;
        DirectX::XMMATRIX World;
        Light directionalLight;
    };
    XMMATRIX view, model, proj;

    // Sun light
    ConstantBuffer cBuffer;
    cBuffer.directionalLight.Direction = XMFLOAT3(1.0f, -0.3f, 0.0f);
    cBuffer.directionalLight.Strength = XMFLOAT3(1.0f, 1.0f, 0.0f);
    //cBuffer.directionalLight.FallOffStart = 0.1f;
    //cBuffer.directionalLight.FallOffEnd = 5.0f;

    auto update_world_matrix = [&](XMFLOAT3 Translate, float Rotation, XMFLOAT3 Scale = XMFLOAT3(1.0f, 1.0f, 1.0f))
    {
            // View Matrix
            XMVECTOR eyePos = XMVectorSet(0.0f, +3.0f, -6.0f, 0.0f);  // Camera position
            XMVECTOR focusPos = XMVectorSet(0.0f, 0.0f, 0.0f, 0.0f);   // Where camera looks
            XMVECTOR upDir = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);   // Up direction

            view = XMMatrixLookAtLH(eyePos, focusPos, upDir);

            // Projection Matrix
            float fovAngleY = XMConvertToRadians(45.0f); // 45 degree FOV vertical
            float aspectRatio = 1920.0f / 1080.0f;            // Width / Height of viewport
            float nearZ = 0.1f;
            float farZ = 100.0f;

            proj = XMMatrixPerspectiveFovLH(fovAngleY, aspectRatio, nearZ, farZ);

            XMMATRIX scale = XMMatrixScaling(Scale.x, Scale.y, Scale.z);
            XMMATRIX rotation = XMMatrixRotationRollPitchYaw(XMConvertToRadians(0.0f), XMConvertToRadians(Rotation), XMConvertToRadians(0.0f));
            XMMATRIX translate = XMMatrixTranslation(Translate.x, Translate.y, Translate.z);

            //model = translate;
            model = scale * rotation * translate;

            //cBuffer.Model = XMMatrixTranspose(XMMatrixInverse(nullptr, model));
            XMMATRIX Transpose = XMMatrixTranspose(rotation);

            cBuffer.Model = XMMatrixInverse(nullptr, Transpose);
            cBuffer.World = XMMatrixTranspose(model * view * proj);
    };
    

    // Constant buffer steps
    // step 1: create 2 resources
    ID3D12Resource* pCBVResource[3];
    void* pCBVBytes[3];

    for (UINT resource = 0; resource < 3; resource++)
    {
        // step 1:
        auto cbuffer_desc = CD3DX12_RESOURCE_DESC::Buffer(512U);
        auto cbuffer_heap_properties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);

        ThrowIfFailed(device->CreateCommittedResource(
            &cbuffer_heap_properties,
            D3D12_HEAP_FLAG_NONE,
            &cbuffer_desc,
            D3D12_RESOURCE_STATE_GENERIC_READ,
            nullptr,
            IID_PPV_ARGS(&pCBVResource[resource])
        ));

        ThrowIfFailed(pCBVResource[resource]->Map(0, nullptr, reinterpret_cast<void**>(&pCBVBytes[resource])));
        CopyMemory(pCBVBytes[resource], &cBuffer, 512U);
        pCBVResource[resource]->Unmap(0, nullptr);
    }

    auto create_vertex_buffer_view = [&device](Vertex *verticies, std::size_t vertexSize) -> D3D12_VERTEX_BUFFER_VIEW
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
    D3D12_RESOURCE_DESC depthDesc;
    ZeroMemory(&depthDesc, sizeof(D3D12_RESOURCE_DESC));

    depthDesc.Format = DXGI_FORMAT_D32_FLOAT;
    depthDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    depthDesc.Width = 1920;
    depthDesc.Height = 1080;
    depthDesc.MipLevels = 1;
    depthDesc.DepthOrArraySize = 1;
    depthDesc.SampleDesc.Count = 1;
    depthDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

    D3D12_CLEAR_VALUE clearValue;
    ZeroMemory(&clearValue, sizeof(D3D12_CLEAR_VALUE));

    clearValue.Format = DXGI_FORMAT_D32_FLOAT;
    clearValue.DepthStencil.Depth = 1.0f;

    auto heapProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
    ThrowIfFailed(device->CreateCommittedResource(
        &heapProps,
        D3D12_HEAP_FLAG_NONE,
        &depthDesc,
        D3D12_RESOURCE_STATE_DEPTH_WRITE,
        &clearValue,
        IID_PPV_ARGS(&depthBufferResource)
    ));
    
    D3D12_DEPTH_STENCIL_VIEW_DESC depthStenvilViewDesc;
    ZeroMemory(&depthStenvilViewDesc, sizeof(D3D12_DEPTH_STENCIL_VIEW_DESC));

    depthStenvilViewDesc.Format = DXGI_FORMAT_D32_FLOAT;
    depthStenvilViewDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
    depthStenvilViewDesc.Flags = D3D12_DSV_FLAG_NONE;

    ID3D12DescriptorHeap* depthDescriptorHeap;
    D3D12_DESCRIPTOR_HEAP_DESC depthdescHeapDesc;
    ZeroMemory(&depthdescHeapDesc, sizeof(D3D12_DESCRIPTOR_HEAP_DESC));

    depthdescHeapDesc.NumDescriptors = 1;
    depthdescHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
    
    ThrowIfFailed(device->CreateDescriptorHeap(&depthdescHeapDesc, IID_PPV_ARGS(&depthDescriptorHeap)));
    device->CreateDepthStencilView(depthBufferResource.Get(), &depthStenvilViewDesc, depthDescriptorHeap->GetCPUDescriptorHandleForHeapStart());

    D3D12_DEPTH_STENCIL_DESC depthStencilView;
    ZeroMemory(&depthStencilView, sizeof(D3D12_DEPTH_STENCIL_VIEW_DESC));

    depthStencilView.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
    depthStencilView.DepthFunc = D3D12_COMPARISON_FUNC_LESS;
    depthStencilView.DepthEnable = true;

    /***********************************************************************************************************************************************************/

    /***********************************************************************************************************************************************************/
    // Shader Resource View


    // step 1: load the texture into a resource and upload heap


    //ThrowIfFailed(commandAllocator->Reset());
    ThrowIfFailed(commandList->Reset(commandAllocator, nullptr));

    ID3D12DescriptorHeap* textureDescriptorHeap = nullptr;

    ComPtr<ID3D12Resource> checkboardResourceView = nullptr;
    ComPtr<ID3D12Resource> checkboardUploadHeap = nullptr;
    ComPtr<ID3D12Resource> stoneResourceView = nullptr;
    ComPtr<ID3D12Resource> stoneUploadHeap = nullptr;


    ThrowIfFailed(CreateDDSTextureFromFile12(device, commandList, L"checkboard.dds", checkboardResourceView, checkboardUploadHeap));
    ThrowIfFailed(CreateDDSTextureFromFile12(device, commandList, L"Stone.dds", stoneResourceView, stoneUploadHeap));

    // step 2: shader resource view:

    D3D12_DESCRIPTOR_HEAP_DESC srvDesc = { };
    srvDesc.NumDescriptors = 2;
    srvDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    srvDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;

    ThrowIfFailed(device->CreateDescriptorHeap(&srvDesc, IID_PPV_ARGS(&textureDescriptorHeap)));


    D3D12_SHADER_RESOURCE_VIEW_DESC shaderResourceViewDesc = { };
    shaderResourceViewDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    shaderResourceViewDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    shaderResourceViewDesc.Format = checkboardResourceView->GetDesc().Format;
    shaderResourceViewDesc.Texture2D.MipLevels = checkboardResourceView->GetDesc().MipLevels;

    device->CreateShaderResourceView(checkboardResourceView.Get(), &shaderResourceViewDesc, textureDescriptorHeap->GetCPUDescriptorHandleForHeapStart());


    shaderResourceViewDesc.Format = stoneResourceView->GetDesc().Format;
    shaderResourceViewDesc.Texture2D.MipLevels = stoneUploadHeap->GetDesc().MipLevels;

    // Next descriptor
    CD3DX12_CPU_DESCRIPTOR_HANDLE texHeapStartCPU(textureDescriptorHeap->GetCPUDescriptorHandleForHeapStart());
    texHeapStartCPU.Offset(1, device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV));

    device->CreateShaderResourceView(stoneResourceView.Get(), &shaderResourceViewDesc, texHeapStartCPU);

    
    // step 3: sampler state:
    CD3DX12_STATIC_SAMPLER_DESC samplerState;
    samplerState.Init(
        0, // base register
        D3D12_FILTER_MIN_MAG_MIP_POINT,
        D3D12_TEXTURE_ADDRESS_MODE_WRAP,
        D3D12_TEXTURE_ADDRESS_MODE_WRAP,
        D3D12_TEXTURE_ADDRESS_MODE_WRAP
    );

    ThrowIfFailed(commandList->Close());

    ID3D12CommandList* cmdLists[] = { commandList };
    commandQueue->ExecuteCommandLists(_countof(cmdLists), cmdLists);
    /***********************************************************************************************************************************************************/


    D3D12_VERTEX_BUFFER_VIEW cube_vertex_buffer_view = create_vertex_buffer_view(cube_verticies, sizeof(cube_verticies));

    D3D12_VERTEX_BUFFER_VIEW quad_vertex_buffer_view = create_vertex_buffer_view(quad_verticies, sizeof(quad_verticies));
    D3D12_INDEX_BUFFER_VIEW quad_index_buffer_view = create_index_buffer_view(quad_indicies, sizeof(quad_indicies));

    // step 12 - root signature (empty for now, we dont have any Constant Buffers or Shader Resources)
    ID3DBlob* signature;
    ID3DBlob* error;
    ID3D12RootSignature* rootSignature;
    CD3DX12_ROOT_SIGNATURE_DESC root_sig_desc;
    ZeroMemory(&root_sig_desc, sizeof(CD3DX12_ROOT_SIGNATURE_DESC));

    // srv desciptor range:
    D3D12_DESCRIPTOR_RANGE srvRange = {};
    srvRange.BaseShaderRegister = 0; // base register
    srvRange.NumDescriptors = 1;
    srvRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
    srvRange.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

    CD3DX12_ROOT_PARAMETER rootParameters[2];
    rootParameters[0].InitAsConstantBufferView(0, 0, D3D12_SHADER_VISIBILITY_ALL);
    rootParameters[1].InitAsDescriptorTable(1, &srvRange, D3D12_SHADER_VISIBILITY_ALL);

    root_sig_desc.Init(_countof(rootParameters), rootParameters, 1, &samplerState, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);
    ThrowIfFailed(D3D12SerializeRootSignature(&root_sig_desc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &error));
    ThrowIfFailed(device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&rootSignature)));


    // step 13 - compile shaders
    ComPtr<ID3DBlob> vs;
    ComPtr<ID3DBlob> ps;
    ComPtr<ID3DBlob> errors;

    HRESULT hr;

    hr = D3DCompileFromFile(L"VS.hlsl", nullptr, nullptr, "VSMain", "vs_5_0", D3DCOMPILE_DEBUG, 0, &vs, &errors);
    if (FAILED(hr))
    {
        OutputDebugStringA((char*)errors->GetBufferPointer());
        __debugbreak();
    }
    
    hr = D3DCompileFromFile(L"PS.hlsl", nullptr, nullptr, "PSMain", "ps_5_0", D3DCOMPILE_DEBUG, 0, &ps, &errors);
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
        { "NORMAL", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 32U, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
    };

    // step 15 - pipeline state object
    D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc;
    ZeroMemory(&psoDesc, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));
    ID3D12PipelineState* pipelineState;

    psoDesc.pRootSignature = rootSignature;
    psoDesc.InputLayout = { inputLayout, _countof(inputLayout) };
    psoDesc.VS = { vs->GetBufferPointer(), vs->GetBufferSize() };
    psoDesc.PS = { ps->GetBufferPointer(), ps->GetBufferSize() };
    psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
    psoDesc.SampleMask = UINT_MAX;
    psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    psoDesc.SampleDesc.Count = 1;
    psoDesc.NumRenderTargets = 1;
    psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
    psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
    psoDesc.DepthStencilState = depthStencilView;

    ThrowIfFailed(device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&pipelineState)));

    D3D12_VIEWPORT viewport;
    ZeroMemory(&viewport, sizeof(D3D12_VIEWPORT));

    viewport.Width = 1920;
    viewport.Height = 1080;
    viewport.MinDepth = 0.0f;
    viewport.MaxDepth = 1.0f;

    D3D12_RECT scissorsRect;
    ZeroMemory(&scissorsRect, sizeof(D3D12_RECT));

    scissorsRect.right = 1920;
    scissorsRect.bottom = 1080;


    //swapchaindesc.
    UINT64 frameIndex = 0;
    UINT64 fenceValue = 0;
    IDXGISwapChain3* swapChain3 = nullptr;
    float angle = 0.f;
    // Main message loop:
    while (msg.message != WM_QUIT)
    {
        angle += 0.2f;

        ThrowIfFailed(swapChain->QueryInterface(IID_PPV_ARGS(&swapChain3)));
        frameIndex = swapChain3->GetCurrentBackBufferIndex();

        //frameIndex = (frameIndex + 1) % 3;
        if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        // reset the command objects
        ThrowIfFailed(commandAllocator->Reset());
        ThrowIfFailed(commandList->Reset(commandAllocator, nullptr));

        // indicate that the back buffer will be used as a render target
        auto present_to_target = CD3DX12_RESOURCE_BARRIER::Transition(rtvResources[frameIndex], D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
        commandList->ResourceBarrier(1, &present_to_target);

        // get the current back buffer resource heap, and set it to the render target
        CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(rtvHeap->GetCPUDescriptorHandleForHeapStart());
        rtvHandle.Offset(frameIndex, device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV));

        auto depth_heap_start = depthDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
        commandList->OMSetRenderTargets(1, &rtvHandle, FALSE, &depth_heap_start);


        // finally clear render target view
        float clear_color[4] = { 0.0f, 0.2f, 0.4f, 1.0f };
        commandList->ClearRenderTargetView(rtvHandle, clear_color, 0, nullptr);

        
        auto target_to_present = CD3DX12_RESOURCE_BARRIER::Transition(rtvResources[frameIndex], D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
        commandList->ResourceBarrier(1, &target_to_present);

        commandList->ClearDepthStencilView(depth_heap_start, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

        commandList->RSSetScissorRects(1, &scissorsRect);
        commandList->RSSetViewports(1, &viewport);

        commandList->SetPipelineState(pipelineState);
        commandList->SetGraphicsRootSignature(rootSignature);

        commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

        commandList->IASetVertexBuffers(0, 1, &cube_vertex_buffer_view);
        commandList->SetDescriptorHeaps(1, &textureDescriptorHeap);


        CD3DX12_GPU_DESCRIPTOR_HANDLE texHeapStartGPU(textureDescriptorHeap->GetGPUDescriptorHandleForHeapStart());
        texHeapStartGPU.Offset(1, device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV));
        commandList->SetGraphicsRootDescriptorTable(1, texHeapStartGPU);

        // Draw Call #1
        update_world_matrix(XMFLOAT3(-1.0f, 0.0f, 0.0f), angle);
        CopyMemory(pCBVBytes[0], &cBuffer, sizeof(cBuffer));
        commandList->SetGraphicsRootConstantBufferView(0, pCBVResource[0]->GetGPUVirtualAddress());
        commandList->DrawInstanced(_countof(cube_verticies), 1, 0, 0);

        // Draw Call #2
        update_world_matrix(XMFLOAT3(+1.0f, 0.0f, 0.0f), -angle);
        CopyMemory(pCBVBytes[1], &cBuffer, sizeof(cBuffer));
        commandList->SetGraphicsRootConstantBufferView(0, pCBVResource[1]->GetGPUVirtualAddress());
        commandList->DrawInstanced(_countof(cube_verticies), 1, 0, 0);


        texHeapStartGPU.Offset(-1, device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV));
        commandList->SetGraphicsRootDescriptorTable(1, texHeapStartGPU);

        // Draw Call #3

        // Swap Vertex buffers and Index buffers, we are drawing a quad:
        commandList->IASetVertexBuffers(0, 1, &quad_vertex_buffer_view);
        commandList->IASetIndexBuffer(&quad_index_buffer_view);

        update_world_matrix(XMFLOAT3(0.0f, -2.0f, +2.0f), 0.0f, XMFLOAT3(6.0f, 1.0f, 5.0f));
        CopyMemory(pCBVBytes[2], &cBuffer, sizeof(cBuffer));
        commandList->SetGraphicsRootConstantBufferView(0, pCBVResource[2]->GetGPUVirtualAddress());
        commandList->DrawIndexedInstanced(_countof(quad_indicies), 1, 0, 0, 0);

        ThrowIfFailed(commandList->Close());

        ID3D12CommandList* cmdLists[] = { commandList };
        commandQueue->ExecuteCommandLists(_countof(cmdLists), cmdLists);

        ThrowIfFailed(swapChain->Present(1, 0));
        //continue;

        // Sync the CPU/GPU
        ThrowIfFailed( commandQueue->Signal(fence, fenceValue) );
        if (fence->GetCompletedValue() < fenceValue)
        {
            fence->SetEventOnCompletion(fenceValue, fenceEvt);
            WaitForSingleObject(fenceEvt, INFINITE);
        }
        fenceValue++;
    }

    return (int) msg.wParam;
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

    wcex.style          = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc    = WndProc;
    wcex.cbClsExtra     = 0;
    wcex.cbWndExtra     = 0;
    wcex.hInstance      = hInstance;
    wcex.hIcon          = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_DIRECTX12PROJ));
    wcex.hCursor        = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground  = (HBRUSH)(COLOR_WINDOW+1);
    wcex.lpszMenuName   = MAKEINTRESOURCEW(IDC_DIRECTX12PROJ);
    wcex.lpszClassName  = szWindowClass;
    wcex.hIconSm        = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));
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

   hWnd = CreateWindowW(szWindowClass, L"D3D12 App", WS_OVERLAPPEDWINDOW,
       CW_USEDEFAULT, 0, 1920, 1080, nullptr, nullptr, hInstance, nullptr);

   if (!hWnd)
   {
      return FALSE;
   }

   ShowWindow(hWnd, nCmdShow);
   UpdateWindow(hWnd);

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
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_KEYDOWN:
        if (wParam == VK_ESCAPE)
        {
            PostQuitMessage(0);
            return 0;

        }
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
