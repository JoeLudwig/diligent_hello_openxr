cmake_minimum_required (VERSION 3.6)

add_executable(HelloDiligentXr WIN32 helloxr.cpp)
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
#    Diligent-GraphicsEngineVk-shared
    openxr_loader
)
copy_required_dlls(HelloDiligentXr)
