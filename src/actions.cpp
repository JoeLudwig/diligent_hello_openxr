#pragma once

#include "actions.h"

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
	return xrCreateActionSet( instance, &m_createInfo, &m_handle );
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


XrResult Action::LocateSpace( XrSpace baseSpace, XrTime time, XrPath subactionPath, XrSpaceLocation* location )
{
	auto i = m_spaces.find( subactionPath );
	if ( i == m_spaces.end() )
		return XR_ERROR_HANDLE_INVALID;

	return xrLocateSpace( i->second, baseSpace, time, location );
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

XrResult XRDE::SuggestBindings( XrInstance instance, XrPath interactionProfile, const std::vector<const Action*>& actions )
{
	std::vector<XrActionSuggestedBinding> bindings;
	for ( const Action* action : actions )
	{
		std::vector<XrActionSuggestedBinding> thisActionBindings = action->CollectBindings( interactionProfile );
		bindings.insert( bindings.end(), thisActionBindings.begin(), thisActionBindings.end() );
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
