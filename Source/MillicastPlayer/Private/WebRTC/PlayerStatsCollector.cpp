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

		Data = {};
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
					auto lastByteCount = Data.VideoTotalReceived;
					auto timestamp = Stats.timestamp_us();

					Data.Width = InboundStat.frame_width.ValueOrDefault(0);
					Data.Height = InboundStat.frame_height.ValueOrDefault(0);
					Data.FramesPerSecond = InboundStat.frames_per_second.ValueOrDefault(0);
					Data.VideoTotalReceived = InboundStat.bytes_received.ValueOrDefault(0);
					Data.VideoPacketLoss = InboundStat.packets_lost.ValueOrDefault(-1);
					Data.VideoJitter = InboundStat.jitter.ValueOrDefault(0) * 1000.;
					Data.VideoDecodeTime = InboundStat.total_decode_time.ValueOrDefault(0);
					Data.VideoDecodeTimeAverage = 1000. * Data.VideoDecodeTime / (double)*InboundStat.frames_decoded;
					Data.FramesDropped = InboundStat.frames_dropped.ValueOrDefault(0);
					Data.VideoPacketDiscarded = InboundStat.packets_discarded.ValueOrDefault(0);
					Data.VideoNackCount = InboundStat.nack_count.ValueOrDefault(0);

					auto videoJitterDelay = InboundStat.jitter_buffer_delay.ValueOrDefault(-1);
					auto videoJitterEmitted = InboundStat.jitter_buffer_emitted_count.ValueOrDefault(0);

					if (videoJitterDelay != 0)
					{
						Data.VideoJitterAverageDelay = 1000. * videoJitterDelay / videoJitterEmitted;
					}
					else
					{
						Data.VideoJitterAverageDelay = 0;
					}

					if (Data.LastVideoStatTimestamp != 0 && Data.VideoTotalReceived != lastByteCount)
					{
						Data.VideoBitrate = (Data.VideoTotalReceived - lastByteCount) * NUM_US * 8. / (timestamp - Data.LastVideoStatTimestamp);
					}

					Data.LastVideoStatTimestamp = timestamp;
					Data.LastVideoReceivedTimestamp = InboundStat.last_packet_received_timestamp.ValueOrDefault(-1);

					auto CodecStats = Report->GetStatsOfType<webrtc::RTCCodecStats>();

					auto it = std::find_if(CodecStats.begin(), CodecStats.end(),
						[&InboundStat](auto&& e) { return e->id() == *InboundStat.codec_id; });

					if (it != CodecStats.end())
					{
						Data.VideoCodec = ToString(*(*it)->mime_type);
					}
				}
				else
				{
					auto lastByteCount = Data.AudioTotalReceived;
					auto timestamp = Stats.timestamp_us();

					Data.AudioTotalReceived = InboundStat.bytes_received.ValueOrDefault(0);
					Data.AudioPacketLoss = InboundStat.packets_lost.ValueOrDefault(-1);
					Data.AudioJitter = InboundStat.jitter.ValueOrDefault(0) * 1000.;
					Data.AudioPacketDiscarded = InboundStat.packets_discarded.ValueOrDefault(0);
					Data.AudioNackCount = InboundStat.nack_count.ValueOrDefault(0);
					Data.ConcealedSamples = InboundStat.concealed_samples.ValueOrDefault(0);
					Data.SilentConcealedSamples = InboundStat.silent_concealed_samples.ValueOrDefault(0);
					Data.AudioLevel = InboundStat.audio_level.ValueOrDefault(0);

					auto audioJitterDelay = InboundStat.jitter_buffer_delay.ValueOrDefault(-1);
					auto audioJitterEmitted = InboundStat.jitter_buffer_emitted_count.ValueOrDefault(0);

					if (audioJitterDelay != 0)
					{
						Data.AudioJitterAverageDelay = 1000. * audioJitterDelay / audioJitterEmitted;
					}
					else
					{
						Data.AudioJitterAverageDelay = 0;
					}

					if (Data.LastAudioStatTimestamp != 0 && Data.AudioTotalReceived != lastByteCount)
					{
						Data.AudioBitrate = (Data.AudioTotalReceived - lastByteCount) * NUM_US * 8 / (timestamp - Data.LastAudioStatTimestamp);
					}

					Data.LastAudioStatTimestamp = timestamp;
					Data.LastAudioReceivedTimestamp = InboundStat.last_packet_received_timestamp.ValueOrDefault(-1);

					auto CodecStats = Report->GetStatsOfType<webrtc::RTCCodecStats>();

					auto it = std::find_if(CodecStats.begin(), CodecStats.end(),
						[&InboundStat](auto&& e) { return e->id() == *InboundStat.codec_id; });

					if (it != CodecStats.end())
					{
						Data.AudioCodec = ToString(*(*it)->mime_type);
					}
				}
			}
			else if (Stats.type() == std::string("candidate-pair"))
			{
				auto CandidateStat = Stats.cast_to<webrtc::RTCIceCandidatePairStats>();
				Data.Rtt = CandidateStat.current_round_trip_time.ValueOrDefault(0.) * 1000.;
			}
		}

		Data.Timestamp = Report->timestamp_us();

		OnStats.Broadcast(Report);
	}

	const FString& FPlayerStatsCollector::GetClusterId() const
	{
		return PeerConnection->ClusterId;
	}

	const FString& FPlayerStatsCollector::GetServerId() const
	{
		return PeerConnection->ServerId;
	}

}
#endif
