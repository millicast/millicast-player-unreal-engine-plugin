// Copyright Millicast 2022. All Rights Reserved.

#pragma once

#include "IMillicastMediaTrack.h"

#include <api/media_stream_interface.h>
#include <UObject/WeakInterfacePtr.h>

#include "MillicastMediaTracks.generated.h"

UCLASS(BlueprintType, Blueprintable, Category = "MIllicastPlayer")
class MILLICASTPLAYER_API UMillicastVideoTrackImpl : public UMillicastVideoTrack, public rtc::VideoSinkInterface<webrtc::VideoFrame>
{
	GENERATED_BODY()

private:
	rtc::scoped_refptr<webrtc::MediaStreamTrackInterface> RtcVideoTrack;
	FString Mid;

	TArray<TWeakInterfacePtr<IMillicastVideoConsumer>> VideoConsumers;

	FCriticalSection CriticalSection;

	TArray<uint8> Buffer;

protected:
	/* VideoSinkInterface */
	void OnFrame(const webrtc::VideoFrame& VideoFrame) override;

public:
	void Initialize(FString InMid, rtc::scoped_refptr<webrtc::MediaStreamTrackInterface> InVideoTrack);

	/* UMillicastMediaTrack overrides */
	FString GetMid() const noexcept override;

	FString GetTrackId() const noexcept override;

	FString GetKind() const noexcept override;

	bool IsEnabled() const noexcept override;

	void SetEnabled(bool Enabled);

	/* UMillicastVideoTrack overrides */
	void AddConsumer(TScriptInterface<IMillicastVideoConsumer> VideoConsumer) override;
};

