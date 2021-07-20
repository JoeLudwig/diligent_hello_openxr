#pragma once

#include <Graphics/GraphicsEngine/interface/GraphicsTypes.h>
#include <Graphics/GraphicsEngine/interface/EngineFactory.h>
#include <Graphics/GraphicsEngine/interface/RenderDevice.h>
#include <Graphics/GraphicsEngine/interface/DeviceContext.h>
#include <Common/interface/RefCntAutoPtr.hpp>

#include <openxr/openxr.h>

#include <memory>
#include <vector>
#include <string>

#define FETCH_AND_DEFINE_XR_FUNCTION( instance, name ) \
    PFN_ ## name name = nullptr; \
    xrGetInstanceProcAddr( instance, #name, (PFN_xrVoidFunction *)&name );


class IGraphicsBinding
{
public:
	virtual ~IGraphicsBinding() {}
	
	static std::unique_ptr<IGraphicsBinding> CreateBindingForDeviceType( Diligent::RENDER_DEVICE_TYPE deviceType );

	virtual std::vector<std::string> GetXrExtensions() = 0;
	virtual XrResult CreateDevice( XrInstance instance, XrSystemId systemId ) = 0;
	virtual Diligent::IEngineFactory* GetEngineFactory() = 0;
	virtual Diligent::IRenderDevice* GetRenderDevice() = 0;
	virtual Diligent::IDeviceContext* GetImmediateContext() = 0;
	virtual std::vector<int64_t> GetRequestedColorFormats() = 0;
	virtual std::vector<int64_t> GetRequestedDepthFormats() = 0;
	virtual void* GetSessionBinding() = 0;
	virtual std::vector< Diligent::RefCntAutoPtr<Diligent::ITexture> > ReadImagesFromSwapchain( XrSwapchain swapchain ) = 0;
};
