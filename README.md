# Description
Minimal project using DiligentEngine and OpenXR

# To build
```
git clone --recurse-submodules https://github.com/JoeLudwig/diligent_hello_openxr.git [<dirname>]
cd <dirname>
mkdir build
cd build
cmake -G "Visual Studio 16 2019" -A x64 ..
start DiligentHelloOpenXR.sln
```

and then build in Visual Studio

# To run
To run in the debugger, select the **HelloDiligentXr** project. The working directory needs to be the assets directory under that project.

Add a command line argument to select the graphics API to use:
* -mode D3D11
* -mode D3D12
* -mode Vulkan (This one isn't implemented yet.)

# What works so far?
D3D11 and D3D12 on Windows.

Additional features I would like to add:
* Linux support
* Vulkan support

Merge requests for any of the above would be welcomed. I'm happy to hear about other stuff that could be added, but I'd like to keep this sample fairly minimal.


