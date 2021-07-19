#pragma once

#include <Graphics/GraphicsEngine/interface/GraphicsTypes.h>
#include <Graphics/GraphicsEngine/interface/EngineFactory.h>
#include "Graphics/GraphicsEngine/interface/RenderDevice.h"
#include "Graphics/GraphicsEngine/interface/DeviceContext.h"

#include <openxr/openxr.h>

#include <memory>

#define FETCH_AND_DEFINE_XR_FUNCTION( instance, name ) \
    PFN_ ## name name = nullptr; \
    xrGetInstanceProcAddr( instance, #name, (PFN_xrVoidFunction *)&name );


class IGraphicsBinding
{
public:
	virtual ~IGraphicsBinding() {}
	
	static std::unique_ptr<IGraphicsBinding> CreateBindingForDeviceType( Diligent::RENDER_DEVICE_TYPE deviceType,
		XrInstance instance, XrSystemId systemId );

	virtual Diligent::IEngineFactory* GetEngineFactory() = 0;
	virtual Diligent::IRenderDevice* GetRenderDevice() = 0;
	virtual Diligent::IDeviceContext* GetImmediateContext() = 0;

protected:
	virtual XrResult Init( XrInstance instance, XrSystemId systemId ) = 0;
};
