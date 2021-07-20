#include "igraphicsbinding.h"

#include "graphicsbinding_d3d11.h"
#include "graphicsbinding_d3d12.h"

using namespace Diligent;

std::unique_ptr<IGraphicsBinding> IGraphicsBinding::CreateBindingForDeviceType( Diligent::RENDER_DEVICE_TYPE deviceType )
{
	std::unique_ptr< IGraphicsBinding > binding;
	switch ( deviceType )
	{
	case RENDER_DEVICE_TYPE_D3D11:
		binding = std::make_unique<GraphicsBinding_D3D11>();
		break;

	case RENDER_DEVICE_TYPE_D3D12:
		binding = std::make_unique<GraphicsBinding_D3D12>();
		break;

	default:
		return nullptr;
	}

	return binding;
};

