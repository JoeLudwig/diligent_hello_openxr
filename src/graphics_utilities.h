#pragma once

// float4x4_createProjection was adapted from https://github.com/KhronosGroup/OpenXR-SDK-Source/blob/master/src/common/xr_linear.h


#include <Common/interface/BasicMath.hpp>
#include <Graphics/GraphicsEngine/interface/GraphicsTypes.h>

// Creates a projection matrix based on the specified dimensions.
// The projection matrix transforms -Z=forward, +Y=up, +X=right to the appropriate clip space for the graphics API.
// The far plane is placed at infinity if farZ <= nearZ.
// An infinite projection matrix is preferred for rasterization because, except for
// things *right* up against the near plane, it always provides better precision:
//              "Tightening the Precision of Perspective Rendering"
//              Paul Upchurch, Mathieu Desbrun
//              Journal of Graphics Tools, Volume 16, Issue 1, 2012
inline static void float4x4_CreateProjection( Diligent::float4x4* result, Diligent::RENDER_DEVICE_TYPE graphicsApi, const float tanAngleLeft,
	const float tanAngleRight, const float tanAngleUp, float const tanAngleDown,
	const float nearZ, const float farZ ) 
{
	const float tanAngleWidth = tanAngleRight - tanAngleLeft;

	// Set to tanAngleDown - tanAngleUp for a clip space with positive Y down (Vulkan).
	// Set to tanAngleUp - tanAngleDown for a clip space with positive Y up (OpenGL / D3D / Metal).
	const float tanAngleHeight = graphicsApi == Diligent::RENDER_DEVICE_TYPE_VULKAN ? ( tanAngleDown - tanAngleUp ) : ( tanAngleUp - tanAngleDown );

	// Set to nearZ for a [-1,1] Z clip space (OpenGL / OpenGL ES).
	// Set to zero for a [0,1] Z clip space (Vulkan / D3D / Metal).
	const float offsetZ = ( graphicsApi == Diligent::RENDER_DEVICE_TYPE_GL || graphicsApi == Diligent::RENDER_DEVICE_TYPE_GLES ) ? nearZ : 0;

	if ( farZ <= nearZ ) {
		// place the far plane at infinity
		result->m00 = 2.0f / tanAngleWidth;
		result->m10 = 0.0f;
		result->m20 = ( tanAngleRight + tanAngleLeft ) / tanAngleWidth;
		result->m30 = 0.0f;

		result->m01 = 0.0f;
		result->m11 = 2.0f / tanAngleHeight;
		result->m21 = ( tanAngleUp + tanAngleDown ) / tanAngleHeight;
		result->m31 = 0.0f;

		result->m02 = 0.0f;
		result->m12 = 0.0f;
		result->m22 = -1.0f;
		result->m32 = -( nearZ + offsetZ );

		result->m03 = 0.0f;
		result->m13 = 0.0f;
		result->m23 = -1.0f;
		result->m33 = 0.0f;
	}
	else 
	{
		// normal projection
		result->m00 = 2.0f / tanAngleWidth;
		result->m10 = 0.0f;
		result->m20 = ( tanAngleRight + tanAngleLeft ) / tanAngleWidth;
		result->m30 = 0.0f;

		result->m01 = 0.0f;
		result->m11 = 2.0f / tanAngleHeight;
		result->m21 = ( tanAngleUp + tanAngleDown ) / tanAngleHeight;
		result->m31 = 0.0f;

		result->m02 = 0.0f;
		result->m12 = 0.0f;
		result->m22 = -( farZ + offsetZ ) / ( farZ - nearZ );
		result->m32 = -( farZ * ( nearZ + offsetZ ) ) / ( farZ - nearZ );

		result->m03 = 0.0f;
		result->m13 = 0.0f;
		result->m23 = -1.0f;
		result->m33 = 0.0f;
	}
}

// Creates a projection matrix based on the specified FOV.
inline static void float4x4_CreateProjection( Diligent::float4x4* result, Diligent::RENDER_DEVICE_TYPE graphicsApi, const XrFovf fov,
	const float nearZ, const float farZ ) 
{
	const float tanLeft = tanf( fov.angleLeft );
	const float tanRight = tanf( fov.angleRight );

	const float tanDown = tanf( fov.angleDown );
	const float tanUp = tanf( fov.angleUp );

	float4x4_CreateProjection( result, graphicsApi, tanLeft, tanRight, tanUp, tanDown, nearZ, farZ );
}

inline Diligent::Quaternion quaternionFromXrQuaternion( const XrQuaternionf & in )
{
	return Diligent::Quaternion( in.x, in.y, in.z, in.w );
}

inline Diligent::float3 vectorFromXrVector( const XrVector3f& in )
{
	return Diligent::float3( in.x, in.y, in.z );
}

inline XrPosef IdentityXrPose()
{
	XrPosef pose;
	pose.orientation = { 0, 0, 0, 1.f };
	pose.position = { 0, 0, 0 };
	return pose;
}

