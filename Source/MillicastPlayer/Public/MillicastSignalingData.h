// Copyright CoSMoSoftware 2021. All Rights Reserved.
#pragma once

#include "MillicastSignalingData.generated.h"

/**
	Describes essential properties used for connection objects over NDI�
*/
USTRUCT(BlueprintType, Blueprintable, Category = "MillicastPlayer", META = (DisplayName = "Millicast Signaling Data"))
struct MILLICASTPLAYER_API FMillicastSignalingData
{
	GENERATED_USTRUCT_BODY()

public:
	  /** The websocket url corresponding to the feed we want to subscribe */
	  UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category = "Properties", META = (DisplayName = "WebSocket URL"))
	  FString WsUrl;

          /** The Json Web Token*/
          UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category = "Properties", META = (DisplayName = "Machine Name"))
          FString Jwt;
};
