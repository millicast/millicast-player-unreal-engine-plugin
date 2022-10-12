#include "MillicastMediaTracks.h"

#include <api/video/i420_buffer.h>
#include <common_video/libyuv/include/webrtc_libyuv.h>

#include "MillicastPlayerPrivate.h"
#include "PeerConnection.h"
#include "WebRTC/AudioDeviceModule.h"

/** Video */

void UMillicastVideoTrackImpl::OnFrame(const webrtc::VideoFrame& VideoFrame)
{
	constexpr auto WEBRTC_PIXEL_FORMAT = webrtc::VideoType::kARGB;

	FScopeLock Lock(&CriticalSection);

	uint32_t Size = webrtc::CalcBufferSize(WEBRTC_PIXEL_FORMAT,
		VideoFrame.width(),
		VideoFrame.height());

	if (Size != Buffer.Num())
	{
		Buffer.Empty();
		Buffer.AddZeroed(Size);
	}

	webrtc::ConvertFromI420(VideoFrame, WEBRTC_PIXEL_FORMAT, 0, Buffer.GetData());
	TArray<TWeakInterfacePtr<IMillicastExternalAudioConsumer>> removals;  //Track the AudioConsumers to clean up

	for (auto& consumer : VideoConsumers)
	{
		if (auto c = consumer.Get())
		{
			c->OnFrame(Buffer, VideoFrame.width(), VideoFrame.height());
		}
		else
		{
			removals.Add(consumer);
		}
	}

	for (auto& consumer : removals)
	{
		VideoConsumers.Remove(consumer);
	}
}

void UMillicastVideoTrackImpl::Initialize(FString InMid, rtc::scoped_refptr<webrtc::MediaStreamTrackInterface> InVideoTrack)
{
	Mid = MoveTemp(InMid);
	RtcVideoTrack = InVideoTrack;
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

void UMillicastVideoTrackImpl::AddConsumer(TScriptInterface<IMillicastVideoConsumer> VideoConsumer)
{
	FScopeLock Lock(&CriticalSection);

	if (VideoConsumers.Num() == 0)
	{
		auto track = static_cast<webrtc::VideoTrackInterface*>(RtcVideoTrack.get());
		track->AddOrUpdateSink(this, rtc::VideoSinkWants{});
	}

	TWeakInterfacePtr<IMillicastVideoConsumer> consumer;
	consumer = VideoConsumer;

	VideoConsumers.Add(consumer);
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

	for (auto& consumer : AudioConsumers)
	{
		if (auto c = consumer.Get())
		{
			auto Params = consumer->GetAudioParameters();
			if (Params.NumberOfChannels != NumberOfChannels)
			{
				Buffer.MixBufferToChannels(Params.NumberOfChannels);
			}

			consumer->QueueAudioData((uint8*)Buffer.GetData(), NumberOfFrames);
		}
	}
}

UMillicastAudioTrackImpl::~UMillicastAudioTrackImpl()
{
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

void UMillicastAudioTrackImpl::AddConsumer(TScriptInterface<IMillicastExternalAudioConsumer> AudioConsumer)
{
	FScopeLock Lock(&CriticalSection);

	if (AudioConsumers.Num() == 0)
	{
		auto track = static_cast<webrtc::AudioTrackInterface*>(RtcAudioTrack.get());
		track->AddSink(this);
	}

	TWeakInterfacePtr<IMillicastExternalAudioConsumer> consumer;
	consumer = AudioConsumer;
	consumer->Initialize();

	AudioConsumers.Add(consumer);
}
