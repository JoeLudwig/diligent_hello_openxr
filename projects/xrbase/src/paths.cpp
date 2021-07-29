#include "paths.h"

#include <vector>

using namespace XRDE;


XrPath XRDE::StringToPath( XrInstance instance, const std::string& pathString )
{
	XrPath path;
	if( XR_FAILED( xrStringToPath( instance, pathString.c_str(), &path ) ) )
		return XR_NULL_PATH;

	return path;
}

std::string XRDE::PathToString( XrInstance instance, XrPath path )
{
	if ( path == XR_NULL_PATH )
	{
		return "XR_NULL_PATH";
	}

	std::vector<char> buf;
	buf.resize( 128 );
	uint32_t requiredSize;
	if( XR_FAILED( xrPathToString( instance, path, (uint32_t)buf.size(), &requiredSize, &buf[0] ) ) )
		return "UNKNOWN";

	if ( requiredSize <= buf.size() )
	{
		return &buf[ 0 ];
	}

	buf.resize( requiredSize );
	if ( XR_FAILED( xrPathToString( instance, path, (uint32_t)buf.size(), &requiredSize, &buf[ 0 ] ) ) )
		return "UNKNOWN";

	if ( requiredSize <= buf.size() )
	{
		return &buf[ 0 ];
	}

	return "UNKNOWN";
}

static StandardPaths g_paths;

XrResult XRDE::InitPaths( XrInstance instance )
{
	g_paths.userHandLeft = StringToPath( instance, "/user/hand/left" );
	g_paths.userHandRight = StringToPath( instance, "/user/hand/right" );
	g_paths.userHandHead = StringToPath( instance, "/user/head" );
	g_paths.userHandGamepad = StringToPath( instance, "/user/gamepad" );
	g_paths.userHandTreadmill = StringToPath( instance, "/user/treadmill" );

	g_paths.interactionProfilesKHRSimpleController = StringToPath( instance, "/interaction_profiles/khr/simple_controller" );
	g_paths.interactionProfilesHPMixedRealityController = StringToPath( instance, "/interaction_profiles/hp/mixed_reality_controller" );
	g_paths.interactionProfilesHTCViveController = StringToPath( instance, "/interaction_profiles/htc/vive_controller" );
	g_paths.interactionProfilesHTCViveCosmosController = StringToPath( instance, "/interaction_profiles/htc/vive_cosmos_controller" );
	g_paths.interactionProfilesHTCVivePro = StringToPath( instance, "/interaction_profiles/htc/vive_pro" );
	g_paths.interactionProfilesMicrosoftMotionController = StringToPath( instance, "/interaction_profiles/microsoft/motion_controller" );
	g_paths.interactionProfilesMicrosoftXboxController = StringToPath( instance, "/interaction_profiles/microsoft/xbox_controller" );
	g_paths.interactionProfilesOculusTouchController = StringToPath( instance, "/interaction_profiles/oculus/touch_controller" );
	g_paths.interactionProfilesValveIndexController = StringToPath( instance, "/interaction_profiles/valve/index_controller" );

	g_paths.gamepadAClick = StringToPath( instance, "/user/gamepad/input/a/click" );
	g_paths.gamepadBClick = StringToPath( instance, "/user/gamepad/input/b/click" );
	g_paths.gamepadDpadDownClick = StringToPath( instance, "/user/gamepad/input/dpad_down/click" );
	g_paths.gamepadDpadLeftClick = StringToPath( instance, "/user/gamepad/input/dpad_left/click" );
	g_paths.gamepadDpadRightClick = StringToPath( instance, "/user/gamepad/input/dpad_right/click" );
	g_paths.gamepadDpadUpClick = StringToPath( instance, "/user/gamepad/input/dpad_up/click" );
	g_paths.gamepadMenuClick = StringToPath( instance, "/user/gamepad/input/menu/click" );
	g_paths.gamepadShoulderLeftClick = StringToPath( instance, "/user/gamepad/input/shoulder_left/click" );
	g_paths.gamepadShoulderRightClick = StringToPath( instance, "/user/gamepad/input/shoulder_right/click" );
	g_paths.gamepadThumbstickLeft = StringToPath( instance, "/user/gamepad/input/thumbstick_left" );
	g_paths.gamepadThumbstickLeftClick = StringToPath( instance, "/user/gamepad/input/thumbstick_left/click" );
	g_paths.gamepadThumbstickLeftX = StringToPath( instance, "/user/gamepad/input/thumbstick_left/x" );
	g_paths.gamepadThumbstickLeftY = StringToPath( instance, "/user/gamepad/input/thumbstick_left/y" );
	g_paths.gamepadThumbstickRight = StringToPath( instance, "/user/gamepad/input/thumbstick_right" );
	g_paths.gamepadThumbstickRightClick = StringToPath( instance, "/user/gamepad/input/thumbstick_right/click" );
	g_paths.gamepadThumbstickRightX = StringToPath( instance, "/user/gamepad/input/thumbstick_right/x" );
	g_paths.gamepadThumbstickRightY = StringToPath( instance, "/user/gamepad/input/thumbstick_right/y" );
	g_paths.gamepadTriggerLeftValue = StringToPath( instance, "/user/gamepad/input/trigger_left/value" );
	g_paths.gamepadTriggerRightValue = StringToPath( instance, "/user/gamepad/input/trigger_right/value" );
	g_paths.gamepadViewClick = StringToPath( instance, "/user/gamepad/input/view/click" );
	g_paths.gamepadXClick = StringToPath( instance, "/user/gamepad/input/x/click" );
	g_paths.gamepadYClick = StringToPath( instance, "/user/gamepad/input/y/click" );
	g_paths.gamepadHapticLeft = StringToPath( instance, "/user/gamepad/output/haptic_left" );
	g_paths.gamepadHapticLeftTrigger = StringToPath( instance, "/user/gamepad/output/haptic_left_trigger" );
	g_paths.gamepadHapticRight = StringToPath( instance, "/user/gamepad/output/haptic_right" );
	g_paths.gamepadHapticRightTrigger = StringToPath( instance, "/user/gamepad/output/haptic_right_trigger" );

	g_paths.leftAClick = StringToPath( instance, "/user/hand/left/input/a/click" );
	g_paths.leftATouch = StringToPath( instance, "/user/hand/left/input/a/touch" );
	g_paths.leftAimPose = StringToPath( instance, "/user/hand/left/input/aim/pose" );
	g_paths.leftBClick = StringToPath( instance, "/user/hand/left/input/b/click" );
	g_paths.leftBTouch = StringToPath( instance, "/user/hand/left/input/b/touch" );
	g_paths.leftBackClick = StringToPath( instance, "/user/hand/left/input/back/click" );
	g_paths.leftGripPose = StringToPath( instance, "/user/hand/left/input/grip/pose" );
	g_paths.leftMenuClick = StringToPath( instance, "/user/hand/left/input/menu/click" );
	g_paths.leftSelectClick = StringToPath( instance, "/user/hand/left/input/select/click" );
	g_paths.leftShoulderClick = StringToPath( instance, "/user/hand/left/input/shoulder/click" );
	g_paths.leftSqueezeClick = StringToPath( instance, "/user/hand/left/input/squeeze/click" );
	g_paths.leftSqueezeForce = StringToPath( instance, "/user/hand/left/input/squeeze/force" );
	g_paths.leftSqueezeValue = StringToPath( instance, "/user/hand/left/input/squeeze/value" );
	g_paths.leftSystemClick = StringToPath( instance, "/user/hand/left/input/system/click" );
	g_paths.leftSystemTouch = StringToPath( instance, "/user/hand/left/input/system/touch" );
	g_paths.leftThumbrestTouch = StringToPath( instance, "/user/hand/left/input/thumbrest/touch" );
	g_paths.leftThumbstick = StringToPath( instance, "/user/hand/left/input/thumbstick" );
	g_paths.leftThumbstickClick = StringToPath( instance, "/user/hand/left/input/thumbstick/click" );
	g_paths.leftThumbstickTouch = StringToPath( instance, "/user/hand/left/input/thumbstick/touch" );
	g_paths.leftThumbstickX = StringToPath( instance, "/user/hand/left/input/thumbstick/x" );
	g_paths.leftThumbstickY = StringToPath( instance, "/user/hand/left/input/thumbstick/y" );
	g_paths.leftTrackpad = StringToPath( instance, "/user/hand/left/input/trackpad" );
	g_paths.leftTrackpadClick = StringToPath( instance, "/user/hand/left/input/trackpad/click" );
	g_paths.leftTrackpadForce = StringToPath( instance, "/user/hand/left/input/trackpad/force" );
	g_paths.leftTrackpadTouch = StringToPath( instance, "/user/hand/left/input/trackpad/touch" );
	g_paths.leftTrackpadX = StringToPath( instance, "/user/hand/left/input/trackpad/x" );
	g_paths.leftTrackpadY = StringToPath( instance, "/user/hand/left/input/trackpad/y" );
	g_paths.leftTrigger = StringToPath( instance, "/user/hand/left/input/trigger" );
	g_paths.leftTriggerClick = StringToPath( instance, "/user/hand/left/input/trigger/click" );
	g_paths.leftTriggerTouch = StringToPath( instance, "/user/hand/left/input/trigger/touch" );
	g_paths.leftTriggerValue = StringToPath( instance, "/user/hand/left/input/trigger/value" );
	g_paths.leftXClick = StringToPath( instance, "/user/hand/left/input/x/click" );
	g_paths.leftXTouch = StringToPath( instance, "/user/hand/left/input/x/touch" );
	g_paths.leftYClick = StringToPath( instance, "/user/hand/left/input/y/click" );
	g_paths.leftYTouch = StringToPath( instance, "/user/hand/left/input/y/touch" );
	g_paths.leftHaptic = StringToPath( instance, "/user/hand/left/output/haptic" );

	g_paths.rightAClick = StringToPath( instance, "/user/hand/right/input/a/click" );
	g_paths.rightATouch = StringToPath( instance, "/user/hand/right/input/a/touch" );
	g_paths.rightAimPose = StringToPath( instance, "/user/hand/right/input/aim/pose" );
	g_paths.rightBClick = StringToPath( instance, "/user/hand/right/input/b/click" );
	g_paths.rightBTouch = StringToPath( instance, "/user/hand/right/input/b/touch" );
	g_paths.rightBackClick = StringToPath( instance, "/user/hand/right/input/back/click" );
	g_paths.rightGripPose = StringToPath( instance, "/user/hand/right/input/grip/pose" );
	g_paths.rightMenuClick = StringToPath( instance, "/user/hand/right/input/menu/click" );
	g_paths.rightSelectClick = StringToPath( instance, "/user/hand/right/input/select/click" );
	g_paths.rightShoulderClick = StringToPath( instance, "/user/hand/right/input/shoulder/click" );
	g_paths.rightSqueezeClick = StringToPath( instance, "/user/hand/right/input/squeeze/click" );
	g_paths.rightSqueezeForce = StringToPath( instance, "/user/hand/right/input/squeeze/force" );
	g_paths.rightSqueezeValue = StringToPath( instance, "/user/hand/right/input/squeeze/value" );
	g_paths.rightSystemClick = StringToPath( instance, "/user/hand/right/input/system/click" );
	g_paths.rightSystemTouch = StringToPath( instance, "/user/hand/right/input/system/touch" );
	g_paths.rightThumbrestTouch = StringToPath( instance, "/user/hand/right/input/thumbrest/touch" );
	g_paths.rightThumbstick = StringToPath( instance, "/user/hand/right/input/thumbstick" );
	g_paths.rightThumbstickClick = StringToPath( instance, "/user/hand/right/input/thumbstick/click" );
	g_paths.rightThumbstickTouch = StringToPath( instance, "/user/hand/right/input/thumbstick/touch" );
	g_paths.rightThumbstickX = StringToPath( instance, "/user/hand/right/input/thumbstick/x" );
	g_paths.rightThumbstickY = StringToPath( instance, "/user/hand/right/input/thumbstick/y" );
	g_paths.rightTrackpad = StringToPath( instance, "/user/hand/right/input/trackpad" );
	g_paths.rightTrackpadClick = StringToPath( instance, "/user/hand/right/input/trackpad/click" );
	g_paths.rightTrackpadForce = StringToPath( instance, "/user/hand/right/input/trackpad/force" );
	g_paths.rightTrackpadTouch = StringToPath( instance, "/user/hand/right/input/trackpad/touch" );
	g_paths.rightTrackpadX = StringToPath( instance, "/user/hand/right/input/trackpad/x" );
	g_paths.rightTrackpadY = StringToPath( instance, "/user/hand/right/input/trackpad/y" );
	g_paths.rightTrigger = StringToPath( instance, "/user/hand/right/input/trigger" );
	g_paths.rightTriggerClick = StringToPath( instance, "/user/hand/right/input/trigger/click" );
	g_paths.rightTriggerTouch = StringToPath( instance, "/user/hand/right/input/trigger/touch" );
	g_paths.rightTriggerValue = StringToPath( instance, "/user/hand/right/input/trigger/value" );
	g_paths.rightHaptic = StringToPath( instance, "/user/hand/right/output/haptic" );

	g_paths.headMuteMicClick = StringToPath( instance, "/user/head/input/mute_mic/click" );
	g_paths.headSystemClick = StringToPath( instance, "/user/head/input/system/click" );
	g_paths.headVolumeDownClick = StringToPath( instance, "/user/head/input/volume_down/click" );
	g_paths.headVolumeUpClick = StringToPath( instance, "/user/head/input/volume_up/click" );

	return XR_SUCCESS;
}

const StandardPaths& XRDE::Paths()
{
	return g_paths;
}
