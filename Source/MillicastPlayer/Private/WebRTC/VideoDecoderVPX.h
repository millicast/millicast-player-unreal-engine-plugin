// Copyright Dolby.io 2024. All Rights Reserved.

#pragma once

#include "WebRTC/WebRTCInc.h"

namespace Millicast::Player
{

	class FVideoDecoderVPX : public webrtc::VideoDecoder
	{
	public:
		FVideoDecoderVPX(int VPXVersion);
		~FVideoDecoderVPX() override;

		bool Configure(const Settings& settings) override;
		int32 Decode(const webrtc::EncodedImage& input_image, bool missing_frames, int64_t render_time_ms) override;
		int32 RegisterDecodeCompleteCallback(webrtc::DecodedImageCallback* callback) override;
		int32 Release() override;

	private:
		std::unique_ptr<webrtc::VideoDecoder> WebRTCDecoder;
	};

}