// Copyright Dolby.io 2024. All Rights Reserved.

#include "MillicastVideoDecoderFactory.h"

#include "absl/strings/match.h"
#include "modules/video_coding/codecs/vp8/include/vp8.h"
#include "modules/video_coding/codecs/vp9/include/vp9.h"

#include "VideoHardwareDecoder.h"
#include "VideoDecoderVPX.h"
#include "MillicastPlayerPrivate.h"

#if ENGINE_MAJOR_VERSION < 5 || ENGINE_MINOR_VERSION == 0
#include "media/base/h264_profile_level_id.h"
#else
#include "api/video_codecs/h264_profile_level_id.h"
#endif

namespace Millicast::Player
{

#if WEBRTC_VERSION < 96
#define H264_Level webrtc::H264::Level
#define H264_Profile webrtc::H264::Profile
#define H264_ProfileLevelId webrtc::H264::ProfileLevelId
#define H264_ProfileLevelIdToString webrtc::H264::ProfileLevelIdToString
#else
#define H264_Level webrtc::H264Level
#define H264_Profile webrtc::H264Profile
#define H264_ProfileLevelId webrtc::H264ProfileLevelId
#define H264_ProfileLevelIdToString webrtc::H264ProfileLevelIdToString

webrtc::SdpVideoFormat CreateH264Format(H264_Profile profile, H264_Level level)
{
	const absl::optional<std::string> profile_string = H264_ProfileLevelIdToString(H264_ProfileLevelId(profile, level));
	check(profile_string);

	return webrtc::SdpVideoFormat(cricket::kH264CodecName,
		{
			{ cricket::kH264FmtpProfileLevelId, *profile_string },
			{ cricket::kH264FmtpLevelAsymmetryAllowed, "1" },
			{ cricket::kH264FmtpPacketizationMode, "1" }
		});
}

#endif

std::vector<webrtc::SdpVideoFormat> FMillicastVideoDecoderFactory::GetSupportedFormats() const
{
	std::vector<webrtc::SdpVideoFormat> VideoFormats;
	// VideoFormats.push_back(CreateH264Format(H264_Profile::kProfileMain, H264_Level::kLevel1));
	VideoFormats.push_back(webrtc::SdpVideoFormat(cricket::kVp8CodecName));
	VideoFormats.push_back(webrtc::SdpVideoFormat(cricket::kVp9CodecName));
	VideoFormats.push_back(CreateH264Format(H264_Profile::kProfileConstrainedBaseline, H264_Level::kLevel3_1));
	VideoFormats.push_back(CreateH264Format(H264_Profile::kProfileBaseline, H264_Level::kLevel3_1));
	return VideoFormats;
}

std::unique_ptr<webrtc::VideoDecoder> FMillicastVideoDecoderFactory::CreateVideoDecoder(
	const webrtc::SdpVideoFormat& format)
{
	if (absl::EqualsIgnoreCase(format.name, cricket::kVp8CodecName))
	{
		return std::make_unique<Millicast::Player::FVideoDecoderVPX>(8);
	}

	if (absl::EqualsIgnoreCase(format.name, cricket::kVp9CodecName))
	{
		return std::make_unique<Millicast::Player::FVideoDecoderVPX>(9);
	}

	if (absl::EqualsIgnoreCase(format.name, cricket::kH264CodecName))
	{
		return std::make_unique<Millicast::Player::FVideoHardwareDecoder>();
	}

	UE_LOG(LogMillicastPlayer, Warning, TEXT("CreateVideoEncoder called with unknown encoder: %s"), *FString(format.name.c_str()));
	return nullptr;
}

}