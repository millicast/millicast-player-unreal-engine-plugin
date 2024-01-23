// Copyright Dolby.io 2024. All Rights Reserved.

#pragma once

#if WITH_AVCODECS

#include "WebRTC/WebRTCInc.h"
#include "Video/Decoders/Configs/VideoDecoderConfigH264.h"
#include "Video/Resources/VideoResourceRHI.h"

namespace Millicast::Player
{
	class FVideoHardwareDecoder : public webrtc::VideoDecoder
	{
	public:
		FVideoHardwareDecoder();
		~FVideoHardwareDecoder();

		bool Configure(const Settings& settings) override;
		int32 Decode(const webrtc::EncodedImage& input_image, bool missing_frames, int64_t render_time_ms) override;
		int32 RegisterDecodeCompleteCallback(webrtc::DecodedImageCallback* callback) override;
		int32 Release() override;

	private:
		TSharedPtr<TVideoDecoder<FVideoResourceRHI>> HawdwareDecoder;

		webrtc::DecodedImageCallback* Callback = nullptr;

		uint32 FrameCount = 0;
	};
}

#endif