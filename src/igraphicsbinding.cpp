#include "igraphicsbinding.h"

#include "graphicsbinding_d3d11.h"

using namespace Diligent;

std::unique_ptr<IGraphicsBinding> IGraphicsBinding::CreateBindingForDeviceType( Diligent::RENDER_DEVICE_TYPE deviceType,
	XrInstance instance, XrSystemId systemId )
{
	std::unique_ptr< IGraphicsBinding > binding;
	switch ( deviceType )
	{
	case RENDER_DEVICE_TYPE_D3D11:
		binding = std::make_unique<GraphicsBinding_D3D11>();
		break;

	default:
		return nullptr;
	}

	if ( !XR_SUCCEEDED( binding->Init( instance, systemId ) ) )
	{
		return nullptr;
	}

	return binding;
};

