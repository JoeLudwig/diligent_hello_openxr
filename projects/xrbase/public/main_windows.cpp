#include <Windows.h>

#include <locale>
#include <codecvt>

#include "iapp.h"

std::unique_ptr<IApp> g_pTheApp;

LRESULT CALLBACK MessageProc( HWND, UINT, WPARAM, LPARAM );
// Main
int WINAPI WinMain( HINSTANCE instance, HINSTANCE, LPSTR, int cmdShow )
{
#if defined(_DEBUG) || defined(DEBUG)
	_CrtSetDbgFlag( _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF );
#endif

	g_pTheApp = CreateApp();

	const auto* cmdLine = GetCommandLineA();
	if ( !g_pTheApp->ProcessCommandLine( cmdLine ) )
		return -1;

	std::string title = g_pTheApp->GetWindowName();
	std::wstring_convert<std::codecvt_utf8_utf16<char16_t>, char16_t> convert;
	std::u16string wideTitle = convert.from_bytes( title );

	// Register our window class
	WNDCLASSEX wcex = { sizeof( WNDCLASSEX ), CS_HREDRAW | CS_VREDRAW, MessageProc,
					   0L, 0L, instance, NULL, NULL, NULL, NULL, L"SampleApp", NULL };
	RegisterClassEx( &wcex );

	// Create a window
	LONG WindowWidth = 1280;
	LONG WindowHeight = 1024;
	RECT rc = { 0, 0, WindowWidth, WindowHeight };
	AdjustWindowRect( &rc, WS_OVERLAPPEDWINDOW, FALSE );
	HWND wnd = CreateWindow( L"SampleApp", (LPCWSTR)wideTitle.c_str(),
		WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT,
		rc.right - rc.left, rc.bottom - rc.top, NULL, NULL, instance, NULL );
	if ( !wnd )
	{
		MessageBox( NULL, L"Cannot create window", L"Error", MB_OK | MB_ICONERROR );
		return 0;
	}
	ShowWindow( wnd, cmdShow );
	UpdateWindow( wnd );

	if ( !g_pTheApp->Initialize( wnd ) )
		return -1;

	// Main message loop
	MSG msg = { 0 };
	while ( WM_QUIT != msg.message )
	{
		if ( PeekMessage( &msg, NULL, 0, 0, PM_REMOVE ) )
		{
			TranslateMessage( &msg );
			DispatchMessage( &msg );
		}
		else
		{
			g_pTheApp->RunMainFrame();
		}
	}

	g_pTheApp.reset();

	return (int)msg.wParam;
}

// Called every time the NativeNativeAppBase receives a message
LRESULT CALLBACK MessageProc( HWND wnd, UINT message, WPARAM wParam, LPARAM lParam )
{
	switch ( message )
	{
	case WM_PAINT:
	{
		PAINTSTRUCT ps;
		BeginPaint( wnd, &ps );
		EndPaint( wnd, &ps );
		return 0;
	}
	case WM_SIZE: // Window size has been changed
		if ( g_pTheApp )
		{
			g_pTheApp->WindowResize( LOWORD( lParam ), HIWORD( lParam ) );
		}
		return 0;

	case WM_CHAR:
		if ( wParam == VK_ESCAPE )
			PostQuitMessage( 0 );
		return 0;

	case WM_DESTROY:
		PostQuitMessage( 0 );
		return 0;

	case WM_GETMINMAXINFO:
	{
		LPMINMAXINFO lpMMI = (LPMINMAXINFO)lParam;

		lpMMI->ptMinTrackSize.x = 320;
		lpMMI->ptMinTrackSize.y = 240;
		return 0;
	}

	default:
		return DefWindowProc( wnd, message, wParam, lParam );
	}
}
