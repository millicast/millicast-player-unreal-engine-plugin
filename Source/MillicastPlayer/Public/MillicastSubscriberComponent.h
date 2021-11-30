// Copyright CoSMoSoftware 2021. All Rights Reserved.

#pragma once

#include <CoreMinimal.h>

#include <Components/ActorComponent.h>
#include "MillicastSignalingData.h"
#include "MillicastMediaSource.h"

#include "MillicastSubscriberComponent.generated.h"

/**
	A component used to receive audio, video from a Millicast feed.
*/
UCLASS(BlueprintType, Blueprintable, Category = "MillicastPlayer",
	   META = (DisplayName = "Millicast Subscriber Component", BlueprintSpawnableComponent))
class MILLICASTPLAYER_API UMillicastSubscriberComponent : public UActorComponent
{
	GENERATED_UCLASS_BODY()

private:
	/** The Millicast Media Source representing the configuration of the network source */
	UPROPERTY(EditDefaultsOnly, Category = "Properties",
			  META = (DisplayName = "Millicast Media Source", AllowPrivateAccess = true))
	UMillicastMediaSource* MillicastMediaSource = nullptr;

public:
	/**
		Initialize this component with the media source required for receiving NDI audio, video, and metadata.
		Returns false, if the MediaSource is already been set. This is usually the case when this component is
		initialized in Blueprints.
	*/
	bool Initialize(UMillicastMediaSource* InMediaSource = nullptr);

	/**
		Begin receiving video from Millicast
	*/
	UFUNCTION(BlueprintCallable, Category = "MillicastPlayer", META = (DisplayName = "Subscribe"))
	bool Subscribe(const FMillicastSignalingData& InConnectionInformation);

	/**
		Attempts to stop receiving video from the Millicast feed
	*/
	UFUNCTION(BlueprintCallable, Category = "MillicastPlayer", META = (DisplayName = "Unsubscribe"))
	void Unsubscribe();
};
