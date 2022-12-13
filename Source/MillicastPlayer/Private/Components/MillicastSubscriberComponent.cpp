// Copyright CoSMoSoftware 2021. All Rights Reserved.

#include "MillicastSubscriberComponent.h"
#include "MillicastPlayerPrivate.h"

#include <string>

#include "Dom/JsonValue.h"
#include "Dom/JsonObject.h"

#include "Serialization/JsonReader.h"
#include "Serialization/JsonWriter.h"

#include "WebSocketsModule.h"
#include "IWebSocket.h"
#include "WebRTC/PeerConnection.h"
#include "WebRTC/MillicastMediaTracks.h"

#include "Util.h"

#define WEAK_CAPTURE WeakThis = TWeakObjectPtr<UMillicastSubscriberComponent>(this)

UMillicastSubscriberComponent::UMillicastSubscriberComponent(const FObjectInitializer& ObjectInitializer) 
	: Super(ObjectInitializer), Subscribed(false)
{
	PeerConnection = nullptr;
	WS = nullptr;

	PeerConnectionConfig = Millicast::Player::FWebRTCPeerConnection::GetDefaultConfig();

	// Json Message received from websocket
	MessageParser.Emplace("response", [this](TSharedPtr<FJsonObject> Msg) { ParseResponse(Msg); });
	MessageParser.Emplace("event", [this](TSharedPtr<FJsonObject> Msg) { ParseEvent(Msg); });
	MessageParser.Emplace("error", [this](TSharedPtr<FJsonObject> Msg) { ParseError(Msg); });

	// Event received from websocket signaling
	EventBroadcaster.Emplace("active", [this](TSharedPtr<FJsonObject> Msg) { ParseActiveEvent(Msg); });
	EventBroadcaster.Emplace("inactive", [this](TSharedPtr<FJsonObject> Msg) { ParseInactiveEvent(Msg); });
	EventBroadcaster.Emplace("stopped", [this](TSharedPtr<FJsonObject> Msg) { ParseStoppedEvent(Msg); });
	EventBroadcaster.Emplace("vad", [this](TSharedPtr<FJsonObject> Msg) { ParseVadEvent(Msg); });
	EventBroadcaster.Emplace("layers", [this](TSharedPtr<FJsonObject> Msg) { ParseLayersEvent(Msg); });
	EventBroadcaster.Emplace("viewercount", [this](TSharedPtr<FJsonObject> Msg) { ParseViewerCountEvent(Msg); });

	UE_LOG(LogMillicastPlayer, Verbose, TEXT("%S"), __FUNCTION__);
}

UMillicastSubscriberComponent::~UMillicastSubscriberComponent()
{
	UE_LOG(LogMillicastPlayer, Verbose, TEXT("%S"), __FUNCTION__);
    Unsubscribe();
}

/**
	Initialize this component with the media source required for receiving Millicast audio, video.
	Returns false, if the MediaSource is already been set. This is usually the case when this component is
	initialized in Blueprints.
*/
bool UMillicastSubscriberComponent::Initialize(UMillicastMediaSource* InMediaSource)
{
	UE_LOG(LogMillicastPlayer, VeryVerbose, TEXT("%S"), __FUNCTION__);

	if (MillicastMediaSource == nullptr && InMediaSource != nullptr)
	{
		MillicastMediaSource = InMediaSource;
	}

	return InMediaSource != nullptr && InMediaSource == MillicastMediaSource;
}

void UMillicastSubscriberComponent::SetMediaSource(UMillicastMediaSource* InMediaSource)
{
	UE_LOG(LogMillicastPlayer, VeryVerbose, TEXT("%S"), __FUNCTION__);

	if (InMediaSource != nullptr)
	{
		MillicastMediaSource = InMediaSource;
	}
	else
	{
		UE_LOG(LogMillicastPlayer, Log, TEXT("Provided MediaSource was nullptr"));
	}
}

/**
	Begin receiving audio, video.
*/
bool UMillicastSubscriberComponent::Subscribe(const FMillicastSignalingData& InSignalingData, TScriptInterface<IMillicastExternalAudioConsumer> InExternalAudioConsumer)
{
	UE_LOG(LogMillicastPlayer, Verbose, TEXT("%S"), __FUNCTION__);

	if (IsSubscribed())
	{
		UE_LOG(LogMillicastPlayer, Error, TEXT("You are already subscribed to a stream. Please unsubscribed first."));
		return false;
	}

	if (InExternalAudioConsumer)
	{
		UE_LOG(LogMillicastPlayer, Verbose, TEXT("Setting external audio consumer"));
		ExternalAudioConsumer = InExternalAudioConsumer;
	}

	for (auto& s : InSignalingData.IceServers) 
	{
		UE_LOG(LogMillicastPlayer, Verbose, TEXT("Adding ice server %s"), *InSignalingData.WsUrl);
		PeerConnectionConfig.servers.push_back(s);
	}

	return IsValid(MillicastMediaSource) &&
	    MillicastMediaSource->Initialize(InSignalingData) &&
	    StartWebSocketConnection(InSignalingData.WsUrl, InSignalingData.Jwt);
}

/**
	Attempts to stop receiving audio, video.
*/
void UMillicastSubscriberComponent::Unsubscribe()
{
	UE_LOG(LogMillicastPlayer, Verbose, TEXT("%S"), __FUNCTION__);
	FScopeLock Lock(&CriticalPcSection);
	
	if (WS)
	{
		UE_LOG(LogMillicastPlayer, Verbose, TEXT("Closing web socket"));
		WS->Close();
		WS = nullptr;
	}

	if(PeerConnection)
	{
		UE_LOG(LogMillicastPlayer, Verbose, TEXT("Clearing audio tracks"));

		for (auto& track : AudioTracks)
		{
			static_cast<UMillicastAudioTrackImpl*>(track)->Terminate();
			track->RemoveFromRoot();
		}

		AudioTracks.Empty();

		UE_LOG(LogMillicastPlayer, Verbose, TEXT("Clearing video tracks"));
		for (auto& track : VideoTracks)
		{
			static_cast<UMillicastVideoTrackImpl*>(track)->Terminate();
			track->RemoveFromRoot();
		}

		VideoTracks.Empty();

		UE_LOG(LogMillicastPlayer, Verbose, TEXT("Destroying peerconnection"));
		delete PeerConnection;
		PeerConnection = nullptr;

		Subscribed = false;
	}
}

bool UMillicastSubscriberComponent::IsSubscribed() const
{
	return Subscribed.Load();
}

void UMillicastSubscriberComponent::Project(const FString& SourceId, const TArray<FMillicastProjectionData>& ProjectionData)
{
	UE_LOG(LogMillicastPlayer, VeryVerbose, TEXT("%S"), __FUNCTION__);

	auto DataJson = MakeShared<FJsonObject>();
	TArray<TSharedPtr<FJsonValue>> ProjectionJson;

	for (auto& d : ProjectionData)
	{
		auto data = MakeShared<FJsonObject>();
		data->SetStringField("trackId", d.TrackId);
		data->SetStringField("mediaId", d.Mid);
		data->SetStringField("media", d.Media);

		auto value = MakeShared<FJsonValueObject>(data);
		ProjectionJson.Emplace(value);
	}

	DataJson->SetStringField("sourceId", SourceId);
	DataJson->SetArrayField("mapping", ProjectionJson);

	SendCommand("project", DataJson);
}

void UMillicastSubscriberComponent::Unproject(const TArray<FString>& Mids)
{
	UE_LOG(LogMillicastPlayer, VeryVerbose, TEXT("%S"), __FUNCTION__);

	auto DataJson = MakeShared<FJsonObject>();
	TArray<TSharedPtr<FJsonValue>> MidsJson;

	for (auto& m : Mids)
	{
		MidsJson.Emplace(MakeShared<FJsonValueString>(m));
	}

	DataJson->SetArrayField("mediaIds", MidsJson);

	SendCommand("unproject", DataJson);
}

void UMillicastSubscriberComponent::AddRemoteTrack(const FString& Kind)
{
	UE_LOG(LogMillicastPlayer, VeryVerbose, TEXT("%S"), __FUNCTION__);

	webrtc::RtpTransceiverInit init;
	init.direction = webrtc::RtpTransceiverDirection::kRecvOnly;

	using cricket::MediaType;
	cricket::MediaType media = (Kind == "audio") ? MediaType::MEDIA_TYPE_AUDIO : MediaType::MEDIA_TYPE_VIDEO;

	auto result = (*PeerConnection)->AddTransceiver(media, init);
	if (result.ok())
	{
		UE_LOG(LogMillicastPlayer, Log, TEXT("Successfully added transceiver for remote track"));
	}
	else
	{
		UE_LOG(LogMillicastPlayer, Error, TEXT("Failed to add transceiver for remote track"));
	}
}

bool UMillicastSubscriberComponent::StartWebSocketConnection(const FString& Url,
                                                     const FString& Jwt)
{
	UE_LOG(LogMillicastPlayer, Verbose, TEXT("%S"), __FUNCTION__);

	if (!FModuleManager::Get().IsModuleLoaded("WebSockets"))
	{
		UE_LOG(LogMillicastPlayer, VeryVerbose, TEXT("Load WebSocket module"));
		FModuleManager::Get().LoadModule("WebSockets");
	}

	WS = FWebSocketsModule::Get().CreateWebSocket(Url + "?token=" + Jwt);

	OnConnectedHandle = WS->OnConnected().AddWeakLambda(this, [this]() { OnConnected(); });
	OnConnectionErrorHandle = WS->OnConnectionError().AddWeakLambda(this, [this](const FString& Error) { OnConnectionError(Error); });
	OnClosedHandle = WS->OnClosed().AddWeakLambda(this, [this](int32 StatusCode, const FString& Reason, bool bWasClean) { OnClosed(StatusCode, Reason, bWasClean); });
	OnMessageHandle = WS->OnMessage().AddWeakLambda(this, [this](const FString& Msg) { OnMessage(Msg); });

	WS->Connect();

	return true;
}

bool UMillicastSubscriberComponent::SubscribeToMillicast()
{
	using namespace Millicast::Player;
	UE_LOG(LogMillicastPlayer, Verbose, TEXT("%S"), __FUNCTION__);

	PeerConnection =
		FWebRTCPeerConnection::Create(FWebRTCPeerConnection::GetDefaultConfig(), ExternalAudioConsumer);

	auto * CreateSessionDescriptionObserver = PeerConnection->GetCreateDescriptionObserver();
	auto * LocalDescriptionObserver  = PeerConnection->GetLocalDescriptionObserver();
	auto * RemoteDescriptionObserver = PeerConnection->GetRemoteDescriptionObserver();

	CreateSessionDescriptionObserver->SetOnSuccessCallback([WEAK_CAPTURE](const std::string& type, const std::string& sdp) {
		if (WeakThis.IsValid())
		{
			FScopeLock Lock(&WeakThis->CriticalPcSection);
			UE_LOG(LogMillicastPlayer, Log, TEXT("pc.createOffer() | sucess\nsdp : %s"), *ToString(sdp));
			if (WeakThis->PeerConnection) WeakThis->PeerConnection->SetLocalDescription(sdp, type);
		}
	});

	CreateSessionDescriptionObserver->SetOnFailureCallback([WEAK_CAPTURE](const std::string& err) {
		UE_LOG(LogMillicastPlayer, Error, TEXT("pc.createOffer() | Error: %s"), *ToString(err));
		if (WeakThis.IsValid())
		{
			WeakThis->OnSubscribedFailure.Broadcast(FString{ err.c_str() });
		}
	});

	LocalDescriptionObserver->SetOnSuccessCallback([WEAK_CAPTURE]() {
		FScopeLock Lock(&WeakThis->CriticalPcSection);
		if (!WeakThis.IsValid() || !WeakThis->PeerConnection) return;

		UE_LOG(LogMillicastPlayer, Log, TEXT("pc.setLocalDescription() | sucess"));
		std::string sdp;
		(*WeakThis->PeerConnection)->local_description()->ToString(&sdp);

		// Add events we want to receive from millicast
		TArray<TSharedPtr<FJsonValue>> eventsJson;
		TArray<FString> EvKeys;
		WeakThis->EventBroadcaster.GetKeys(EvKeys);

		for (auto& ev : EvKeys)
		{
			eventsJson.Add(MakeShared<FJsonValueString>(ev));
		}

		// Fill Signaling data
		auto DataJson = MakeShared<FJsonObject>();
		DataJson->SetStringField("streamId", WeakThis->MillicastMediaSource->StreamName);
		DataJson->SetStringField("sdp", ToString(sdp));
		DataJson->SetArrayField("events", eventsJson);

		WeakThis->SendCommand("view", DataJson);
	});

	LocalDescriptionObserver->SetOnFailureCallback([WEAK_CAPTURE](const std::string& err) {
		UE_LOG(LogMillicastPlayer, Error, TEXT("Set local description failed : %s"), *ToString(err));
		if (WeakThis.IsValid())
		{
			WeakThis->OnSubscribedFailure.Broadcast(FString{ err.c_str() });
		}
	});

	RemoteDescriptionObserver->SetOnSuccessCallback([WEAK_CAPTURE]() {
		UE_LOG(LogMillicastPlayer, Log, TEXT("Set remote description suceeded"));
		if (WeakThis.IsValid())
		{
			WeakThis->Subscribed = true;
			WeakThis->OnSubscribed.Broadcast();
		}
	});
	RemoteDescriptionObserver->SetOnFailureCallback([WEAK_CAPTURE](const std::string& err) {
		UE_LOG(LogMillicastPlayer, Error, TEXT("Set remote description failed : %s"), *ToString(err));
		if (WeakThis.IsValid())
		{
			WeakThis->OnSubscribedFailure.Broadcast(FString{ err.c_str() });
		}
	});

	PeerConnection->OaOptions.offer_to_receive_video = true;
	PeerConnection->OaOptions.offer_to_receive_audio = true;

	using RtcTrack = rtc::scoped_refptr<webrtc::MediaStreamTrackInterface>;

	PeerConnection->OnVideoTrack = [WEAK_CAPTURE](const std::string& Mid, RtcTrack Track) {
		UE_LOG(LogMillicastPlayer, Log, TEXT("OnVideoTrack"));

		AsyncTask(ENamedThreads::GameThread, [WeakThis, Mid, Track]() {
			if (WeakThis.IsValid())
			{
				UE_LOG(LogMillicastPlayer, Verbose, TEXT("Create video track object"));
				auto videoTrack = NewObject<UMillicastVideoTrackImpl>();
				videoTrack->Initialize(Mid.c_str(), Track);
				videoTrack->AddToRoot();
				WeakThis->OnVideoTrack.Broadcast(videoTrack);
				WeakThis->VideoTracks.Add(videoTrack);
			}
			else
			{
				UE_LOG(LogMillicastPlayer, Warning, TEXT("SubscriberComponent no longer valid to add video track"));
			}
		});
	};
	PeerConnection->OnAudioTrack = [WEAK_CAPTURE](const std::string& mid, RtcTrack Track) {
		UE_LOG(LogMillicastPlayer, Log, TEXT("OnAudioTrack"));
		AsyncTask(ENamedThreads::GameThread, [WeakThis, mid, Track]() {
			if (WeakThis.IsValid())
			{
				UE_LOG(LogMillicastPlayer, Verbose, TEXT("Create audio track object"));
				auto audioTrack = NewObject<UMillicastAudioTrackImpl>();
				audioTrack->Initialize(mid.c_str(), Track);
				audioTrack->AddToRoot();

				WeakThis->OnAudioTrack.Broadcast(audioTrack);
				WeakThis->AudioTracks.Add(audioTrack); // keep reference to delete it later
			}
			else
			{
				UE_LOG(LogMillicastPlayer, Warning, TEXT("SubscriberComponent no longer valid to add audio track"));
			}
		});
	};

	PeerConnection->CreateOffer();
	PeerConnection->EnableStats(true);

	return true;
}

/* WebSocket Callback
*****************************************************************************/

void UMillicastSubscriberComponent::OnConnected()
{
	UE_LOG(LogMillicastPlayer, Log, TEXT("Millicast WebSocket Connected"));
	SubscribeToMillicast();
}

void UMillicastSubscriberComponent::OnConnectionError(const FString& Error)
{
	UE_LOG(LogMillicastPlayer, Log, TEXT("Millicast WebSocket Connection error : %s"), *Error);
}

void UMillicastSubscriberComponent::OnClosed(int32 StatusCode,
                                     const FString& Reason,
                                     bool bWasClean)
{
	UE_LOG(LogMillicastPlayer, Log, TEXT("Millicast WebSocket Closed"))
}

void UMillicastSubscriberComponent::OnMessage(const FString& Msg)
{
	FScopeLock Lock(&CriticalPcSection);

	UE_LOG(LogMillicastPlayer, Log, TEXT("Millicast WebSocket new Message : %s"), *Msg);

	TSharedPtr<FJsonObject> ResponseJson;
	auto Reader = TJsonReaderFactory<>::Create(Msg);

	if(FJsonSerializer::Deserialize(Reader, ResponseJson)) 
	{
		FString Type;
		if(!ResponseJson->TryGetStringField("type", Type)) return;

		auto * func = MessageParser.Find(Type);

		if (func == nullptr)
		{
			UE_LOG(LogMillicastPlayer, Warning, TEXT("WebSocket response type not handled (yet?) %s"), *Type);
			return;
		}

		(*func)(ResponseJson);
	}
}

void UMillicastSubscriberComponent::SendCommand(const FString& Name, TSharedPtr<FJsonObject> Data)
{
	auto Payload = MakeShared<FJsonObject>();
	Payload->SetStringField("type", "cmd");
	Payload->SetNumberField("transId", std::rand());
	Payload->SetStringField("name", *Name);
	Payload->SetObjectField("data", Data);

	FString StringStream;
	auto Writer = TJsonWriterFactory<>::Create(&StringStream);
	FJsonSerializer::Serialize(Payload, Writer);

	UE_LOG(LogMillicastPlayer, Log, TEXT("Send command : %s \n Data : %s"), *Name, *StringStream);

	if (WS) WS->Send(StringStream);
}

void UMillicastSubscriberComponent::ParseResponse(TSharedPtr<FJsonObject> JsonMsg)
{
	const TSharedPtr<FJsonObject>* DataJson;
	if (JsonMsg->TryGetObjectField("data", DataJson))
	{
		FString Sdp = (*DataJson)->GetStringField("sdp");
		FString ServerId = (*DataJson)->GetStringField("subscriberId");
		FString ClusterId = (*DataJson)->GetStringField("clusterId");

		UE_LOG(LogMillicastPlayer, Log, TEXT("Server Id : %s"), *ServerId);
		UE_LOG(LogMillicastPlayer, Log, TEXT("Cluster Id : %s"), *ClusterId);

		PeerConnection->SetRemoteDescription(Millicast::Player::to_string(Sdp));
		PeerConnection->ServerId = MoveTemp(ServerId);
		PeerConnection->ClusterId = MoveTemp(ClusterId);
	}
}

void UMillicastSubscriberComponent::ParseError(TSharedPtr<FJsonObject> JsonMsg)
{
	FString errorMessage;
	auto dataJson = JsonMsg->TryGetStringField("data", errorMessage);

	UE_LOG(LogMillicastPlayer, Error, TEXT("WebSocket error : %s"), *errorMessage);
}

void UMillicastSubscriberComponent::ParseEvent(TSharedPtr<FJsonObject> JsonMsg)
{
	FString eventName;
	JsonMsg->TryGetStringField("name", eventName);

	UE_LOG(LogMillicastPlayer, Log, TEXT("Received event : %s"), *eventName);

	EventBroadcaster[eventName](JsonMsg);
}

void UMillicastSubscriberComponent::ParseActiveEvent(TSharedPtr<FJsonObject> JsonMsg)
{
	auto DataJson = JsonMsg->GetObjectField("data");

	FString StreamId = DataJson->GetStringField("streamId");
	FString SourceId;
	TArray<FMillicastTrackInfo> Tracks;
	const TArray<TSharedPtr<FJsonValue>>* TracksJson;

	DataJson->TryGetStringField("sourceId", SourceId);

	if (DataJson->TryGetArrayField("tracks", TracksJson))
	{
		for (auto& t : *TracksJson)
		{
			FMillicastTrackInfo TrackInfo;
			TrackInfo.Media = t->AsObject()->GetStringField("media");
			TrackInfo.TrackId = t->AsObject()->GetStringField("trackId");

			Tracks.Emplace(MoveTemp(TrackInfo));
		}
	}

	OnActive.Broadcast(StreamId, Tracks, SourceId);
}

void UMillicastSubscriberComponent::ParseInactiveEvent(TSharedPtr<FJsonObject> JsonMsg)
{
	auto DataJson = JsonMsg->GetObjectField("data");

	FString StreamId = DataJson->GetStringField("streamId");
	FString SourceId;
	DataJson->TryGetStringField("sourceId", SourceId);

	OnInactive.Broadcast(StreamId, SourceId);
}

void UMillicastSubscriberComponent::ParseStoppedEvent(TSharedPtr<FJsonObject>)
{
	OnStopped.Broadcast();
}
void UMillicastSubscriberComponent::ParseVadEvent(TSharedPtr<FJsonObject> JsonMsg)
{
	auto DataJson = JsonMsg->GetObjectField("data");

	FString Mid = DataJson->GetStringField("mediaId");
	FString SourceId;
	DataJson->TryGetStringField("sourceId", SourceId);

	OnVad.Broadcast(Mid, SourceId);
}

void UMillicastSubscriberComponent::ParseLayersEvent(TSharedPtr<FJsonObject> JsonMsg)
{
	auto DataJson = JsonMsg->GetObjectField("data");
	auto MediaJson = DataJson->GetObjectField("medias");

	for (auto& it : MediaJson->Values)
	{
		TArray<FMillicastLayerData> ActiveLayers;
		TArray<FMillicastLayerData> InactiveLayers;

		const TArray<TSharedPtr<FJsonValue>>* layers;
		if (it.Value->AsObject()->TryGetArrayField("layers", layers))
		{
			for (auto& l : *layers)
			{
				FMillicastLayerData data;
				data.EncodingId = l->AsObject()->GetStringField("encodingId");
				data.TemporalLayerId = l->AsObject()->GetIntegerField("temporalLayerId");
				data.SpatialLayerId = l->AsObject()->GetIntegerField("spatialLayerId");

				ActiveLayers.Push(MoveTemp(data));
			}
		}

		OnLayers.Broadcast(it.Key, ActiveLayers, InactiveLayers);
	}
}

void UMillicastSubscriberComponent::ParseViewerCountEvent(TSharedPtr<FJsonObject> JsonMsg)
{
	auto DataJson = JsonMsg->GetObjectField("data");

	int Count = DataJson->GetIntegerField("viewercount");

	OnViewerCount.Broadcast(Count);
}