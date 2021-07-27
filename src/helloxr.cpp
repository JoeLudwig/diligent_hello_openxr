/*
 *  Copyright 2019-2021 Diligent Graphics LLC
 *  Copyright 2015-2019 Egor Yusov
 *  
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *  
 *	  http://www.apache.org/licenses/LICENSE-2.0
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

#include "xrappbase.h"

#include <memory>
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

#include <Graphics/GraphicsEngine/interface/EngineFactory.h>

#include "Graphics/GraphicsEngineD3D11/interface/EngineFactoryD3D11.h"
#include "Graphics/GraphicsEngineD3D12/interface/EngineFactoryD3D12.h"
#include "Graphics/GraphicsEngineOpenGL/interface/EngineFactoryOpenGL.h"
#include "Graphics/GraphicsEngineVulkan/interface/EngineFactoryVk.h"
#include "Common/interface/AdvancedMath.hpp"
#include "Common/interface/Timer.hpp"

#include "Graphics/GraphicsEngine/interface/RenderDevice.h"
#include "Graphics/GraphicsEngine/interface/DeviceContext.h"
#include "Graphics/GraphicsEngine/interface/SwapChain.h"

#include "Common/interface/RefCntAutoPtr.hpp"

#include "Graphics/GraphicsTools/interface/MapHelper.hpp"
#include "Graphics/GraphicsTools/interface/GraphicsUtilities.h"
#include <AssetLoader/interface/GLTFLoader.hpp>
#include "GLTF_PBR_Renderer.hpp"
#include <TextureLoader/interface/TextureUtilities.h>
#include "openxr/openxr.h"

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

#include "actions.h"
#include "paths.h"

namespace Diligent
{
#include <Shaders/Common/public/BasicStructures.fxh>
};

using namespace Diligent;


typedef std::map< std::string, uint32_t > XrExtensionMap;

class HelloXrApp: public XrAppBase
{
	typedef XrAppBase super;
public:
	HelloXrApp()
	{
	}

	virtual ~HelloXrApp()
	{
		if (m_pGraphicsBinding)
		{
			m_pGraphicsBinding->GetImmediateContext()->Flush();
		}
	}

	bool Initialize( HWND hWnd )
	{
		if ( !super::Initialize( hWnd ) )
			return false;

		CreatePipelineState();
		CreateVertexBuffer();
		CreateIndexBuffer();

		return true;
	}


	virtual void Render() override;
	virtual void Update( double currTime, double elapsedTime, XrTime displayTime ) override;
	virtual bool PreSession() override;
	virtual bool PostSession() override;
	virtual std::vector<std::string> GetDesiredExtensions();

	void CreatePipelineState();
	void CreateVertexBuffer();
	void CreateIndexBuffer();

	virtual bool RenderEye( int eye ) override;
	virtual void UpdateEyeTransforms( float4x4 eyeToProj, float4x4 stageToEye, XrView& view ) override;

private:
	RefCntAutoPtr<IPipelineState>		 m_pPSO;
	RefCntAutoPtr<IShaderResourceBinding> m_pSRB;
	RefCntAutoPtr<IBuffer>				m_CubeVertexBuffer;
	RefCntAutoPtr<IBuffer>				m_CubeIndexBuffer;
	RefCntAutoPtr<IBuffer>				m_VSConstants;
	float4x4							  m_CubeToWorld;
	float4x4							  m_ViewToProj;
	float4x4							m_handCubeToWorld[ 2 ];
	bool								m_handCubeToWorldValid[ 2 ] = { false, false };
	bool								m_hideCube[ 2 ] = { false, false };

	std::unique_ptr< XRDE::ActionSet > m_handActionSet;
	XRDE::Action * m_handAction;
	XRDE::Action * m_hideCubeAction;
	XRDE::Action * m_hapticAction;

	std::unique_ptr<GLTF::Model> m_leftHandModel;
	std::unique_ptr<GLTF::Model> m_rightHandModel;
};


std::vector<std::string> HelloXrApp::GetDesiredExtensions() 
{ 
	return 
	{
		XR_EXT_HAND_TRACKING_EXTENSION_NAME,
		XR_EXT_HP_MIXED_REALITY_CONTROLLER_EXTENSION_NAME,
		XR_KHR_COMPOSITION_LAYER_DEPTH_EXTENSION_NAME,
	}; 
}


using namespace XRDE;

bool HelloXrApp::PreSession()
{
	XrPath leftHand = StringToPath( m_instance, k_userHandLeft );
	XrPath rightHand = StringToPath( m_instance, k_userHandRight );

	m_handActionSet = std::make_unique<ActionSet>( "hands", "Hands", 0 );

	m_handAction = m_handActionSet->AddAction( "handpose", "Hand Location", XR_ACTION_TYPE_POSE_INPUT, 
		std::vector( { leftHand, rightHand } ) );
	m_handAction->AddGlobalBinding( Paths().rightGripPose );
	m_handAction->AddGlobalBinding( Paths().leftGripPose );

	m_hideCubeAction = m_handActionSet->AddAction( "hidecube", "Hide Cube", XR_ACTION_TYPE_BOOLEAN_INPUT,
		std::vector( { leftHand, rightHand } ) );
	m_hideCubeAction->AddGlobalBinding( Paths().rightTrigger );
	m_hideCubeAction->AddGlobalBinding( Paths().leftTrigger );

	m_hapticAction = m_handActionSet->AddAction( "haptics", "Cube Haptics", XR_ACTION_TYPE_VIBRATION_OUTPUT,
		std::vector( { leftHand, rightHand } ) );
	m_hapticAction->AddGlobalBinding( Paths().rightHaptic);
	m_hapticAction->AddGlobalBinding( Paths().leftHaptic );

	CHECK_XR_RESULT( m_handActionSet->Init( m_instance ) );

	std::vector<const ActionSet*> actionSets(
		{
			&*m_handActionSet,
		} );

	CHECK_XR_RESULT( SuggestBindings( m_instance, Paths().interactionProfilesValveIndexController, actionSets ) );

	return true;
}


bool HelloXrApp::PostSession()
{
	CHECK_XR_RESULT( AttachActionSets( m_session, { &*m_handActionSet } ) );

	CHECK_XR_RESULT( m_handActionSet->SessionInit( m_session ) );

	SetPbrEnvironmentMap( "textures/papermill.ktx" );

	m_leftHandModel = LoadGltfModel( "models/skinned_hand_left.glb" );
	m_rightHandModel = LoadGltfModel( "models/skinned_hand_right.glb" );

	return true;
}

void HelloXrApp::UpdateEyeTransforms( float4x4 eyeToProj, float4x4 stageToEye, XrView& view )
{
	// Map the buffer and write current world-view-projection matrix
	MapHelper<float4x4> CBConstants( m_pGraphicsBinding->GetImmediateContext(), m_VSConstants, MAP_WRITE, MAP_FLAG_DISCARD );
	*CBConstants = ( m_CubeToWorld * stageToEye * eyeToProj ).Transpose();
	m_ViewToProj = eyeToProj;
};


bool HelloXrApp::RenderEye( int eye )
{
	// Bind vertex and index buffers
	Uint32   offset = 0;
	IBuffer* pBuffs[] = { m_CubeVertexBuffer };
	m_pGraphicsBinding->GetImmediateContext()->SetVertexBuffers( 0, 1, pBuffs, &offset, RESOURCE_STATE_TRANSITION_MODE_TRANSITION, SET_VERTEX_BUFFERS_FLAG_RESET );
	m_pGraphicsBinding->GetImmediateContext()->SetIndexBuffer( m_CubeIndexBuffer, 0, RESOURCE_STATE_TRANSITION_MODE_TRANSITION );

	// Set the pipeline state
	m_pGraphicsBinding->GetImmediateContext()->SetPipelineState( m_pPSO );
	// Commit shader resources. RESOURCE_STATE_TRANSITION_MODE_TRANSITION mode
	// makes sure that resources are transitioned to required states.
	m_pGraphicsBinding->GetImmediateContext()->CommitShaderResources( m_pSRB, RESOURCE_STATE_TRANSITION_MODE_TRANSITION );

	DrawIndexedAttribs DrawAttrs;	 // This is an indexed draw call
	DrawAttrs.IndexType = VT_UINT32; // Index type
	DrawAttrs.NumIndices = 36;
	// Verify the state of vertex and index buffers
	DrawAttrs.Flags = DRAW_FLAG_VERIFY_ALL;
	m_pGraphicsBinding->GetImmediateContext()->DrawIndexed( DrawAttrs );

	m_gltfRenderer->Begin( m_pGraphicsBinding->GetRenderDevice(), m_pGraphicsBinding->GetImmediateContext(),
		m_CacheUseInfo, m_CacheBindings, m_CameraAttribsCB, m_LightAttribsCB );

	// draw the hands if they're available
	for ( int cube = 0; cube < 2; cube++ )
	{
		if ( !m_handCubeToWorldValid[ cube ] )
			continue;

		if ( m_hideCube[ cube ] )
			continue;

		GLTF_PBR_Renderer::RenderInfo renderInfo;
		renderInfo.ModelTransform = /*float4x4::RotationX( (float) PI ) **/ float4x4::RotationY( (float)PI ) 
			* m_handCubeToWorld[ cube ];

		if ( cube == 0 )
		{
			m_gltfRenderer->Render( m_pGraphicsBinding->GetImmediateContext(), *m_leftHandModel, renderInfo,
				nullptr, &m_CacheBindings );
		}
		else
		{
			m_gltfRenderer->Render( m_pGraphicsBinding->GetImmediateContext(), *m_rightHandModel, renderInfo,
				nullptr, &m_CacheBindings );
		}
	}

	return true;
}


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
	PSOCreateInfo.GraphicsPipeline.RasterizerDesc.FrontCounterClockwise = true;
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
	m_pGraphicsBinding->GetEngineFactory()->CreateDefaultShaderSourceStreamFactory( nullptr, &pShaderSourceFactory );
	ShaderCI.pShaderSourceStreamFactory = pShaderSourceFactory;
	// Create a vertex shader
	RefCntAutoPtr<IShader> pVS;
	{
		ShaderCI.Desc.ShaderType = SHADER_TYPE_VERTEX;
		ShaderCI.EntryPoint = "main";
		ShaderCI.Desc.Name = "Cube VS";
		ShaderCI.FilePath = "cube.vsh";
		m_pGraphicsBinding->GetRenderDevice()->CreateShader( ShaderCI, &pVS );
		// Create dynamic uniform buffer that will store our transformation matrix
		// Dynamic buffers can be frequently updated by the CPU
		BufferDesc CBDesc;
		CBDesc.Name = "VS constants CB";
		CBDesc.uiSizeInBytes = sizeof( float4x4 );
		CBDesc.Usage = USAGE_DYNAMIC;
		CBDesc.BindFlags = BIND_UNIFORM_BUFFER;
		CBDesc.CPUAccessFlags = CPU_ACCESS_WRITE;
		m_pGraphicsBinding->GetRenderDevice()->CreateBuffer( CBDesc, nullptr, &m_VSConstants );
	}

	// Create a pixel shader
	RefCntAutoPtr<IShader> pPS;
	{
		ShaderCI.Desc.ShaderType = SHADER_TYPE_PIXEL;
		ShaderCI.EntryPoint = "main";
		ShaderCI.Desc.Name = "Cube PS";
		ShaderCI.FilePath = "cube.psh";
		m_pGraphicsBinding->GetRenderDevice()->CreateShader( ShaderCI, &pPS );
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

	m_pGraphicsBinding->GetRenderDevice()->CreateGraphicsPipelineState( PSOCreateInfo, &m_pPSO );

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

	//	  (-1,+1,+1)________________(+1,+1,+1)
	//			   /|			  /|
	//			  / |			 / |
	//			 /  |			/  |
	//			/   |		   /   |
	//(-1,-1,+1) /____|__________/(+1,-1,+1)
	//		   |	|__________|____|
	//		   |   /(-1,+1,-1) |	/(+1,+1,-1)
	//		   |  /			|   /
	//		   | /			 |  /
	//		   |/			  | /
	//		   /_______________|/
	//		(-1,-1,-1)	   (+1,-1,-1)
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
	m_pGraphicsBinding->GetRenderDevice()->CreateBuffer( VertBuffDesc, &VBData, &m_CubeVertexBuffer );
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
	m_pGraphicsBinding->GetRenderDevice()->CreateBuffer( IndBuffDesc, &IBData, &m_CubeIndexBuffer );
}

// Render a frame
void HelloXrApp::Render()
{
	auto* pRTV = m_pSwapChain->GetCurrentBackBufferRTV();
	auto* pDSV = m_pSwapChain->GetDepthBufferDSV();
	// Clear the back buffer
	const float ClearColor[] = { 0.350f, 0.350f, 0.350f, 1.0f };
	m_pGraphicsBinding->GetImmediateContext()->SetRenderTargets( 1, &pRTV, pDSV, RESOURCE_STATE_TRANSITION_MODE_TRANSITION );
	m_pGraphicsBinding->GetImmediateContext()->ClearRenderTarget( pRTV, ClearColor, RESOURCE_STATE_TRANSITION_MODE_TRANSITION );
	m_pGraphicsBinding->GetImmediateContext()->ClearDepthStencil( pDSV, CLEAR_DEPTH_FLAG, 1.f, 0, RESOURCE_STATE_TRANSITION_MODE_TRANSITION );

	float4x4 stageToDesktopView = float4x4::Translation( 0.f, 0.0f, -2.5f );
	{
		// Map the buffer and write current world-view-projection matrix
		MapHelper<float4x4> CBConstants( m_pGraphicsBinding->GetImmediateContext(), m_VSConstants, MAP_WRITE, MAP_FLAG_DISCARD );
		*CBConstants = ( m_CubeToWorld * stageToDesktopView * m_ViewToProj ).Transpose();
	}

	// Bind vertex and index buffers
	Uint32   offset = 0;
	IBuffer* pBuffs[] = { m_CubeVertexBuffer };
	m_pGraphicsBinding->GetImmediateContext()->SetVertexBuffers( 0, 1, pBuffs, &offset, RESOURCE_STATE_TRANSITION_MODE_TRANSITION, SET_VERTEX_BUFFERS_FLAG_RESET );
	m_pGraphicsBinding->GetImmediateContext()->SetIndexBuffer( m_CubeIndexBuffer, 0, RESOURCE_STATE_TRANSITION_MODE_TRANSITION );

	// Set the pipeline state
	m_pGraphicsBinding->GetImmediateContext()->SetPipelineState( m_pPSO );
	// Commit shader resources. RESOURCE_STATE_TRANSITION_MODE_TRANSITION mode
	// makes sure that resources are transitioned to required states.
	m_pGraphicsBinding->GetImmediateContext()->CommitShaderResources( m_pSRB, RESOURCE_STATE_TRANSITION_MODE_TRANSITION );

	DrawIndexedAttribs DrawAttrs;	 // This is an indexed draw call
	DrawAttrs.IndexType = VT_UINT32; // Index type
	DrawAttrs.NumIndices = 36;
	// Verify the state of vertex and index buffers
	DrawAttrs.Flags = DRAW_FLAG_VERIFY_ALL;
	m_pGraphicsBinding->GetImmediateContext()->DrawIndexed( DrawAttrs );
}


void HelloXrApp::Update( double CurrTime, double ElapsedTime, XrTime displayTime )
{
	// read input
	XrActiveActionSet activeActionSets[] =
	{
		{ m_handActionSet->Handle(), Paths().userHandLeft },
		{ m_handActionSet->Handle(), Paths().userHandRight },
	};
	XrActionsSyncInfo syncInfo = { XR_TYPE_ACTIONS_SYNC_INFO };
	syncInfo.activeActionSets = activeActionSets;
	syncInfo.countActiveActionSets = sizeof( activeActionSets ) / sizeof( activeActionSets[ 0 ] );
	xrSyncActions( m_session, &syncInfo );

	XrSpaceLocation spaceLocation = { XR_TYPE_SPACE_LOCATION };
	if( XR_SUCCEEDED( m_handAction->LocateSpace( m_stageSpace, displayTime, Paths().userHandLeft, &spaceLocation ) ) 
		&& ( spaceLocation.locationFlags & XR_SPACE_LOCATION_POSITION_VALID_BIT ) != 0 )
	{
		m_handCubeToWorld[0] = matrixFromPose( spaceLocation.pose );
		m_handCubeToWorldValid[ 0 ] = true;
	}
	else
	{
		m_handCubeToWorldValid[ 0 ] = false;
	}
	if ( XR_SUCCEEDED( m_handAction->LocateSpace( m_stageSpace, displayTime, Paths().userHandRight, &spaceLocation ) )
		&& ( spaceLocation.locationFlags & XR_SPACE_LOCATION_POSITION_VALID_BIT ) != 0 )
	{
		m_handCubeToWorld[ 1 ] = matrixFromPose( spaceLocation.pose );
		m_handCubeToWorldValid[ 1 ] = true;
	}
	else
	{
		m_handCubeToWorldValid[ 1 ] = false;
	}

	bool oldHideCube[ 2 ] = { m_hideCube[ 0 ], m_hideCube[ 1 ] };
	m_hideCube[ 0 ] = m_hideCubeAction->GetBooleanState( m_session, Paths().userHandLeft );
	m_hideCube[ 1 ] = m_hideCubeAction->GetBooleanState( m_session, Paths().userHandRight );
	if ( oldHideCube[ 0 ] && !m_hideCube[ 0 ] )
	{
		m_hapticAction->ApplyHapticFeedback( m_session, Paths().userHandLeft, 3, 20, 1 );
	}
	else if ( !oldHideCube[ 0 ] && m_hideCube[ 0 ] )
	{
		m_hapticAction->StopApplyingHapticFeecback( m_session, Paths().userHandLeft );
	}
	if ( oldHideCube[ 1 ] && !m_hideCube[ 1 ] )
	{
		m_hapticAction->ApplyHapticFeedback( m_session, Paths().userHandRight, 3, 20, 1 );
	}
	else if ( !oldHideCube[ 1 ] && m_hideCube[ 1 ] )
	{
		m_hapticAction->StopApplyingHapticFeecback( m_session, Paths().userHandRight );
	}

	// Apply rotation
	m_CubeToWorld = float4x4::Scale( 0.5f ) 
		* float4x4::RotationY( static_cast<float>( CurrTime ) * 1.0f ) 
		* float4x4::RotationX( -PI_F * 0.1f );


}


std::unique_ptr<HelloXrApp> g_pTheApp;

std::unique_ptr<IApp> CreateApp()
{
	return std::make_unique<HelloXrApp>();
}

// TODO:
// - Vulkan device creation needs to happen in the runtime
// - D3D12 session creation needs to include a command queue
