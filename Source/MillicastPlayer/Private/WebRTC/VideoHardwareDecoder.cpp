// Copyright Dolby.io 2024. All Rights Reserved.

#include "VideoHardwareDecoder.h"

#include "MillicastPlayerPrivate.h"

namespace Millicast::Player
{

struct FFakeDeleter
{
	void operator()(uint8* Object) const {}
};

FVideoHardwareDecoder::FVideoHardwareDecoder() 
{
	FVideoDecoderConfigH264 DecoderConfig;
	HawdwareDecoder = FVideoDecoder::CreateChecked<FVideoResourceRHI>(FAVDevice::GetHardwareDevice(), DecoderConfig);
}

FVideoHardwareDecoder::~FVideoHardwareDecoder()
{

}

bool FVideoHardwareDecoder::Configure(const Settings& settings)
{
	return true;
}

int32 FVideoHardwareDecoder::Decode(const webrtc::EncodedImage& input_image, bool missing_frames, int64_t render_time_ms)
{
	const int64 TimestampDecodeStart = rtc::TimeMillis();

	FAVResult Result = HawdwareDecoder->SendPacket(FVideoPacket(
		MakeShareable<uint8>(input_image.GetEncodedData()->data(), FFakeDeleter()),
		input_image.GetEncodedData()->size(),
		input_image.Timestamp(),
		FrameCount++,
		input_image.qp_,
		input_image._frameType == webrtc::VideoFrameType::kVideoFrameKey));

	if (Callback != nullptr && Result.IsNotError())
	{
		FResolvableVideoResourceRHI DecoderResource;
		const FAVResult DecodeResult = HawdwareDecoder->ReceiveFrame(DecoderResource);

		if (DecodeResult.IsSuccess())
		{
// #if WEBRTC_5414
			// rtc::scoped_refptr<webrtc::VideoFrameBuffer> FrameBuffer = rtc::make_ref_counted<UE::PixelStreaming::FFrameBufferRHI>(DecoderResource);
// #else
			// rtc::scoped_refptr<webrtc::VideoFrameBuffer> FrameBuffer = new rtc::RefCountedObject<UE::PixelStreaming::FFrameBufferRHI>(DecoderResource);
// #endif
			// check(FrameBuffer->width() != 0 && FrameBuffer->height() != 0); // TODO we should probably check that we are getting the frame back that we are expecting

			auto Texture = DecoderResource->GetRaw().Texture;
			auto FrameBuffer = webrtc::I420Buffer::Create(DecoderResource->GetDescriptor().Width, DecoderResource->GetDescriptor().Height);

			webrtc::VideoFrame Frame = webrtc::VideoFrame::Builder()
				.set_video_frame_buffer(FrameBuffer)
				.set_timestamp_rtp(input_image.Timestamp()) // NOTE This timestamp is load bearing as WebRTC stores the frames in a map with this as the index
				.set_color_space(input_image.ColorSpace())
				.build();

			Callback->Decoded(Frame, rtc::TimeMillis() - TimestampDecodeStart, input_image.qp_);

			return WEBRTC_VIDEO_CODEC_OK;
		}
	}
	else
	{
		UE_LOG(LogMillicastPlayer, Warning, TEXT("FVideoDecoderHardware failed to decode frame"));

		return WEBRTC_VIDEO_CODEC_OK_REQUEST_KEYFRAME;
	}

	UE_LOG(LogMillicastPlayer, Error, TEXT("FVideoDecoderHardware could not decode frame"));

	return WEBRTC_VIDEO_CODEC_ERROR;
}
int32 FVideoHardwareDecoder::RegisterDecodeCompleteCallback(webrtc::DecodedImageCallback* callback)
{
	Callback = callback;
	return 0;
}
int32 FVideoHardwareDecoder::Release()
{
	HawdwareDecoder.Reset();
	return 0;
}
	
}
