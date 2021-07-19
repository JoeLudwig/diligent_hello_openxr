#pragma once

#include "igraphicsbinding.h"

#include "Common/interface/RefCntAutoPtr.hpp"
#include "Graphics/GraphicsEngineD3D11/interface/EngineFactoryD3D11.h"

class GraphicsBinding_D3D11 : public IGraphicsBinding
{
public:
	virtual Diligent::IEngineFactory* GetEngineFactory() override;
	virtual Diligent::IRenderDevice* GetRenderDevice() override { return m_pDevice.RawPtr(); }
	virtual Diligent::IDeviceContext* GetImmediateContext() override { return m_pImmediateContext.RawPtr(); }

protected:
	virtual XrResult Init( XrInstance instance, XrSystemId systemId ) override;

private:
	Diligent::RefCntAutoPtr<Diligent::IEngineFactoryD3D11>         m_pEngineFactory;
	Diligent::RefCntAutoPtr<Diligent::IRenderDevice>  m_pDevice;
	Diligent::RefCntAutoPtr<Diligent::IDeviceContext> m_pImmediateContext;

	XrInstance m_instance = XR_NULL_HANDLE;
	XrSystemId m_systemId;
};