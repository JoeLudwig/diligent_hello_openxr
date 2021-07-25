#include "xrappbase.h"

#include <iomanip>
#include <iostream>
#include <vector>
#include <string>
#include <map>

#ifndef NOMINMAX
#	define NOMINMAX
#endif
#include <Windows.h>
#include <crtdbg.h>

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

#include "Graphics/GraphicsEngineD3D11/interface/EngineFactoryD3D11.h"
#include "Graphics/GraphicsEngineD3D12/interface/EngineFactoryD3D12.h"
#include "Graphics/GraphicsEngineOpenGL/interface/EngineFactoryOpenGL.h"
#include "Graphics/GraphicsEngineVulkan/interface/EngineFactoryVk.h"

#include "Graphics/GraphicsTools/interface/MapHelper.hpp"
#include <AssetLoader/interface/GLTFLoader.hpp>

 // Make sure the supported OpenXR graphics APIs are defined
#if D3D11_SUPPORTED
#include <d3d11.h>
#	define XR_USE_GRAPHICS_API_D3D11
#include "Graphics/GraphicsEngineD3D11/interface/RenderDeviceD3D11.h"
#endif

#if D3D12_SUPPORTED
#include <d3d12.h>
#	define XR_USE_GRAPHICS_API_D3D12
#include "Graphics/GraphicsEngineD3D12/interface/RenderDeviceD3D12.h"
#endif

#if GL_SUPPORTED
#	define XR_USE_GRAPHICS_API_OPENGL
#endif

#if VULKAN_SUPPORTED
#	define XR_USE_GRAPHICS_API_VULKAN
#include <vulkan/vulkan.h>
#endif

#include <openxr/openxr_platform.h>
#include "graphics_utilities.h"
#include "igraphicsbinding.h"

#include "iapp.h"
#include "paths.h"

using namespace Diligent;


typedef std::map< std::string, uint32_t > XrExtensionMap;

XrExtensionMap GetAvailableOpenXRExtensions()
{
	uint32_t neededSize = 0;
	if ( XR_FAILED( xrEnumerateInstanceExtensionProperties( nullptr, 0, &neededSize, nullptr ) ) )
	{
		return {};
	}

	if ( !neededSize )
	{
		// How can a runtime not enumerate at least one graphics binding extension?
		return {};
	}

	std::vector< XrExtensionProperties > properties;
	properties.resize( neededSize, { XR_TYPE_EXTENSION_PROPERTIES } );
	uint32_t readSize = 0;
	if ( XR_FAILED( xrEnumerateInstanceExtensionProperties( nullptr, neededSize, &readSize, &properties[ 0 ] ) ) )
	{
		return {};
	}

	XrExtensionMap res;
	for ( auto& prop : properties )
	{
		res.insert( std::make_pair( std::string( prop.extensionName ), prop.extensionVersion ) );
	}
	return res;
}

XrAppBase::~XrAppBase()
{
	if ( m_pGraphicsBinding )
	{
		m_pGraphicsBinding->GetImmediateContext()->Flush();
	}
}

std::string XrAppBase::GetWindowName() 
{
	std::string title( "Tutorial00: Hello Win32" );
	switch ( GetDeviceType() )
	{
	case RENDER_DEVICE_TYPE_D3D11: title.append( " (D3D11)" ); break;
	case RENDER_DEVICE_TYPE_D3D12: title.append( " (D3D12)" ); break;
	case RENDER_DEVICE_TYPE_GL: title.append( " (GL)" ); break;
	case RENDER_DEVICE_TYPE_VULKAN: title.append( " (VK)" ); break;
	}
	return title;
}

bool XrAppBase::Initialize( HWND hWnd )
{
	m_pGraphicsBinding = IGraphicsBinding::CreateBindingForDeviceType( m_DeviceType );
	if ( !m_pGraphicsBinding )
	{
		return false;
	}

	// create the OpenXR instance first because it will have an opinion about device creation
	if ( !InitializeOpenXr() )
	{
		return false;
	}

	if ( !XR_SUCCEEDED( m_pGraphicsBinding->CreateDevice( m_instance, m_systemId ) ) )
	{
		return false;
	}


	m_prevFrameTime = m_frameTimer.GetElapsedTime();

	Win32NativeWindow Window { hWnd };
	SwapChainDesc SCDesc;
	switch ( m_DeviceType )
	{
#if D3D11_SUPPORTED
	case RENDER_DEVICE_TYPE_D3D11:
	{
#	if ENGINE_DLL
		// Load the dll and import GetEngineFactoryD3D11() function
		auto* GetEngineFactoryD3D11 = LoadGraphicsEngineD3D11();
#	endif
		auto* pFactoryD3D11 = GetEngineFactoryD3D11();
		pFactoryD3D11->CreateSwapChainD3D11( m_pGraphicsBinding->GetRenderDevice(), m_pGraphicsBinding->GetImmediateContext(), SCDesc, FullScreenModeDesc {}, Window, &m_pSwapChain );
	}
	break;
#endif
#if D3D12_SUPPORTED
	case RENDER_DEVICE_TYPE_D3D12:
	{
#	if ENGINE_DLL
		// Load the dll and import GetEngineFactoryD3D11() function
		auto* GetEngineFactoryD3D12 = LoadGraphicsEngineD3D12();
#	endif
		auto* pFactoryD3D12 = GetEngineFactoryD3D12();
		pFactoryD3D12->CreateSwapChainD3D12( m_pGraphicsBinding->GetRenderDevice(), m_pGraphicsBinding->GetImmediateContext(), SCDesc, FullScreenModeDesc {}, Window, &m_pSwapChain );
	}
	break;
#endif

	//#if GL_SUPPORTED
	//			case RENDER_DEVICE_TYPE_GL:
	//			{
	//				FETCH_AND_DEFINE_XR_FUNCTION( m_instance, xrGetOpenGLGraphicsRequirementsKHR );
	//				// we don't have any way to specify the adapter in OpenGL, but we're required to get the graphics
	//				// requirements anyway
	//				XrGraphicsRequirementsOpenGLKHR graphicsRequirements = { XR_TYPE_GRAPHICS_REQUIREMENTS_OPENGL_KHR };
	//				XrResult res = xrGetOpenGLGraphicsRequirementsKHR( m_instance, m_systemId, &graphicsRequirements );
	//				if ( XR_FAILED( res ) )
	//				{
	//					return false;
	//				}
	//#	if EXPLICITLY_LOAD_ENGINE_GL_DLL
	//				// Load the dll and import GetEngineFactoryOpenGL() function
	//				auto GetEngineFactoryOpenGL = LoadGraphicsEngineOpenGL();
	//#	endif
	//				auto* pFactoryOpenGL = GetEngineFactoryOpenGL();
	//				m_pEngineFactory = pFactoryOpenGL;
	//
	//				EngineGLCreateInfo EngineCI;
	//				EngineCI.Window.hWnd = hWnd;
	//
	//				pFactoryOpenGL->CreateDeviceAndSwapChainGL(EngineCI, &m_pGraphicsBinding->GetRenderDevice(), &m_pGraphicsBinding->GetImmediateContext(), SCDesc, &m_pSwapChain);
	//			}
	//			break;
	//#endif
	//
	//
	//#if VULKAN_SUPPORTED
	//			case RENDER_DEVICE_TYPE_VULKAN:
	//			{
	//				// TODO: Vulkan requires that the runtime create the instance. That's going to require a change to 
	//				// CrateDeviceAndContextsVk, probably
	//#	if EXPLICITLY_LOAD_ENGINE_VK_DLL
	//				// Load the dll and import GetEngineFactoryVk() function
	//				auto GetEngineFactoryVk = LoadGraphicsEngineVk();
	//#	endif
	//				EngineVkCreateInfo EngineCI;
	//
	//				auto* pFactoryVk = GetEngineFactoryVk();
	//				m_pEngineFactory = pFactoryVk;
	//				pFactoryVk->CreateDeviceAndContextsVk(EngineCI, &m_pGraphicsBinding->GetRenderDevice(), &m_pGraphicsBinding->GetImmediateContext());
	//
	//				if (!m_pSwapChain && hWnd != nullptr)
	//				{
	//					Win32NativeWindow Window{hWnd};
	//					pFactoryVk->CreateSwapChainVk(m_pGraphicsBinding->GetRenderDevice(), m_pGraphicsBinding->GetImmediateContext(), SCDesc, Window, &m_pSwapChain);
	//				}
	//			}
	//			break;
	//#endif


	default:
		std::cerr << "Unknown/unsupported device type";
		return false;
		break;
	}

	CreateGLTFResourceCache();

	if ( !PreSession() )
		return false;

	if ( !CreateSession() )
		return false;

	if ( !PostSession() )
		return false;

	return true;
}


bool XrAppBase::InitializeOpenXr()
{
	XrExtensionMap availableExtensions = GetAvailableOpenXRExtensions();

	std::vector< std::string > xrExtensions = m_pGraphicsBinding->GetXrExtensions();

	//		switch ( m_DeviceType )
	//		{
	//#if D3D11_SUPPORTED
	//		case RENDER_DEVICE_TYPE_D3D11:
	//		{
	//			xrExtensions.push_back( XR_KHR_D3D11_ENABLE_EXTENSION_NAME );
	//		}
	//		break;
	//#endif
	//
	//
	//#if D3D12_SUPPORTED
	//		case RENDER_DEVICE_TYPE_D3D12:
	//		{
	//			xrExtensions.push_back( XR_KHR_D3D12_ENABLE_EXTENSION_NAME );
	//		}
	//		break;
	//#endif
	//
	//
	//#if GL_SUPPORTED
	//		case RENDER_DEVICE_TYPE_GL:
	//		{
	//			xrExtensions.push_back( XR_KHR_OPENGL_ENABLE_EXTENSION_NAME );
	//		}
	//		break;
	//#endif
	//
	//
	//#if VULKAN_SUPPORTED
	//		case RENDER_DEVICE_TYPE_VULKAN:
	//		{
	//			xrExtensions.push_back( XR_KHR_VULKAN_ENABLE2_EXTENSION_NAME );
	//		}
	//		break;
	//#endif
	//		}

	if ( xrExtensions.empty() )
	{
		// we can't create an instance without at least a graphics extension
		return false;
	}

	// CODE GOES HERE: Add any additional extensions required by your application

	std::vector<const char*> extensionPointers;
	for ( auto& ext : xrExtensions )
	{
		extensionPointers.push_back( ext.c_str() );
	}

	XrInstanceCreateInfo createInfo = { XR_TYPE_INSTANCE_CREATE_INFO };
	createInfo.enabledExtensionCount = (uint32_t)extensionPointers.size();
	createInfo.enabledExtensionNames = &extensionPointers[ 0 ];
	strcpy_s( createInfo.applicationInfo.applicationName, "Hello Diligent XR" );
	createInfo.applicationInfo.applicationVersion = 1;
	strcpy_s( createInfo.applicationInfo.engineName, "DiligentEngine" );
	createInfo.applicationInfo.engineVersion = DILIGENT_API_VERSION;
	createInfo.applicationInfo.apiVersion = XR_CURRENT_API_VERSION;

	XrResult res = xrCreateInstance( &createInfo, &m_instance );
	if ( XR_FAILED( res ) )
	{
		return false;
	}

	XRDE::InitPaths( m_instance );

	XrSystemGetInfo getInfo = { XR_TYPE_SYSTEM_GET_INFO };
	getInfo.formFactor = XR_FORM_FACTOR_HEAD_MOUNTED_DISPLAY;
	res = xrGetSystem( m_instance, &getInfo, &m_systemId );

	if ( XR_FAILED( res ) )
	{
		return false;
	}

	m_views[ 0 ].type = XR_TYPE_VIEW_CONFIGURATION_VIEW;
	m_views[ 1 ].type = XR_TYPE_VIEW_CONFIGURATION_VIEW;

	uint32_t viewCountOutput = 0;
	if ( XR_FAILED( xrEnumerateViewConfigurationViews( m_instance, m_systemId, XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO,
		2, &viewCountOutput, m_views ) ) )
	{
		return false;
	}

	return XR_SUCCEEDED( res );
}

bool XrAppBase::CreateSession()
{
	XrSessionCreateInfo createInfo = { XR_TYPE_SESSION_CREATE_INFO };
	createInfo.systemId = m_systemId;
	createInfo.createFlags = 0;

	std::vector< int64_t > requestedFormats = m_pGraphicsBinding->GetRequestedColorFormats();
	std::vector< int64_t > requestedDepthFormats = m_pGraphicsBinding->GetRequestedDepthFormats();
	createInfo.next = m_pGraphicsBinding->GetSessionBinding();
	CHECK_XR_RESULT( xrCreateSession( m_instance, &createInfo, &m_session ) );

	uint32_t swapchainFormatCount;
	CHECK_XR_RESULT( xrEnumerateSwapchainFormats( m_session, 0, &swapchainFormatCount, nullptr ) );
	std::vector<int64_t> supportedFormats;
	supportedFormats.resize( swapchainFormatCount );
	CHECK_XR_RESULT( xrEnumerateSwapchainFormats( m_session, swapchainFormatCount, &swapchainFormatCount, &supportedFormats[ 0 ] ) );
	if ( requestedFormats.empty() || supportedFormats.empty() )
		return false;

	XrSwapchainCreateInfo scCreateInfo = { XR_TYPE_SWAPCHAIN_CREATE_INFO };
	scCreateInfo.arraySize = 2;
	scCreateInfo.width = m_views[ 0 ].recommendedImageRectWidth;
	scCreateInfo.height = m_views[ 0 ].recommendedImageRectHeight;
	scCreateInfo.createFlags = 0;
	scCreateInfo.format = supportedFormats[ 0 ];
	scCreateInfo.mipCount = 1;
	scCreateInfo.sampleCount = 1;
	scCreateInfo.faceCount = 1;

	XrSwapchainCreateInfo depthCreateInfo = { XR_TYPE_SWAPCHAIN_CREATE_INFO };
	depthCreateInfo.arraySize = 2;
	depthCreateInfo.width = m_views[ 0 ].recommendedImageRectWidth;
	depthCreateInfo.height = m_views[ 0 ].recommendedImageRectHeight;
	depthCreateInfo.createFlags = 0;
	depthCreateInfo.usageFlags = XR_SWAPCHAIN_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
	depthCreateInfo.format = requestedDepthFormats[ 0 ];
	depthCreateInfo.mipCount = 1;
	depthCreateInfo.sampleCount = 1;
	depthCreateInfo.faceCount = 1;

	// find the format on our list that's earliest on the runtime's list
	for ( int64_t supported : supportedFormats )
	{
		if ( std::find( requestedFormats.begin(), requestedFormats.end(), supported )
			!= requestedFormats.end() )
		{
			scCreateInfo.format = supported;
			break;
		}
	}
	for ( int64_t supported : supportedFormats )
	{
		if ( std::find( requestedDepthFormats.begin(), requestedDepthFormats.end(), supported )
			!= requestedDepthFormats.end() )
		{
			depthCreateInfo.format = supported;
			break;
		}
	}

	CHECK_XR_RESULT( xrCreateSwapchain( m_session, &scCreateInfo, &m_swapchain ) );
	CHECK_XR_RESULT( xrCreateSwapchain( m_session, &depthCreateInfo, &m_depthSwapchain ) );

	uint32_t imageCount, depthImageCount;
	CHECK_XR_RESULT( xrEnumerateSwapchainImages( m_swapchain, 0, &imageCount, nullptr ) );
	CHECK_XR_RESULT( xrEnumerateSwapchainImages( m_depthSwapchain, 0, &depthImageCount, nullptr ) );

	std::vector< RefCntAutoPtr<ITexture> > textures = m_pGraphicsBinding->ReadImagesFromSwapchain( m_swapchain );
	for ( RefCntAutoPtr<ITexture>& pTexture : textures )
	{
		TextureViewDesc viewDesc;
		viewDesc.ViewType = TEXTURE_VIEW_RENDER_TARGET;
		viewDesc.FirstArraySlice = 0;
		viewDesc.NumArraySlices = 1;
		viewDesc.AccessFlags = UAV_ACCESS_FLAG_WRITE;

		RefCntAutoPtr< ITextureView > pLeftEyeView;
		pTexture->CreateView( viewDesc, &pLeftEyeView );
		m_rpEyeSwapchainViews[ 0 ].push_back( pLeftEyeView );

		viewDesc.FirstArraySlice = 1;
		RefCntAutoPtr< ITextureView > pRightEyeView;
		pTexture->CreateView( viewDesc, &pRightEyeView );
		m_rpEyeSwapchainViews[ 1 ].push_back( pRightEyeView );
	}

	std::vector< RefCntAutoPtr<ITexture> > depthTextures = m_pGraphicsBinding->ReadImagesFromSwapchain( m_depthSwapchain );
	for ( RefCntAutoPtr<ITexture>& pTexture : depthTextures )
	{
		TextureViewDesc viewDesc;
		viewDesc.ViewType = TEXTURE_VIEW_DEPTH_STENCIL;
		viewDesc.FirstArraySlice = 0;
		viewDesc.NumArraySlices = 1;
		viewDesc.AccessFlags = UAV_ACCESS_FLAG_WRITE;

		RefCntAutoPtr< ITextureView > pLeftEyeView;
		pTexture->CreateView( viewDesc, &pLeftEyeView );
		m_rpEyeDepthViews[ 0 ].push_back( pLeftEyeView );

		viewDesc.FirstArraySlice = 1;
		RefCntAutoPtr< ITextureView > pRightEyeView;
		pTexture->CreateView( viewDesc, &pRightEyeView );
		m_rpEyeDepthViews[ 1 ].push_back( pRightEyeView );
	}

	XrReferenceSpaceCreateInfo spaceCreateInfo = { XR_TYPE_REFERENCE_SPACE_CREATE_INFO };
	spaceCreateInfo.referenceSpaceType = XR_REFERENCE_SPACE_TYPE_STAGE;
	spaceCreateInfo.poseInReferenceSpace = IdentityXrPose();
	CHECK_XR_RESULT( xrCreateReferenceSpace( m_session, &spaceCreateInfo, &m_stageSpace ) );

	return true;
}


bool XrAppBase::ProcessCommandLine( const std::string& cmdLine )
{
	const auto* Key = "-mode ";
	const auto* pos = strstr( cmdLine.c_str(), Key );
	if ( pos != nullptr )
	{
		pos += strlen( Key );
		if ( _stricmp( pos, "D3D11" ) == 0 )
		{
#if D3D11_SUPPORTED
			m_DeviceType = RENDER_DEVICE_TYPE_D3D11;
#else
			std::cerr << "Direct3D11 is not supported. Please select another device type";
			return false;
#endif
		}
		else if ( _stricmp( pos, "D3D12" ) == 0 )
		{
#if D3D12_SUPPORTED
			m_DeviceType = RENDER_DEVICE_TYPE_D3D12;
#else
			std::cerr << "Direct3D12 is not supported. Please select another device type";
			return false;
#endif
		}
		else if ( _stricmp( pos, "GL" ) == 0 )
		{
#if GL_SUPPORTED
			m_DeviceType = RENDER_DEVICE_TYPE_GL;
#else
			std::cerr << "OpenGL is not supported. Please select another device type";
			return false;
#endif
		}
		else if ( _stricmp( pos, "VK" ) == 0 )
		{
#if VULKAN_SUPPORTED
			m_DeviceType = RENDER_DEVICE_TYPE_VULKAN;
#else
			std::cerr << "Vulkan is not supported. Please select another device type";
			return false;
#endif
		}
		else
		{
			std::cerr << "Unknown device type. Only the following types are supported: D3D11, D3D12, GL, VK";
			return false;
		}
	}
	else
	{
#if D3D12_SUPPORTED
		m_DeviceType = RENDER_DEVICE_TYPE_D3D12;
#elif VULKAN_SUPPORTED
		m_DeviceType = RENDER_DEVICE_TYPE_VULKAN;
#elif D3D11_SUPPORTED
		m_DeviceType = RENDER_DEVICE_TYPE_D3D11;
#elif GL_SUPPORTED
		m_DeviceType = RENDER_DEVICE_TYPE_GL;
#endif
	}
	return true;
}

void XrAppBase::RunMainFrame()
{
	XrTime displayTime;
	RunXrFrame( &displayTime );

	auto currTIme = m_frameTimer.GetElapsedTime();
	auto elapsedTime = currTIme - m_prevFrameTime;
	m_prevFrameTime = currTIme;
	Update( currTIme, elapsedTime, displayTime );

	Render();
	Present();
}

void XrAppBase::Present()
{
	// We use a swap interval of 0 here so the desktop window won't wait. We want all the waiting to happen because 
	// of xrWaitFrame.
	m_pSwapChain->Present( 0 );
}

void XrAppBase::WindowResize( uint32_t Width, uint32_t Height )
{
	if ( m_pSwapChain )
		m_pSwapChain->Resize( Width, Height );
}


bool XrAppBase::ShouldRender() const
{
	return m_sessionState == XR_SESSION_STATE_VISIBLE
		|| m_sessionState == XR_SESSION_STATE_FOCUSED;
}
bool XrAppBase::ShouldWait() const
{
	return m_sessionState == XR_SESSION_STATE_READY
		|| m_sessionState == XR_SESSION_STATE_SYNCHRONIZED
		|| m_sessionState == XR_SESSION_STATE_VISIBLE
		|| m_sessionState == XR_SESSION_STATE_FOCUSED;
}

void XrAppBase::ProcessOpenXrEvents()
{
	while ( true )
	{
		XrEventDataBuffer eventData = { XR_TYPE_EVENT_DATA_BUFFER };
		XrResult res = xrPollEvent( m_instance, &eventData );
		if ( res == XR_EVENT_UNAVAILABLE )
			break;

		if ( XR_FAILED( res ) )
		{
			// log something?
			break;
		}

		switch ( eventData.type )
		{
		case XR_TYPE_EVENT_DATA_SESSION_STATE_CHANGED:
		{
			const XrEventDataSessionStateChanged* event = (const XrEventDataSessionStateChanged*)( &eventData );
			switch ( event->state )
			{
			case XR_SESSION_STATE_READY:
			{
				XrSessionBeginInfo beginInfo = { XR_TYPE_SESSION_BEGIN_INFO };
				beginInfo.primaryViewConfigurationType = XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO;
				xrBeginSession( m_session, &beginInfo );
			}
			break;

			case XR_SESSION_STATE_STOPPING:
			{
				xrEndSession( m_session );
			}
			break;

			default:
				// nothing special to do for this session state
				break;
			}

			m_sessionState = event->state;
		}
		break;

		default:
			// ignoring this event
			break;
		}
	}
}

bool XrAppBase::RunXrFrame( XrTime *displayTime )
{
	ProcessOpenXrEvents();

	*displayTime = 0;

	if ( !ShouldWait() )
		return true;

	XrFrameState frameState = { XR_TYPE_FRAME_STATE };
	XrFrameWaitInfo waitInfo = { XR_TYPE_FRAME_WAIT_INFO };
	CHECK_XR_RESULT( xrWaitFrame( m_session, &waitInfo, &frameState ) );

	XrFrameBeginInfo beginInfo = { XR_TYPE_FRAME_BEGIN_INFO };
	CHECK_XR_RESULT( xrBeginFrame( m_session, &beginInfo ) );

	XrFrameEndInfo frameEndInfo = { XR_TYPE_FRAME_END_INFO };
	frameEndInfo.displayTime = frameState.predictedDisplayTime;
	frameEndInfo.environmentBlendMode = XR_ENVIRONMENT_BLEND_MODE_OPAQUE;
	*displayTime = frameState.predictedDisplayTime;

	XrCompositionLayerProjectionView projectionViews[ 2 ] = { { XR_TYPE_COMPOSITION_LAYER_PROJECTION_VIEW }, { XR_TYPE_COMPOSITION_LAYER_PROJECTION_VIEW } };
	XrCompositionLayerProjection projectionLayer = { XR_TYPE_COMPOSITION_LAYER_PROJECTION };
	XrCompositionLayerBaseHeader* layers[] = { (XrCompositionLayerBaseHeader*)&projectionLayer };

	if ( frameState.shouldRender && ShouldRender() )
	{
		// acquire the image index for this swapchain
		XrSwapchainImageAcquireInfo acquireInfo = { XR_TYPE_SWAPCHAIN_IMAGE_ACQUIRE_INFO };
		uint32_t index;
		CHECK_XR_RESULT( xrAcquireSwapchainImage( m_swapchain, &acquireInfo, &index ) );

		// wait for swap chains
		XrSwapchainImageWaitInfo waitInfo = { XR_TYPE_SWAPCHAIN_IMAGE_WAIT_INFO };
		waitInfo.timeout = 999999;
		CHECK_XR_RESULT( xrWaitSwapchainImage( m_swapchain, &waitInfo ) );

		XrViewLocateInfo locateInfo = { XR_TYPE_VIEW_LOCATE_INFO };
		locateInfo.displayTime = frameState.predictedDisplayTime;
		locateInfo.space = m_stageSpace;
		locateInfo.viewConfigurationType = XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO;

		XrViewState viewState = { XR_TYPE_VIEW_STATE };
		XrView views[ 2 ] = { { XR_TYPE_VIEW }, { XR_TYPE_VIEW } };
		uint32_t viewCount;
		CHECK_XR_RESULT( xrLocateViews( m_session, &locateInfo, &viewState, 2, &viewCount, views ) );

		// render
		RenderEye( views[ 0 ], m_rpEyeSwapchainViews[ 0 ][ index ], m_rpEyeDepthViews[ 0 ][ index ] );
		RenderEye( views[ 1 ], m_rpEyeSwapchainViews[ 1 ][ index ], m_rpEyeDepthViews[ 1 ][ index ] );

		// release the image we just rendered into
		XrSwapchainImageReleaseInfo releaseInfo = { XR_TYPE_SWAPCHAIN_IMAGE_RELEASE_INFO };
		CHECK_XR_RESULT( xrReleaseSwapchainImage( m_swapchain, &releaseInfo ) );

		for ( uint32_t i = 0; i < 2; i++ )
		{
			projectionViews[ i ].fov = views[ i ].fov;
			projectionViews[ i ].pose = views[ i ].pose;
			projectionViews[ i ].subImage.swapchain = m_swapchain;
			projectionViews[ i ].subImage.imageArrayIndex = i;
			projectionViews[ i ].subImage.imageRect =
			{
				{ 0, 0 },
				{
					(int32_t)m_views[ 0 ].recommendedImageRectWidth,
					(int32_t)m_views[ 0 ].recommendedImageRectHeight
				}
			};
		}

		projectionLayer.space = m_stageSpace;
		projectionLayer.viewCount = 2;
		projectionLayer.views = projectionViews;

		frameEndInfo.layers = layers;
		frameEndInfo.layerCount = 1;
	}

	CHECK_XR_RESULT( xrEndFrame( m_session, &frameEndInfo ) );

	return true;
}


void XrAppBase::CreateGLTFResourceCache()
{
	std::array<BufferSuballocatorCreateInfo, 3> Buffers = {};

	Buffers[ 0 ].Desc.Name = "GLTF basic vertex attribs buffer";
	Buffers[ 0 ].Desc.BindFlags = BIND_VERTEX_BUFFER;
	Buffers[ 0 ].Desc.Usage = USAGE_DEFAULT;
	Buffers[ 0 ].Desc.uiSizeInBytes = sizeof( GLTF::Model::VertexBasicAttribs ) * 16 << 10;

	Buffers[ 1 ].Desc.Name = "GLTF skin attribs buffer";
	Buffers[ 1 ].Desc.BindFlags = BIND_VERTEX_BUFFER;
	Buffers[ 1 ].Desc.Usage = USAGE_DEFAULT;
	Buffers[ 1 ].Desc.uiSizeInBytes = sizeof( GLTF::Model::VertexSkinAttribs ) * 16 << 10;

	Buffers[ 2 ].Desc.Name = "GLTF index buffer";
	Buffers[ 2 ].Desc.BindFlags = BIND_INDEX_BUFFER;
	Buffers[ 2 ].Desc.Usage = USAGE_DEFAULT;
	Buffers[ 2 ].Desc.uiSizeInBytes = sizeof( Uint32 ) * 8 << 10;

	std::array<DynamicTextureAtlasCreateInfo, 1> Atlases;
	Atlases[ 0 ].Desc.Name = "GLTF texture atlas";
	Atlases[ 0 ].Desc.Type = RESOURCE_DIM_TEX_2D_ARRAY;
	Atlases[ 0 ].Desc.Usage = USAGE_DEFAULT;
	Atlases[ 0 ].Desc.BindFlags = BIND_SHADER_RESOURCE;
	Atlases[ 0 ].Desc.Format = TEX_FORMAT_RGBA8_UNORM;
	Atlases[ 0 ].Desc.Width = 4096;
	Atlases[ 0 ].Desc.Height = 4096;
	Atlases[ 0 ].Desc.MipLevels = 6;

	GLTF::ResourceManager::CreateInfo ResourceMgrCI;
	ResourceMgrCI.BuffSuballocators = Buffers.data();
	ResourceMgrCI.NumBuffSuballocators = static_cast<Uint32>( Buffers.size() );
	ResourceMgrCI.TexAtlases = Atlases.data();
	ResourceMgrCI.NumTexAtlases = static_cast<Uint32>( Atlases.size() );

	ResourceMgrCI.DefaultAtlasDesc.Desc.Type = RESOURCE_DIM_TEX_2D_ARRAY;
	ResourceMgrCI.DefaultAtlasDesc.Desc.Usage = USAGE_DEFAULT;
	ResourceMgrCI.DefaultAtlasDesc.Desc.BindFlags = BIND_SHADER_RESOURCE;
	ResourceMgrCI.DefaultAtlasDesc.Desc.Width = 4096;
	ResourceMgrCI.DefaultAtlasDesc.Desc.Height = 4096;
	ResourceMgrCI.DefaultAtlasDesc.Desc.MipLevels = 6;

	m_pResourceMgr = GLTF::ResourceManager::Create( m_pGraphicsBinding->GetRenderDevice(), ResourceMgrCI );

	m_CacheUseInfo.pResourceMgr = m_pResourceMgr;
	m_CacheUseInfo.VertexBuffer0Idx = 0;
	m_CacheUseInfo.VertexBuffer1Idx = 1;
	m_CacheUseInfo.IndexBufferIdx = 2;

	m_CacheUseInfo.BaseColorFormat = TEX_FORMAT_RGBA8_UNORM;
	m_CacheUseInfo.PhysicalDescFormat = TEX_FORMAT_RGBA8_UNORM;
	m_CacheUseInfo.NormalFormat = TEX_FORMAT_RGBA8_UNORM;
	m_CacheUseInfo.OcclusionFormat = TEX_FORMAT_RGBA8_UNORM;
	m_CacheUseInfo.EmissiveFormat = TEX_FORMAT_RGBA8_UNORM;
}



std::unique_ptr<GLTF::Model> XrAppBase::LoadGltfModel( const std::string& path )
{
	GLTF::Model::CreateInfo ci;
	ci.FileName = path.c_str();
	ci.LoadAnimationAndSkin = true;
	ci.pCacheInfo = &m_CacheUseInfo;

	auto model = std::make_unique<GLTF::Model>(
		m_pGraphicsBinding->GetRenderDevice(), m_pGraphicsBinding->GetImmediateContext(), ci );

	return model;
}
