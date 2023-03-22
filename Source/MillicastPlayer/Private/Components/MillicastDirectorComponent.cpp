// Copyright CoSMoSoftware 2021. All Rights Reserved.

#include "MillicastDirectorComponent.h"

#include "Http.h"

#include "Dom/JsonValue.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"

#include "Util.h"

#include "MillicastPlayerPrivate.h"

void UMillicastDirectorComponent::BeginPlay()
{
	Super::BeginPlay();

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

void UMillicastDirectorComponent::ParseIceServers(const TArray<TSharedPtr<FJsonValue>>& IceServersField, FMillicastSignalingData& SignalingData)
{
	using namespace Millicast::Player;

	SignalingData.IceServers.Empty();
	for (auto& elt : IceServersField)
	{
		const TSharedPtr<FJsonObject>* IceServerJson;
		const bool Success = elt->TryGetObject(IceServerJson);
		if (!Success)
		{
			UE_LOG(LogMillicastPlayer, Warning, TEXT("Could not read ice server json"));
			continue;
		}

		webrtc::PeerConnectionInterface::IceServer IceServer;
		{
			TArray<FString> IceServerUrls;
			if ((*IceServerJson)->TryGetStringArrayField("urls", IceServerUrls))
			{
				for (auto& Url : IceServerUrls)
				{
					IceServer.urls.push_back(to_string(Url));
				}
			}
		}

		{
			FString IceServerUsername;
			if ((*IceServerJson)->TryGetStringField("username", IceServerUsername))
			{
				IceServer.username = to_string(IceServerUsername);
			}
		}

		{
			FString IceServerPassword;
			if((*IceServerJson)->TryGetStringField("credential", IceServerPassword))
			{
				IceServer.password = to_string(IceServerPassword);
			}
		}
		
		SignalingData.IceServers.Emplace(MoveTemp(IceServer));
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
	if (!IsValid(MillicastMediaSource))
	{
		return false;
	}

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
		.BindLambda([this](FHttpRequestPtr Request, FHttpResponsePtr Response, bool bConnectedSuccessfully)
	{
		if(!Response)
		{
			UE_LOG(LogMillicastPlayer, Error, TEXT("Director HTTP request failed without a response"));
			OnAuthenticationFailure.Broadcast(-1, TEXT("No response"));
			return;
		}
			
		if (!bConnectedSuccessfully || Response->GetResponseCode() != 200 /*HTTP_OK*/)
		{
			UE_LOG(LogMillicastPlayer, Error, TEXT("Director HTTP request failed [code] %d [response] %s \n [body] %s"), Response->GetResponseCode(), *Response->GetContentType(), *Response->GetContentAsString());

			const FString& ErrorMsg = Response->GetContentAsString();
			OnAuthenticationFailure.Broadcast(Response->GetResponseCode(), ErrorMsg);
			return;
		}

		ParseDirectorResponse(Response);
	});

	return PostHttpRequest->ProcessRequest();
}

