#include "MillicastMediaTracks.h"
#include "MillicastPlayerPrivate.h"
#include "PeerConnection.h"
#include "SampleBuffer.h"
#include "Async/Async.h"
#include "WebRTC/AudioDeviceModule.h"
#include "Util.h"
#include <common_video/libyuv/include/webrtc_libyuv.h>

#include "MillicastUtil.h"

/** Video */

void UMillicastVideoTrackImpl::OnFrame(const webrtc::VideoFrame& VideoFrame)
{
	const int32 Width = VideoFrame.width();
	const int32 Height = VideoFrame.height();
	
	if(CachedResolution.X != Width || CachedResolution.Y != Height)
	{
		CachedResolution.X = Width;
		CachedResolution.Y = Height;
		
#if MILLICAST_HAS_CXX20
		AsyncTask(ENamedThreads::GameThread, [=, this]()
#else
		AsyncTask(ENamedThreads::GameThread, [=]()
#endif
		{
			OnVideoResolutionChanged.Broadcast(Width, Height);
		});
	}
	
#if MILLICAST_HAS_CXX20
	AsyncGameThreadTask(this, [=, this]()
#else
	AsyncGameThreadTask(this, [=]()
#endif
	{
		constexpr auto WEBRTC_PIXEL_FORMAT = webrtc::VideoType::kARGB;

		const size_t Size = webrtc::CalcBufferSize(WEBRTC_PIXEL_FORMAT, VideoFrame.width(), VideoFrame.height());
		if (Size != Buffer.Num())
		{
			Buffer.Empty();
			Buffer.AddZeroed(Size);
		}

		webrtc::ConvertFromI420(VideoFrame, WEBRTC_PIXEL_FORMAT, 0, Buffer.GetData());
		
		{
			FScopeLock Lock(&CriticalSection);

			for (int32 Index = VideoConsumers.Num() - 1; Index >= 0; --Index)
			{
				auto& ConsumerRef = VideoConsumers[Index];
				if (auto* Consumer = ConsumerRef.Get())
				{
					Consumer->OnFrame(Buffer, VideoFrame.width(), VideoFrame.height());
					continue;
				}

				UE_LOG(LogMillicastPlayer, Warning, TEXT("Removing invalid consumer"));
				VideoConsumers.RemoveAtSwap(Index);
			}
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

	if (!VideoConsumer)
	{
		UE_LOG(LogMillicastPlayer, Warning, TEXT("Could not add video consumer. Object was null"));
		return;
	}

	TWeakInterfacePtr<IMillicastVideoConsumer> consumer;
	consumer = VideoConsumer;

	{
		FScopeLock Lock(&CriticalSection);

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
}

void UMillicastVideoTrackImpl::RemoveConsumer(TScriptInterface<IMillicastVideoConsumer> VideoConsumer)
{
	UE_LOG(LogMillicastPlayer, Verbose, TEXT("%S"), __FUNCTION__);

	TWeakInterfacePtr<IMillicastVideoConsumer> consumer;
	consumer = VideoConsumer;

	{
		FScopeLock Lock(&CriticalSection);

		VideoConsumers.Remove(consumer);

		if (VideoConsumers.Num() == 0)
		{
			UE_LOG(LogMillicastPlayer, VeryVerbose, TEXT("Remove Video Sink"));
			auto track = static_cast<webrtc::VideoTrackInterface*>(RtcVideoTrack.get());
			track->RemoveSink(this);
		}
	}
}

/** Audio */

void UMillicastAudioTrackImpl::OnData(const void* AudioData, int BitPerSample, int SampleRate, size_t NumberOfChannels, size_t NumberOfFrames)
{
	// TODO [RW] this is bad, we limit usage to one AudioDeviceModule. Needs to not be static anymore
	if (!Millicast::Player::FAudioDeviceModule::ReadDataAvailable)
	{
		// Do not error log here, this is normal if no audio is being streamed
		return;
	}
	
	if (SampleRate != 48000)
	{
		UE_LOG(LogMillicastPlayer, Verbose, TEXT("Audio sample rate is not 48kHz"));
		return;
	}

	Audio::TSampleBuffer<int16> Buffer(static_cast<const int16*>(AudioData), NumberOfFrames * NumberOfChannels, NumberOfChannels, SampleRate);

	// Queue Audio Data for Consumers
	{
		FScopeLock Lock(&CriticalSection);

		for (auto& ConsumerRef : AudioConsumers)
		{
			if (auto* Consumer = ConsumerRef.Get())
			{
				const auto& Params = Consumer->GetAudioParameters();
				if (Params.NumberOfChannels != NumberOfChannels)
				{
					Buffer.MixBufferToChannels(Params.NumberOfChannels);
				}

				Consumer->QueueAudioData((uint8*)Buffer.GetData(), NumberOfFrames);
			}
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

	if (!AudioConsumer)
	{
		UE_LOG(LogMillicastPlayer, Warning, TEXT("Could not add audio consumer. Object was null"));
		return;
	}

	TWeakInterfacePtr<IMillicastExternalAudioConsumer> WeakConsumer;
	WeakConsumer = AudioConsumer;
	
	{
		FScopeLock Lock(&CriticalSection);

		// Don't add consumer if already there
		if (AudioConsumers.Contains(WeakConsumer))
		{
			UE_LOG(LogMillicastPlayer, Warning, TEXT("[UMillicastAudioTrackImpl::AddConsumer] already contains consumer that was attempted to be added"));
			return;
		}

		if (AudioConsumers.Num() == 0)
		{
			UE_LOG(LogMillicastPlayer, VeryVerbose, TEXT("Adding audio sink"));
			auto track = static_cast<webrtc::AudioTrackInterface*>(RtcAudioTrack.get());
			track->AddSink(this);
		}

		WeakConsumer->Initialize();

		AudioConsumers.Add(WeakConsumer);
	}
}

void UMillicastAudioTrackImpl::RemoveConsumer(TScriptInterface<IMillicastExternalAudioConsumer> AudioConsumer)
{
	UE_LOG(LogMillicastPlayer, Verbose, TEXT("%S"), __FUNCTION__);

	TWeakInterfacePtr<IMillicastExternalAudioConsumer> consumer;
	consumer = AudioConsumer;
	consumer->Shutdown();

	{
		FScopeLock Lock(&CriticalSection);

		AudioConsumers.Remove(consumer);

		if (AudioConsumers.Num() == 0)
		{
			UE_LOG(LogMillicastPlayer, VeryVerbose, TEXT("Remove audio sink"));
			auto track = static_cast<webrtc::AudioTrackInterface*>(RtcAudioTrack.get());
			track->RemoveSink(this);
		}
	}	
}
