#pragma once

#if D3D11_SUPPORTED
#include "igraphicsbinding.h"

#include "Graphics/GraphicsEngineD3D11/interface/EngineFactoryD3D11.h"

namespace Diligent
{
	struct IRenderDeviceD3D11;
}

struct XrGraphicsBindingD3D11KHR;

class GraphicsBinding_D3D11 : public IGraphicsBinding
{
public:
	virtual ~GraphicsBinding_D3D11();

	virtual std::vector<std::string> GetXrExtensions() override;
	virtual XrResult CreateDevice( XrInstance instance, XrSystemId systemId ) override;
	virtual Diligent::IEngineFactory* GetEngineFactory() override;
	virtual Diligent::IRenderDevice* GetRenderDevice() override { return m_pDevice.RawPtr(); }
	virtual Diligent::IDeviceContext* GetImmediateContext() override { return m_pImmediateContext.RawPtr(); }
	virtual std::vector<int64_t> GetRequestedColorFormats() override;
	virtual std::vector<int64_t> GetRequestedDepthFormats() override;
	virtual void* GetSessionBinding() override;
	virtual std::vector< Diligent::RefCntAutoPtr<Diligent::ITexture> > ReadImagesFromSwapchain( XrSwapchain swapchain ) override;


private:
	Diligent::IRenderDeviceD3D11* GetD3D11Device() { return (Diligent::IRenderDeviceD3D11*)GetRenderDevice(); }

	Diligent::RefCntAutoPtr<Diligent::IEngineFactoryD3D11>         m_pEngineFactory;
	Diligent::RefCntAutoPtr<Diligent::IRenderDevice>  m_pDevice;
	Diligent::RefCntAutoPtr<Diligent::IDeviceContext> m_pImmediateContext;

	XrInstance m_instance = XR_NULL_HANDLE;
	XrSystemId m_systemId;

	XrGraphicsBindingD3D11KHR *m_d3d11Binding = nullptr;
};
#endif

#if D3D11_SUPPORTED || D3D12_SUPPORTED
#include <dxgi.h>
UINT GetAdapterIndexFromLuid( LUID adapterId );
#endif