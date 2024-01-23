// Copyright Dolby.io 2024. All Rights Reserved.

#pragma once

#include "WebRTC/WebRTCInc.h"

namespace Millicast::Player
{
	class FMillicastVideoDecoderFactoryBase: public webrtc::VideoDecoderFactory
	{
		TUniquePtr<webrtc::VideoDecoderFactory> DecoderImpl;

	public:

		FMillicastVideoDecoderFactoryBase();
		~FMillicastVideoDecoderFactoryBase() = default;

		virtual std::vector<webrtc::SdpVideoFormat> GetSupportedFormats() const override;
		virtual std::unique_ptr<webrtc::VideoDecoder> CreateVideoDecoder(const webrtc::SdpVideoFormat& format) override;
	};

	class FMillicastVideoDecoderFactory : public webrtc::VideoDecoderFactory
	{
	public:
		virtual std::vector<webrtc::SdpVideoFormat> GetSupportedFormats() const override;
		virtual std::unique_ptr<webrtc::VideoDecoder> CreateVideoDecoder(const webrtc::SdpVideoFormat& format) override;
	};
}