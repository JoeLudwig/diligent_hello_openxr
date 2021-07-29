cmake_minimum_required (VERSION 3.6)

add_library(xrbase 
		public/graphics_utilities.h
		public/iapp.h
		src/igraphicsbinding.cpp
		public/igraphicsbinding.h
		src/graphicsbinding_d3d11.cpp
		src/graphicsbinding_d3d11.h
		src/graphicsbinding_d3d12.cpp
		src/graphicsbinding_d3d12.h
		src/xrappbase.cpp
		public/xrappbase.h
		src/actions.cpp
		public/actions.h
		src/paths.cpp
		public/paths.h
)

target_compile_definitions( xrbase 
	PUBLIC 
		_SILENCE_CXX17_CODECVT_HEADER_DEPRECATION_WARNING
		NOMINMAX
		UNICODE
		ENGINE_DLL
		)

target_include_directories(xrbase 
PUBLIC
	"../../thirdparty/DiligentEngine/DiligentCore"
	"../../thirdparty/DiligentEngine/DiligentFx"
	"../../thirdparty/DiligentEngine/DiligentTools"
	"../../thirdparty/OpenXR-SDK/include"

	# for VulkanHeaders
	"../../thirdparty/DiligentEngine/DiligentCore/ThirdParty/Vulkan-Headers/include"

	"public"
)

add_dependencies( xrbase openxr_loader )

target_link_libraries( xrbase
PUBLIC
	Diligent-GraphicsEngineD3D11-shared
	Diligent-GraphicsEngineOpenGL-shared
	Diligent-GraphicsEngineD3D12-shared
#	Diligent-GraphicsEngineVk-shared
	Diligent-Win32Platform
	Diligent-Common
	DiligentFX
	openxr_loader
	DXGI
)

