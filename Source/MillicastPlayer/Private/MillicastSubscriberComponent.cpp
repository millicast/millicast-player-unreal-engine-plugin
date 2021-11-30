// Copyright CoSMoSoftware 2021. All Rights Reserved.

#include "MillicastSubscriberComponent.h"
#include "MillicastPlayerPrivate.h"

UMillicastSubscriberComponent::UMillicastSubscriberComponent(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {}

/**
	Initialize this component with the media source required for receiving Millicast audio, video.
	Returns false, if the MediaSource is already been set. This is usually the case when this component is
	initialized in Blueprints.
*/
bool UMillicastSubscriberComponent::Initialize(UMillicastMediaSource* InMediaSource)
{
  UE_LOG(LogMillicastPlayer, Log, TEXT("Initialize Subsciber Component"));
	if (MillicastMediaSource == nullptr && InMediaSource != nullptr)
	{
		MillicastMediaSource = InMediaSource;
	}

	return InMediaSource != nullptr && InMediaSource == MillicastMediaSource;
}

/**
	Begin receiving audio, video.
*/
bool UMillicastSubscriberComponent::Subscribe(const FMillicastSignalingData& InSignalingData)
{
  UE_LOG(LogMillicastPlayer, Log, TEXT("Subscribe"));
	return IsValid(MillicastMediaSource) &&
	    MillicastMediaSource->Initialize(InSignalingData);
}

/**
	Attempts to stop receiving audio, video.
*/
void UMillicastSubscriberComponent::Unsubscribe()
{
	if (IsValid(MillicastMediaSource))
	{
		// MillicastMediaSource->Shutdown();
	}
}
