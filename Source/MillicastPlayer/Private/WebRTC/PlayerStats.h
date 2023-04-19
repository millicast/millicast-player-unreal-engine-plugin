// Copyright Millicast 2022. All Rights Reserved.

#pragma once

#include "Runtime/Launch/Resources/Version.h"

#if ENGINE_MAJOR_VERSION >= 5 && ENGINE_MINOR_VERSION > 0

#include "Tickable.h"
#include "WebRTCInc.h"

class FCanvas;
class FCommonViewportClient;
class FViewport;

namespace Millicast::Player
{

	class FPlayerStatsCollector : public webrtc::RTCStatsCollectorCallback
	{
		double LastVideoStatTimestamp;
		double LastAudioStatTimestamp;

	public:
		FPlayerStatsCollector(class FWebRTCPeerConnection* InPeerConnection);
		~FPlayerStatsCollector();

		void Poll();

		void AddRef() const override;
		rtc::RefCountReleaseStatus Release() const override;

		// Begin RTCStatsCollectorCallback interface
		void OnStatsDelivered(const rtc::scoped_refptr<const webrtc::RTCStatsReport>& report) override;

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

		const FString& Cluster() const;
		const FString& Server() const;

	private:
		FWebRTCPeerConnection* PeerConnection;
		mutable int32 RefCount;
	};

	/*
	 * Some basic performance stats about how the publisher is running, e.g. how long capture/encode takes.
	 * Stats are drawn to screen for now as it is useful to observe them in realtime.
	 */
	class FPlayerStats : FTickableGameObject
	{
	public:
		TArray<FPlayerStatsCollector*> StatsCollectors;

	private:
		// Intent is to access through FPublisherStats::Get()
		static FPlayerStats Instance;
		bool bRegisterEngineStats = false;
		bool LogStatsEnabled = true;

	public:
		static FPlayerStats& Get() { return Instance; }

	        ~FPlayerStats() = default;

		void Tick(float DeltaTime);
		FORCEINLINE TStatId GetStatId() const { RETURN_QUICK_DECLARE_CYCLE_STAT(MillicastPlayerProducerStats, STATGROUP_Tickables); }

		template<typename T>
		std::tuple<T, FString> GetInUnit(T Value, const FString& Unit)
		{
			constexpr auto MEGA = 1'000'000.;
			constexpr auto KILO = 1'000.;

			if (Value >= MEGA)
			{
				return { Value / MEGA, TEXT("M") + Unit };
			}

			if (Value >= KILO)
			{
				return { Value / KILO, TEXT("K") + Unit };
			}

			return { Value, Unit };
		}

		void RegisterStatsCollector(FPlayerStatsCollector* Collector);
		void UnregisterStatsCollector(FPlayerStatsCollector* Collector);

		bool OnToggleStats(UWorld* World, FCommonViewportClient* ViewportClient, const TCHAR* Stream);
		int32 OnRenderStats(UWorld* World, FViewport* Viewport, FCanvas* Canvas, int32 X, int32 Y, const FVector* ViewLocation, const FRotator* ViewRotation);
		void RegisterEngineHooks();
	};

}

#endif
