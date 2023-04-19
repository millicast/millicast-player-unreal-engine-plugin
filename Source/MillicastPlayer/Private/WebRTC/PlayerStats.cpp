#include "PlayerStats.h"

#if ENGINE_MAJOR_VERSION >= 5 && ENGINE_MINOR_VERSION > 0

#include "MillicastPlayerPrivate.h"

#include "Engine/Engine.h"
#include "PeerConnection.h"
#include "ProfilingDebugging/CsvProfiler.h"
#include "Util.h"

DECLARE_LOG_CATEGORY_EXTERN(LogMillicastPlayerStats, Log, All);
DEFINE_LOG_CATEGORY(LogMillicastPlayerStats);

CSV_DEFINE_CATEGORY(Millicast_Player, false);

// --------- FPlayerStatsCollector ------

namespace Millicast::Player
{
	FPlayerStatsCollector::FPlayerStatsCollector(FWebRTCPeerConnection* InPeerConnection)
	{
		FPlayerStats::Get().RegisterStatsCollector(this);

		PeerConnection = InPeerConnection;

		Rtt = 0;
		Width = 0;
		Height = 0;
		FramePerSecond = 0;
		VideoBitrate = 0;
		AudioBitrate = 0;
		VideoTotalReceived = 0;
		AudioTotalReceived = 0;
		VideoPacketLoss = 0;
		AudioPacketLoss = 0;
		VideoJitterAverageDelay = 0;
		AudioJitterAverageDelay = 0;
		LastVideoReceivedTimestamp = 0;
		LastAudioReceivedTimestamp = 0;

		LastAudioStatTimestamp = 0;
		LastVideoStatTimestamp = 0;
	}

	FPlayerStatsCollector::~FPlayerStatsCollector()
	{
		FPlayerStats::Get().UnregisterStatsCollector(this);
	}

	void FPlayerStatsCollector::Poll()
	{
		PeerConnection->PollStats();
	}

	void FPlayerStatsCollector::AddRef() const
	{
		FPlatformAtomics::InterlockedIncrement(&RefCount);
	}

	rtc::RefCountReleaseStatus FPlayerStatsCollector::Release() const
	{
		if (FPlatformAtomics::InterlockedDecrement(&RefCount) == 0)
		{
			return rtc::RefCountReleaseStatus::kDroppedLastRef;
		}

		return rtc::RefCountReleaseStatus::kOtherRefsRemained;
	}

	void FPlayerStatsCollector::OnStatsDelivered(const rtc::scoped_refptr<const webrtc::RTCStatsReport>& Report)
	{
		constexpr uint32_t NUM_US = 1'000'000; // number of microseconds in 1 second

		for (const webrtc::RTCStats& Stats : *Report)
		{
			const FString StatsType = FString(Stats.type());
			const FString StatsId = FString(Stats.id().c_str());

			//UE_LOG(LogMillicastStats, Log, TEXT("Type: %s Id: %s"), *StatsType, *StatsId);

			if (Stats.type() == std::string("inbound-rtp"))
			{
				auto InboundStat = Stats.cast_to<webrtc::RTCInboundRTPStreamStats>();

				if (*InboundStat.kind == webrtc::RTCMediaStreamTrackKind::kVideo)
				{
					auto lastByteCount = VideoTotalReceived;
					auto timestamp = Stats.timestamp_us();

					Width = InboundStat.frame_width.ValueOrDefault(0);
					Height = InboundStat.frame_height.ValueOrDefault(0);
					FramePerSecond = InboundStat.frames_per_second.ValueOrDefault(0);
					VideoTotalReceived = InboundStat.bytes_received.ValueOrDefault(0);
					VideoPacketLoss = InboundStat.packets_lost.ValueOrDefault(-1);
					VideoJitter = InboundStat.jitter.ValueOrDefault(0) * 1000.;
					VideoDecodeTime = InboundStat.total_decode_time.ValueOrDefault(0);
					VideoDecodeTimeAverage = 1000. * VideoDecodeTime / (double)*InboundStat.frames_decoded;
					FramesDropped = InboundStat.frames_dropped.ValueOrDefault(0);
					VideoPacketDiscarded = InboundStat.packets_discarded.ValueOrDefault(0);
					VideoNackCount = InboundStat.nack_count.ValueOrDefault(0);

					auto videoJitterDelay = InboundStat.jitter_buffer_delay.ValueOrDefault(-1);
					auto videoJitterEmitted = InboundStat.jitter_buffer_emitted_count.ValueOrDefault(0);

					if (videoJitterDelay != 0)
					{
						VideoJitterAverageDelay = 1000. * videoJitterDelay / videoJitterEmitted;
					}
					else
					{
						VideoJitterAverageDelay = 0;
					}

					if (LastVideoStatTimestamp != 0 && VideoTotalReceived != lastByteCount)
					{
						VideoBitrate = (VideoTotalReceived - lastByteCount) * NUM_US * 8. / (timestamp - LastVideoStatTimestamp);
					}

					LastVideoStatTimestamp = timestamp;
					LastVideoReceivedTimestamp = InboundStat.last_packet_received_timestamp.ValueOrDefault(-1);

					auto CodecStats = Report->GetStatsOfType<webrtc::RTCCodecStats>();

					auto it = std::find_if(CodecStats.begin(), CodecStats.end(),
						[&InboundStat](auto&& e) { return e->id() == *InboundStat.codec_id; });

					if (it != CodecStats.end())
					{
						VideoCodec = ToString(*(*it)->mime_type);
					}
				}
				else
				{
					auto lastByteCount = AudioTotalReceived;
					auto timestamp = Stats.timestamp_us();

					AudioTotalReceived = InboundStat.bytes_received.ValueOrDefault(0);
					AudioPacketLoss = InboundStat.packets_lost.ValueOrDefault(-1);
					AudioJitter = InboundStat.jitter.ValueOrDefault(0) * 1000.;
					AudioPacketDiscarded = InboundStat.packets_discarded.ValueOrDefault(0);
					AudioNackCount = InboundStat.nack_count.ValueOrDefault(0);
					ConcealedSamples = InboundStat.concealed_samples.ValueOrDefault(0);
					SilentConcealedSamples = InboundStat.silent_concealed_samples.ValueOrDefault(0);

					auto audioJitterDelay = InboundStat.jitter_buffer_delay.ValueOrDefault(-1);
					auto audioJitterEmitted = InboundStat.jitter_buffer_emitted_count.ValueOrDefault(0);

					if (audioJitterDelay != 0)
					{
						AudioJitterAverageDelay = 1000. * audioJitterDelay / audioJitterEmitted;
					}
					else
					{
						AudioJitterAverageDelay = 0;
					}

					if (LastAudioStatTimestamp != 0 && AudioTotalReceived != lastByteCount)
					{
						AudioBitrate = (AudioTotalReceived - lastByteCount) * NUM_US * 8 / (timestamp - LastAudioStatTimestamp);
					}

					LastAudioStatTimestamp = timestamp;
					LastAudioReceivedTimestamp = InboundStat.last_packet_received_timestamp.ValueOrDefault(-1);


					auto CodecStats = Report->GetStatsOfType<webrtc::RTCCodecStats>();

					auto it = std::find_if(CodecStats.begin(), CodecStats.end(),
						[&InboundStat](auto&& e) { return e->id() == *InboundStat.codec_id; });

					if (it != CodecStats.end())
					{
						AudioCodec = ToString(*(*it)->mime_type);
					}
				}
			}
			else if (Stats.type() == std::string("candidate-pair"))
			{
				auto CandidateStat = Stats.cast_to<webrtc::RTCIceCandidatePairStats>();
				Rtt = CandidateStat.current_round_trip_time.ValueOrDefault(0.) * 1000.;
			}
		}

		Timestamp = Report->timestamp_us();
	}

	const FString& FPlayerStatsCollector::Cluster() const
	{
		return PeerConnection->ClusterId;
	}

	const FString& FPlayerStatsCollector::Server() const
	{
		return PeerConnection->ServerId;
	}

	// --------- FPlayerStats -------------

	FPlayerStats FPlayerStats::Instance;

	void FPlayerStats::Tick(float DeltaTime)
	{
		if (!GEngine)
		{
			return;
		}

		if (!bRegisterEngineStats)
		{
			RegisterEngineHooks();
		}
	}

	int32 FPlayerStats::OnRenderStats(UWorld* World, FViewport* Viewport, FCanvas* Canvas, int32 X, int32 Y, const FVector* ViewLocation, const FRotator* ViewRotation)
	{
		int MessageKey = 100;
		int i = 0;

		for (FPlayerStatsCollector* Collector : StatsCollectors)
		{
			Collector->Poll();
			GEngine->AddOnScreenDebugMessage(MessageKey++, 0.0f, FColor::Green, FString::Printf(TEXT("RTT = %.2f ms"), Collector->Rtt), true);
			GEngine->AddOnScreenDebugMessage(MessageKey++, 0.0f, FColor::Green, FString::Printf(TEXT("Video resolution = %dx%d"), Collector->Width, Collector->Height), true);
			GEngine->AddOnScreenDebugMessage(MessageKey++, 0.0f, FColor::Green, FString::Printf(TEXT("FPS = %d"), Collector->FramePerSecond), true);

			auto [VideoBitrate, VideoBitrateUnit] = GetInUnit(Collector->VideoBitrate, TEXT("bps"));
			auto [AudioBitrate, AudioBitrateUnit] = GetInUnit(Collector->AudioBitrate, TEXT("bps"));

			auto [VideoBytes, VideoBytesUnit] = GetInUnit(Collector->VideoTotalReceived, TEXT("B"));
			auto [AudioBytes, AudioBytesUnit] = GetInUnit(Collector->AudioTotalReceived, TEXT("B"));

			GEngine->AddOnScreenDebugMessage(MessageKey++, 0.0f, FColor::Green, FString::Printf(TEXT("Video Bitrate = %.2f %s"), VideoBitrate, *VideoBitrateUnit), true);
			GEngine->AddOnScreenDebugMessage(MessageKey++, 0.0f, FColor::Green, FString::Printf(TEXT("Audio Bitrate = %.2f %s"), AudioBitrate, *AudioBitrateUnit), true);
			GEngine->AddOnScreenDebugMessage(MessageKey++, 0.0f, FColor::Green, FString::Printf(TEXT("Video Total Received = %lld %s"), VideoBytes, *VideoBytesUnit), true);
			GEngine->AddOnScreenDebugMessage(MessageKey++, 0.0f, FColor::Green, FString::Printf(TEXT("Audio Total Received = %lld %s"), AudioBytes, *AudioBytesUnit), true);
			GEngine->AddOnScreenDebugMessage(MessageKey++, 0.0f, FColor::Green, FString::Printf(TEXT("Average Video decode time = %.2f ms"), Collector->VideoDecodeTimeAverage), true);
			GEngine->AddOnScreenDebugMessage(MessageKey++, 0.0f, FColor::Green, FString::Printf(TEXT("Video Packet Loss = %d"), Collector->VideoPacketLoss), true);
			GEngine->AddOnScreenDebugMessage(MessageKey++, 0.0f, FColor::Green, FString::Printf(TEXT("Audio Packet Loss = %d"), Collector->AudioPacketLoss), true);
			GEngine->AddOnScreenDebugMessage(MessageKey++, 0.0f, FColor::Green, FString::Printf(TEXT("Video Jitter Delay = %.2f ms"), Collector->VideoJitterAverageDelay), true);
			GEngine->AddOnScreenDebugMessage(MessageKey++, 0.0f, FColor::Green, FString::Printf(TEXT("Audio Jitter Delay = %.2f ms"), Collector->AudioJitterAverageDelay), true);
			GEngine->AddOnScreenDebugMessage(MessageKey++, 0.0f, FColor::Green, FString::Printf(TEXT("Video Jitter = %.2f ms"), Collector->VideoJitter), true);
			GEngine->AddOnScreenDebugMessage(MessageKey++, 0.0f, FColor::Green, FString::Printf(TEXT("Audio Jitter = %.2f ms"), Collector->AudioJitter), true);
			GEngine->AddOnScreenDebugMessage(MessageKey++, 0.0f, FColor::Green, FString::Printf(TEXT("Codecs = %s,%s"), *Collector->VideoCodec, *Collector->AudioCodec), true);
			GEngine->AddOnScreenDebugMessage(MessageKey++, 0.0f, FColor::Green, FString::Printf(TEXT("Cluster = %s"), *Collector->Cluster()), true);
			GEngine->AddOnScreenDebugMessage(MessageKey++, 0.0f, FColor::Green, FString::Printf(TEXT("Server = %s"), *Collector->Server()), true);

			GEngine->AddOnScreenDebugMessage(MessageKey++, 0.0f, FColor::Green, FString::Printf(TEXT("Stats Collector %d"), i), true);

			CSV_CUSTOM_STAT(Millicast_Player, Rtt, Collector->Rtt, ECsvCustomStatOp::Set);
			CSV_CUSTOM_STAT(Millicast_Player, Width, (int)Collector->Width, ECsvCustomStatOp::Set);
			CSV_CUSTOM_STAT(Millicast_Player, Height, (int)Collector->Height, ECsvCustomStatOp::Set);
			CSV_CUSTOM_STAT(Millicast_Player, FramePerSecond, (int)Collector->FramePerSecond, ECsvCustomStatOp::Set);
			CSV_CUSTOM_STAT(Millicast_Player, VideoBitrate, Collector->VideoBitrate, ECsvCustomStatOp::Set);
			CSV_CUSTOM_STAT(Millicast_Player, AudioBitrate, Collector->AudioBitrate, ECsvCustomStatOp::Set);
			CSV_CUSTOM_STAT(Millicast_Player, VideoTotalReceived, (int)Collector->VideoTotalReceived, ECsvCustomStatOp::Set);
			CSV_CUSTOM_STAT(Millicast_Player, AudioTotalReceived, (int)Collector->AudioTotalReceived, ECsvCustomStatOp::Set);
			CSV_CUSTOM_STAT(Millicast_Player, VideoPacketLoss, Collector->VideoPacketLoss, ECsvCustomStatOp::Set);
			CSV_CUSTOM_STAT(Millicast_Player, AudioPacketLoss, Collector->AudioPacketLoss, ECsvCustomStatOp::Set);
			CSV_CUSTOM_STAT(Millicast_Player, VideoJitter, Collector->VideoJitter, ECsvCustomStatOp::Set);
			CSV_CUSTOM_STAT(Millicast_Player, AudioJitter, Collector->AudioJitter, ECsvCustomStatOp::Set);
			CSV_CUSTOM_STAT(Millicast_Player, VideoJitterAverageDelay, Collector->VideoJitterAverageDelay, ECsvCustomStatOp::Set);
			CSV_CUSTOM_STAT(Millicast_Player, AudioJitterAverageDelay, Collector->AudioJitterAverageDelay, ECsvCustomStatOp::Set);
			// CSV_CUSTOM_STAT(Millicast_Player, VideoCodec, *Collector->VideoCodec, ECsvCustomStatOp::Set);
			// CSV_CUSTOM_STAT(Millicast_Player, AudioCodec, *Collector->AudioCodec, ECsvCustomStatOp::Set);
			CSV_CUSTOM_STAT(Millicast_Player, LastVideoReceivedTimestamp, Collector->LastVideoReceivedTimestamp, ECsvCustomStatOp::Set);
			CSV_CUSTOM_STAT(Millicast_Player, LastAudioReceivedTimestamp, Collector->LastAudioReceivedTimestamp, ECsvCustomStatOp::Set);
			CSV_CUSTOM_STAT(Millicast_Player, VideoDecodeTime, Collector->VideoDecodeTime, ECsvCustomStatOp::Set);
			CSV_CUSTOM_STAT(Millicast_Player, VideoDecodeTimeAverage, Collector->VideoDecodeTimeAverage, ECsvCustomStatOp::Set);
			CSV_CUSTOM_STAT(Millicast_Player, Timestamp, Collector->Timestamp, ECsvCustomStatOp::Set);
			CSV_CUSTOM_STAT(Millicast_Player, FramesDropped, Collector->FramesDropped, ECsvCustomStatOp::Set);
			CSV_CUSTOM_STAT(Millicast_Player, SilentConcealedSamples, Collector->SilentConcealedSamples, ECsvCustomStatOp::Set);
			CSV_CUSTOM_STAT(Millicast_Player, ConcealedSamples, Collector->ConcealedSamples, ECsvCustomStatOp::Set);
			CSV_CUSTOM_STAT(Millicast_Player, AudioPacketDiscarded, Collector->AudioPacketDiscarded, ECsvCustomStatOp::Set);
			CSV_CUSTOM_STAT(Millicast_Player, VideoPacketDiscarded, Collector->VideoPacketDiscarded, ECsvCustomStatOp::Set);
			CSV_CUSTOM_STAT(Millicast_Player, AudioNackCount, Collector->AudioNackCount, ECsvCustomStatOp::Set);
			CSV_CUSTOM_STAT(Millicast_Player, VideoNackCount, Collector->VideoNackCount, ECsvCustomStatOp::Set);

			++i;
		}

		return Y;
	}

	void FPlayerStats::RegisterEngineHooks()
	{
		UE_LOG(LogMillicastPlayer, Verbose, TEXT("%S"), __FUNCTION__);
		const FName StatName("STAT_Millicast_Player");
		const FName StatCategory("STATCAT_Millicast_Player");
		const FText StatDescription(FText::FromString("Millicast Player streaming stats."));
		UEngine::FEngineStatRender RenderStatFunc = UEngine::FEngineStatRender::CreateRaw(this, &FPlayerStats::OnRenderStats);
		UEngine::FEngineStatToggle ToggleStatFunc = UEngine::FEngineStatToggle::CreateRaw(this, &FPlayerStats::OnToggleStats);
		GEngine->AddEngineStat(StatName, StatCategory, StatDescription, RenderStatFunc, ToggleStatFunc, false);

		bRegisterEngineStats = true;
	}

	bool FPlayerStats::OnToggleStats(UWorld* World, FCommonViewportClient* ViewportClient, const TCHAR* Stream)
	{
		UE_LOG(LogMillicastPlayer, Verbose, TEXT("%S"), __FUNCTION__);
		return true;
	}

	void FPlayerStats::RegisterStatsCollector(FPlayerStatsCollector* Collector)
	{
		UE_LOG(LogMillicastPlayer, Verbose, TEXT("%S"), __FUNCTION__);
		StatsCollectors.Add(Collector);
	}

	void FPlayerStats::UnregisterStatsCollector(FPlayerStatsCollector* Collector)
	{
		UE_LOG(LogMillicastPlayer, Verbose, TEXT("%S"), __FUNCTION__);
		StatsCollectors.Remove(Collector);
	}
}

#endif
