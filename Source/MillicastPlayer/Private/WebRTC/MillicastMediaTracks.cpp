#include "MillicastMediaTracks.h"

#include <api/video/i420_buffer.h>
#include <common_video/libyuv/include/webrtc_libyuv.h>

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

	for (auto& consumer : VideoConsumers)
	{
		if (auto c = consumer.Get())
		{
			c->OnFrame(Buffer, VideoFrame.width(), VideoFrame.height());
		}
		else
		{
			VideoConsumers.Remove(consumer);
		}
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
	return false;
}

void UMillicastVideoTrackImpl::SetEnabled(bool Enabled)
{
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
