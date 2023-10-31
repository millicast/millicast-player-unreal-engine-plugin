// Copyright CoSMoSoftware 2021. All Rights Reserved.

#include "Components/MillicastSubscriberComponent.h"
#include "Audio/MillicastAudioInstance.h"
#include "Components/MillicastDirectorComponent.h"
#include "MillicastPlayerPrivate.h"
#include "MillicastUtil.h"
#include "Util.h"
#include "IWebSocket.h"
#include "WebSocketsModule.h"
#include "Async/Async.h"
#include "Dom/JsonValue.h"
#include "Dom/JsonObject.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Runtime/Launch/Resources/Version.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"
#include "Subsystems/MillicastAudioSubsystem.h"
#include "WebRTC/PeerConnection.h"
#include "WebRTC/PlayerStatsData.h"
#include "WebRTC/MillicastMediaTracks.h"
#include <string>

#include "WebRTC/PlayerStatsCollector.h"

UMillicastSubscriberComponent::UMillicastSubscriberComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
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
	EventBroadcaster.Emplace("migrate", [this](TSharedPtr<FJsonObject> Msg) { ParseMigrateEvent(Msg); });
}

void UMillicastSubscriberComponent::BeginPlay()
{
	Super::BeginPlay();

	UE_LOG(LogMillicastPlayer, Verbose, TEXT("%S"), __FUNCTION__);
}

void UMillicastSubscriberComponent::EndPlay(EEndPlayReason::Type Reason)
{
	Super::EndPlay(Reason);

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

void UMillicastSubscriberComponent::RegisterAudioComponent(UAudioComponent* Component)
{
	AudioComponents.AddUnique(Component);
}

void UMillicastSubscriberComponent::RegisterVideoConsumer(TScriptInterface<IMillicastVideoConsumer> Consumer)
{
	VideoConsumers.AddUnique(Consumer);
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
bool UMillicastSubscriberComponent::Subscribe(UMillicastDirectorComponent* DirectorComponent, const FMillicastSignalingData& InSignalingData)
{
	UE_LOG(LogMillicastPlayer, Verbose, TEXT("%S"), __FUNCTION__);
	
	if (IsConnectionActive())
	{
		UE_LOG(LogMillicastPlayer, Error, TEXT("You are already subscribed to a stream. Please unsubscribed first."));
		return false;
	}

	if (!IsValid(MillicastMediaSource))
	{
		UE_LOG(LogMillicastPlayer, Error, TEXT("Trying to subscribe with invalid media source."));
		return false;
	}

	CachedDirectorComponent = DirectorComponent;
	State = EMillicastSubscriberState::Connecting;
	bShouldReconnect = true;

	for (auto& s : InSignalingData.IceServers)
	{
		UE_LOG(LogMillicastPlayer, Verbose, TEXT("Adding ice server %s"), *InSignalingData.WsUrl);
		PeerConnectionConfig.servers.push_back(s);
	}

	return StartWebSocketConnection(InSignalingData.WsUrl, InSignalingData.Jwt);
}

/**
	Attempts to stop receiving audio, video.
*/
void UMillicastSubscriberComponent::Unsubscribe()
{
	bShouldReconnect = false;
	
	if (WS)
	{
		UE_LOG(LogMillicastPlayer, Verbose, TEXT("Closing web socket"));
		WS->Close();
		WS = nullptr;
	}

	if (!IsConnectionActive())
	{
		return;
	}
	State = EMillicastSubscriberState::Disconnected;

	UE_LOG(LogMillicastPlayer, Verbose, TEXT("%S"), __FUNCTION__);
	FScopeLock Lock(&CriticalPcSection);

	if (PeerConnection)
	{
		UE_LOG(LogMillicastPlayer, Verbose, TEXT("Clearing audio tracks"));

		for (auto& track : AudioTracks)
		{
			static_cast<UMillicastAudioTrackImpl*>(track)->Terminate();
		}

		AudioTracks.Empty();

		UE_LOG(LogMillicastPlayer, Verbose, TEXT("Clearing video tracks"));
		for (auto& track : VideoTracks)
		{
			static_cast<UMillicastVideoTrackImpl*>(track)->Terminate();
		}

		VideoTracks.Empty();

		UE_LOG(LogMillicastPlayer, Verbose, TEXT("Destroying peerconnection"));
		delete PeerConnection;
		PeerConnection = nullptr;
	}
}

void UMillicastSubscriberComponent::EnableFrameTransformer(bool Enable)
{
	bUseFrameTransformer = Enable;
}

void UMillicastSubscriberComponent::Select(const FMillicastLayerData& Layer)
{
	UE_LOG(LogMillicastPlayer, VeryVerbose, TEXT("%S"), __FUNCTION__);

	auto DataJson = MakeShared<FJsonObject>();

	auto data = MakeShared<FJsonObject>();
	data->SetStringField("encodingId", Layer.EncodingId);
	data->SetNumberField("spatialLayerId", Layer.SpatialLayerId);
	data->SetNumberField("temporalLayerId", Layer.TemporalLayerId);

	DataJson->SetObjectField("layer", data);

	SendCommand("select", DataJson);
}

void UMillicastSubscriberComponent::AutoSelect()
{
	UE_LOG(LogMillicastPlayer, VeryVerbose, TEXT("%S"), __FUNCTION__);

	auto DataJson = MakeShared<FJsonObject>();
	auto data = MakeShared<FJsonObject>();

	// send empty object for auto layer selection
	DataJson->SetObjectField("layer", data);

	SendCommand("select", DataJson);
}

FPlayerStatsData UMillicastSubscriberComponent::GetStats() const
{
#if ENGINE_MAJOR_VERSION >= 5 && ENGINE_MINOR_VERSION > 0
	const auto* StatsCollector = GetStatsCollector();
	if(!StatsCollector)
	{
		return {};
	}
	
	return StatsCollector->GetData();
#else
	return {};
#endif
}

#if ENGINE_MAJOR_VERSION >= 5 && ENGINE_MINOR_VERSION > 0
Millicast::Player::FPlayerStatsCollector* UMillicastSubscriberComponent::GetStatsCollector() const
{
	if(!PeerConnection)
	{
		UE_LOG(LogMillicastPlayer, Warning, TEXT("UMillicastSubscriberComponent::GetStatsCollector called without a peer connection"));
		return nullptr;
	}

	return PeerConnection->GetStatsCollector();
}
#endif

bool UMillicastSubscriberComponent::IsConnectionActive() const
{
	return State.Load() != EMillicastSubscriberState::Disconnected;
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

	webrtc::RtpTransceiverInit Init;
	Init.direction = webrtc::RtpTransceiverDirection::kRecvOnly;

	const auto MediaType = (Kind == "audio") ? cricket::MediaType::MEDIA_TYPE_AUDIO : cricket::MediaType::MEDIA_TYPE_VIDEO;

	const auto Result = (*PeerConnection)->AddTransceiver(MediaType, Init);
	if (Result.ok())
	{
		UE_LOG(LogMillicastPlayer, Log, TEXT("Successfully added transceiver for remote track"));
	}
	else
	{
		UE_LOG(LogMillicastPlayer, Error, TEXT("Failed to add transceiver for remote track"));
	}
}

bool UMillicastSubscriberComponent::StartWebSocketConnection(const FString& Url, const FString& Jwt)
{
	UE_LOG(LogMillicastPlayer, Verbose, TEXT("%S"), __FUNCTION__);

	if (!FModuleManager::Get().IsModuleLoaded("WebSockets"))
	{
		UE_LOG(LogMillicastPlayer, VeryVerbose, TEXT("Load WebSocket module"));
		FModuleManager::Get().LoadModule("WebSockets");
	}

	WS = FWebSocketsModule::Get().CreateWebSocket(Url + "?token=" + Jwt);

	// TODO [RW] do we need better multi-threaded handling for these?
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

	PeerConnection = FWebRTCPeerConnection::Create(FWebRTCPeerConnection::GetDefaultConfig());

	auto* CreateSessionDescriptionObserver = PeerConnection->GetCreateDescriptionObserver();
	auto* LocalDescriptionObserver = PeerConnection->GetLocalDescriptionObserver();
	auto* RemoteDescriptionObserver = PeerConnection->GetRemoteDescriptionObserver();

	TWeakObjectPtr<UMillicastSubscriberComponent> WeakThis(this);
	
	CreateSessionDescriptionObserver->SetOnSuccessCallback([=, this](const std::string& type, const std::string& sdp)
	{
		UE_LOG(LogMillicastPlayer, Log, TEXT("pc.createOffer() | sucess\nsdp : %s"), *FString(sdp.c_str()));
		AsyncGameThreadTaskWithCapture(WeakThis, [=, this]()
		{
				// Search for this expression and add the stereo flag to enable stereo
				const std::string s = "minptime=10;useinbandfec=1";
				std::string sdp_non_const = sdp;
				std::ostringstream oss;
				oss << s << "; stereo=1";

				auto pos = sdp.find(s);
				if (pos != std::string::npos)
				{
					sdp_non_const.replace(sdp.find(s), s.size(), oss.str());
				}

				{
					FScopeLock Lock(&CriticalPcSection);
					if (PeerConnection)
					{
						PeerConnection->SetLocalDescription(sdp_non_const, type);
					}
				}
		});
	});

	CreateSessionDescriptionObserver->SetOnFailureCallback([=, this](const std::string& err)
	{
		UE_LOG(LogMillicastPlayer, Error, TEXT("pc.createOffer() | Error: %s"), *FString(err.c_str()));
		AsyncGameThreadTaskWithCapture(WeakThis, [=, this]()
		{
			OnSubscribedFailure.Broadcast(FString{ err.c_str() });
		});
	});

	LocalDescriptionObserver->SetOnSuccessCallback([=, this]()
	{
		UE_LOG(LogMillicastPlayer, Log, TEXT("pc.setLocalDescription() | sucess"));

		AsyncGameThreadTaskWithCapture(WeakThis, [=, this]()
		{
			FScopeLock Lock(&CriticalPcSection);
			if (!PeerConnection)
			{
				return;
			}

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
	});

	LocalDescriptionObserver->SetOnFailureCallback([=, this](const std::string& err)
	{
		UE_LOG(LogMillicastPlayer, Error, TEXT("Set local description failed : %s"), *FString(err.c_str()));

		AsyncGameThreadTaskWithCapture(WeakThis, [=, this]()
		{
			OnSubscribedFailure.Broadcast(FString{ err.c_str() });
		});
	});

	RemoteDescriptionObserver->SetOnSuccessCallback([=, this]()
	{
		UE_LOG(LogMillicastPlayer, Log, TEXT("Set remote description suceeded"));

		AsyncGameThreadTaskWithCapture(WeakThis, [=, this]()
		{
			State = EMillicastSubscriberState::Connected;
			OnSubscribed.Broadcast();
		});
	});

	RemoteDescriptionObserver->SetOnFailureCallback([=, this](const std::string& err)
	{
		UE_LOG(LogMillicastPlayer, Error, TEXT("Set remote description failed : %s"), *FString(err.c_str()));

		AsyncGameThreadTaskWithCapture(WeakThis, [=, this]()
		{
			OnSubscribedFailure.Broadcast(FString{ err.c_str() });
		});
	});

	PeerConnection->OaOptions.offer_to_receive_video = true;
	PeerConnection->OaOptions.offer_to_receive_audio = true;

	using RtcTrack = rtc::scoped_refptr<webrtc::MediaStreamTrackInterface>;

	PeerConnection->OnVideoTrack = [=, this](const std::string& Mid, RtcTrack Track)
	{
		UE_LOG(LogMillicastPlayer, Log, TEXT("OnVideoTrack"));

		AsyncGameThreadTaskWithCapture(WeakThis, [=, this]()
		{
			UE_LOG(LogMillicastPlayer, Verbose, TEXT("Create video track object"));
			auto VideoTrack = NewObject<UMillicastVideoTrackImpl>();
			VideoTrack->Initialize(Mid.c_str(), Track);

			// Registers all VideoConsumers with the track
			for(const auto& VideoConsumer : VideoConsumers)
			{
				VideoTrack->AddConsumer(VideoConsumer);
			}
			
			OnVideoTrack.Broadcast(VideoTrack);
			VideoTracks.Add(VideoTrack);
		});
	};

	PeerConnection->OnAudioTrack = [=, this](const std::string& mid, RtcTrack Track)
	{
		UE_LOG(LogMillicastPlayer, Log, TEXT("OnAudioTrack"));

		AsyncGameThreadTaskWithCapture(WeakThis, [=, this]()
		{
			UE_LOG(LogMillicastPlayer, Verbose, TEXT("Create audio track object"));
			auto* AudioTrack = NewObject<UMillicastAudioTrackImpl>();
			AudioTrack->Initialize(mid.c_str(), Track);

			// Registers all AudioComponents with the track
			auto* Subsystem = GetWorld()->GetGameInstance()->GetSubsystem<UMillicastAudioSubsystem>();
			for(auto* AudioComponent : AudioComponents)
			{
				Subsystem->Register(AudioComponent);
				AudioTrack->AddConsumer(Subsystem->GetInstance(AudioComponent));
			}
			
			OnAudioTrack.Broadcast(AudioTrack);
			AudioTracks.Add(AudioTrack); // keep reference to delete it later
		});
	};

	PeerConnection->OnFrameMetadata = [=, this](uint32 Ssrc, uint32 Timestamp, const TArray<uint8>& Metadata)
	{
		AsyncGameThreadTaskWithCapture(WeakThis, [=, this]()
		{
			OnFrameMetadata.Broadcast(Ssrc, Timestamp, Metadata);
		});
	};

	PeerConnection->EnableFrameTransformer(bUseFrameTransformer);
	PeerConnection->CreateOffer();

#if ENGINE_MAJOR_VERSION >= 5 && ENGINE_MINOR_VERSION > 0
	PeerConnection->EnableStats(true);
#endif

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
	State = EMillicastSubscriberState::Disconnected;

	UE_LOG(LogMillicastPlayer, Log, TEXT("Millicast WebSocket Connection error : %s"), *Error);

	OnDisconnectedInternal(Error);
}

void UMillicastSubscriberComponent::OnClosed(int32 StatusCode,
	const FString& Reason,
	bool bWasClean)
{
	State = EMillicastSubscriberState::Disconnected;

	UE_LOG(LogMillicastPlayer, Log, TEXT("Millicast WebSocket Closed"));

	OnDisconnectedInternal(Reason);
}

void UMillicastSubscriberComponent::OnDisconnectedInternal(const FString& Reason)
{
	OnDisconnected.Broadcast(Reason, bShouldReconnect);

	if( !bShouldReconnect )
	{
		return;
	}

	if( !IsValid(CachedDirectorComponent) )
	{
		UE_LOG(LogMillicastPlayer, Error, TEXT("UMillicastSubscriberComponent disconnected without valid cached director component"));
		return;
	}

	// Need to grab a new JWT so that the connection will succeed. Simply reconnecting the WS will not always work
	CachedDirectorComponent->RetryAuthenticateWithDelay();
}

void UMillicastSubscriberComponent::OnMessage(const FString& Msg)
{
	FScopeLock Lock(&CriticalPcSection);

	UE_LOG(LogMillicastPlayer, Log, TEXT("Millicast WebSocket new Message : %s"), *Msg);

	TSharedPtr<FJsonObject> ResponseJson;
	auto Reader = TJsonReaderFactory<>::Create(Msg);

	if (!FJsonSerializer::Deserialize(Reader, ResponseJson))
	{
		UE_LOG(LogMillicastPlayer, Error, TEXT("[UMillicastSubscriberComponent::OnMessage] Failed to deserialize"));
		return;
	}

	FString Type;
	if (!ResponseJson->TryGetStringField("type", Type))
	{
		UE_LOG(LogMillicastPlayer, Error, TEXT("[UMillicastSubscriberComponent::OnMessage] Missing type field"));
		return;
	}

	auto* func = MessageParser.Find(Type);
	if (!func)
	{
		UE_LOG(LogMillicastPlayer, Warning, TEXT("WebSocket response type not handled (yet?) %s"), *Type);
		return;
	}

	(*func)(ResponseJson);
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

	if (WS)
	{
		WS->Send(StringStream);
	}
}

void UMillicastSubscriberComponent::ParseResponse(TSharedPtr<FJsonObject> JsonMsg)
{
	const TSharedPtr<FJsonObject>* DataJson;
	if (!JsonMsg->TryGetObjectField("data", DataJson))
	{
		return;
	}

	FString Sdp = (*DataJson)->GetStringField("sdp");
	FString ServerId = (*DataJson)->GetStringField("subscriberId");
	FString ClusterId = (*DataJson)->GetStringField("clusterId");

	UE_LOG(LogMillicastPlayer, Log, TEXT("Server Id : %s"), *ServerId);
	UE_LOG(LogMillicastPlayer, Log, TEXT("Cluster Id : %s"), *ClusterId);

	PeerConnection->SetRemoteDescription(Millicast::Player::to_string(Sdp));
	PeerConnection->ServerId = MoveTemp(ServerId);
	PeerConnection->ClusterId = MoveTemp(ClusterId);
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

void UMillicastSubscriberComponent::ParseMigrateEvent(TSharedPtr<FJsonObject> JsonMsg)
{
	bShouldReconnect = true;
	OnDisconnectedInternal("Received migrate event");
}