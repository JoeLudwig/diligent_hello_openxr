cmake_minimum_required (VERSION 3.6)

add_executable(HelloDiligentXr WIN32 helloxr.cpp)
target_compile_options(HelloDiligentXr PRIVATE -DUNICODE -DENGINE_DLL)
target_include_directories(HelloDiligentXr PRIVATE "../thirdparty/DiligentEngine/DiligentCore")

target_link_libraries(HelloDiligentXr
PRIVATE
#    Diligent-GraphicsEngineD3D11-shared
#    Diligent-GraphicsEngineOpenGL-shared
#    Diligent-GraphicsEngineD3D12-shared
    Diligent-GraphicsEngineVk-shared
)
copy_required_dlls(HelloDiligentXr)
