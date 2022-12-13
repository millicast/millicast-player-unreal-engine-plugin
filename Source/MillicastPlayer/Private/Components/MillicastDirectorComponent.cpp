// Copyright CoSMoSoftware 2021. All Rights Reserved.

#include "MillicastDirectorComponent.h"

#include "Http.h"

#include "Dom/JsonValue.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonWriter.h"

#include "Util.h"

#include "MillicastPlayerPrivate.h"

constexpr auto HTTP_OK = 200;

UMillicastDirectorComponent::UMillicastDirectorComponent(const FObjectInitializer& ObjectInitializer) 
	: Super(ObjectInitializer) 
{
	UE_LOG(LogMillicastPlayer, Verbose, TEXT("%S"), __FUNCTION__);
}

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

void UMillicastDirectorComponent::SetMediaSource(UMillicastMediaSource* InMediaSource)
{
	if (InMediaSource != nullptr)
	{
		MillicastMediaSource = InMediaSource;
	}
	else
	{
		UE_LOG(LogMillicastPlayer, Log, TEXT("Provided MediaSource was nullptr"));
	}
}

void UMillicastDirectorComponent::ParseIceServers(const TArray<TSharedPtr<FJsonValue>>& IceServersField,
	FMillicastSignalingData& SignalingData)
{
	using namespace Millicast::Player;

	SignalingData.IceServers.Empty();
	for (auto& elt : IceServersField)
	{
		const TSharedPtr<FJsonObject>* IceServerJson;
		bool ok = elt->TryGetObject(IceServerJson);

		if (!ok)
		{
			UE_LOG(LogMillicastPlayer, Warning, TEXT("Could not read ice server json"));
			continue;
		}

		TArray<FString> iceServerUrls;
		FString iceServerPassword, iceServerUsername;

		bool hasUrls = (*IceServerJson)->TryGetStringArrayField("urls", iceServerUrls);
		bool hasUsername = (*IceServerJson)->TryGetStringField("username", iceServerUsername);
		bool hasPassword = (*IceServerJson)->TryGetStringField("credential", iceServerPassword);

		webrtc::PeerConnectionInterface::IceServer iceServer;
		if (hasUrls)
		{
			for (auto& url : iceServerUrls)
			{
				iceServer.urls.push_back(to_string(url));
			}
		}
		if (hasUsername)
		{
			iceServer.username = to_string(iceServerUsername);
		}
		if (hasPassword)
		{
			iceServer.password = to_string(iceServerPassword);
		}

		SignalingData.IceServers.Emplace(MoveTemp(iceServer));
	}
}

void UMillicastDirectorComponent::ParseDirectorResponse(FHttpResponsePtr Response)
{
	FString ResponseDataString = Response->GetContentAsString();
	UE_LOG(LogMillicastPlayer, Log, TEXT("Director response : \n %s \n"), *ResponseDataString);

	TSharedPtr<FJsonObject> ResponseDataJson;
	auto JsonReader = TJsonReaderFactory<>::Create(ResponseDataString);

	// Deserialize received JSON message
	if (FJsonSerializer::Deserialize(JsonReader, ResponseDataJson))
	{
		FMillicastSignalingData SignalingData;
		TSharedPtr<FJsonObject> DataField = ResponseDataJson->GetObjectField("data");

		// Extract JSON WebToken, Websocket URL and ice servers configuration
		SignalingData.Jwt = DataField->GetStringField("jwt");
		auto WebSocketUrlField = DataField->GetArrayField("urls")[0];
		auto IceServersField = DataField->GetArrayField("iceServers");

		WebSocketUrlField->TryGetString(SignalingData.WsUrl);

		UE_LOG(LogMillicastPlayer, Log, TEXT("WsUrl : %s \njwt : %s"), *SignalingData.WsUrl, *SignalingData.Jwt);

		ParseIceServers(IceServersField, SignalingData);

		OnAuthenticated.Broadcast(SignalingData);
	}
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
		if(bConnectedSuccessfully && Response->GetResponseCode() == HTTP_OK) 
		{
			ParseDirectorResponse(Response);
		}
		else 
		{
			UE_LOG(LogMillicastPlayer, Error, TEXT("Director HTTP request failed %d %s"), Response->GetResponseCode(), *Response->GetContentType());
			FString ErrorMsg = Response->GetContentAsString();
			OnAuthenticationFailure.Broadcast(Response->GetResponseCode(), ErrorMsg);
		}
	});

	return PostHttpRequest->ProcessRequest();
}

