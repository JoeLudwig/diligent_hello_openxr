cmake_minimum_required (VERSION 3.6)

add_executable(HelloDiligentXr 
	WIN32 
		helloxr.cpp
		graphics_utilities.h
		iapp.h
		igraphicsbinding.cpp
		igraphicsbinding.h
		graphicsbinding_d3d11.cpp
		graphicsbinding_d3d11.h
		graphicsbinding_d3d12.cpp
		graphicsbinding_d3d12.h
		main_windows.cpp
)
set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT bar)

target_compile_definitions( HelloDiligentXr PRIVATE _SILENCE_CXX17_CODECVT_HEADER_DEPRECATION_WARNING )

target_compile_options(HelloDiligentXr PRIVATE -DUNICODE -DENGINE_DLL)
target_include_directories(HelloDiligentXr 
PRIVATE 
	"../thirdparty/DiligentEngine/DiligentCore"
	"../thirdparty/OpenXR-SDK/include"

	# for VulkanHeaders
	"../thirdparty/DiligentEngine/DiligentCore/ThirdParty/Vulkan-Headers/include"
)

add_dependencies( HelloDiligentXr openxr_loader )

target_link_libraries(HelloDiligentXr
PRIVATE
	Diligent-GraphicsEngineD3D11-shared
	Diligent-GraphicsEngineOpenGL-shared
	Diligent-GraphicsEngineD3D12-shared
#	Diligent-GraphicsEngineVk-shared
	Diligent-Win32Platform
	Diligent-Common
	openxr_loader
	DXGI
)

if(PLATFORM_WIN32 OR PLATFORM_LINUX)
	# Copy assets to target folder
	add_custom_command(TARGET HelloDiligentXr POST_BUILD
		COMMAND ${CMAKE_COMMAND} -E copy_directory
			"${CMAKE_CURRENT_SOURCE_DIR}/assets"
			"\"$<TARGET_FILE_DIR:HelloDiligentXr>\"")
endif()

if( PLATFORM_WIN32 )
	set_target_properties(HelloDiligentXr PROPERTIES 
		VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/assets"
	)
endif()

copy_required_dlls(HelloDiligentXr)
