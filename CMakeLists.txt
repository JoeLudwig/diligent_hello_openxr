cmake_minimum_required ( VERSION 3.14)
project( DiligentHelloOpenXR VERSION 0.1.0
					DESCRIPTION "Hello World project using DiligentEngine and OpenXR"
					LANGUAGES CXX)
set(NAME DiligentHelloOpenXR)

add_subdirectory( thirdparty/DiligentEngine/DiligentCore )
add_subdirectory( thirdparty/DiligentEngine/DiligentTools )
add_subdirectory( thirdparty/DiligentEngine/DiligentFx )
add_subdirectory( thirdparty/OpenXR-SDK )

add_compile_options( "/std:c++17" )

add_subdirectory( projects )

