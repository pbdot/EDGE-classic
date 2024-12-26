#pragma once

void R_InitD3D11(SDL_Window *window);

/* D3D11: get pointer to ID3D11Device object */
const void* sapp_d3d11_get_device(void);
/* D3D11: get pointer to ID3D11DeviceContext object */
const void* sapp_d3d11_get_device_context(void);
/* D3D11: get pointer to IDXGISwapChain object */
const void* sapp_d3d11_get_swap_chain(void);
/* D3D11: get pointer to ID3D11RenderTargetView object for rendering */
const void* sapp_d3d11_get_render_view(void);
/* D3D11: get pointer ID3D11RenderTargetView object for msaa-resolve (may return null) */
const void* sapp_d3d11_get_resolve_view(void);
/* D3D11: get pointer ID3D11DepthStencilView */
const void* sapp_d3d11_get_depth_stencil_view(void);

void sapp_d3d11_present(bool do_not_wait);
