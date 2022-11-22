#include "MillicastMediaTracks.h"

#include <api/video/i420_buffer.h>
#include <common_video/libyuv/include/webrtc_libyuv.h>

#include "MillicastPlayerPrivate.h"
#include "PeerConnection.h"
#include "WebRTC/AudioDeviceModule.h"

#define WEAK_CAPTURE_VIDEO_TRACK WeakThis = TWeakObjectPtr<UMillicastVideoTrackImpl>(this)

/** Video */

void UMillicastVideoTrackImpl::OnFrame(const webrtc::VideoFrame& VideoFrame)
{
	AsyncTask(ENamedThreads::GameThread, [WEAK_CAPTURE_VIDEO_TRACK, VideoFrame]() {
		constexpr auto WEBRTC_PIXEL_FORMAT = webrtc::VideoType::kARGB;
		
		if (!WeakThis.IsValid())
		{
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
	Mid = MoveTemp(InMid);
	RtcVideoTrack = InVideoTrack;
}

UMillicastVideoTrackImpl::~UMillicastVideoTrackImpl()
{
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
	RtcVideoTrack->set_enabled(Enabled);
}

void UMillicastVideoTrackImpl::Terminate()
{
	RtcVideoTrack = nullptr;
	VideoConsumers.Empty();
}

void UMillicastVideoTrackImpl::AddConsumer(TScriptInterface<IMillicastVideoConsumer> VideoConsumer)
{
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
		return;
	}

	if (VideoConsumers.Num() == 0)
	{
		auto track = static_cast<webrtc::VideoTrackInterface*>(RtcVideoTrack.get());
		track->AddOrUpdateSink(this, rtc::VideoSinkWants{});
	}

	VideoConsumers.Add(consumer);
}

void UMillicastVideoTrackImpl::RemoveConsumer(TScriptInterface<IMillicastVideoConsumer> VideoConsumer)
{
	FScopeLock Lock(&CriticalSection);

	TWeakInterfacePtr<IMillicastVideoConsumer> consumer;
	consumer = VideoConsumer;
	VideoConsumers.Remove(consumer);

	if (VideoConsumers.Num() == 0)
	{
		auto track = static_cast<webrtc::VideoTrackInterface*>(RtcVideoTrack.get());
		track->RemoveSink(this);
	}
}

/** Audio */

void UMillicastAudioTrackImpl::OnData(const void* AudioData, int BitPerSample, int SampleRate, size_t NumberOfChannels, 
	size_t NumberOfFrames)
{
	FScopeLock Lock(&CriticalSection);

	if (!FAudioDeviceModule::ReadDataAvailable || SampleRate != 48000)
	{
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
	if (AudioConsumers.Num() > 0 && RtcAudioTrack)
	{
		auto track = static_cast<webrtc::AudioTrackInterface*>(RtcAudioTrack.get());
		track->RemoveSink(this);
	}
}

void UMillicastAudioTrackImpl::Initialize(FString InMid, rtc::scoped_refptr<webrtc::MediaStreamTrackInterface> InAudioTrack)
{
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
	RtcAudioTrack->set_enabled(Enabled);
}

void UMillicastAudioTrackImpl::Terminate()
{
	RtcAudioTrack = nullptr;
	AudioConsumers.Empty();
}

void UMillicastAudioTrackImpl::AddConsumer(TScriptInterface<IMillicastExternalAudioConsumer> AudioConsumer)
{
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
		return;
	}

	if (AudioConsumers.Num() == 0)
	{
		auto track = static_cast<webrtc::AudioTrackInterface*>(RtcAudioTrack.get());
		track->AddSink(this);
	}

	consumer->Initialize();

	AudioConsumers.Add(consumer);
}

void UMillicastAudioTrackImpl::RemoveConsumer(TScriptInterface<IMillicastExternalAudioConsumer> AudioConsumer)
{
	FScopeLock Lock(&CriticalSection);

	TWeakInterfacePtr<IMillicastExternalAudioConsumer> consumer;
	consumer = AudioConsumer;
	consumer->Shutdown();

	AudioConsumers.Remove(consumer);

	if (AudioConsumers.Num() == 0)
	{
		auto track = static_cast<webrtc::AudioTrackInterface*>(RtcAudioTrack.get());
		track->RemoveSink(this);
	}
}
