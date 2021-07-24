#pragma once

#include "actions.h"
#include "graphics_utilities.h"

using namespace XRDE;

ActionSet::ActionSet( const std::string& name, const std::string& localizedName, uint32_t priority )
{
	m_createInfo = { XR_TYPE_ACTION_SET_CREATE_INFO };
	strcpy_s( m_createInfo.actionSetName, sizeof( m_createInfo.actionSetName ), name.c_str() );
	strcpy_s( m_createInfo.localizedActionSetName, sizeof( m_createInfo.localizedActionSetName ), localizedName.c_str() );
	m_createInfo.priority = priority;
}

XrResult ActionSet::Init( XrInstance instance )
{
	XrResult res = xrCreateActionSet( instance, &m_createInfo, &m_handle );
	if ( XR_FAILED( res ) )
		return res;

	for ( auto& action : m_actions )
	{
		res = action->Init( instance );
		if ( XR_FAILED( res ) )
			return res;
	}

	return XR_SUCCESS;
}


XrResult ActionSet::SessionInit( XrSession session )
{
	for ( auto& action : m_actions )
	{
		if ( action->ActionType() != XR_ACTION_TYPE_POSE_INPUT )
			continue;

		XrResult res = action->CreateSpaces( session );
		if ( XR_FAILED( res ) )
			return res;
	}
	return XR_SUCCESS;
}


Action* ActionSet::AddAction( const std::string& name, const std::string& localizedName, XrActionType type, 
	const std::vector<XrPath>& subactionPaths )
{
	std::unique_ptr<Action> action( new Action( name, localizedName, type, this, subactionPaths ) );
	Action* actionPtr = action.get();
	m_actions.push_back( std::move( action ) );
	return actionPtr;
}


Action::Action( const std::string& name, const std::string& localizedName, XrActionType type, ActionSet* actionSet, 
	const std::vector<XrPath>& subactionPaths )
{
	m_subactionPaths = subactionPaths;
	m_actionSet = actionSet;

	m_createInfo = { XR_TYPE_ACTION_CREATE_INFO };
	strcpy_s( m_createInfo.actionName, sizeof( m_createInfo.actionName ), name.c_str() );
	strcpy_s( m_createInfo.localizedActionName, sizeof( m_createInfo.localizedActionName ), localizedName.c_str() );
	m_createInfo.actionType = type;

	if ( !m_subactionPaths.empty() )
	{
		m_createInfo.countSubactionPaths = (uint32_t)m_subactionPaths.size();
		m_createInfo.subactionPaths = &m_subactionPaths[ 0 ];
	}
}


XrResult Action::Init( XrInstance instance )
{
	return xrCreateAction( m_actionSet->Handle(), &m_createInfo, &m_handle );
}


XrResult Action::CreateSpace( XrSession session, XrPath subactionPath, const XrPosef& poseInActionSpace )
{
	XrActionSpaceCreateInfo createInfo = { XR_TYPE_ACTION_SPACE_CREATE_INFO };
	createInfo.action = Handle();
	createInfo.poseInActionSpace = poseInActionSpace;
	createInfo.subactionPath = subactionPath;

	XrSpace space;
	XrResult res = xrCreateActionSpace( session, &createInfo, &space );
	if ( XR_SUCCEEDED( res ) )
	{
		m_spaces[ subactionPath ] = space;
	}
	return res;
}

XrResult Action::CreateSpaces( XrSession session )
{
	for ( XrPath subactionPath : m_subactionPaths )
	{
		XrResult res = CreateSpace( session, subactionPath, IdentityXrPose() );
		if ( XR_FAILED( res ) )
			return res;
	}

	return XR_SUCCESS;
}


XrResult Action::LocateSpace( XrSpace baseSpace, XrTime time, XrPath subactionPath, XrSpaceLocation* location )
{
	auto i = m_spaces.find( subactionPath );
	if ( i == m_spaces.end() )
		return XR_ERROR_HANDLE_INVALID;

	return xrLocateSpace( i->second, baseSpace, time, location );
}


bool Action::GetBooleanState( XrSession session, XrPath subactionPath )
{
	XrActionStateGetInfo getInfo = { XR_TYPE_ACTION_STATE_GET_INFO };
	getInfo.action = Handle();
	getInfo.subactionPath = subactionPath;

	XrActionStateBoolean getState = { XR_TYPE_ACTION_STATE_BOOLEAN };
	if ( XR_FAILED( xrGetActionStateBoolean( session, &getInfo, &getState ) ) )
		return false;

	return getState.currentState;
}

float Action::GetFloatState( XrSession session, XrPath subactionPath )
{
	XrActionStateGetInfo getInfo = { XR_TYPE_ACTION_STATE_GET_INFO };
	getInfo.action = Handle();
	getInfo.subactionPath = subactionPath;

	XrActionStateFloat getState = { XR_TYPE_ACTION_STATE_BOOLEAN };
	if ( XR_FAILED( xrGetActionStateFloat( session, &getInfo, &getState ) ) )
		return 0.f;

	return getState.currentState;
}


Diligent::float2 Action::GetVector2State( XrSession session, XrPath subactionPath )
{
	XrActionStateGetInfo getInfo = { XR_TYPE_ACTION_STATE_GET_INFO };
	getInfo.action = Handle();
	getInfo.subactionPath = subactionPath;

	XrActionStateVector2f getState = { XR_TYPE_ACTION_STATE_VECTOR2F};
	if ( XR_FAILED( xrGetActionStateVector2f( session, &getInfo, &getState ) ) )
		return { 0.f, 0.f };

	return { getState.currentState.x, getState.currentState.y };
}

void Action::ApplyHapticFeedback( XrSession session, XrPath subactionPath, float durationSeconds, float frequency, float amplitude )
{
	XrHapticActionInfo actionInfo = { XR_TYPE_HAPTIC_ACTION_INFO };
	actionInfo.action = Handle();
	actionInfo.subactionPath = subactionPath;

	XrHapticVibration vibration = { XR_TYPE_HAPTIC_VIBRATION };
	vibration.duration = (XrDuration)( durationSeconds * 1000000000.f );
	vibration.frequency = frequency;
	vibration.amplitude = amplitude;
	xrApplyHapticFeedback( session, &actionInfo, (XrHapticBaseHeader*)&vibration );
}

void Action::StopApplyingHapticFeecback( XrSession session, XrPath subactionPath )
{
	XrHapticActionInfo actionInfo = { XR_TYPE_HAPTIC_ACTION_INFO };
	actionInfo.action = Handle();
	actionInfo.subactionPath = subactionPath;

	xrStopHapticFeedback( session, &actionInfo );
}


void Action::AddIPBinding( XrPath interactionProfile, XrPath bindingPath )
{
	m_bindings[ interactionProfile ].push_back( bindingPath );
}

void Action::AddGlobalBinding( XrPath bindingPath )
{
	AddIPBinding( XR_NULL_PATH, bindingPath );
}


std::vector< XrActionSuggestedBinding > Action::CollectBindings( XrPath interactionProfile ) const
{
	std::vector<XrActionSuggestedBinding> out;

	auto i = m_bindings.find( XR_NULL_PATH );
	if ( i != m_bindings.end() )
	{
		for ( XrPath binding : i->second )
		{
			out.push_back( { m_handle, binding } );
		}
	}

	i = m_bindings.find( interactionProfile );
	if ( i != m_bindings.end() )
	{
		for ( XrPath binding : i->second )
		{
			out.push_back( { m_handle, binding } );
		}
	}

	return out;
}

XrResult XRDE::SuggestBindings( XrInstance instance, XrPath interactionProfile, const std::vector<const ActionSet*>& actionSets )
{
	std::vector<XrActionSuggestedBinding> bindings;
	for ( const ActionSet* actionSet : actionSets )
	{
		for ( auto i = actionSet->begin(); i != actionSet->end(); i++ )
		{
			std::vector<XrActionSuggestedBinding> thisActionBindings = (*i)->CollectBindings( interactionProfile );
			bindings.insert( bindings.end(), thisActionBindings.begin(), thisActionBindings.end() );
		}
	}

	XrInteractionProfileSuggestedBinding suggestedBindings = { XR_TYPE_INTERACTION_PROFILE_SUGGESTED_BINDING };
	suggestedBindings.interactionProfile = interactionProfile;
	suggestedBindings.suggestedBindings = bindings.empty() ? nullptr : &bindings[ 0 ];
	suggestedBindings.countSuggestedBindings = (uint32_t)bindings.size();
	return xrSuggestInteractionProfileBindings( instance, &suggestedBindings );
}

XrResult XRDE::AttachActionSets( XrSession session, const std::vector< const ActionSet*>& actionSets )
{
	if ( actionSets.empty() )
		return XR_SUCCESS;

	std::vector<XrActionSet> actionSetHandles;
	actionSetHandles.reserve( actionSets.size() );
	for ( auto& actionSet : actionSets )
	{
		actionSetHandles.push_back( actionSet->Handle() );
	}

	XrSessionActionSetsAttachInfo attachInfo = { XR_TYPE_SESSION_ACTION_SETS_ATTACH_INFO };
	attachInfo.actionSets = &actionSetHandles[ 0 ];
	attachInfo.countActionSets = (uint32_t)actionSetHandles.size();

	return xrAttachSessionActionSets( session, &attachInfo );
}
