/*
 *  Copyright 2019-2021 Diligent Graphics LLC
 *  Copyright 2015-2019 Egor Yusov
 *  
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *  
 *      http://www.apache.org/licenses/LICENSE-2.0
 *  
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *
 *  In no event and under no legal theory, whether in tort (including negligence), 
 *  contract, or otherwise, unless required by applicable law (such as deliberate 
 *  and grossly negligent acts) or agreed to in writing, shall any Contributor be
 *  liable for any damages, including any direct, indirect, special, incidental, 
 *  or consequential damages of any character arising as a result of this License or 
 *  out of the use or inability to use the software (including but not limited to damages 
 *  for loss of goodwill, work stoppage, computer failure or malfunction, or any and 
 *  all other commercial damages or losses), even if such Contributor has been advised 
 *  of the possibility of such damages.
 */

#include <memory>
#include <iomanip>
#include <iostream>
#include <vector>
#include <string>
#include <map>

#ifndef NOMINMAX
#    define NOMINMAX
#endif
#include <Windows.h>
#include <crtdbg.h>

#ifndef PLATFORM_WIN32
#    define PLATFORM_WIN32 1
#endif

#ifndef ENGINE_DLL
#    define ENGINE_DLL 1
#endif

#ifndef D3D11_SUPPORTED
#    define D3D11_SUPPORTED 1
#endif

#ifndef D3D12_SUPPORTED
#    define D3D12_SUPPORTED 1
#endif

#ifndef GL_SUPPORTED
#    define GL_SUPPORTED 1
#endif

#ifndef VULKAN_SUPPORTED
#    define VULKAN_SUPPORTED 1
#endif

#include "Graphics/GraphicsEngineD3D11/interface/EngineFactoryD3D11.h"
#include "Graphics/GraphicsEngineD3D12/interface/EngineFactoryD3D12.h"
#include "Graphics/GraphicsEngineOpenGL/interface/EngineFactoryOpenGL.h"
#include "Graphics/GraphicsEngineVulkan/interface/EngineFactoryVk.h"
#include "Common/interface/BasicMath.hpp"
#include "Common/interface/Timer.hpp"

#include "Graphics/GraphicsEngine/interface/RenderDevice.h"
#include "Graphics/GraphicsEngine/interface/DeviceContext.h"
#include "Graphics/GraphicsEngine/interface/SwapChain.h"

#include "Common/interface/RefCntAutoPtr.hpp"

#include "Graphics/GraphicsTools/interface/MapHelper.hpp"

#include "openxr/openxr.h"

// Make sure the supported OpenXR graphics APIs are defined
#if D3D11_SUPPORTED
#include <d3d11.h>
#    define XR_USE_GRAPHICS_API_D3D11
#include "Graphics/GraphicsEngineD3D11/interface/RenderDeviceD3D11.h"
#endif

#if D3D12_SUPPORTED
#include <d3d12.h>
#    define XR_USE_GRAPHICS_API_D3D12
#include "Graphics/GraphicsEngineD3D12/interface/RenderDeviceD3D12.h"
#endif

#if GL_SUPPORTED
#    define XR_USE_GRAPHICS_API_OPENGL
#endif

#if VULKAN_SUPPORTED
#    define XR_USE_GRAPHICS_API_VULKAN
#include <vulkan/vulkan.h>
#endif

#include "openxr/openxr_platform.h"

#define CHECK_XR_RESULT( res ) \
    do { \
        if( FAILED( res ) ) \
            return false; \
    } \
    while ( 0 );

using namespace Diligent;

// For this tutorial, we will use simple vertex shader
// that creates a procedural triangle

// Diligent Engine can use HLSL source on all supported platforms.
// It will convert HLSL to GLSL in OpenGL mode, while Vulkan backend will compile it directly to SPIRV.

static const char* VSSource = R"(
struct PSInput 
{ 
    float4 Pos   : SV_POSITION; 
    float3 Color : COLOR; 
};

void main(in  uint    VertId : SV_VertexID,
          out PSInput PSIn) 
{
    float4 Pos[3];
    Pos[0] = float4(-0.5, -0.5, 0.0, 1.0);
    Pos[1] = float4( 0.0, +0.5, 0.0, 1.0);
    Pos[2] = float4(+0.5, -0.5, 0.0, 1.0);

    float3 Col[3];
    Col[0] = float3(1.0, 0.0, 0.0); // red
    Col[1] = float3(0.0, 1.0, 0.0); // green
    Col[2] = float3(0.0, 0.0, 1.0); // blue

    PSIn.Pos   = Pos[VertId];
    PSIn.Color = Col[VertId];
}
)";

// Pixel shader simply outputs interpolated vertex color
static const char* PSSource = R"(
struct PSInput 
{ 
    float4 Pos   : SV_POSITION; 
    float3 Color : COLOR; 
};

struct PSOutput
{ 
    float4 Color : SV_TARGET; 
};

void main(in  PSInput  PSIn,
          out PSOutput PSOut)
{
    PSOut.Color = float4(PSIn.Color.rgb, 1.0);
}
)";

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


#define FETCH_AND_DEFINE_XR_FUNCTION( instance, name ) \
    PFN_ ## name name = nullptr; \
    xrGetInstanceProcAddr( instance, #name, (PFN_xrVoidFunction *)&name );

class HelloXrApp
{
public:
    HelloXrApp()
    {
    }

    ~HelloXrApp()
    {
        m_pImmediateContext->Flush();
    }

    bool Initialize(HWND hWnd)
    {
		// create the OpenXR instance first because it will have an opinion about device creation
        if( !InitializeOpenXr() )
        {
            return false;
        }

        SwapChainDesc SCDesc;
        switch (m_DeviceType)
        {
#if D3D11_SUPPORTED
            case RENDER_DEVICE_TYPE_D3D11:
            {
                FETCH_AND_DEFINE_XR_FUNCTION( m_instance, xrGetD3D11GraphicsRequirementsKHR );
                XrGraphicsRequirementsD3D11KHR graphicsRequirements = { XR_TYPE_GRAPHICS_REQUIREMENTS_D3D11_KHR };
                XrResult res = xrGetD3D11GraphicsRequirementsKHR( m_instance, m_systemId, &graphicsRequirements );
                if ( XR_FAILED( res ) )
                {
                    return false;
                }

                EngineD3D11CreateInfo EngineCI;
                EngineCI.AdapterId = GetAdapterIndexFromLuid( graphicsRequirements.adapterLuid );
                EngineCI.GraphicsAPIVersion = Version { 11, 0 };
#    if ENGINE_DLL
                // Load the dll and import GetEngineFactoryD3D11() function
                auto* GetEngineFactoryD3D11 = LoadGraphicsEngineD3D11();
#    endif
                auto* pFactoryD3D11 = GetEngineFactoryD3D11();
                m_pEngineFactory = pFactoryD3D11;
                pFactoryD3D11->CreateDeviceAndContextsD3D11(EngineCI, &m_pDevice, &m_pImmediateContext);
                Win32NativeWindow Window{hWnd};
                pFactoryD3D11->CreateSwapChainD3D11(m_pDevice, m_pImmediateContext, SCDesc, FullScreenModeDesc{}, Window, &m_pSwapChain);
            }
            break;
#endif


#if D3D12_SUPPORTED
            case RENDER_DEVICE_TYPE_D3D12:
            {
				FETCH_AND_DEFINE_XR_FUNCTION( m_instance, xrGetD3D12GraphicsRequirementsKHR );
				XrGraphicsRequirementsD3D12KHR graphicsRequirements = { XR_TYPE_GRAPHICS_REQUIREMENTS_D3D12_KHR };
				XrResult res = xrGetD3D12GraphicsRequirementsKHR( m_instance, m_systemId, &graphicsRequirements );
				if ( XR_FAILED( res ) )
				{
					return false;
				}

#    if ENGINE_DLL
                // Load the dll and import GetEngineFactoryD3D12() function
                auto GetEngineFactoryD3D12 = LoadGraphicsEngineD3D12();
#    endif
                EngineD3D12CreateInfo EngineCI;
				EngineCI.AdapterId = GetAdapterIndexFromLuid( graphicsRequirements.adapterLuid );
				EngineCI.GraphicsAPIVersion = Version { 12, 0 };

                auto* pFactoryD3D12 = GetEngineFactoryD3D12();
				m_pEngineFactory = pFactoryD3D12;
                pFactoryD3D12->CreateDeviceAndContextsD3D12(EngineCI, &m_pDevice, &m_pImmediateContext);
                Win32NativeWindow Window{hWnd};
                pFactoryD3D12->CreateSwapChainD3D12(m_pDevice, m_pImmediateContext, SCDesc, FullScreenModeDesc{}, Window, &m_pSwapChain);
            }
            break;
#endif


#if GL_SUPPORTED
            case RENDER_DEVICE_TYPE_GL:
            {
				FETCH_AND_DEFINE_XR_FUNCTION( m_instance, xrGetOpenGLGraphicsRequirementsKHR );
				// we don't have any way to specify the adapter in OpenGL, but we're required to get the graphics
                // requirements anyway
				XrGraphicsRequirementsOpenGLKHR graphicsRequirements = { XR_TYPE_GRAPHICS_REQUIREMENTS_OPENGL_KHR };
				XrResult res = xrGetOpenGLGraphicsRequirementsKHR( m_instance, m_systemId, &graphicsRequirements );
				if ( XR_FAILED( res ) )
				{
					return false;
				}
#    if EXPLICITLY_LOAD_ENGINE_GL_DLL
                // Load the dll and import GetEngineFactoryOpenGL() function
                auto GetEngineFactoryOpenGL = LoadGraphicsEngineOpenGL();
#    endif
                auto* pFactoryOpenGL = GetEngineFactoryOpenGL();
				m_pEngineFactory = pFactoryOpenGL;

                EngineGLCreateInfo EngineCI;
                EngineCI.Window.hWnd = hWnd;

                pFactoryOpenGL->CreateDeviceAndSwapChainGL(EngineCI, &m_pDevice, &m_pImmediateContext, SCDesc, &m_pSwapChain);
            }
            break;
#endif


#if VULKAN_SUPPORTED
            case RENDER_DEVICE_TYPE_VULKAN:
            {
                // TODO: Vulkan requires that the runtime create the instance. That's going to require a change to 
                // CrateDeviceAndContextsVk, probably
#    if EXPLICITLY_LOAD_ENGINE_VK_DLL
                // Load the dll and import GetEngineFactoryVk() function
                auto GetEngineFactoryVk = LoadGraphicsEngineVk();
#    endif
                EngineVkCreateInfo EngineCI;

                auto* pFactoryVk = GetEngineFactoryVk();
				m_pEngineFactory = pFactoryVk;
                pFactoryVk->CreateDeviceAndContextsVk(EngineCI, &m_pDevice, &m_pImmediateContext);

                if (!m_pSwapChain && hWnd != nullptr)
                {
                    Win32NativeWindow Window{hWnd};
                    pFactoryVk->CreateSwapChainVk(m_pDevice, m_pImmediateContext, SCDesc, Window, &m_pSwapChain);
                }
            }
            break;
#endif


            default:
                std::cerr << "Unknown/unsupported device type";
                return false;
                break;
        }

        if ( !CreateSession() )
            return false;

        CreatePipelineState();
        CreateVertexBuffer();
        CreateIndexBuffer();

        return true;
    }


    bool InitializeOpenXr()
    {
        XrExtensionMap availableExtensions = GetAvailableOpenXRExtensions();

         std::vector< std::string > xrExtensions;

        switch ( m_DeviceType )
        {
#if D3D11_SUPPORTED
        case RENDER_DEVICE_TYPE_D3D11:
        {
            xrExtensions.push_back( XR_KHR_D3D11_ENABLE_EXTENSION_NAME );
        }
        break;
#endif


#if D3D12_SUPPORTED
        case RENDER_DEVICE_TYPE_D3D12:
        {
            xrExtensions.push_back( XR_KHR_D3D12_ENABLE_EXTENSION_NAME );
        }
        break;
#endif


#if GL_SUPPORTED
        case RENDER_DEVICE_TYPE_GL:
        {
            xrExtensions.push_back( XR_KHR_OPENGL_ENABLE_EXTENSION_NAME );
        }
        break;
#endif


#if VULKAN_SUPPORTED
        case RENDER_DEVICE_TYPE_VULKAN:
        {
            xrExtensions.push_back( XR_KHR_VULKAN_ENABLE2_EXTENSION_NAME );
        }
        break;
#endif
        }

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

    bool CreateSession()
    {
        XrSessionCreateInfo createInfo = { XR_TYPE_SESSION_CREATE_INFO };
        createInfo.systemId = m_systemId;
        createInfo.createFlags = 0;

        std::vector< int64_t > requestedFormats;
        switch ( m_DeviceType )
        {
#if D3D11_SUPPORTED
		case RENDER_DEVICE_TYPE_D3D11:
		{
            requestedFormats.push_back( DXGI_FORMAT_R16G16B16A16_FLOAT );
			requestedFormats.push_back( DXGI_FORMAT_R8G8B8A8_UNORM );

            static XrGraphicsBindingD3D11KHR d3d11Binding = { XR_TYPE_GRAPHICS_BINDING_D3D11_KHR };
            createInfo.next = &d3d11Binding;

            d3d11Binding.device = GetD3D11Device()->GetD3D11Device();
		}
		break;
#endif


#if D3D12_SUPPORTED
		case RENDER_DEVICE_TYPE_D3D12:
		{
			requestedFormats.push_back( DXGI_FORMAT_R16G16B16A16_FLOAT );
			requestedFormats.push_back( DXGI_FORMAT_R8G8B8A8_UNORM );

			static XrGraphicsBindingD3D12KHR d3d12Binding = { XR_TYPE_GRAPHICS_BINDING_D3D12_KHR };
			createInfo.next = &d3d12Binding;

			d3d12Binding.device = GetD3D12Device()->GetD3D12Device();
		}
		break;
#endif


#if GL_SUPPORTED
		case RENDER_DEVICE_TYPE_GL:
		{
		}
		break;
#endif


#if VULKAN_SUPPORTED
		case RENDER_DEVICE_TYPE_VULKAN:
		{
		}
		break;
#endif
        }

        CHECK_XR_RESULT( xrCreateSession( m_instance, &createInfo, &m_session ) );

        uint32_t swapchainFormatCount;
        CHECK_XR_RESULT( xrEnumerateSwapchainFormats( m_session, 0, &swapchainFormatCount, nullptr ) );
        std::vector<int64_t> supportedFormats;
        supportedFormats.resize( swapchainFormatCount );
		CHECK_XR_RESULT( xrEnumerateSwapchainFormats( m_session, swapchainFormatCount, &swapchainFormatCount, &supportedFormats[0] ) );
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

        CHECK_XR_RESULT( xrCreateSwapchain( m_session, &scCreateInfo, &m_swapchain ) );

        uint32_t imageCount;
        CHECK_XR_RESULT( xrEnumerateSwapchainImages( m_swapchain, 0, &imageCount, nullptr ) );

        std::vector< RefCntAutoPtr<ITexture> > textures;

		switch ( m_DeviceType )
		{
#if D3D11_SUPPORTED
		case RENDER_DEVICE_TYPE_D3D11:
		{
            std::vector< XrSwapchainImageD3D11KHR > images;
            images.resize( imageCount, { XR_TYPE_SWAPCHAIN_IMAGE_D3D11_KHR } );
			CHECK_XR_RESULT( xrEnumerateSwapchainImages( m_swapchain, 
                imageCount, &imageCount, (XrSwapchainImageBaseHeader *)&images[0] ) );

            for ( const XrSwapchainImageD3D11KHR& image : images )
            {
                RefCntAutoPtr< ITexture > pTexture;
                GetD3D11Device()->CreateTexture2DFromD3DResource( image.texture, RESOURCE_STATE_UNKNOWN, &pTexture );
                textures.push_back( pTexture );
            }
		}
		break;
#endif


#if D3D12_SUPPORTED
		case RENDER_DEVICE_TYPE_D3D12:
		{
			requestedFormats.push_back( DXGI_FORMAT_R16G16B16A16_FLOAT );
			requestedFormats.push_back( DXGI_FORMAT_R8G8B8A8_UNORM );

			static XrGraphicsBindingD3D12KHR d3d12Binding = { XR_TYPE_GRAPHICS_BINDING_D3D12_KHR };
			createInfo.next = &d3d12Binding;

			d3d12Binding.device = GetD3D12Device()->GetD3D12Device();
		}
		break;
#endif


#if GL_SUPPORTED
		case RENDER_DEVICE_TYPE_GL:
		{
		}
		break;
#endif


#if VULKAN_SUPPORTED
		case RENDER_DEVICE_TYPE_VULKAN:
		{
		}
		break;
#endif
		}

		for ( RefCntAutoPtr<ITexture> & pTexture: textures )
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

        return true;

    }


    bool ProcessCommandLine(const char* CmdLine)
    {
        const auto* Key = "-mode ";
        const auto* pos = strstr(CmdLine, Key);
        if (pos != nullptr)
        {
            pos += strlen(Key);
            if (_stricmp(pos, "D3D11") == 0)
            {
#if D3D11_SUPPORTED
                m_DeviceType = RENDER_DEVICE_TYPE_D3D11;
#else
                std::cerr << "Direct3D11 is not supported. Please select another device type";
                return false;
#endif
            }
            else if (_stricmp(pos, "D3D12") == 0)
            {
#if D3D12_SUPPORTED
                m_DeviceType = RENDER_DEVICE_TYPE_D3D12;
#else
                std::cerr << "Direct3D12 is not supported. Please select another device type";
                return false;
#endif
            }
            else if (_stricmp(pos, "GL") == 0)
            {
#if GL_SUPPORTED
                m_DeviceType = RENDER_DEVICE_TYPE_GL;
#else
                std::cerr << "OpenGL is not supported. Please select another device type";
                return false;
#endif
            }
            else if (_stricmp(pos, "VK") == 0)
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


#if D3D11_SUPPORTED
    IRenderDeviceD3D11* GetD3D11Device() { return (IRenderDeviceD3D11 *)m_pDevice.RawPtr(); }
#endif

#if D3D12_SUPPORTED
	IRenderDeviceD3D12* GetD3D12Device() { return (IRenderDeviceD3D12*)m_pDevice.RawPtr(); }
#endif

    void Render();
    void Update( double currTime, double elapsedTime );

    void Present()
    {
        m_pSwapChain->Present();
    }

    void WindowResize(Uint32 Width, Uint32 Height)
    {
        if (m_pSwapChain)
            m_pSwapChain->Resize(Width, Height);
    }

    RENDER_DEVICE_TYPE GetDeviceType() const { return m_DeviceType; }

	void CreatePipelineState();
	void CreateVertexBuffer();
	void CreateIndexBuffer();

private:
	RefCntAutoPtr<IPipelineState>         m_pPSO;
	RefCntAutoPtr<IShaderResourceBinding> m_pSRB;
	RefCntAutoPtr<IBuffer>                m_CubeVertexBuffer;
	RefCntAutoPtr<IBuffer>                m_CubeIndexBuffer;
	RefCntAutoPtr<IBuffer>                m_VSConstants;
	float4x4                              m_WorldViewProjMatrix;
	RefCntAutoPtr<IEngineFactory>         m_pEngineFactory;

    RefCntAutoPtr<IRenderDevice>  m_pDevice;
    RefCntAutoPtr<IDeviceContext> m_pImmediateContext;
    RefCntAutoPtr<ISwapChain>     m_pSwapChain;
	std::vector< RefCntAutoPtr<ITextureView> >  m_rpEyeSwapchainViews[2];
    RENDER_DEVICE_TYPE            m_DeviceType = RENDER_DEVICE_TYPE_D3D11;

    XrInstance m_instance;
    XrSystemId m_systemId;
    XrSession m_session;
    XrViewConfigurationView m_views[ 2 ] = {};
    XrSwapchain m_swapchain = XR_NULL_HANDLE;
};

void HelloXrApp::CreatePipelineState()
{
	// Pipeline state object encompasses configuration of all GPU stages

	GraphicsPipelineStateCreateInfo PSOCreateInfo;

	// Pipeline state name is used by the engine to report issues.
	// It is always a good idea to give objects descriptive names.
	PSOCreateInfo.PSODesc.Name = "Cube PSO";

	// This is a graphics pipeline
	PSOCreateInfo.PSODesc.PipelineType = PIPELINE_TYPE_GRAPHICS;

	// clang-format off
	// This tutorial will render to a single render target
	PSOCreateInfo.GraphicsPipeline.NumRenderTargets = 1;
	// Set render target format which is the format of the swap chain's color buffer
	PSOCreateInfo.GraphicsPipeline.RTVFormats[ 0 ] = m_pSwapChain->GetDesc().ColorBufferFormat;
	// Set depth buffer format which is the format of the swap chain's back buffer
	PSOCreateInfo.GraphicsPipeline.DSVFormat = m_pSwapChain->GetDesc().DepthBufferFormat;
	// Primitive topology defines what kind of primitives will be rendered by this pipeline state
	PSOCreateInfo.GraphicsPipeline.PrimitiveTopology = PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	// Cull back faces
	PSOCreateInfo.GraphicsPipeline.RasterizerDesc.CullMode = CULL_MODE_BACK;
	// Enable depth testing
	PSOCreateInfo.GraphicsPipeline.DepthStencilDesc.DepthEnable = True;
	// clang-format on

	ShaderCreateInfo ShaderCI;
	// Tell the system that the shader source code is in HLSL.
	// For OpenGL, the engine will convert this into GLSL under the hood.
	ShaderCI.SourceLanguage = SHADER_SOURCE_LANGUAGE_HLSL;

	// OpenGL backend requires emulated combined HLSL texture samplers (g_Texture + g_Texture_sampler combination)
	ShaderCI.UseCombinedTextureSamplers = true;

	// In this tutorial, we will load shaders from file. To be able to do that,
	// we need to create a shader source stream factory
	RefCntAutoPtr<IShaderSourceInputStreamFactory> pShaderSourceFactory;
	m_pEngineFactory->CreateDefaultShaderSourceStreamFactory( nullptr, &pShaderSourceFactory );
	ShaderCI.pShaderSourceStreamFactory = pShaderSourceFactory;
	// Create a vertex shader
	RefCntAutoPtr<IShader> pVS;
	{
		ShaderCI.Desc.ShaderType = SHADER_TYPE_VERTEX;
		ShaderCI.EntryPoint = "main";
		ShaderCI.Desc.Name = "Cube VS";
		ShaderCI.FilePath = "cube.vsh";
		m_pDevice->CreateShader( ShaderCI, &pVS );
		// Create dynamic uniform buffer that will store our transformation matrix
		// Dynamic buffers can be frequently updated by the CPU
		BufferDesc CBDesc;
		CBDesc.Name = "VS constants CB";
		CBDesc.uiSizeInBytes = sizeof( float4x4 );
		CBDesc.Usage = USAGE_DYNAMIC;
		CBDesc.BindFlags = BIND_UNIFORM_BUFFER;
		CBDesc.CPUAccessFlags = CPU_ACCESS_WRITE;
		m_pDevice->CreateBuffer( CBDesc, nullptr, &m_VSConstants );
	}

	// Create a pixel shader
	RefCntAutoPtr<IShader> pPS;
	{
		ShaderCI.Desc.ShaderType = SHADER_TYPE_PIXEL;
		ShaderCI.EntryPoint = "main";
		ShaderCI.Desc.Name = "Cube PS";
		ShaderCI.FilePath = "cube.psh";
		m_pDevice->CreateShader( ShaderCI, &pPS );
	}

	// clang-format off
	// Define vertex shader input layout
	LayoutElement LayoutElems[] =
	{
		// Attribute 0 - vertex position
		LayoutElement{0, 0, 3, VT_FLOAT32, False},
		// Attribute 1 - vertex color
		LayoutElement{1, 0, 4, VT_FLOAT32, False}
	};
	// clang-format on
	PSOCreateInfo.GraphicsPipeline.InputLayout.LayoutElements = LayoutElems;
	PSOCreateInfo.GraphicsPipeline.InputLayout.NumElements = _countof( LayoutElems );

	PSOCreateInfo.pVS = pVS;
	PSOCreateInfo.pPS = pPS;

	// Define variable type that will be used by default
	PSOCreateInfo.PSODesc.ResourceLayout.DefaultVariableType = SHADER_RESOURCE_VARIABLE_TYPE_STATIC;

	m_pDevice->CreateGraphicsPipelineState( PSOCreateInfo, &m_pPSO );

	// Since we did not explcitly specify the type for 'Constants' variable, default
	// type (SHADER_RESOURCE_VARIABLE_TYPE_STATIC) will be used. Static variables never
	// change and are bound directly through the pipeline state object.
	m_pPSO->GetStaticVariableByName( SHADER_TYPE_VERTEX, "Constants" )->Set( m_VSConstants );

	// Create a shader resource binding object and bind all static resources in it
	m_pPSO->CreateShaderResourceBinding( &m_pSRB, true );
}

void HelloXrApp::CreateVertexBuffer()
{
	// Layout of this structure matches the one we defined in the pipeline state
	struct Vertex
	{
		float3 pos;
		float4 color;
	};

	// Cube vertices

	//      (-1,+1,+1)________________(+1,+1,+1)
	//               /|              /|
	//              / |             / |
	//             /  |            /  |
	//            /   |           /   |
	//(-1,-1,+1) /____|__________/(+1,-1,+1)
	//           |    |__________|____|
	//           |   /(-1,+1,-1) |    /(+1,+1,-1)
	//           |  /            |   /
	//           | /             |  /
	//           |/              | /
	//           /_______________|/
	//        (-1,-1,-1)       (+1,-1,-1)
	//

	// clang-format off
	Vertex CubeVerts[ 8 ] =
	{
		{float3( -1,-1,-1 ), float4( 1,0,0,1 )},
		{float3( -1,+1,-1 ), float4( 0,1,0,1 )},
		{float3( +1,+1,-1 ), float4( 0,0,1,1 )},
		{float3( +1,-1,-1 ), float4( 1,1,1,1 )},

		{float3( -1,-1,+1 ), float4( 1,1,0,1 )},
		{float3( -1,+1,+1 ), float4( 0,1,1,1 )},
		{float3( +1,+1,+1 ), float4( 1,0,1,1 )},
		{float3( +1,-1,+1 ), float4( 0.2f,0.2f,0.2f,1 )},
	};
	// clang-format on

	// Create a vertex buffer that stores cube vertices
	BufferDesc VertBuffDesc;
	VertBuffDesc.Name = "Cube vertex buffer";
	VertBuffDesc.Usage = USAGE_IMMUTABLE;
	VertBuffDesc.BindFlags = BIND_VERTEX_BUFFER;
	VertBuffDesc.uiSizeInBytes = sizeof( CubeVerts );
	BufferData VBData;
	VBData.pData = CubeVerts;
	VBData.DataSize = sizeof( CubeVerts );
	m_pDevice->CreateBuffer( VertBuffDesc, &VBData, &m_CubeVertexBuffer );
}

void HelloXrApp::CreateIndexBuffer()
{
	// clang-format off
	Uint32 Indices[] =
	{
		2,0,1, 2,3,0,
		4,6,5, 4,7,6,
		0,7,4, 0,3,7,
		1,0,4, 1,4,5,
		1,5,2, 5,6,2,
		3,6,7, 3,2,6
	};
	// clang-format on

	BufferDesc IndBuffDesc;
	IndBuffDesc.Name = "Cube index buffer";
	IndBuffDesc.Usage = USAGE_IMMUTABLE;
	IndBuffDesc.BindFlags = BIND_INDEX_BUFFER;
	IndBuffDesc.uiSizeInBytes = sizeof( Indices );
	BufferData IBData;
	IBData.pData = Indices;
	IBData.DataSize = sizeof( Indices );
	m_pDevice->CreateBuffer( IndBuffDesc, &IBData, &m_CubeIndexBuffer );
}

// Render a frame
void HelloXrApp::Render()
{
	auto* pRTV = m_pSwapChain->GetCurrentBackBufferRTV();
	auto* pDSV = m_pSwapChain->GetDepthBufferDSV();
	// Clear the back buffer
	const float ClearColor[] = { 0.350f, 0.350f, 0.350f, 1.0f };
    m_pImmediateContext->SetRenderTargets( 1, &pRTV, pDSV, RESOURCE_STATE_TRANSITION_MODE_TRANSITION );
	m_pImmediateContext->ClearRenderTarget( pRTV, ClearColor, RESOURCE_STATE_TRANSITION_MODE_TRANSITION );
	m_pImmediateContext->ClearDepthStencil( pDSV, CLEAR_DEPTH_FLAG, 1.f, 0, RESOURCE_STATE_TRANSITION_MODE_TRANSITION );

	{
		// Map the buffer and write current world-view-projection matrix
		MapHelper<float4x4> CBConstants( m_pImmediateContext, m_VSConstants, MAP_WRITE, MAP_FLAG_DISCARD );
		*CBConstants = m_WorldViewProjMatrix.Transpose();
	}

	// Bind vertex and index buffers
	Uint32   offset = 0;
	IBuffer* pBuffs[] = { m_CubeVertexBuffer };
	m_pImmediateContext->SetVertexBuffers( 0, 1, pBuffs, &offset, RESOURCE_STATE_TRANSITION_MODE_TRANSITION, SET_VERTEX_BUFFERS_FLAG_RESET );
	m_pImmediateContext->SetIndexBuffer( m_CubeIndexBuffer, 0, RESOURCE_STATE_TRANSITION_MODE_TRANSITION );

	// Set the pipeline state
	m_pImmediateContext->SetPipelineState( m_pPSO );
	// Commit shader resources. RESOURCE_STATE_TRANSITION_MODE_TRANSITION mode
	// makes sure that resources are transitioned to required states.
	m_pImmediateContext->CommitShaderResources( m_pSRB, RESOURCE_STATE_TRANSITION_MODE_TRANSITION );

	DrawIndexedAttribs DrawAttrs;     // This is an indexed draw call
	DrawAttrs.IndexType = VT_UINT32; // Index type
	DrawAttrs.NumIndices = 36;
	// Verify the state of vertex and index buffers
	DrawAttrs.Flags = DRAW_FLAG_VERIFY_ALL;
	m_pImmediateContext->DrawIndexed( DrawAttrs );
}


void HelloXrApp::Update( double CurrTime, double ElapsedTime )
{
	// Apply rotation
	float4x4 CubeModelTransform = float4x4::RotationY( static_cast<float>( CurrTime ) * 1.0f ) * float4x4::RotationX( -PI_F * 0.1f );

	// Camera is at (0, 0, -5) looking along the Z axis
	float4x4 View = float4x4::Translation( 0.f, 0.0f, 5.0f );

	// Get pretransform matrix that rotates the scene according the surface orientation
    auto SrfPreTransform = float4x4::Identity(); // GetSurfacePretransformMatrix( float3 { 0, 0, 1 } );

	// Get projection matrix adjusted to the current screen orientation
    auto Proj = float4x4::Projection( 90, 16.f / 9.f, 0.01f, 10.f, false );

	// Compute world-view-projection matrix
	m_WorldViewProjMatrix = CubeModelTransform * View * SrfPreTransform * Proj;
}


std::unique_ptr<HelloXrApp> g_pTheApp;

LRESULT CALLBACK MessageProc(HWND, UINT, WPARAM, LPARAM);
// Main
int WINAPI WinMain(HINSTANCE instance, HINSTANCE, LPSTR, int cmdShow)
{
#if defined(_DEBUG) || defined(DEBUG)
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif

    g_pTheApp.reset(new HelloXrApp);

    const auto* cmdLine = GetCommandLineA();
    if (!g_pTheApp->ProcessCommandLine(cmdLine))
        return -1;

    std::wstring Title(L"Tutorial00: Hello Win32");
    switch (g_pTheApp->GetDeviceType())
    {
        case RENDER_DEVICE_TYPE_D3D11: Title.append(L" (D3D11)"); break;
        case RENDER_DEVICE_TYPE_D3D12: Title.append(L" (D3D12)"); break;
        case RENDER_DEVICE_TYPE_GL: Title.append(L" (GL)"); break;
        case RENDER_DEVICE_TYPE_VULKAN: Title.append(L" (VK)"); break;
    }
    // Register our window class
    WNDCLASSEX wcex = {sizeof(WNDCLASSEX), CS_HREDRAW | CS_VREDRAW, MessageProc,
                       0L, 0L, instance, NULL, NULL, NULL, NULL, L"SampleApp", NULL};
    RegisterClassEx(&wcex);

    // Create a window
    LONG WindowWidth  = 1280;
    LONG WindowHeight = 1024;
    RECT rc           = {0, 0, WindowWidth, WindowHeight};
    AdjustWindowRect(&rc, WS_OVERLAPPEDWINDOW, FALSE);
    HWND wnd = CreateWindow(L"SampleApp", Title.c_str(),
                            WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT,
                            rc.right - rc.left, rc.bottom - rc.top, NULL, NULL, instance, NULL);
    if (!wnd)
    {
        MessageBox(NULL, L"Cannot create window", L"Error", MB_OK | MB_ICONERROR);
        return 0;
    }
    ShowWindow(wnd, cmdShow);
    UpdateWindow(wnd);

    if (!g_pTheApp->Initialize(wnd))
        return -1;

	Diligent::Timer Timer;

	auto   PrevTime = Timer.GetElapsedTime();

    // Main message loop
    MSG msg = {0};
    while (WM_QUIT != msg.message)
    {
        if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        else
        {
			auto CurrTime = Timer.GetElapsedTime();
			auto ElapsedTime = CurrTime - PrevTime;
			PrevTime = CurrTime;
            g_pTheApp->Update( CurrTime, ElapsedTime );

            g_pTheApp->Render();
            g_pTheApp->Present();
        }
    }

    g_pTheApp.reset();

    return (int)msg.wParam;
}

// Called every time the NativeNativeAppBase receives a message
LRESULT CALLBACK MessageProc(HWND wnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
        case WM_PAINT:
        {
            PAINTSTRUCT ps;
            BeginPaint(wnd, &ps);
            EndPaint(wnd, &ps);
            return 0;
        }
        case WM_SIZE: // Window size has been changed
            if (g_pTheApp)
            {
                g_pTheApp->WindowResize(LOWORD(lParam), HIWORD(lParam));
            }
            return 0;

        case WM_CHAR:
            if (wParam == VK_ESCAPE)
                PostQuitMessage(0);
            return 0;

        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;

        case WM_GETMINMAXINFO:
        {
            LPMINMAXINFO lpMMI = (LPMINMAXINFO)lParam;

            lpMMI->ptMinTrackSize.x = 320;
            lpMMI->ptMinTrackSize.y = 240;
            return 0;
        }

        default:
            return DefWindowProc(wnd, message, wParam, lParam);
    }
}

// TODO:
// - Vulkan device creation needs to happen in the runtime
// - D3D12 session creation needs to include a command queue
