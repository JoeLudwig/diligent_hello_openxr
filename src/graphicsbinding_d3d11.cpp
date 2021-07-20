
#if D3D11_SUPPORTED
#include <d3d11.h>
#    define XR_USE_GRAPHICS_API_D3D11
#include "Graphics/GraphicsEngineD3D11/interface/RenderDeviceD3D11.h"
#endif

#include <openxr/openxr_platform.h>

#include "graphicsbinding_d3d11.h"

using namespace Diligent;

#if D3D11_SUPPORTED || D3D12_SUPPORTED
#include <wrl/client.h>

UINT GetAdapterIndexFromLuid( LUID adapterId )
{
	// Create the DXGI factory.
	Microsoft::WRL::ComPtr<IDXGIFactory1> dxgiFactory;
	if ( FAILED( CreateDXGIFactory1( __uuidof( IDXGIFactory1 ), reinterpret_cast<void**>( dxgiFactory.ReleaseAndGetAddressOf() ) ) ) )
		return 0;

	for ( UINT adapterIndex = 0;; adapterIndex++ )
	{
		// EnumAdapters1 will fail with DXGI_ERROR_NOT_FOUND when there are no more adapters to enumerate.
		Microsoft::WRL::ComPtr<IDXGIAdapter1> dxgiAdapter;
		if ( FAILED( dxgiFactory->EnumAdapters1( adapterIndex, dxgiAdapter.ReleaseAndGetAddressOf() ) ) )
			break;

		DXGI_ADAPTER_DESC1 adapterDesc;
		if ( FAILED( dxgiAdapter->GetDesc1( &adapterDesc ) ) )
			continue;
		if ( memcmp( &adapterDesc.AdapterLuid, &adapterId, sizeof( adapterId ) ) == 0 ) {
			return adapterIndex;
		}
	}
	return 0;
}
#endif

GraphicsBinding_D3D11::~GraphicsBinding_D3D11()
{
	delete m_d3d11Binding;
}


std::vector<std::string> GraphicsBinding_D3D11::GetXrExtensions()
{
	return { XR_KHR_D3D11_ENABLE_EXTENSION_NAME };
}


XrResult GraphicsBinding_D3D11::CreateDevice( XrInstance instance, XrSystemId systemId )
{
	m_instance = instance;
	m_systemId = systemId;

#    if ENGINE_DLL
	// Load the dll and import GetEngineFactoryD3D11() function
	auto* GetEngineFactoryD3D11 = LoadGraphicsEngineD3D11();
#    endif
	auto* pFactoryD3D11 = GetEngineFactoryD3D11();
	m_pEngineFactory = pFactoryD3D11;

	FETCH_AND_DEFINE_XR_FUNCTION( m_instance, xrGetD3D11GraphicsRequirementsKHR );
	XrGraphicsRequirementsD3D11KHR graphicsRequirements = { XR_TYPE_GRAPHICS_REQUIREMENTS_D3D11_KHR };
	XrResult res = xrGetD3D11GraphicsRequirementsKHR( m_instance, m_systemId, &graphicsRequirements );
	if ( XR_FAILED( res ) )
	{
		return res;
	}

	EngineD3D11CreateInfo EngineCI;
	EngineCI.AdapterId = GetAdapterIndexFromLuid( graphicsRequirements.adapterLuid );
	EngineCI.GraphicsAPIVersion = Version { 11, 0 };
	pFactoryD3D11->CreateDeviceAndContextsD3D11( EngineCI, &m_pDevice, &m_pImmediateContext );

	m_d3d11Binding = new XrGraphicsBindingD3D11KHR( { XR_TYPE_GRAPHICS_BINDING_D3D11_KHR } );
	m_d3d11Binding->device = GetD3D11Device()->GetD3D11Device();

	return XR_SUCCESS;
}


Diligent::IEngineFactory* GraphicsBinding_D3D11::GetEngineFactory()
{
	return m_pEngineFactory.RawPtr();
}

std::vector<int64_t> GraphicsBinding_D3D11::GetRequestedColorFormats()
{
	return
	{
		DXGI_FORMAT_R16G16B16A16_FLOAT,
		DXGI_FORMAT_R8G8B8A8_UNORM_SRGB
	};
}

std::vector<int64_t> GraphicsBinding_D3D11::GetRequestedDepthFormats()
{
	return 
	{
		DXGI_FORMAT_D32_FLOAT,
		DXGI_FORMAT_D16_UNORM,
	};
}


void* GraphicsBinding_D3D11::GetSessionBinding()
{
	return m_d3d11Binding;
}

std::vector< RefCntAutoPtr<ITexture> > GraphicsBinding_D3D11::ReadImagesFromSwapchain( XrSwapchain swapchain )
{
	std::vector< RefCntAutoPtr< ITexture > > textures;

	uint32_t imageCount, depthImageCount;
	if ( XR_FAILED( xrEnumerateSwapchainImages( swapchain, 0, &imageCount, nullptr ) ) )
		return {};

	std::vector< XrSwapchainImageD3D11KHR > images;
	images.resize( imageCount, { XR_TYPE_SWAPCHAIN_IMAGE_D3D11_KHR } );
	if ( XR_FAILED( xrEnumerateSwapchainImages( swapchain, 
		imageCount, &imageCount, (XrSwapchainImageBaseHeader*)&images[ 0 ] ) ) )
		return {};

	for ( const XrSwapchainImageD3D11KHR& image : images )
	{
		RefCntAutoPtr< ITexture > pTexture;
		GetD3D11Device()->CreateTexture2DFromD3DResource( image.texture, RESOURCE_STATE_UNKNOWN, &pTexture );
		textures.push_back( pTexture );
	}

	return textures;
}

