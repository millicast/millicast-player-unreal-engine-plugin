// Copyright CoSMoSoftware 2021. All Rights Reserved.

#include "MillicastDirectorComponent.h"

#include "Http.h"

#include "Dom/JsonValue.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonWriter.h"

#include "MillicastPlayerPrivate.h"

UMillicastDirectorComponent::UMillicastDirectorComponent(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {}

/**
	Initialize this component with the media source required for receiving Millicast audio, video.
	Returns false, if the MediaSource is already been set. This is usually the case when this component is
	initialized in Blueprints.
*/
bool UMillicastDirectorComponent::Initialize(UMillicastMediaSource* InMediaSource)
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
bool UMillicastDirectorComponent::Authenticate()
{
  constexpr auto HTTP_OK = 200;

  UE_LOG(LogMillicastPlayer, Log, TEXT("Authenticate"));
  if(!IsValid(MillicastMediaSource)) return false;

  UE_LOG(LogMillicastPlayer, Log, TEXT("Calling director api"));
  auto PostHttpRequest = FHttpModule::Get().CreateRequest();
  PostHttpRequest->SetURL(MillicastMediaSource->GetUrl());
  PostHttpRequest->SetVerb("POST");
  PostHttpRequest->SetHeader("Content-Type", "application/json");
  PostHttpRequest->SetHeader("Authorization", "NoAuth");
  auto data = MakeShared<FJsonObject>();

  data->SetStringField("streamAccountId", MillicastMediaSource->AccountId);
  data->SetStringField("streamName", MillicastMediaSource->StreamName);
  data->SetStringField("unauthorizedSubscribe", "true");

  FString stream;
  auto writer = TJsonWriterFactory<>::Create(&stream);
  FJsonSerializer::Serialize(data, writer);

  PostHttpRequest->SetContentAsString(stream);

  PostHttpRequest->OnProcessRequestComplete()
      .BindLambda([this, &PostHttpRequest](FHttpRequestPtr Request,
                  FHttpResponsePtr Response,
                  bool bConnectedSuccessfully) {
    if(bConnectedSuccessfully && Response->GetResponseCode() == HTTP_OK) {
          UE_LOG(LogMillicastPlayer, Log, TEXT("Director HTTP resquest successfull"));
          FString DataString = Response->GetContentAsString();
          TSharedPtr<FJsonObject> DataJson;
          auto Reader = TJsonReaderFactory<>::Create(DataString);

          if(FJsonSerializer::Deserialize(Reader, DataJson)) {
              FMillicastSignalingData data;
              TSharedPtr<FJsonObject> DataField = DataJson->GetObjectField("data");
              data.Jwt = DataField->GetStringField("jwt");
              auto ws = DataField->GetArrayField("urls")[0];
              FString WsUrl;
              ws->TryGetString(data.WsUrl);

              UE_LOG(LogMillicastPlayer, Log, TEXT("WsUrl : %s \njwt : %s"),
                     *data.WsUrl, *data.Jwt);

              OnAuthenticated.Broadcast(data);
          }
    }
    else {
        UE_LOG(LogMillicastPlayer, Error, TEXT("Director HTTP request failed %d %s"), Response->GetResponseCode(), *Response->GetContentType());
        FString ErrorMsg = Response->GetContentAsString();
        OnAuthenticationFailure.Broadcast(Response->GetResponseCode(), ErrorMsg);
    }
  });

  return PostHttpRequest->ProcessRequest();
}

