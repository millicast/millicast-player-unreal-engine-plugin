// Copyright CoSMoSoftware 2021. All Rights Reserved.

#include "MillicastDirectorComponent.h"

#include "Http.h"

#include "Dom/JsonValue.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonWriter.h"

#include "MillicastPlayerPrivate.h"

constexpr auto HTTP_OK = 200;

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
	if(!IsValid(MillicastMediaSource)) return false;

	auto PostHttpRequest = FHttpModule::Get().CreateRequest();
	PostHttpRequest->SetURL(MillicastMediaSource->GetUrl());
	PostHttpRequest->SetVerb("POST");
	PostHttpRequest->SetHeader("Content-Type", "application/json");
	auto RequestData = MakeShared<FJsonObject>();

	RequestData->SetStringField("streamAccountId", MillicastMediaSource->AccountId);
	RequestData->SetStringField("streamName", MillicastMediaSource->StreamName);

	if (MillicastMediaSource->bUseSubscribeToken)
	{
		UE_LOG(LogMillicastPlayer, Log, TEXT("Using secure viewer"));
		PostHttpRequest->SetHeader("Authorization", "Bearer " + MillicastMediaSource->SubscribeToken);
	}
	else
	{
		UE_LOG(LogMillicastPlayer, Log, TEXT("Using unsecure viewer"));
		PostHttpRequest->SetHeader("Authorization", "NoAuth");
		RequestData->SetStringField("unauthorizedSubscribe", "true");
	}

	FString SerializedRequestData;
	auto JsonWriter = TJsonWriterFactory<>::Create(&SerializedRequestData);
	FJsonSerializer::Serialize(RequestData, JsonWriter);

	PostHttpRequest->SetContentAsString(SerializedRequestData);

	PostHttpRequest->OnProcessRequestComplete()
	  .BindLambda([this](FHttpRequestPtr Request,
				  FHttpResponsePtr Response,
				  bool bConnectedSuccessfully) {
		if(bConnectedSuccessfully && Response->GetResponseCode() == HTTP_OK) {
			FString ResponseDataString = Response->GetContentAsString();
			TSharedPtr<FJsonObject> ResponseDataJson;
			auto JsonReader = TJsonReaderFactory<>::Create(ResponseDataString);

			if(FJsonSerializer::Deserialize(JsonReader, ResponseDataJson)) {
				FMillicastSignalingData SignalingData;
				TSharedPtr<FJsonObject> DataField = ResponseDataJson->GetObjectField("data");
				SignalingData.Jwt = DataField->GetStringField("jwt");
				auto WebSocketUrl = DataField->GetArrayField("urls")[0];
				WebSocketUrl->TryGetString(SignalingData.WsUrl);

				UE_LOG(LogMillicastPlayer, Log, TEXT("WsUrl : %s \njwt : %s"),
					 *SignalingData.WsUrl, *SignalingData.Jwt);

				OnAuthenticated.Broadcast(SignalingData);
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

