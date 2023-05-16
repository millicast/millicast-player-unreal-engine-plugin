// Copyright Millicast 2022. All Rights Reserved.
#pragma once

#include "Runtime/Launch/Resources/Version.h"

#if ENGINE_MAJOR_VERSION >= 5 && ENGINE_MINOR_VERSION > 0

#include "WebRTCInc.h"

class FCommonViewportClient;

namespace Millicast::Player
{
	class MILLICASTPLAYER_API FPlayerStatsCollector : public webrtc::RTCStatsCollectorCallback
	{
		double LastVideoStatTimestamp;
		double LastAudioStatTimestamp;

	public:
		DECLARE_EVENT_OneParam(FPlayerStatsCollector, FPlayerStatsCollectorOnStats, const rtc::scoped_refptr<const webrtc::RTCStatsReport>& /*Report*/);
		FPlayerStatsCollectorOnStats OnStats;

	public:
		FPlayerStatsCollector(class FWebRTCPeerConnection* InPeerConnection);
		~FPlayerStatsCollector();

		void Poll();

		const FString& GetClusterId() const;
		const FString& GetServerId() const;

		double Rtt; // ms
		size_t Width; // px
		size_t Height; // px
		size_t FramePerSecond;
		double VideoBitrate; // bps
		double AudioBitrate; // bps
		size_t VideoTotalReceived; // bytes
		size_t AudioTotalReceived; // bytes
		int VideoPacketLoss; // num packets
		int AudioPacketLoss; // num packets
		double VideoJitter;
		double AudioJitter;
		double VideoJitterAverageDelay; // ms
		double AudioJitterAverageDelay; // ms
		FString VideoCodec; // mimetype
		FString AudioCodec; // mimetype
		double LastVideoReceivedTimestamp; // us
		double LastAudioReceivedTimestamp; // us
		double VideoDecodeTime; // s
		double VideoDecodeTimeAverage; // ms
		// int PauseCount; // Not yet in unreal libwebrtc
		// int freezeCount; // Not yet in unreal libwebrtc
		int FramesDropped;
		int SilentConcealedSamples;
		int ConcealedSamples;
		int AudioPacketDiscarded;
		int VideoPacketDiscarded;
		// double AudioProcessingDelay; Not yet in this libwebrtc
		// double VideoProcessingDelay; Not yet in this libwebrtc
		int AudioNackCount;
		int VideoNackCount;

		double Timestamp; // us

	protected:
		void AddRef() const override;
		rtc::RefCountReleaseStatus Release() const override;

		// Begin RTCStatsCollectorCallback interface
		void OnStatsDelivered(const rtc::scoped_refptr<const webrtc::RTCStatsReport>& report) override;

	private:
		FWebRTCPeerConnection* PeerConnection;
		mutable int32 RefCount;
	};
}
#endif
