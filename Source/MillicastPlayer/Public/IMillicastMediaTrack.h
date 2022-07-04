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

	UFUNCTION(BlueprintCallable, Category = "MillicastPlayer", META = (DisplayName = "GetMid"))
	virtual FString GetMid() const noexcept PURE_VIRTUAL(UMillicastMediaTrack::GetMid, return FString(););

	UFUNCTION(BlueprintCallable, Category = "MillicastPlayer", META = (DisplayName = "GetTrackId"))
	virtual FString GetTrackId() const noexcept PURE_VIRTUAL(UMillicastMediaTrack::GetTrackId, return FString(););

	UFUNCTION(BlueprintCallable, Category = "MillicastPlayer", META = (DisplayName = "GetKind"))
	virtual FString GetKind() const noexcept PURE_VIRTUAL(UMillicastMediaTrack::GetKind, return FString(););

	UFUNCTION(BlueprintCallable, Category = "MillicastPlayer", META = (DisplayName = "IsEnabled"))
	virtual bool IsEnabled() const noexcept PURE_VIRTUAL(UMillicastMediaTrack::IsEnabled, return bool(););

	UFUNCTION(BlueprintCallable, Category = "MillicastPlayer", META = (DisplayName = "SetEnabled"))
	virtual void SetEnabled(bool Enabled) PURE_VIRTUAL(UMillicastMediaTrack::SetEnabled);
};

UCLASS(Abstract, BlueprintType, Blueprintable, Category = "MillicastPlayer")
class MILLICASTPLAYER_API UMillicastVideoTrack : public UMillicastMediaTrack
{
	GENERATED_BODY()

public:

	UFUNCTION(BlueprintCallable, Category = "MillicastPlayer", META = (DisplayName = "AddConsumer"))
		virtual void AddConsumer(TScriptInterface<IMillicastVideoConsumer> VideoConsumer) PURE_VIRTUAL(UMillicastVideoTrack::AddConsumer);
};


UCLASS(Abstract, BlueprintType, Blueprintable, Category = "MillicastPlayer")
class MILLICASTPLAYER_API UMillicastAudioTrack : public UMillicastMediaTrack
{
	GENERATED_BODY()

public:

	UFUNCTION(BlueprintCallable, Category = "MillicastPlayer", META = (DisplayName = "AddConsumer"))
	virtual void AddConsumer(TScriptInterface<IMillicastExternalAudioConsumer> AudioConsumer) PURE_VIRTUAL(UMillicastAudioTrack::AddConsumer);
};