// Copyright CoSMoSoftware 2021. All Rights Reserved.

#pragma once

#include <CoreMinimal.h>

#include <Components/ActorComponent.h>
#include "IMillicastExternalAudioConsumer.h"
#include "MillicastSignalingData.h"
#include "MillicastMediaSource.h"
#include "UObject/WeakInterfacePtr.h"

#include "MillicastSubscriberComponent.generated.h"

class IWebSocket;

class FWebRTCPeerConnection;

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
    virtual ~UMillicastSubscriberComponent() override;

	/**
		Initialize this component with the media source required for receiving Millicast audio, video, and metadata.
		Returns false, if the MediaSource is already been set. This is usually the case when this component is
		initialized in Blueprints.
	*/
	bool Initialize(UMillicastMediaSource* InMediaSource = nullptr);

	/**
		Begin receiving video from Millicast. The optional ExternalAudioConsumer allows to perform custom audio handling.
		If nullptr is passed in, the default Unreal audio device is used.
	*/
	UFUNCTION(BlueprintCallable, Category = "MillicastPlayer", META = (DisplayName = "Subscribe"))
	bool Subscribe(const FMillicastSignalingData& InConnectionInformation, TScriptInterface<IMillicastExternalAudioConsumer> ExternalAudioConsumer);

	/**
		Attempts to stop receiving video from the Millicast feed
	*/
	UFUNCTION(BlueprintCallable, Category = "MillicastPlayer", META = (DisplayName = "Unsubscribe"))
	void Unsubscribe();

private:
	/** Websocket Connection */
	bool StartWebSocketConnection(const FString& url, const FString& jwt);
	void OnConnected();
	void OnConnectionError(const FString& Error);
	void OnClosed(int32 StatusCode, const FString& Reason, bool bWasClean);
	void OnMessage(const FString& Msg);

	/** Create the peerconnection and starts subscribing*/
	bool SubscribeToMillicast();

	/** WebSocket Connection */
	TSharedPtr<IWebSocket> WS;
	FDelegateHandle OnConnectedHandle;
	FDelegateHandle OnConnectionErrorHandle;
	FDelegateHandle OnClosedHandle;
	FDelegateHandle OnMessageHandle;

	FWebRTCPeerConnection* PeerConnection;

    TWeakInterfacePtr<IMillicastExternalAudioConsumer> ExternalAudioConsumer;
};
