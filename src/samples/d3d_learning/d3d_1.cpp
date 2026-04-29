#include "core/core_include.h"
#include "core/core_include.cpp"

#include "windows.h"
#pragma comment (lib, "user32.lib")

#include "d3d11.h"
#include "dxgi.h"
#include "dxgidebug.h"
#include "dxgi1_3.h"
#include "d3dcompiler.h"
#pragma comment (lib, "d3d11.lib")
#pragma comment (lib, "dxgi.lib")
#pragma comment (lib, "dxguid.lib")    // This is for the ids for the all the interfaces
#pragma comment (lib, "d3dcompiler.lib")

LRESULT win_proc(
  HWND window_handle,
  UINT message,
  WPARAM w_param,
  LPARAM l_param
) {
  LRESULT result = {};
  B32 matched_the_message = true;
  switch (message)
  {
    default: { result = DefWindowProc(window_handle, message, w_param, l_param); } break;
    
    case WM_SIZE: 
    {

    } break;

    case WM_CLOSE: 
    {
      DestroyWindow(window_handle);
    } break;

    case WM_DESTROY: 
    {
      PostQuitMessage(0);
    } break;
  }

  return result;
}

int WinMain(HINSTANCE app_instance, HINSTANCE __not_used__, LPSTR cmd, int show)
{
  WNDCLASSEXA wc = {};
  wc.cbSize        = sizeof(wc);
  wc.style         = 0;
  wc.lpfnWndProc   = win_proc;
  wc.hInstance     = app_instance;
  wc.hIcon         = Null;
  wc.hCursor       = Null;
  wc.hbrBackground = Null;
  wc.lpszMenuName  = Null;
  wc.lpszClassName = "d3d_1_window_class_name";
  wc.hIconSm       = Null;

  ATOM wc_atom = RegisterClassExA(&wc);
  Assert(wc_atom != 0);

  HWND window_handle = CreateWindowExA(
    0,
    wc.lpszClassName,
    "d3d_1",
    WS_OVERLAPPEDWINDOW,
    CW_USEDEFAULT, CW_USEDEFAULT,
    800, 600,
    Null,
    Null,
    app_instance, 
    Null
  );
  Assert(window_handle != 0);

  ShowWindow(window_handle, SW_SHOW);

  // == Done setting up the window

  HRESULT hr = S_OK;
  #define HR(hr_value) Assert(hr == S_OK)

  IDXGIFactory2* dxgi_factory = 0;
  hr = CreateDXGIFactory2(0, IID_IDXGIFactory2, (void**)&dxgi_factory);
  HR(hr);

  IDXGIAdapter* dxgi_adapter = 0;
  hr = dxgi_factory->EnumAdapters(0, &dxgi_adapter);
  HR(hr);

  D3D_FEATURE_LEVEL levels[] = { D3D_FEATURE_LEVEL_11_1 };

  ID3D11Device* d3d_device = 0;
  ID3D11DeviceContext* d3d_context = 0;
  hr = D3D11CreateDevice(
    dxgi_adapter, D3D_DRIVER_TYPE_UNKNOWN /*D3D_DRIVER_TYPE_HARDWARE*/, Null, 
    D3D11_CREATE_DEVICE_DEBUG, levels, ArrayCount(levels),
    D3D11_SDK_VERSION, &d3d_device, Null, &d3d_context
  );
  HR(hr);

  // Debug
  {
    // Debug for device
    ID3D11InfoQueue* debug_q = 0;
    hr = d3d_device->QueryInterface(IID_ID3D11InfoQueue, (void**)&debug_q);
    HR(hr);

    debug_q->SetBreakOnSeverity(D3D11_MESSAGE_SEVERITY_CORRUPTION, true);
    debug_q->SetBreakOnSeverity(D3D11_MESSAGE_SEVERITY_ERROR, true);
    debug_q->SetBreakOnSeverity(D3D11_MESSAGE_SEVERITY_WARNING, true);
    debug_q->Release();

    // Debug for dxgi
    IDXGIInfoQueue* dxgi_debug = 0;
    hr = DXGIGetDebugInterface1(0, IID_IDXGIInfoQueue, (void**)&dxgi_debug);
    HR(hr);
    dxgi_debug->SetBreakOnSeverity(DXGI_DEBUG_ALL, DXGI_INFO_QUEUE_MESSAGE_SEVERITY_CORRUPTION, true);
    dxgi_debug->SetBreakOnSeverity(DXGI_DEBUG_ALL, DXGI_INFO_QUEUE_MESSAGE_SEVERITY_ERROR, true);
    dxgi_debug->SetBreakOnSeverity(DXGI_DEBUG_ALL, DXGI_INFO_QUEUE_MESSAGE_SEVERITY_WARNING, true);
    dxgi_debug->Release();
  }

  IDXGISwapChain1* d3d_swap_chain = 0;
  {
    DXGI_SWAP_CHAIN_DESC1 desc = {};
    desc.Width       = 0;
    desc.Height      = 0;
    desc.Format      = DXGI_FORMAT_R8G8B8A8_UNORM;
    desc.Stereo      = FALSE;
    desc.SampleDesc  = { 1, 0 };
    desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    desc.BufferCount = 2;
    desc.Scaling     = DXGI_SCALING_NONE;
    desc.SwapEffect  = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    desc.AlphaMode   = DXGI_ALPHA_MODE_UNSPECIFIED;
    desc.Flags       = 0;
    hr = dxgi_factory->CreateSwapChainForHwnd(d3d_device, window_handle, &desc, Null, Null, &d3d_swap_chain);
    HR(hr);
  }

  struct Vertex {
    // F32 position[3];
    F32 color[4];
  };

  ID3D11InputLayout* input_layout = 0;
  ID3D11VertexShader* v_shader = 0;
  ID3D11PixelShader* p_shader = 0;
  {
    ID3DBlob* error_blob = 0;
    ID3DBlob* v_blob = 0;
    ID3DBlob* p_blob = 0;

    hr = D3DCompileFromFile(
      L"../src/samples/d3d_learning/shader.hlsl", Null, Null,
      "vs_main", "vs_5_0", Null, Null, &v_blob, &error_blob
    );
    if (error_blob != 0)
    {
      char* v_shader_error = (char*)error_blob->GetBufferPointer();
      OutputDebugStringA(v_shader_error);
      InvalidCodePath();
    }
    HR(hr);

    hr = D3DCompileFromFile(
      L"../src/samples/d3d_learning/shader.hlsl", Null, Null,
      "ps_main", "ps_5_0", Null, Null, &p_blob, &error_blob
    );
    if (error_blob != 0)
    {
      char* p_shader_error = (char*)error_blob->GetBufferPointer();
      OutputDebugStringA(p_shader_error);
      InvalidCodePath();
    }
    HR(hr);

    D3D11_INPUT_ELEMENT_DESC decss[] = 
    {
      // { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, offsetof(Vertex, position), D3D11_INPUT_PER_VERTEX_DATA, 0 },
      { "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, offsetof(Vertex, color), D3D11_INPUT_PER_VERTEX_DATA, 0 },
    };

    hr = d3d_device->CreateVertexShader(v_blob->GetBufferPointer(), v_blob->GetBufferSize(), Null, &v_shader);
    HR(hr);
    hr = d3d_device->CreatePixelShader(p_blob->GetBufferPointer(), p_blob->GetBufferSize(), Null, &p_shader);
    HR(hr);
    hr = d3d_device->CreateInputLayout(decss, ArrayCount(decss), v_blob->GetBufferPointer(), v_blob->GetBufferSize(), &input_layout);
    HR(hr);

    v_blob->Release();
    if (error_blob != 0) { error_blob->Release(); }
  }

  ID3D11RasterizerState* rasterizer_state = 0;
  {
    // disable culling
    D3D11_RASTERIZER_DESC desc = {};
    desc.FillMode = D3D11_FILL_SOLID;
    desc.CullMode = D3D11_CULL_NONE;
    desc.DepthClipEnable = true;
    d3d_device->CreateRasterizerState(&desc, &rasterizer_state);
  }

  ID3D11Buffer* vertex_buffer = 0;
  {
    Vertex buffer[] = 
    { 
      {1, 1, 1, 1},
      {0, 0, 0, 1},
      {1, 0, 1, 1},
      
      {0, 0, 0, 1},
      {1, 0, 1, 1},
      {0, 0, 0, 1},

      // { {-1.0f, +1.0f, 0.5f,}, {1, 1, 1, 1}  }, 
      // { {-1.0f, -1.0f, 0.5f,}, {0, 0, 0, 1}  }, 
      // { {+1.0f, +1.0f, 0.5f,}, {1, 0, 1, 1}  }, 
      // { {-1.0f, -1.0f, 0.5f,}, {0, 0, 0, 1}  }, 
      // { {+1.0f, +1.0f, 0.5f,}, {1, 0, 1, 1}  }, 
      // { {+1.0f, -1.0f, 0.5f,}, {0, 0, 0, 1}  }, 
    };

    D3D11_BUFFER_DESC desc = {};
    desc.ByteWidth      = sizeof(buffer);
    desc.Usage          = D3D11_USAGE_IMMUTABLE;
    desc.BindFlags      = D3D11_BIND_VERTEX_BUFFER;
    desc.CPUAccessFlags = 0;
    desc.MiscFlags      = 0;
    desc.StructureByteStride = sizeof(Vertex);

    D3D11_SUBRESOURCE_DATA sub = {};
    sub.pSysMem          = buffer;
    sub.SysMemPitch      = Null;
    sub.SysMemSlicePitch = Null;

    hr = d3d_device->CreateBuffer(&desc, &sub, &vertex_buffer);
    HR(hr);
  }

  ID3D11RenderTargetView* render_targer = 0;
  U32 w_w_prev = 0;
  U32 w_h_prev = 0;

  for (;;)
  {
    // Processing messages
    B32 stop_the_app = false;
    MSG msg;
    for (;PeekMessageW(&msg, NULL, 0, 0, PM_REMOVE);)
    {
      if (msg.message == WM_QUIT) { stop_the_app = true; break; }
      TranslateMessage(&msg);
      DispatchMessageA(&msg);
    }
    if (stop_the_app) { break; }

    RECT rect = {};
    GetClientRect(window_handle, &rect);
    U32 w_w = rect.right - rect.left;  
    U32 w_h = rect.bottom - rect.top;  

    if (w_w != w_w_prev || w_h != w_h_prev)
    {
      if (render_targer)
      {
        d3d_context->ClearState();
        render_targer->Release();
        render_targer = 0;
      }

      if (render_targer == 0 && w_w != 0 && w_h != 0)
      {
        hr = d3d_swap_chain->ResizeBuffers(0, w_w, w_h, DXGI_FORMAT_UNKNOWN, 0);
        HR(hr);
  
        ID3D11Texture2D* backbuffer = 0;
        d3d_swap_chain->GetBuffer(0, IID_ID3D11Texture2D, (void**)&backbuffer);
        d3d_device->CreateRenderTargetView((ID3D11Resource*)backbuffer, NULL, &render_targer);
        backbuffer->Release();
      }
    }

    if (render_targer)
    {
      d3d_context->IASetInputLayout(input_layout);
      d3d_context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST); // todo: What if i dont set this ?
      UINT stride = sizeof(Vertex);
      UINT offset = 0;
      // d3d_context->IASetVertexBuffers(0, 1, &vertex_buffer, &stride, &offset);

      d3d_context->VSSetShader(v_shader, NULL, 0);

      D3D11_VIEWPORT d3d_vp = {};
      d3d_vp.TopLeftX = 0;
      d3d_vp.TopLeftY = 0;
      d3d_vp.Width    = (F32)w_w;
      d3d_vp.Height   = (F32)w_h;
      d3d_vp.MinDepth = 0;
      d3d_vp.MaxDepth = 1;
      d3d_context->RSSetViewports(1, &d3d_vp);

      d3d_context->PSSetShader(p_shader, NULL, 0);

      d3d_context->OMSetRenderTargets(1, &render_targer, Null);
      d3d_context->RSSetState(rasterizer_state);

      F32 color[] = { 0.0f, 0.0f, 0.0f, 1.0f };
      d3d_context->ClearRenderTargetView(render_targer, color);
    
      d3d_context->Draw(9, 0);

      d3d_swap_chain->Present(0, 0);
    }

    w_w_prev = w_w;
    w_h_prev = w_h;
  }

  return 0;
}