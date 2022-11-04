// Copyright Millicast 2022. All Rights Reserved.

#pragma once

#include <CoreMinimal.h>

#include "IMillicastExternalAudioConsumer.h"
#include "IMillicastVideoConsumer.h"

#include "IMillicastMediaTrack.generated.h"

UCLASS(Abstract, BlueprintType, Blueprintable)
class MILLICASTPLAYER_API UMillicastMediaTrack : public UObject
{
	GENERATED_BODY()

public:
	virtual ~UMillicastMediaTrack() = default;

	/**
	* Get the media id associated to the track
	*/
	UFUNCTION(BlueprintCallable, Category = "MillicastPlayer", META = (DisplayName = "GetMid"))
	virtual FString GetMid() const noexcept PURE_VIRTUAL(UMillicastMediaTrack::GetMid, return FString(););

	/**
	* Get the identifier of the track
	*/
	UFUNCTION(BlueprintCallable, Category = "MillicastPlayer", META = (DisplayName = "GetTrackId"))
	virtual FString GetTrackId() const noexcept PURE_VIRTUAL(UMillicastMediaTrack::GetTrackId, return FString(););

	/**
	* Get the kind of the track (audio or video)
	*/
	UFUNCTION(BlueprintCallable, Category = "MillicastPlayer", META = (DisplayName = "GetKind"))
	virtual FString GetKind() const noexcept PURE_VIRTUAL(UMillicastMediaTrack::GetKind, return FString(););

	/**
	* Check if the track is enabled.
	*/
	UFUNCTION(BlueprintCallable, Category = "MillicastPlayer", META = (DisplayName = "IsEnabled"))
	virtual bool IsEnabled() const noexcept PURE_VIRTUAL(UMillicastMediaTrack::IsEnabled, return bool(););

	/**
	* Enable or disable the media track. A disabled media track does not send/receive any media.
	*/
	UFUNCTION(BlueprintCallable, Category = "MillicastPlayer", META = (DisplayName = "SetEnabled"))
	virtual void SetEnabled(bool Enabled) PURE_VIRTUAL(UMillicastMediaTrack::SetEnabled);
};

UCLASS(Abstract, BlueprintType, Blueprintable, Category = "MillicastPlayer")
class MILLICASTPLAYER_API UMillicastVideoTrack : public UMillicastMediaTrack
{
	GENERATED_BODY()

public:

	/**
	* Add a consumer to this track to consume the incoming video frames.
	*/
	UFUNCTION(BlueprintCallable, Category = "MillicastPlayer", META = (DisplayName = "AddConsumer"))
	virtual void AddConsumer(TScriptInterface<IMillicastVideoConsumer> VideoConsumer) PURE_VIRTUAL(UMillicastVideoTrack::AddConsumer);

	/**
	* Remove one of the consumer of this track
	*/
	UFUNCTION(BlueprintCallable, Category = "MillicastPlayer", META = (DisplayName = "RemoveConsumer"))
	virtual void RemoveConsumer(TScriptInterface<IMillicastVideoConsumer> VideoConsumer) PURE_VIRTUAL(UMillicastVideoTrack::RemoveConsumer);
};


UCLASS(Abstract, BlueprintType, Blueprintable, Category = "MillicastPlayer")
class MILLICASTPLAYER_API UMillicastAudioTrack : public UMillicastMediaTrack
{
	GENERATED_BODY()

public:

	/**
	* Add a consumer to this track to consume the incoming audio frames.
	*/
	UFUNCTION(BlueprintCallable, Category = "MillicastPlayer", META = (DisplayName = "AddConsumer"))
	virtual void AddConsumer(TScriptInterface<IMillicastExternalAudioConsumer> AudioConsumer) PURE_VIRTUAL(UMillicastAudioTrack::AddConsumer);

	/**
	* Remove one of the consumer of this track
	*/
	UFUNCTION(BlueprintCallable, Category = "MillicastPlayer", META = (DisplayName = "RemoveConsumer"))
	virtual void RemoveConsumer(TScriptInterface<IMillicastExternalAudioConsumer> AudioConsumer) PURE_VIRTUAL(UMillicastAudioTrack::RemoveConsumer);
};