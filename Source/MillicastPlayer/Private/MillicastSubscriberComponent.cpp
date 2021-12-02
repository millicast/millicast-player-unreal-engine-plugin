// Copyright CoSMoSoftware 2021. All Rights Reserved.

#include "MillicastSubscriberComponent.h"
#include "MillicastPlayerPrivate.h"

#include <string>

#include "Json.h"
#include "WebSocketsModule.h"
#include "IWebSocket.h"
#include "PeerConnection.h"

inline std::string to_string(const FString& Str)
{
  auto Ansi = StringCast<ANSICHAR>(*Str, Str.Len());
  std::string Res{ Ansi.Get(), static_cast<SIZE_T>(Ansi.Length()) };
  return Res;
}

inline FString ToString(const std::string& Str)
{
  auto Conv = StringCast<TCHAR>(Str.c_str(), Str.size());
  FString Res{ Conv.Length(), Conv.Get() };
  return Res;
}


UMillicastSubscriberComponent::UMillicastSubscriberComponent(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
  PeerConnection = nullptr;
}

/**
	Initialize this component with the media source required for receiving Millicast audio, video.
	Returns false, if the MediaSource is already been set. This is usually the case when this component is
	initialized in Blueprints.
*/
bool UMillicastSubscriberComponent::Initialize(UMillicastMediaSource* InMediaSource)
{
  UE_LOG(LogMillicastPlayer, Log, TEXT("Initialize Subsciber Component"));
	if (MillicastMediaSource == nullptr && InMediaSource != nullptr)
	{
		MillicastMediaSource = InMediaSource;
	}

	return InMediaSource != nullptr && InMediaSource == MillicastMediaSource;
}

/**
	Begin receiving audio, video.
*/
bool UMillicastSubscriberComponent::Subscribe(const FMillicastSignalingData& InSignalingData)
{
  UE_LOG(LogMillicastPlayer, Log, TEXT("Subscribe"));
	return IsValid(MillicastMediaSource) &&
	    MillicastMediaSource->Initialize(InSignalingData) &&
	    StartWebSocketConnection(InSignalingData.WsUrl, InSignalingData.Jwt);
}

/**
	Attempts to stop receiving audio, video.
*/
void UMillicastSubscriberComponent::Unsubscribe()
{
  if(PeerConnection)
  {
    delete PeerConnection;
    PeerConnection = nullptr;

    WS->Close();
    WS = nullptr;
  }
  if (IsValid(MillicastMediaSource))
  {
          // MillicastMediaSource->Shutdown();
  }
}

bool UMillicastSubscriberComponent::StartWebSocketConnection(const FString& Url,
                                                     const FString& Jwt)
{
  UE_LOG(LogMillicastPlayer, Log, TEXT("Create WebSocket connection"));

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
  UE_LOG(LogMillicastPlayer, Log, TEXT("Subscribe to Millicast Stream"));

  PeerConnection =
		FWebRTCPeerConnection::Create(FWebRTCPeerConnection::GetDefaultConfig());

  auto * create_sdo = PeerConnection->GetCreateDescriptionObserver();
  auto * local_sdo  = PeerConnection->GetLocalDescriptionObserver();
  auto * remote_sdo = PeerConnection->GetRemoteDescriptionObserver();

  create_sdo->on_success([this](const std::string& type, const std::string& sdp) {
      UE_LOG(LogMillicastPlayer, Log, TEXT("pc.createOffer() | sucess\nsdp : %s"), *ToString(sdp));
	  PeerConnection->SetLocalDescription(sdp, type);
    });

  create_sdo->on_failure([](const std::string& err) {
    UE_LOG(LogMillicastPlayer, Error, TEXT("pc.createOffer() | Error: %s"), *ToString(err));
  });

  local_sdo->on_success([this]() {
    UE_LOG(LogMillicastPlayer, Log, TEXT("pc.setLocalDescription() | sucess"));
    std::string sdp;
    (*PeerConnection)->local_description()->ToString(&sdp);

    auto DataJson = MakeShared<FJsonObject>();
    DataJson->SetStringField("streamId", "kaqs278x");
    DataJson->SetStringField("sdp", ToString(sdp));

    auto Payload = MakeShared<FJsonObject>();
    Payload->SetStringField("type", "cmd");
    Payload->SetNumberField("transId", std::rand());
    Payload->SetStringField("name", "view");
    Payload->SetObjectField("data", DataJson);

    FString StringStream;
    auto Writer = TJsonWriterFactory<>::Create(&StringStream);
    FJsonSerializer::Serialize(Payload, Writer);

    WS->Send(StringStream);
  });

  local_sdo->on_failure([](const std::string& err) {
     UE_LOG(LogMillicastPlayer, Error, TEXT("Set local description failed : %s"), *ToString(err));
  });

  remote_sdo->on_success([this]() {
     UE_LOG(LogMillicastPlayer, Log, TEXT("Set remote description suceeded"));
  });
  remote_sdo->on_failure([](const std::string& err) {
    UE_LOG(LogMillicastPlayer, Error, TEXT("Set remote description failed : %s"), *ToString(err));
  });

  PeerConnection->OaOptions.offer_to_receive_video = true;
  PeerConnection->OaOptions.offer_to_receive_audio = true;
  PeerConnection->SetVideoSink(MillicastMediaSource);

  UE_LOG(LogMillicastPlayer, Log, TEXT("Creating offer ..."));
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
  UE_LOG(LogMillicastPlayer, Log, TEXT("Millicast WebSocket new Message : %s"), *Msg);

  TSharedPtr<FJsonObject> ResponseJson;
  auto Reader = TJsonReaderFactory<>::Create(Msg);

  if(FJsonSerializer::Deserialize(Reader, ResponseJson)) {
    FString Type;
    if(!ResponseJson->TryGetStringField("type", Type)) return;

    if(Type == "response") {
      auto DataJson = ResponseJson->GetObjectField("data");
      FString Sdp = DataJson->GetStringField("sdp");
	  PeerConnection->SetRemoteDescription(to_string(Sdp));
    }
    else if(Type == "error") {

    }
    else if(Type == "event") {

    }
    else {

    }
  }
}
