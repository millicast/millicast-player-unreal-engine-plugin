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

UMillicastSubscriberComponent::UMillicastSubscriberComponent(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	PeerConnection = nullptr;
	WS = nullptr;

	PeerConnectionConfig = FWebRTCPeerConnection::GetDefaultConfig();

	// Event received from websocket signaling
	EventBroadcaster.Emplace("active", [this](TSharedPtr<FJsonObject> Msg) { ParseActiveEvent(Msg); });
	EventBroadcaster.Emplace("inactive", [this](TSharedPtr<FJsonObject> Msg) { ParseInactiveEvent(Msg); });
	EventBroadcaster.Emplace("stopped", [this](TSharedPtr<FJsonObject> Msg) { ParseStoppedEvent(Msg); });
	EventBroadcaster.Emplace("vad", [this](TSharedPtr<FJsonObject> Msg) { ParseVadEvent(Msg); });
	EventBroadcaster.Emplace("layers", [this](TSharedPtr<FJsonObject> Msg) { ParseLayersEvent(Msg); });
	EventBroadcaster.Emplace("viewercount", [this](TSharedPtr<FJsonObject> Msg) { ParseViewerCountEvent(Msg); });
}

UMillicastSubscriberComponent::~UMillicastSubscriberComponent()
{
    Unsubscribe();
}

/**
	Initialize this component with the media source required for receiving Millicast audio, video.
	Returns false, if the MediaSource is already been set. This is usually the case when this component is
	initialized in Blueprints.
*/
bool UMillicastSubscriberComponent::Initialize(UMillicastMediaSource* InMediaSource)
{
	if (MillicastMediaSource == nullptr && InMediaSource != nullptr)
	{
		MillicastMediaSource = InMediaSource;
	}

	return InMediaSource != nullptr && InMediaSource == MillicastMediaSource;
}

/**
	Begin receiving audio, video.
*/
bool UMillicastSubscriberComponent::Subscribe(const FMillicastSignalingData& InSignalingData, TScriptInterface<IMillicastExternalAudioConsumer> InExternalAudioConsumer)
{
    ExternalAudioConsumer = InExternalAudioConsumer;

	for (auto& s : InSignalingData.IceServers) 
	{
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
	FScopeLock Lock(&CriticalPcSection);
	
	if (WS)
	{
		WS->Close();
		WS = nullptr;
	}

	if(PeerConnection)
	{
		delete PeerConnection;
		PeerConnection = nullptr;
	}
}

void UMillicastSubscriberComponent::Project(const FString& SourceId, const TArray<FMillicastProjectionData>& ProjectionData)
{
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
	if (!FModuleManager::Get().IsModuleLoaded("WebSockets"))
	{
		FModuleManager::Get().LoadModule("WebSockets");
	}

	WS = FWebSocketsModule::Get().CreateWebSocket(Url + "?token=" + Jwt);

	OnConnectedHandle = WS->OnConnected().AddLambda([this]() { OnConnected(); });
	OnConnectionErrorHandle = WS->OnConnectionError().AddLambda([this](const FString& Error) { OnConnectionError(Error); });
	OnClosedHandle = WS->OnClosed().AddLambda([this](int32 StatusCode, const FString& Reason, bool bWasClean) { OnClosed(StatusCode, Reason, bWasClean); });
	OnMessageHandle = WS->OnMessage().AddLambda([this](const FString& Msg) { OnMessage(Msg); });

	WS->Connect();

	return true;
}

bool UMillicastSubscriberComponent::SubscribeToMillicast()
{
	PeerConnection =
		FWebRTCPeerConnection::Create(FWebRTCPeerConnection::GetDefaultConfig(), ExternalAudioConsumer);

	auto * CreateSessionDescriptionObserver = PeerConnection->GetCreateDescriptionObserver();
	auto * LocalDescriptionObserver  = PeerConnection->GetLocalDescriptionObserver();
	auto * RemoteDescriptionObserver = PeerConnection->GetRemoteDescriptionObserver();

	CreateSessionDescriptionObserver->SetOnSuccessCallback([this](const std::string& type, const std::string& sdp) {
		FScopeLock Lock(&CriticalPcSection);
		UE_LOG(LogMillicastPlayer, Log, TEXT("pc.createOffer() | sucess\nsdp : %s"), *ToString(sdp));
		if(PeerConnection) PeerConnection->SetLocalDescription(sdp, type);
	});

	CreateSessionDescriptionObserver->SetOnFailureCallback([this](const std::string& err) {
		UE_LOG(LogMillicastPlayer, Error, TEXT("pc.createOffer() | Error: %s"), *ToString(err));
		OnSubscribedFailure.Broadcast(FString{ err.c_str() });
	});

	LocalDescriptionObserver->SetOnSuccessCallback([this]() {
		FScopeLock Lock(&CriticalPcSection);
		if (!PeerConnection) return;

		UE_LOG(LogMillicastPlayer, Log, TEXT("pc.setLocalDescription() | sucess"));
		std::string sdp;
		(*PeerConnection)->local_description()->ToString(&sdp);

		// Add events we want to receive from millicast
		TArray<TSharedPtr<FJsonValue>> eventsJson;
		TArray<FString> EvKeys;
		EventBroadcaster.GetKeys(EvKeys);

		for (auto& ev : EvKeys)
		{
			eventsJson.Add(MakeShared<FJsonValueString>(ev));
		}

		// Fill Signaling data
		auto DataJson = MakeShared<FJsonObject>();
		DataJson->SetStringField("streamId", MillicastMediaSource->StreamName);
		DataJson->SetStringField("sdp", ToString(sdp));
		DataJson->SetArrayField("events", eventsJson);

		SendCommand("view", DataJson);
	});

	LocalDescriptionObserver->SetOnFailureCallback([this](const std::string& err) {
		UE_LOG(LogMillicastPlayer, Error, TEXT("Set local description failed : %s"), *ToString(err));
		OnSubscribedFailure.Broadcast(FString{ err.c_str() });
	});

	RemoteDescriptionObserver->SetOnSuccessCallback([this]() {
		UE_LOG(LogMillicastPlayer, Log, TEXT("Set remote description suceeded"));
		OnSubscribed.Broadcast();
	});
	RemoteDescriptionObserver->SetOnFailureCallback([this](const std::string& err) {
		UE_LOG(LogMillicastPlayer, Error, TEXT("Set remote description failed : %s"), *ToString(err));
		OnSubscribedFailure.Broadcast(FString{ err.c_str()});
	});

	PeerConnection->OaOptions.offer_to_receive_video = true;
	PeerConnection->OaOptions.offer_to_receive_audio = true;

	using RtcTrack = rtc::scoped_refptr<webrtc::MediaStreamTrackInterface>;
	PeerConnection->OnVideoTrack = [this](const std::string& Mid, RtcTrack Track) {
		auto videoTrack = NewObject<UMillicastVideoTrackImpl>();
		videoTrack->Initialize(Mid.c_str(), Track);

		OnVideoTrack.Broadcast(videoTrack);
	};
	PeerConnection->OnAudioTrack = [this](const std::string& mid, RtcTrack Track) {

	};

	PeerConnection->CreateOffer();

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

		if(Type == "response") 
		{
			const TSharedPtr<FJsonObject>* DataJson;
			if (ResponseJson->TryGetObjectField("data", DataJson))
			{
				FString Sdp = (* DataJson)->GetStringField("sdp");
				PeerConnection->SetRemoteDescription(to_string(Sdp));
			}
		}
		else if(Type == "error")
		{
			FString errorMessage;
			auto dataJson = ResponseJson->TryGetStringField("data", errorMessage);

			UE_LOG(LogMillicastPlayer, Error, TEXT("WebSocket error : %s"), *errorMessage);
		}
		else if(Type == "event") 
		{
			FString eventName;
			ResponseJson->TryGetStringField("name", eventName);

			UE_LOG(LogMillicastPlayer, Log, TEXT("Received event : %s"), *eventName);

			EventBroadcaster[eventName](ResponseJson);
		}
		else 
		{
			UE_LOG(LogMillicastPlayer, Warning, TEXT("WebSocket response type not handled (yet?) %s"), *Type);
		}
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