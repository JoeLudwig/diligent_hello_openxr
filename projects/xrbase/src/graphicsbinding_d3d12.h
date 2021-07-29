#pragma once

#include "igraphicsbinding.h"

#include "Graphics/GraphicsEngineD3D12/interface/EngineFactoryD3D12.h"

namespace Diligent
{
	struct IRenderDeviceD3D12;
}

struct XrGraphicsBindingD3D12KHR;

class GraphicsBinding_D3D12 : public IGraphicsBinding
{
public:
	virtual ~GraphicsBinding_D3D12();

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
	Diligent::IRenderDeviceD3D12* GetD3D12Device() { return (Diligent::IRenderDeviceD3D12*)GetRenderDevice(); }

	Diligent::RefCntAutoPtr<Diligent::IEngineFactoryD3D12>         m_pEngineFactory;
	Diligent::RefCntAutoPtr<Diligent::IRenderDevice>  m_pDevice;
	Diligent::RefCntAutoPtr<Diligent::IDeviceContext> m_pImmediateContext;

	XrInstance m_instance = XR_NULL_HANDLE;
	XrSystemId m_systemId;

	XrGraphicsBindingD3D12KHR *m_d3d12Binding = nullptr;
};