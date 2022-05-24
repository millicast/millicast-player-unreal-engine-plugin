// Copyright CoSMoSoftware 2021. All Rights Reserved.
#pragma once

#if PLATFORM_WINDOWS

#include "Windows/AllowWindowsPlatformTypes.h"
#include "Windows/PreWindowsApi.h"

// C4582: constructor is not implicitly called in "api/rtcerror.h", treated as an error by UnrealEngine
// C6319: Use of the comma-operator in a tested expression causes the left argument to be ignored when it has no side-effects.
// C6323: Use of arithmetic operator on Boolean type(s).
#pragma warning(push)
#pragma warning(disable: 4582 4583 6319 6323)

#endif // PLATFORM_WINDOWS

#include "api/peer_connection_interface.h"

#if PLATFORM_WINDOWS
#pragma warning(pop)

#include "Windows/PostWindowsApi.h"
#include "Windows/HideWindowsPlatformTypes.h"

#else

#ifdef PF_MAX
#undef PF_MAX
#endif

#endif //PLATFORM_WINDOWS

#include "MillicastSignalingData.generated.h"

/**
	Data used to establish the websocket connection to Millicast
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
	  UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category = "Properties", META = (DisplayName = "JSON Web Token"))
	  FString Jwt;

	  /** STUN/TURN config */
	  TArray<webrtc::PeerConnectionInterface::IceServer> IceServers;
};
