#pragma once

#include <openxr/openxr.h>
#include <string>

namespace XRDE
{
	XrPath StringToPath( XrInstance instance, const std::string& pathString );
	std::string PathToString( XrInstance instance, XrPath path );

	struct StandardPaths
	{
		XrPath userHandLeft;
		XrPath userHandRight;
		XrPath userHandHead;
		XrPath userHandGamepad;
		XrPath userHandTreadmill;

		XrPath interactionProfilesKHRSimpleController;
		XrPath interactionProfilesHPMixedRealityController;
		XrPath interactionProfilesHTCViveController;
		XrPath interactionProfilesHTCViveCosmosController;
		XrPath interactionProfilesHTCVivePro;
		XrPath interactionProfilesMicrosoftMotionController;
		XrPath interactionProfilesMicrosoftXboxController;
		XrPath interactionProfilesOculusTouchController;
		XrPath interactionProfilesValveIndexController;

		XrPath gamepadAClick;
		XrPath gamepadBClick;
		XrPath gamepadDpadDownClick;
		XrPath gamepadDpadLeftClick;
		XrPath gamepadDpadRightClick;
		XrPath gamepadDpadUpClick;
		XrPath gamepadMenuClick;
		XrPath gamepadShoulderLeftClick;
		XrPath gamepadShoulderRightClick;
		XrPath gamepadThumbstickLeft;
		XrPath gamepadThumbstickLeftClick;
		XrPath gamepadThumbstickLeftX;
		XrPath gamepadThumbstickLeftY;
		XrPath gamepadThumbstickRight;
		XrPath gamepadThumbstickRightClick;
		XrPath gamepadThumbstickRightX;
		XrPath gamepadThumbstickRightY;
		XrPath gamepadTriggerLeftValue;
		XrPath gamepadTriggerRightValue;
		XrPath gamepadViewClick;
		XrPath gamepadXClick;
		XrPath gamepadYClick;
		XrPath gamepadHapticLeft;
		XrPath gamepadHapticLeftTrigger;
		XrPath gamepadHapticRight;
		XrPath gamepadHapticRightTrigger;

		XrPath leftAClick;
		XrPath leftATouch;
		XrPath leftAimPose;
		XrPath leftBClick;
		XrPath leftBTouch;
		XrPath leftBackClick;
		XrPath leftGripPose;
		XrPath leftMenuClick;
		XrPath leftSelectClick;
		XrPath leftShoulderClick;
		XrPath leftSqueezeClick;
		XrPath leftSqueezeForce;
		XrPath leftSqueezeValue;
		XrPath leftSystemClick;
		XrPath leftSystemTouch;
		XrPath leftThumbrestTouch;
		XrPath leftThumbstick;
		XrPath leftThumbstickClick;
		XrPath leftThumbstickTouch;
		XrPath leftThumbstickX;
		XrPath leftThumbstickY;
		XrPath leftTrackpad;
		XrPath leftTrackpadClick;
		XrPath leftTrackpadForce;
		XrPath leftTrackpadTouch;
		XrPath leftTrackpadX;
		XrPath leftTrackpadY;
		XrPath leftTrigger;
		XrPath leftTriggerClick;
		XrPath leftTriggerTouch;
		XrPath leftTriggerValue;
		XrPath leftXClick;
		XrPath leftXTouch;
		XrPath leftYClick;
		XrPath leftYTouch;
		XrPath leftHaptic;

		XrPath rightAClick;
		XrPath rightATouch;
		XrPath rightAimPose;
		XrPath rightBClick;
		XrPath rightBTouch;
		XrPath rightBackClick;
		XrPath rightGripPose;
		XrPath rightMenuClick;
		XrPath rightSelectClick;
		XrPath rightShoulderClick;
		XrPath rightSqueezeClick;
		XrPath rightSqueezeForce;
		XrPath rightSqueezeValue;
		XrPath rightSystemClick;
		XrPath rightSystemTouch;
		XrPath rightThumbrestTouch;
		XrPath rightThumbstick;
		XrPath rightThumbstickClick;
		XrPath rightThumbstickTouch;
		XrPath rightThumbstickX;
		XrPath rightThumbstickY;
		XrPath rightTrackpad;
		XrPath rightTrackpadClick;
		XrPath rightTrackpadForce;
		XrPath rightTrackpadTouch;
		XrPath rightTrackpadX;
		XrPath rightTrackpadY;
		XrPath rightTrigger;
		XrPath rightTriggerClick;
		XrPath rightTriggerTouch;
		XrPath rightTriggerValue;
		XrPath rightHaptic;

		XrPath headMuteMicClick;
		XrPath headSystemClick;
		XrPath headVolumeDownClick;
		XrPath headVolumeUpClick;
	};

	XrResult InitPaths( XrInstance instance );
	const StandardPaths& Paths();
}