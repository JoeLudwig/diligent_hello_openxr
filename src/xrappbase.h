#pragma once

#include <memory>
#include <vector>
#include <string>

#ifndef PLATFORM_WIN32
#	define PLATFORM_WIN32 1
#endif

#ifndef ENGINE_DLL
#	define ENGINE_DLL 1
#endif

#ifndef D3D11_SUPPORTED
#	define D3D11_SUPPORTED 1
#endif

#ifndef D3D12_SUPPORTED
#	define D3D12_SUPPORTED 1
#endif

#ifndef GL_SUPPORTED
#	define GL_SUPPORTED 1
#endif

#ifndef VULKAN_SUPPORTED
#	define VULKAN_SUPPORTED 1
#endif

#include <Graphics/GraphicsEngine/interface/EngineFactory.h>
#include "Common/interface/BasicMath.hpp"
#include "Common/interface/Timer.hpp"

#include "Graphics/GraphicsEngine/interface/RenderDevice.h"
#include "Graphics/GraphicsEngine/interface/DeviceContext.h"
#include "Graphics/GraphicsEngine/interface/SwapChain.h"

#include "Common/interface/RefCntAutoPtr.hpp"

#include "openxr/openxr.h"


#include <openxr/openxr_platform.h>
#include "graphics_utilities.h"
#include "igraphicsbinding.h"

#include "iapp.h"

#define CHECK_XR_RESULT( res ) \
	do { \
		if( FAILED( res ) ) \
			return false; \
	} \
	while ( 0 );


class XrAppBase : public IApp
{
public:
	XrAppBase()
	{
	}

	virtual ~XrAppBase();
	virtual std::string GetWindowName() override;
	virtual bool Initialize( HWND hWnd ) override;
	bool InitializeOpenXr();
	bool CreateSession();
	virtual bool ProcessCommandLine( const std::string& cmdLine ) override;
	virtual void RunMainFrame() override;
	virtual bool PreSession() {	return true; }
	virtual bool PostSession() { return true; }

	virtual void Render() = 0;
	virtual void Update( double currTime, double elapsedTime, XrTime displayTime ) = 0;

	void Present();

	virtual void WindowResize( uint32_t Width, uint32_t Height ) override;

	Diligent::RENDER_DEVICE_TYPE GetDeviceType() const { return m_DeviceType; }

	void ProcessOpenXrEvents();
	bool ShouldRender() const;
	bool ShouldWait() const;

	bool RunXrFrame( XrTime *displayTime );
	virtual bool RenderEye( const XrView& view, Diligent::ITextureView* eyeBuffer, Diligent::ITextureView* depthBuffer ) = 0;

protected:
	Diligent::float4x4							  m_ViewToProj;

	Diligent::RefCntAutoPtr<Diligent::ISwapChain>	 m_pSwapChain;
	std::vector< Diligent::RefCntAutoPtr<Diligent::ITextureView> >  m_rpEyeSwapchainViews[ 2 ];
	std::vector< Diligent::RefCntAutoPtr<Diligent::ITextureView> >  m_rpEyeDepthViews[ 2 ];
	Diligent::RENDER_DEVICE_TYPE			m_DeviceType = Diligent::RENDER_DEVICE_TYPE_D3D11;
	std::unique_ptr<IGraphicsBinding> m_pGraphicsBinding;

	XrInstance m_instance = XR_NULL_HANDLE;
	XrSystemId m_systemId = XR_NULL_SYSTEM_ID;
	XrSession m_session = XR_NULL_HANDLE;
	XrViewConfigurationView m_views[ 2 ] = {};
	XrSwapchain m_swapchain = XR_NULL_HANDLE;
	XrSwapchain m_depthSwapchain = XR_NULL_HANDLE;
	XrSpace m_stageSpace = XR_NULL_HANDLE;
	XrSessionState m_sessionState = XR_SESSION_STATE_UNKNOWN;

	Diligent::Timer m_frameTimer;
	double m_prevFrameTime = 0;
};
