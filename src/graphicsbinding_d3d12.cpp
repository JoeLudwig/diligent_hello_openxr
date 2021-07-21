#if D3D12_SUPPORTED
#include <dxgi.h>
#include <d3d12.h>
#	define XR_USE_GRAPHICS_API_D3D12
#include "Graphics/GraphicsEngineD3D12/interface/RenderDeviceD3D12.h"
#include "Graphics/GraphicsEngineD3D12/interface/DeviceContextD3D12.h"

#include <openxr/openxr_platform.h>

#include "graphicsbinding_d3d11.h" // For GetAdapterIndexFromLuid
#include "graphicsbinding_d3d12.h"

using namespace Diligent;

#include <wrl/client.h>

GraphicsBinding_D3D12::~GraphicsBinding_D3D12()
{
	if (m_d3d12Binding)
	{
		delete m_d3d12Binding;
	}
}


std::vector<std::string> GraphicsBinding_D3D12::GetXrExtensions()
{
	return { XR_KHR_D3D12_ENABLE_EXTENSION_NAME };
}


XrResult GraphicsBinding_D3D12::CreateDevice( XrInstance instance, XrSystemId systemId )
{
	m_instance = instance;
	m_systemId = systemId;

#	if ENGINE_DLL
	// Load the dll and import GetEngineFactoryD3D12() function
	auto* GetEngineFactoryD3D12 = LoadGraphicsEngineD3D12();
#	endif
	auto* pFactoryD3D12 = GetEngineFactoryD3D12();
	m_pEngineFactory = pFactoryD3D12;

	FETCH_AND_DEFINE_XR_FUNCTION( m_instance, xrGetD3D12GraphicsRequirementsKHR );
	XrGraphicsRequirementsD3D12KHR graphicsRequirements = { XR_TYPE_GRAPHICS_REQUIREMENTS_D3D12_KHR };
	XrResult res = xrGetD3D12GraphicsRequirementsKHR( m_instance, m_systemId, &graphicsRequirements );
	if ( XR_FAILED( res ) )
	{
		return res;
	}

	EngineD3D12CreateInfo EngineCI;
	EngineCI.AdapterId = GetAdapterIndexFromLuid( graphicsRequirements.adapterLuid );
	EngineCI.GraphicsAPIVersion = Version { 11, 0 };
	pFactoryD3D12->CreateDeviceAndContextsD3D12( EngineCI, &m_pDevice, &m_pImmediateContext );

	m_d3d12Binding = new XrGraphicsBindingD3D12KHR( { XR_TYPE_GRAPHICS_BINDING_D3D12_KHR } );
	m_d3d12Binding->device = GetD3D12Device()->GetD3D12Device();

	IDeviceContextD3D12* d3d12DeviceContext = (IDeviceContextD3D12*)GetImmediateContext();
	ICommandQueueD3D12* d3d12CommandQueue = (ICommandQueueD3D12*)d3d12DeviceContext->LockCommandQueue();
	m_d3d12Binding->queue = d3d12CommandQueue->GetD3D12CommandQueue();
	d3d12DeviceContext->UnlockCommandQueue();

	return XR_SUCCESS;
}


Diligent::IEngineFactory* GraphicsBinding_D3D12::GetEngineFactory()
{
	return m_pEngineFactory.RawPtr();
}

std::vector<int64_t> GraphicsBinding_D3D12::GetRequestedColorFormats()
{
	return
	{
		DXGI_FORMAT_R16G16B16A16_FLOAT,
		DXGI_FORMAT_R8G8B8A8_UNORM_SRGB
	};
}

std::vector<int64_t> GraphicsBinding_D3D12::GetRequestedDepthFormats()
{
	return 
	{
		DXGI_FORMAT_D32_FLOAT,
		DXGI_FORMAT_D16_UNORM,
	};
}


void* GraphicsBinding_D3D12::GetSessionBinding()
{
	return m_d3d12Binding;
}

std::vector< RefCntAutoPtr<ITexture> > GraphicsBinding_D3D12::ReadImagesFromSwapchain( XrSwapchain swapchain )
{
	std::vector< RefCntAutoPtr< ITexture > > textures;

	uint32_t imageCount;
	if ( XR_FAILED( xrEnumerateSwapchainImages( swapchain, 0, &imageCount, nullptr ) ) )
		return {};

	std::vector< XrSwapchainImageD3D12KHR > images;
	images.resize( imageCount, { XR_TYPE_SWAPCHAIN_IMAGE_D3D12_KHR } );
	if ( XR_FAILED( xrEnumerateSwapchainImages( swapchain, 
		imageCount, &imageCount, (XrSwapchainImageBaseHeader*)&images[ 0 ] ) ) )
		return {};

	for ( const XrSwapchainImageD3D12KHR& image : images )
	{
		RefCntAutoPtr< ITexture > pTexture;
		GetD3D12Device()->CreateTextureFromD3DResource( image.texture, RESOURCE_STATE_UNKNOWN, &pTexture );
		textures.push_back( pTexture );
	}

	return textures;
}

#endif
