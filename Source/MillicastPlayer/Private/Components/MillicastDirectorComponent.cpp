// Copyright CoSMoSoftware 2021. All Rights Reserved.

#include "Components/MillicastDirectorComponent.h"

#include "Http.h"

#include "Dom/JsonValue.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"

#include "Util.h"

#include "MillicastPlayerPrivate.h"

UMillicastDirectorComponent::UMillicastDirectorComponent(const FObjectInitializer& Initializer)
	: Super(Initializer)
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
}

void UMillicastDirectorComponent::BeginPlay()
{
	Super::BeginPlay();

	UE_LOG(LogMillicastPlayer, Verbose, TEXT("%S"), __FUNCTION__);
}

void UMillicastDirectorComponent::ChangeTimeUntilNextRetryInSeconds(float Value)
{
	TimeUntilNextRetryInSeconds = Value;
	SetComponentTickEnabled(Value > 0.0f);
}

void UMillicastDirectorComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if(TimeUntilNextRetryInSeconds <= 0.0f)
	{
		return;
	}

	if(TimeUntilNextRetryInSeconds - DeltaTime > 0.0f)
	{
		TimeUntilNextRetryInSeconds -= DeltaTime;
		return;
	}

	Authenticate();
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

		OnAuthenticated.Broadcast(this, SignalingData);
	}
}

void UMillicastDirectorComponent::CancelAuthenticateRetry()
{
	ChangeTimeUntilNextRetryInSeconds(0.0f);
}

void UMillicastDirectorComponent::RetryAuthenticateWithDelay()
{
	// We are already trying
	if(TimeUntilNextRetryInSeconds > 0.0f)
	{
		return;
	}
	
	// Retry every 1-3 seconds. Do not reconnect at the exact same time to distribute load
	ChangeTimeUntilNextRetryInSeconds(1.0f + rand() % 2);

	OnAuthenticationRetry.Broadcast(TimeUntilNextRetryInSeconds);
}

/**
	Begin receiving audio, video.
*/
bool UMillicastDirectorComponent::Authenticate()
{
	// TODO [RW] need to gate this, so that we cannot run more than one authenticate call at the same time
	// As a hotfix, to make sure that automatic and manual authenticate cannot cause a double call, we are resetting the automatic retry time here
	ChangeTimeUntilNextRetryInSeconds(0.0f);

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

#if MILLICAST_HAS_CXX20
	PostHttpRequest->OnProcessRequestComplete().BindWeakLambda(this, [=, this](FHttpRequestPtr Request, FHttpResponsePtr Response, bool bConnectedSuccessfully)
#else
	PostHttpRequest->OnProcessRequestComplete().BindWeakLambda(this, [=](FHttpRequestPtr Request, FHttpResponsePtr Response, bool bConnectedSuccessfully)
#endif
	{
		check(IsInGameThread());
		
		if(!Response)
		{
			UE_LOG(LogMillicastPlayer, Error, TEXT("Director HTTP request failed without a response"));
			OnAuthenticationFailure.Broadcast(-1, TEXT("No response"));
			return;
		}
			
		if (!bConnectedSuccessfully || Response->GetResponseCode() != 200 /*HTTP_OK*/)
		{
			UE_LOG(LogMillicastPlayer, Warning, TEXT("Director HTTP request failed [code] %d [response] %s \n [body] %s"), Response->GetResponseCode(), *Response->GetContentType(), *Response->GetContentAsString());

			// Authentication failure, do not retry
			if(Response->GetResponseCode() == 401)
			{
				const FString& ErrorMsg = Response->GetContentAsString();
				OnAuthenticationFailure.Broadcast(Response->GetResponseCode(), ErrorMsg);
				return;
			}

			// Otherwise retry with an exponential backoff, and add a few seconds variation. Limit the base of the backoff to 64s
			RetryAuthenticateWithDelay();
			return;
		}

		ParseDirectorResponse(Response);
	});

	return PostHttpRequest->ProcessRequest();
}

