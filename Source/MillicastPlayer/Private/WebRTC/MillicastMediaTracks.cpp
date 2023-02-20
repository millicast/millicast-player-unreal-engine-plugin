#include "MillicastMediaTracks.h"

#include <api/video/i420_buffer.h>
#include <common_video/libyuv/include/webrtc_libyuv.h>

#include "MillicastPlayerPrivate.h"
#include "PeerConnection.h"
#include "WebRTC/AudioDeviceModule.h"
#include "SampleBuffer.h"

#define WEAK_CAPTURE_VIDEO_TRACK WeakThis = TWeakObjectPtr<UMillicastVideoTrackImpl>(this)

/** Video */

void UMillicastVideoTrackImpl::OnFrame(const webrtc::VideoFrame& VideoFrame)
{
	AsyncTask(ENamedThreads::GameThread, [WEAK_CAPTURE_VIDEO_TRACK, VideoFrame]() {
		constexpr auto WEBRTC_PIXEL_FORMAT = webrtc::VideoType::kARGB;
		
		if (!WeakThis.IsValid())
		{
			UE_LOG(LogMillicastPlayer, Verbose, TEXT("This video track is no more valid."));
			return;
		}

		FScopeLock Lock(&WeakThis->CriticalSection);

		uint32_t Size = webrtc::CalcBufferSize(WEBRTC_PIXEL_FORMAT,
			VideoFrame.width(),
			VideoFrame.height());

		if (Size != WeakThis->Buffer.Num())
		{
			WeakThis->Buffer.Empty();
			WeakThis->Buffer.AddZeroed(Size);
		}

		webrtc::ConvertFromI420(VideoFrame, WEBRTC_PIXEL_FORMAT, 0, WeakThis->Buffer.GetData());
		TArray<TWeakInterfacePtr<IMillicastVideoConsumer>> removals;  //Track the AudioConsumers to clean up

		for (auto& consumer : WeakThis->VideoConsumers)
		{
			if (auto c = consumer.Get())
			{
				c->OnFrame(WeakThis->Buffer, VideoFrame.width(), VideoFrame.height());
			}
			else
			{
				UE_LOG(LogMillicastPlayer, Verbose, TEXT("Removing no more valid consumer"));
				removals.Add(consumer);
			}
		}

		for (auto& consumer : removals)
		{
			WeakThis->VideoConsumers.Remove(consumer);
		}
	});
}

void UMillicastVideoTrackImpl::Initialize(FString InMid, rtc::scoped_refptr<webrtc::MediaStreamTrackInterface> InVideoTrack)
{
	UE_LOG(LogMillicastPlayer, Verbose, TEXT("%S"), __FUNCTION__);

	Mid = MoveTemp(InMid);
	RtcVideoTrack = InVideoTrack;
}

UMillicastVideoTrackImpl::~UMillicastVideoTrackImpl()
{
	UE_LOG(LogMillicastPlayer, Verbose, TEXT("%S"), __FUNCTION__);

	if (VideoConsumers.Num() > 0 && RtcVideoTrack)
	{
		auto track = static_cast<webrtc::VideoTrackInterface*>(RtcVideoTrack.get());
		track->RemoveSink(this);
	}
}

FString UMillicastVideoTrackImpl::GetMid() const noexcept
{
	return Mid;
}

FString UMillicastVideoTrackImpl::GetTrackId() const noexcept
{
	return FString(RtcVideoTrack->id().c_str());
}

FString UMillicastVideoTrackImpl::GetKind() const noexcept
{
	return FString("video");
}

bool UMillicastVideoTrackImpl::IsEnabled() const noexcept
{
	return RtcVideoTrack->enabled();
}

void UMillicastVideoTrackImpl::SetEnabled(bool Enabled)
{
	UE_LOG(LogMillicastPlayer, Verbose, TEXT("%S"), __FUNCTION__);

	RtcVideoTrack->set_enabled(Enabled);
}

void UMillicastVideoTrackImpl::Terminate()
{
	UE_LOG(LogMillicastPlayer, Verbose, TEXT("%S"), __FUNCTION__);

	RtcVideoTrack = nullptr;
	VideoConsumers.Empty();
}

void UMillicastVideoTrackImpl::AddConsumer(TScriptInterface<IMillicastVideoConsumer> VideoConsumer)
{
	UE_LOG(LogMillicastPlayer, Verbose, TEXT("%S"), __FUNCTION__);

	FScopeLock Lock(&CriticalSection);

	if (!VideoConsumer)
	{
		UE_LOG(LogMillicastPlayer, Warning, TEXT("Could not add video consumer. Object was null"));
		return;
	}

	TWeakInterfacePtr<IMillicastVideoConsumer> consumer;
	consumer = VideoConsumer;

	// don't add consumer if already there
	if (VideoConsumers.Contains(consumer))
	{
		UE_LOG(LogMillicastPlayer, Verbose, TEXT("Consumer has already been added"));
		return;
	}

	if (VideoConsumers.Num() == 0)
	{
		UE_LOG(LogMillicastPlayer, VeryVerbose, TEXT("Add Video Sink"));
		auto track = static_cast<webrtc::VideoTrackInterface*>(RtcVideoTrack.get());
		track->AddOrUpdateSink(this, rtc::VideoSinkWants{});
	}

	VideoConsumers.Add(consumer);
}

void UMillicastVideoTrackImpl::RemoveConsumer(TScriptInterface<IMillicastVideoConsumer> VideoConsumer)
{
	UE_LOG(LogMillicastPlayer, Verbose, TEXT("%S"), __FUNCTION__);

	FScopeLock Lock(&CriticalSection);

	TWeakInterfacePtr<IMillicastVideoConsumer> consumer;
	consumer = VideoConsumer;
	VideoConsumers.Remove(consumer);

	if (VideoConsumers.Num() == 0)
	{
		UE_LOG(LogMillicastPlayer, VeryVerbose, TEXT("Remove Video Sink"));
		auto track = static_cast<webrtc::VideoTrackInterface*>(RtcVideoTrack.get());
		track->RemoveSink(this);
	}
}

/** Audio */

void UMillicastAudioTrackImpl::OnData(const void* AudioData, int BitPerSample, int SampleRate, size_t NumberOfChannels, 
	size_t NumberOfFrames)
{
	FScopeLock Lock(&CriticalSection);

	if (!Millicast::Player::FAudioDeviceModule::ReadDataAvailable || SampleRate != 48000)
	{
		UE_LOG(LogMillicastPlayer, Verbose, TEXT("Could not read audio data or the sample rate is not 48kHz"));
		return;
	}

	Audio::TSampleBuffer<int16> Buffer((int16*)AudioData, NumberOfFrames, NumberOfChannels, SampleRate);

	// AudioConsumers.RemoveAll([](auto& consumer) { return consumer.Get() == nullptr; });

	for (auto& consumer : AudioConsumers)
	{
		if (auto c = consumer.Get())
		{
			auto Params = c->GetAudioParameters();
			if (Params.NumberOfChannels != NumberOfChannels)
			{
				Buffer.MixBufferToChannels(Params.NumberOfChannels);
			}

			c->QueueAudioData((uint8*)Buffer.GetData(), NumberOfFrames);
		}
	}
}

UMillicastAudioTrackImpl::~UMillicastAudioTrackImpl()
{
	UE_LOG(LogMillicastPlayer, Verbose, TEXT("%S"), __FUNCTION__);

	if (AudioConsumers.Num() > 0 && RtcAudioTrack)
	{
		UE_LOG(LogMillicastPlayer, Verbose, TEXT("Clearing audio consumers"));
		auto track = static_cast<webrtc::AudioTrackInterface*>(RtcAudioTrack.get());
		track->RemoveSink(this);
	}
}

void UMillicastAudioTrackImpl::Initialize(FString InMid, rtc::scoped_refptr<webrtc::MediaStreamTrackInterface> InAudioTrack)
{
	UE_LOG(LogMillicastPlayer, Verbose, TEXT("%S"), __FUNCTION__);

	Mid = MoveTemp(InMid);
	RtcAudioTrack = InAudioTrack;
}

FString UMillicastAudioTrackImpl::GetMid() const noexcept
{
	return Mid;
}

FString UMillicastAudioTrackImpl::GetTrackId() const noexcept
{
	return FString(RtcAudioTrack->id().c_str());
}

FString UMillicastAudioTrackImpl::GetKind() const noexcept
{
	return FString("audio");
}

bool UMillicastAudioTrackImpl::IsEnabled() const noexcept
{
	return RtcAudioTrack->enabled();
}

void UMillicastAudioTrackImpl::SetEnabled(bool Enabled)
{
	UE_LOG(LogMillicastPlayer, Verbose, TEXT("%S"), __FUNCTION__);
	RtcAudioTrack->set_enabled(Enabled);
}

void UMillicastAudioTrackImpl::Terminate()
{
	UE_LOG(LogMillicastPlayer, Verbose, TEXT("%S"), __FUNCTION__);
	RtcAudioTrack = nullptr;
	AudioConsumers.Empty();
}

void UMillicastAudioTrackImpl::AddConsumer(TScriptInterface<IMillicastExternalAudioConsumer> AudioConsumer)
{
	UE_LOG(LogMillicastPlayer, Verbose, TEXT("%S"), __FUNCTION__);
	FScopeLock Lock(&CriticalSection);

	if (!AudioConsumer)
	{
		UE_LOG(LogMillicastPlayer, Warning, TEXT("Could not add audio consumer. Object was null"));
		return;
	}

	TWeakInterfacePtr<IMillicastExternalAudioConsumer> consumer;
	consumer = AudioConsumer;

	// Don't add consumer if already there
	if (AudioConsumers.Contains(consumer))
	{
		UE_LOG(LogMillicastPlayer, Warning, TEXT("Audio tracks already contains this audio consumer"));
		return;
	}

	if (AudioConsumers.Num() == 0)
	{
		UE_LOG(LogMillicastPlayer, VeryVerbose, TEXT("Adding audio sink"));
		auto track = static_cast<webrtc::AudioTrackInterface*>(RtcAudioTrack.get());
		track->AddSink(this);
	}

	consumer->Initialize();

	AudioConsumers.Add(consumer);
}

void UMillicastAudioTrackImpl::RemoveConsumer(TScriptInterface<IMillicastExternalAudioConsumer> AudioConsumer)
{
	UE_LOG(LogMillicastPlayer, Verbose, TEXT("%S"), __FUNCTION__);
	FScopeLock Lock(&CriticalSection);

	TWeakInterfacePtr<IMillicastExternalAudioConsumer> consumer;
	consumer = AudioConsumer;
	consumer->Shutdown();

	AudioConsumers.Remove(consumer);

	if (AudioConsumers.Num() == 0)
	{
		UE_LOG(LogMillicastPlayer, VeryVerbose, TEXT("Remove audio sink"));
		auto track = static_cast<webrtc::AudioTrackInterface*>(RtcAudioTrack.get());
		track->RemoveSink(this);
	}
}
