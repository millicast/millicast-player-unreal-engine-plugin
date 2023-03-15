// Copyright CoSMoSoftware 2021. All Rights Reserved.

#include "MillicastMediaSource.h"
#include "MillicastPlayerPrivate.h"

#include <api/video/i420_buffer.h>
#include <common_video/libyuv/include/webrtc_libyuv.h>

#include "Async/Async.h"
#include <RenderTargetPool.h>

UMillicastMediaSource::UMillicastMediaSource()
	: Buffer(nullptr)
{
	StreamUrl = "https://director.millicast.com/api/director/subscribe";
}

bool UMillicastMediaSource::Initialize(const FMillicastSignalingData& /*data*/)
{
	Buffer = nullptr;
	BufferSize = 0;
	return true;
}

void UMillicastMediaSource::BeginDestroy()
{
	delete [] Buffer;
	Buffer = nullptr;
	BufferSize = 0;

	AsyncTask(ENamedThreads::ActualRenderingThread, [this]() {
		FScopeLock Lock(&RenderSyncContext);
		RenderTarget = nullptr;
	});

	Super::BeginDestroy();
}

/*
 * IMediaOptions interface
 */

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

/*
 * UMediaSource interface
 */

FString UMillicastMediaSource::GetUrl() const
{
	return StreamUrl;
}

bool UMillicastMediaSource::Validate() const
{
	return !StreamName.IsEmpty() && !AccountId.IsEmpty() &&
		(!bUseSubscribeToken || (bUseSubscribeToken && !SubscribeToken.IsEmpty()));
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

void UMillicastMediaSource::PostEditChangeChainProperty(struct FPropertyChangedChainEvent& InPropertyChangedEvent)
{
	Super::PostEditChangeChainProperty(InPropertyChangedEvent);
}
#endif //WITH_EDITOR
