// Copyright CoSMoSoftware 2021. All Rights Reserved.

#include "MillicastMediaPlayer.h"

#include <api/video/i420_buffer.h>
#include <common_video/libyuv/include/webrtc_libyuv.h>

#include "MillicastPlayerPrivate.h"
#include "MillicastMediaSource.h"
#include "IMillicastPlayerModule.h"

#include "HAL/CriticalSection.h"
#include "HAL/PlatformProcess.h"
#include "Misc/ScopeLock.h"
#include "Templates/Atomic.h"

#include "IMediaEventSink.h"
#include "IMediaOptions.h"

#include "MediaIOCoreEncodeTime.h"
#include "MediaIOCoreFileWriter.h"
#include "MediaIOCoreSamples.h"

#include "Engine/GameEngine.h"
#include "Misc/App.h"
#include "Slate/SceneViewport.h"
#include "Stats/Stats2.h"
#include "Styling/SlateStyle.h"
#include "Http.h"
#include "Json.h"
#include "Serialization/JsonWriter.h"

#include "WebSocketsModule.h"
#include "IWebSocket.h"

#include "peerconnection.h"

#if WITH_EDITOR
#include "EngineAnalytics.h"
#endif


#define LOCTEXT_NAMESPACE "MillicastMediaPlayer"

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

/* WebSocket Callback
*****************************************************************************/

void FMillicastMediaPlayer::OnConnected()
{
  UE_LOG(LogMillicastPlayer, Log, TEXT("Millicast WebSocket Connected"));
  SubscribeToMillicast();
}

void FMillicastMediaPlayer::OnConnectionError(const FString& Error)
{
  UE_LOG(LogMillicastPlayer, Log, TEXT("Millicast WebSocket Connection error : %s"), *Error);
}

void FMillicastMediaPlayer::OnClosed(int32 StatusCode,
                                     const FString& Reason,
                                     bool bWasClean)
{
  UE_LOG(LogMillicastPlayer, Log, TEXT("Millicast WebSocket Closed"))
}

void FMillicastMediaPlayer::OnMessage(const FString& Msg)
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
      PeerConnection->set_remote_desc(to_string(Sdp));
    }
    else if(Type == "error") {

    }
    else if(Type == "event") {

    }
    else {

    }
  }
}

/* Millicast API
*****************************************************************************/

bool FMillicastMediaPlayer::AuthenticateToMillicast()
{
  UE_LOG(LogMillicastPlayer, Log, TEXT("Calling director api"));
  auto PostHttpRequest = FHttpModule::Get().CreateRequest();
  PostHttpRequest->SetURL(TEXT("https://director.millicast.com/api/director/subscribe"));
  PostHttpRequest->SetVerb("POST");
  PostHttpRequest->SetHeader("Content-Type", "application/json");
  PostHttpRequest->SetHeader("Authorization", "NoAuth");
  auto data = MakeShared<FJsonObject>();

  data->SetStringField("streamAccountId", "UQH6Fr");
  data->SetStringField("streamName", "kaqs278x");
  data->SetStringField("unauthorizedSubscribe", "true");

  FString stream;
  auto writer = TJsonWriterFactory<>::Create(&stream);
  FJsonSerializer::Serialize(data, writer);

  PostHttpRequest->SetContentAsString(stream);

  PostHttpRequest->OnProcessRequestComplete()
      .BindLambda([this, &PostHttpRequest](FHttpRequestPtr Request,
                  FHttpResponsePtr Response,
                  bool bConnectedSuccessfully) {
    if(bConnectedSuccessfully) {
          UE_LOG(LogMillicastPlayer, Log, TEXT("Director HTTP resquest successfull"));
          FString DataString = Response->GetContentAsString();
          TSharedPtr<FJsonObject> DataJson;
          auto Reader = TJsonReaderFactory<>::Create(DataString);

          if(FJsonSerializer::Deserialize(Reader, DataJson)) {
              TSharedPtr<FJsonObject> DataField = DataJson->GetObjectField("data");
              FString Jwt = DataField->GetStringField("jwt");
              auto ws = DataField->GetArrayField("urls")[0];
              FString WsUrl;
              ws->TryGetString(WsUrl);

              UE_LOG(LogMillicastPlayer, Log, TEXT("WsUrl : %s \njwt : %s"), *WsUrl, *Jwt);
              StartWebSocketConnection(WsUrl, Jwt);
          }
    }
    else {
        UE_LOG(LogMillicastPlayer, Error, TEXT("Director HTTP request failed %d %s"), Response->GetResponseCode(), *Response->GetContentType());
    }
    // PostHttpRequest.Reset();
  });
  return PostHttpRequest->ProcessRequest();
}

bool FMillicastMediaPlayer::StartWebSocketConnection(const FString& Url,
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

bool FMillicastMediaPlayer::SubscribeToMillicast()
{
  UE_LOG(LogMillicastPlayer, Log, TEXT("Subscribe to Millicast Stream"));

  PeerConnection =
      TUniquePtr<millicast::PeerConnection>(
        millicast::PeerConnection::create(millicast::PeerConnection::get_default_config())
        );
  auto * create_sdo = PeerConnection->create_desc_observer();
  auto * local_sdo  = PeerConnection->local_desc_observer();
  auto * remote_sdo = PeerConnection->remote_desc_observer();

  create_sdo->on_success([this](const std::string& type, const std::string& sdp) {
      UE_LOG(LogMillicastPlayer, Log, TEXT("pc.createOffer() | sucess\nsdp : %s"), *ToString(sdp));
      PeerConnection->set_local_desc(sdp, type);
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

  PeerConnection->oa_options.offer_to_receive_video = true;
  PeerConnection->oa_options.offer_to_receive_audio = true;
  PeerConnection->set_video_sink(this);

  UE_LOG(LogMillicastPlayer, Log, TEXT("Creating offer ..."));
  PeerConnection->create_offer();

  return true;
}

/* Video sink interface
*****************************************************************************/
void FMillicastMediaPlayer::OnFrame(const webrtc::VideoFrame& frame)
{
  UE_LOG(LogMillicastPlayer, Log, TEXT("OnFrame"));
  uint32_t Size = webrtc::CalcBufferSize(webrtc::VideoType::kI420,
                                         frame.width(),
                                         frame.height());

  if(Size > BufferSize) {
    if(BufferSize == 0) {
        FMediaIOSamplingSettings VideoSettings = BaseSettings;
        VideoSettings.BufferSize = Size;
        Samples->InitializeVideoBuffer(VideoSettings);
    }

    delete [] Buffer;
    Buffer = new uint8_t[Size];
    BufferSize = Size;
  }

  webrtc::ExtractBuffer(frame, Size, Buffer);

  auto ProcessingTime = frame.processing_time();
  FTimespan DecodedTime(ProcessingTime->Elapsed().ns() / ETimespan::NanosecondsPerTick);
  auto TextureSample = TextureSamplePool->AcquireShared();
  if (TextureSample->Initialize(Buffer
          , Size
          , frame.width()
          , frame.width()
          , frame.height()
          , EMediaTextureSampleFormat::CharUYVY
          , DecodedTime
          , FFrameRate(30,1)
          , TOptional<FTimecode>{}
          , false))
  {
      UE_LOG(LogMillicastPlayer, Log, TEXT("Adding Video Sample"));
          Samples->AddVideo(TextureSample);
  }
  else {
    UE_LOG(LogMillicastPlayer, Error, TEXT("Can't add texture"));
  }
}

/* FMillicastVideoPlayer structors
*****************************************************************************/

FMillicastMediaPlayer::FMillicastMediaPlayer(IMediaEventSink& InEventSink)
	: Super(InEventSink)
	// , EventCallback(nullptr)
	// , AudioSamplePool(new FMillicastMediaAudioSamplePool)
	, TextureSamplePool(new FMillicastMediaTextureSamplePool)
	// , bVerifyFrameDropCount(false)
	, SupportedSampleTypes(EMediaIOSampleType::None)
{
  Buffer = nullptr;
  BufferSize = 0;
}

FMillicastMediaPlayer::~FMillicastMediaPlayer()
{
	Close();
	delete TextureSamplePool;
	// delete AudioSamplePool;
}

/* IMediaPlayer interface
*****************************************************************************/

void FMillicastMediaPlayer::Close()
{
	// AudioSamplePool->Reset();
	TextureSamplePool->Reset();

	Super::Close();
}

FGuid FMillicastMediaPlayer::GetPlayerPluginGUID() const
{
	static FGuid PlayerPluginGUID(0x62a47ff5, 0xf61243a1, 0x9b377536, 0xc906c883);
	return PlayerPluginGUID;
}

/**
 * @EventName MediaFramework.MillicastMediaSourceOpened
 * @Trigger Triggered when a Millicast media source is opened through a media player.
 * @Type Client
 * @Owner MediaIO Team
 */
bool FMillicastMediaPlayer::Open(const FString& Url, const IMediaOptions* Options)
{
  bool bSuccess = false;

  UE_LOG(LogMillicastPlayer, Warning, TEXT("Opening (partially implemented)"));
  bSuccess = AuthenticateToMillicast();
#if WITH_EDITOR
	if (FEngineAnalytics::IsAvailable())
	{
		TArray<FAnalyticsEventAttribute> EventAttributes;

		const int64 ResolutionWidth = Options->GetMediaOption( FMediaIOCoreMediaOption::ResolutionWidth, (int64)1920);
		const int64 ResolutionHeight = Options->GetMediaOption( FMediaIOCoreMediaOption::ResolutionHeight, (int64)1080);

		EventAttributes.Add(FAnalyticsEventAttribute(TEXT("ResolutionWidth"), FString::Printf(TEXT("%d"), ResolutionWidth)));
		EventAttributes.Add(FAnalyticsEventAttribute(TEXT("ResolutionHeight"), FString::Printf(TEXT("%d"), ResolutionHeight)));
		EventAttributes.Add(FAnalyticsEventAttribute(TEXT("FrameRate"), *VideoFrameRate.ToPrettyText().ToString()));

		FEngineAnalytics::GetProvider().RecordEvent(TEXT("MediaFramework.MillicastMediaSourceOpened"), EventAttributes);
	}
#endif

  return bSuccess;
}


void FMillicastMediaPlayer::TickInput(FTimespan, FTimespan)
{}

void FMillicastMediaPlayer::TickFetch(FTimespan, FTimespan)
{}

#if WITH_EDITOR
const FSlateBrush* FMillicastMediaPlayer::GetDisplayIcon() const
{
	return IMillicastPlayerModule::Get().GetStyle()->GetBrush("MillicastMediaIcon");
}
#endif //WITH_EDITOR

bool FMillicastMediaPlayer::IsHardwareReady() const
{
	return true;
}

void FMillicastMediaPlayer::SetupSampleChannels()
{}

#undef LOCTEXT_NAMESPACE
