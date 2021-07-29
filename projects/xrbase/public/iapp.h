#pragma once

#include <windows.h>
#include <memory>
#include <string>

class IApp
{
public:
	virtual ~IApp() {}

	virtual bool ProcessCommandLine( const std::string& commandLine ) = 0;
	virtual std::string GetWindowName() = 0;
	virtual bool Initialize( HWND hwnd ) = 0;
	virtual void WindowResize( uint32_t width, uint32_t height ) = 0;
	virtual void RunMainFrame() = 0;

};

// the actual app CPP needs to define this
std::unique_ptr<IApp> CreateApp();

