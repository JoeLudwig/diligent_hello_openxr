#pragma once

#include <openxr/openxr.h>
#include <string>
#include <vector>
#include <map>

#include <Common/interface/BasicMath.hpp>

namespace XRDE
{

static const char* k_userHandLeft = "/user/hand/left";
static const char* k_userHandRight = "/user/hand/right";

class Action;

class ActionSet
{
public:
	ActionSet( const std::string& name, const std::string& localizedName, uint32_t priority );

	XrResult Init( XrInstance instance );
	XrResult SessionInit( XrSession session );
	Action* AddAction( const std::string& name, const std::string& localizedName, XrActionType type, const std::vector<XrPath>& subactionPaths );

	XrActionSet Handle() const { return m_handle; }

	std::vector< std::unique_ptr< Action > >::const_iterator begin() const { return m_actions.begin();  }
	std::vector< std::unique_ptr< Action > >::const_iterator end() const { return m_actions.end(); }
protected:
	XrActionSet m_handle;
	XrActionSetCreateInfo m_createInfo;
	std::vector< std::unique_ptr< Action > > m_actions;
};


class Action
{
	friend ActionSet;
public:


	void AddIPBinding( XrPath interactionProfile, XrPath bindingPath );
	void AddGlobalBinding( XrPath bindingPath );

	XrResult LocateSpace( XrSpace baseSpace, XrTime time, XrPath subactionPath, XrSpaceLocation* location );
	bool GetBooleanState( XrSession session, XrPath subactionPath );
	float GetFloatState( XrSession session, XrPath subactionPath );
	Diligent::float2 GetVector2State( XrSession session, XrPath subactionPath );

	void ApplyHapticFeedback( XrSession session, XrPath subactionPath, float durationSeconds, float frequency, float amplitude );
	void StopApplyingHapticFeecback( XrSession session, XrPath subactionPath );

	std::vector< XrActionSuggestedBinding > CollectBindings( XrPath interactionProfile ) const;

	XrAction Handle() const { return m_handle; }
	XrActionType ActionType() const { return m_createInfo.actionType;  }

protected:
	Action( const std::string& name, const std::string& localizedName, XrActionType type, ActionSet* actionSet,
		const std::vector<XrPath>& subactionPaths );
	XrResult Init( XrInstance instance );
	XrResult CreateSpace( XrSession session, XrPath subactionPath, const XrPosef& poseInActionSpace );
	XrResult CreateSpaces( XrSession session );

	ActionSet* m_actionSet;

	XrAction m_handle;
	XrActionCreateInfo m_createInfo;
	std::vector<XrPath> m_subactionPaths;
	std::map<XrPath, std::vector<XrPath>> m_bindings;
	std::map<XrPath, XrSpace> m_spaces;
};

XrResult SuggestBindings( XrInstance instance, XrPath interactionProfile, const std::vector<const ActionSet*> & actionSets );
XrResult AttachActionSets( XrSession session, const std::vector< const ActionSet*>& actionSets );

}
