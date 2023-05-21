// Copyright Millicast 2022. All Rights Reserved.
#pragma once

#include "Runtime/Launch/Resources/Version.h"

#if ENGINE_MAJOR_VERSION >= 5 && ENGINE_MINOR_VERSION > 0

#include "PlayerStatsData.h"
#include "WebRTCInc.h"

class FCommonViewportClient;

namespace Millicast::Player
{
	class MILLICASTPLAYER_API FPlayerStatsCollector : public webrtc::RTCStatsCollectorCallback
	{
	public:
		DECLARE_EVENT_OneParam(FPlayerStatsCollector, FPlayerStatsCollectorOnStats, const rtc::scoped_refptr<const webrtc::RTCStatsReport>& /*Report*/);
		FPlayerStatsCollectorOnStats OnStats;

	public:
		FPlayerStatsCollector(class FWebRTCPeerConnection* InPeerConnection);
		~FPlayerStatsCollector();

		void Poll();

		const FString& GetClusterId() const;
		const FString& GetServerId() const;

		const FPlayerStatsData& GetData() const { return Data; }

	protected:
		void AddRef() const override;
		rtc::RefCountReleaseStatus Release() const override;

		// Begin RTCStatsCollectorCallback interface
		void OnStatsDelivered(const rtc::scoped_refptr<const webrtc::RTCStatsReport>& report) override;

	private:
		FWebRTCPeerConnection* PeerConnection;
		mutable int32 RefCount;
		FPlayerStatsData Data;
	};
}
#endif
