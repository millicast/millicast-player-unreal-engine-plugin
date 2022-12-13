// Copyright CoSMoSoftware 2021. All Rights Reserved.

#pragma once

#include <CoreMinimal.h>

#include <Components/ActorComponent.h>
#include "MillicastSignalingData.h"
#include "MillicastMediaSource.h"

#include "MillicastDirectorComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_SPARSE_DELEGATE_OneParam(FMillicastDirectorComponentAuthenticated, UMillicastDirectorComponent, OnAuthenticated, const FMillicastSignalingData&, SignalingData);
DECLARE_DYNAMIC_MULTICAST_SPARSE_DELEGATE_TwoParams(FMillicastDirectorComponentAuthenticationFailure, UMillicastDirectorComponent, OnAuthenticationFailure, int, Code, const FString&, Msg);

class IHttpResponse;

/**
	A component to make request to the Millicast director API
	in order to get the WebSocket url and the JsonWebToken
	of the corresponding stream
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
		Initialize this component with the media source required for receiving Millicast audio and video.
		Returns false, if the MediaSource is already been set. This is usually the case when this component is
		initialized in Blueprints.
	*/
	bool Initialize(UMillicastMediaSource* InMediaSource = nullptr);

	/**
	* Change the Millicast Media Source of this object
	* Note that you have to change it before calling subscribe in order to have effect.
	*/
	UFUNCTION(BlueprintCallable, Category = "MillicastPlayer", META = (DisplayName = "SetMediaSource"))
		void SetMediaSource(UMillicastMediaSource* InMediaSource);

	/**
		Connect to the Millicast platform
	*/
	UFUNCTION(BlueprintCallable, Category = "MillicastPlayer", META = (DisplayName = "Authenticate"))
	bool Authenticate();
public:
	/** Called when the response from the director api is successfull */
	UPROPERTY(BlueprintAssignable, Category = "Components|Activation")
	FMillicastDirectorComponentAuthenticated OnAuthenticated;

	/** Called when the response from the director api is an error */
	UPROPERTY(BlueprintAssignable, Category = "Components|Activation")
	FMillicastDirectorComponentAuthenticationFailure OnAuthenticationFailure;

private:
	void ParseIceServers(const TArray<TSharedPtr<FJsonValue>>& IceServersField,
		FMillicastSignalingData& SignalingData);
	void ParseDirectorResponse(TSharedPtr<IHttpResponse, ESPMode::ThreadSafe> Response);
};
