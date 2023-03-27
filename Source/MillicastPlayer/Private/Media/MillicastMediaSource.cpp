// Copyright CoSMoSoftware 2021. All Rights Reserved.

#include "MillicastMediaSource.h"
#include "MillicastPlayerPrivate.h"
#include "Async/Async.h"

UMillicastMediaSource::UMillicastMediaSource()
{
	StreamUrl = "https://director.millicast.com/api/director/subscribe";
}

void UMillicastMediaSource::BeginDestroy()
{
	UE_LOG(LogMillicastPlayer, Verbose, TEXT("%S"), __FUNCTION__);

	AsyncTask(ENamedThreads::ActualRenderingThread, [this](){
		FScopeLock Lock(&RenderSyncContext);
		RenderTarget = nullptr;
	});

	Super::BeginDestroy();
}

FString UMillicastMediaSource::GetMediaOption(const FName& Key, const FString& DefaultValue) const
{
	if (Key == MillicastPlayerOption::StreamName)
	{
		return StreamName;
	}

	if (Key == MillicastPlayerOption::AccountId)
	{
		return AccountId;
	}

	if (Key == MillicastPlayerOption::SubscribeToken)
	{
		return SubscribeToken;
	}
	
	return Super::GetMediaOption(Key, DefaultValue);
}

bool UMillicastMediaSource::HasMediaOption(const FName& Key) const
{
	if (Key == MillicastPlayerOption::StreamName || 
		Key == MillicastPlayerOption::AccountId ||
		Key == MillicastPlayerOption::SubscribeToken)
	{
		return true;
	}

	return Super::HasMediaOption(Key);
}

bool UMillicastMediaSource::Validate() const
{
	if( StreamName.IsEmpty() || AccountId.IsEmpty() )
	{
		return false;
	}

	return (!bUseSubscribeToken || !SubscribeToken.IsEmpty());
}

#if WITH_EDITOR
bool UMillicastMediaSource::CanEditChange(const FProperty* InProperty) const
{
	if (!Super::CanEditChange(InProperty))
	{
		return false;
	}

	FString Name;
	InProperty->GetName(Name);

	if (Name == MillicastPlayerOption::SubscribeToken.ToString())
	{
		return bUseSubscribeToken;
	}

	return true;
}
#endif //WITH_EDITOR
