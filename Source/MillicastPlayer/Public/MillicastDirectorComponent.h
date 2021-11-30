// Copyright CoSMoSoftware 2021. All Rights Reserved.

#pragma once

#include <CoreMinimal.h>

#include <Components/ActorComponent.h>
#include "MillicastSignalingData.h"
#include "MillicastMediaSource.h"

#include "MillicastDirectorComponent.generated.h"

/**
	A component used to receive audio, video from a Millicast feed.
*/
UCLASS(BlueprintType, Blueprintable, Category = "MillicastPlayer",
	   META = (DisplayName = "Millicast Director Component", BlueprintSpawnableComponent))
class MILLICASTPLAYER_API UMillicastDirectorComponent : public UActorComponent
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
		Connect to the Millicast platform
	*/
	UFUNCTION(BlueprintCallable, Category = "MillicastPlayer", META = (DisplayName = "Authenticate"))
	bool Authenticate();


	/** Event which is triggered when the HTTP request to Millicast is successfull */
	DECLARE_EVENT_OneParam(UMillicastDirectorComponent, FOnMillicastPlayerConnected, FMillicastSignalingData&)
	FOnMillicastPlayerConnected OnConnected() const { return OnMillicastPlayerConnected; }

	/** Event which is triggered when the HTTP request failed or is rejected by Millicast */
	DECLARE_EVENT_TwoParams(UMillicastDirectorComponent, FOnMillicastPlayerConnectedError, int, FString&)
	FOnMillicastPlayerConnectedError OnConnectedError() const { return OnMillicastPlayerConnectedError; }

private:
	FOnMillicastPlayerConnected OnMillicastPlayerConnected;
	FOnMillicastPlayerConnectedError OnMillicastPlayerConnectedError;


};
