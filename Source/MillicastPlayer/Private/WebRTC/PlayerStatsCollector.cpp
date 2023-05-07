#include "WebRTC/PlayerStatsCollector.h"

#if ENGINE_MAJOR_VERSION >= 5 && ENGINE_MINOR_VERSION > 0

#include "Engine/Engine.h"
#include "PeerConnection.h"
#include "ProfilingDebugging/CsvProfiler.h"
#include "Util.h"
#include "WebRTC/PlayerStats.h"

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

		OnStats.Broadcast(Report);
	}

	const FString& FPlayerStatsCollector::Cluster() const
	{
		return PeerConnection->ClusterId;
	}

	const FString& FPlayerStatsCollector::Server() const
	{
		return PeerConnection->ServerId;
	}

}
#endif
