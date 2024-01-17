// Copyright Dolby.io 2024. All Rights Reserved.

#include "VideoDecoderVPX.h"

namespace Millicast::Player
{

	FVideoDecoderVPX::FVideoDecoderVPX(int VPXVersion)
	{
		if (VPXVersion == 8)
		{
			WebRTCDecoder = webrtc::VP8Decoder::Create();
		}
		else if (VPXVersion == 9)
		{
			WebRTCDecoder = webrtc::VP9Decoder::Create();
		}
		else
		{
			checkf(false, TEXT("Bad VPX version number supplied to VideoDecoderVPX"));
		}
	}

	FVideoDecoderVPX::~FVideoDecoderVPX()
	{
	}

	bool FVideoDecoderVPX::Configure(const Settings& settings)
	{
		return WebRTCDecoder->Configure(settings);
	}

	int32 FVideoDecoderVPX::Decode(const webrtc::EncodedImage& input_image, bool missing_frames, int64_t render_time_ms)
	{
		return WebRTCDecoder->Decode(input_image, missing_frames, render_time_ms);
	}

	int32 FVideoDecoderVPX::RegisterDecodeCompleteCallback(webrtc::DecodedImageCallback* callback)
	{
		return WebRTCDecoder->RegisterDecodeCompleteCallback(callback);
	}

	int32 FVideoDecoderVPX::Release()
	{
		return WebRTCDecoder->Release();
	}

}