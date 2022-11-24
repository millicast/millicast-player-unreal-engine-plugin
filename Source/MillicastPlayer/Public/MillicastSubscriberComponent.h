// Copyright CoSMoSoftware 2021. All Rights Reserved.

#pragma once

#include <CoreMinimal.h>

#include <Components/ActorComponent.h>
#include "IMillicastExternalAudioConsumer.h"
#include "MillicastSignalingData.h"
#include "MillicastMediaSource.h"
#include "UObject/WeakInterfacePtr.h"

#include "IMillicastMediaTrack.h"

#include "MillicastSubscriberComponent.generated.h"

class IWebSocket;

class FWebRTCPeerConnection;

USTRUCT(BlueprintType, Blueprintable, Category = "MillicastPlayer")
struct MILLICASTPLAYER_API FMillicastTrackInfo
{
	GENERATED_BODY();

	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "MillicastPlayer")
	FString Media;

	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "MillicastPlayer")
	FString TrackId;
};

USTRUCT(BlueprintType, Blueprintable, Category = "MillicastPlayer")
struct MILLICASTPLAYER_API FMillicastLayerData
{
	GENERATED_BODY();

	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "MillicastPlayer")
	FString EncodingId;

	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "MillicastPlayer")
	int SpatialLayerId;

	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "MillicastPlayer")
	int TemporalLayerId;
};

USTRUCT(BlueprintType, Blueprintable, Category = "MillicastPlayer", META=(BlueprintSpawnableComponent))
struct MILLICASTPLAYER_API FMillicastProjectionData
{
	GENERATED_BODY();

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "MillicastPlayer")
	FString TrackId;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "MillicastPlayer")
	FString Mid;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "MillicastPlayer")
	FString Media;
};

DECLARE_DYNAMIC_MULTICAST_SPARSE_DELEGATE(FMillicastSubscriberComponentSubscribed, UMillicastSubscriberComponent, OnSubscribed);
DECLARE_DYNAMIC_MULTICAST_SPARSE_DELEGATE_OneParam(FMillicastSubscriberComponentSubscribedFailure, UMillicastSubscriberComponent, OnSubscribedFailure, const FString&, Msg);

// On Tracks
DECLARE_DYNAMIC_MULTICAST_SPARSE_DELEGATE_OneParam(FMillicastSubscriberComponentVideoTrack, UMillicastSubscriberComponent, OnVideoTrack, UMillicastVideoTrack*, VideoTrack);
DECLARE_DYNAMIC_MULTICAST_SPARSE_DELEGATE_OneParam(FMillicastSubscriberComponentAudioTrack, UMillicastSubscriberComponent, OnAudioTrack, UMillicastAudioTrack*, AudioTrack);

// Broadcast event
DECLARE_DYNAMIC_MULTICAST_SPARSE_DELEGATE_ThreeParams(FMillicastSubscriberComponentActive, UMillicastSubscriberComponent, OnActive, const FString&, StreamId, const TArray<FMillicastTrackInfo>&, Tracks, const FString&, SourceId);
DECLARE_DYNAMIC_MULTICAST_SPARSE_DELEGATE_TwoParams(FMillicastSubscriberComponentInactive, UMillicastSubscriberComponent, OnInactive, const FString&, StreamId, const FString&, SourceId);
DECLARE_DYNAMIC_MULTICAST_SPARSE_DELEGATE(FMillicastSubscriberComponentStopped, UMillicastSubscriberComponent, OnStopped);
DECLARE_DYNAMIC_MULTICAST_SPARSE_DELEGATE_TwoParams(FMillicastSubscriberComponentVad, UMillicastSubscriberComponent, OnVad, const FString&, Mid, const FString&, SourceId);
DECLARE_DYNAMIC_MULTICAST_SPARSE_DELEGATE_ThreeParams(FMillicastSubscriberComponentLayers, UMillicastSubscriberComponent, OnLayers, const FString&, Mid, const TArray<FMillicastLayerData>&, ActiveLayers, const TArray<FMillicastLayerData>&, InactiveLayers);
DECLARE_DYNAMIC_MULTICAST_SPARSE_DELEGATE_OneParam(FMillicastSubscriberComponentViewerCount, UMillicastSubscriberComponent, OnViewerCount, int, Count);

/**
	A component used to receive audio, video from a Millicast feed.
*/
UCLASS(BlueprintType, Blueprintable, Category = "MillicastPlayer",
	   META = (DisplayName = "Millicast Subscriber Component", BlueprintSpawnableComponent))
class MILLICASTPLAYER_API UMillicastSubscriberComponent : public UActorComponent
{
	GENERATED_UCLASS_BODY()

private:
	TMap <FString, TFunction<void(TSharedPtr<FJsonObject>)>> EventBroadcaster;

	/** The Millicast Media Source representing the configuration of the network source */
	UPROPERTY(EditDefaultsOnly, Category = "Properties",
			  META = (DisplayName = "Millicast Media Source", AllowPrivateAccess = true))
	UMillicastMediaSource* MillicastMediaSource = nullptr;

private:
	void SendCommand(const FString& Name, TSharedPtr<FJsonObject> Data);

	void ParseActiveEvent(TSharedPtr<FJsonObject> JsonMsg);
	void ParseInactiveEvent(TSharedPtr<FJsonObject> JsonMsg);
	void ParseStoppedEvent(TSharedPtr<FJsonObject> JsonMsg);
	void ParseVadEvent(TSharedPtr<FJsonObject> JsonMsg);
	void ParseLayersEvent(TSharedPtr<FJsonObject> JsonMsg);
	void ParseViewerCountEvent(TSharedPtr<FJsonObject> JsonMsg);

public:
	virtual ~UMillicastSubscriberComponent() override;

	/**
		Initialize this component with the media source required for receiving Millicast audio, video, and metadata.
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

	/*
		Returns if the subscriber is currently subscribed or not.
	*/
	bool IsSubscribed() const;

	/**
	* Project a media track into a given transceiver mid
	*/
	UFUNCTION(BlueprintCallable, Category = "MillicastPlayer", META = (DisplayName = "Project"))
	void Project(const FString& SourceId, const TArray<FMillicastProjectionData>& ProjectionData);

	/**
	* Unproject a track from a given transceiver mid
	*/
	UFUNCTION(BlueprintCallable, Category = "MillicastPlayer", META = (DisplayName = "Unproject"))
	void Unproject(const TArray<FString>& Mids);

	/**
	* Dynamically add a new track to the peerconnection and locally renegociate the SDP
	* When the new track is created, OnTrack event will be called
	*/
	UFUNCTION(BlueprintCallable, Category = "MillicastPlayer", META = (DisplayName = "AddRemoteTrack"))
	void AddRemoteTrack(const FString& Kind);

public:
	/** Called when the response from the director api is successfull */
	UPROPERTY(BlueprintAssignable, Category = "Components|Activation")
	FMillicastSubscriberComponentSubscribed OnSubscribed;

	/** Called when the response from the director api is an error */
	UPROPERTY(BlueprintAssignable, Category = "Components|Activation")
	FMillicastSubscriberComponentSubscribedFailure OnSubscribedFailure;

	/** Called when a new source has been published within the stream */
	UPROPERTY(BlueprintAssignable, Category = "Components|Activation")
	FMillicastSubscriberComponentActive OnActive;

	/** Called when a source has been unpublished within the stream */
	UPROPERTY(BlueprintAssignable, Category = "Components|Activation")
	FMillicastSubscriberComponentInactive OnInactive;

	/** Called when the stream is no longer available */
	UPROPERTY(BlueprintAssignable, Category = "Components|Activation")
	FMillicastSubscriberComponentStopped OnStopped;

	/** Called when a source id is being multiplexed into the audio track based on the voice activity level */
	UPROPERTY(BlueprintAssignable, Category = "Components|Activation")
	FMillicastSubscriberComponentVad OnVad;

	/** Called the simulcast/svc layer information for the published video tracks of each source */
	UPROPERTY(BlueprintAssignable, Category = "Components|Activation")
	FMillicastSubscriberComponentLayers OnLayers;

	/** Called the number of viewver has changed */
	UPROPERTY(BlueprintAssignable, Category = "Components|Activation")
	FMillicastSubscriberComponentViewerCount OnViewerCount;

	/** Called when a video track is received */
	UPROPERTY(BlueprintAssignable, Category = "Components|Activation")
	FMillicastSubscriberComponentVideoTrack OnVideoTrack;

	/** Called when a audio track is received */
	UPROPERTY(BlueprintAssignable, Category = "Components|Activation")
	FMillicastSubscriberComponentAudioTrack OnAudioTrack;

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
	webrtc::PeerConnectionInterface::RTCConfiguration PeerConnectionConfig;

	TWeakInterfacePtr<IMillicastExternalAudioConsumer> ExternalAudioConsumer;
	FCriticalSection CriticalPcSection;

	TAtomic<bool> Subscribed;

	TArray<UMillicastAudioTrack*> AudioTracks;
	TArray<UMillicastVideoTrack*> VideoTracks;
};
