#include "WebRTC/PlayerStats.h"

#if ENGINE_MAJOR_VERSION >= 5 && ENGINE_MINOR_VERSION > 0

#include "Engine/Engine.h"
#include "MillicastPlayerPrivate.h"
#include "ProfilingDebugging/CsvProfiler.h"
#include "WebRTC/PlayerStatsCollector.h"

DECLARE_LOG_CATEGORY_EXTERN(LogMillicastPlayerStats, Log, All);
DEFINE_LOG_CATEGORY(LogMillicastPlayerStats);

CSV_DEFINE_CATEGORY(Millicast_Player, false);

// --------- FPlayerStatsCollector ------

namespace Millicast::Player
{
	FPlayerStats& FPlayerStats::Get()
	{
		static FPlayerStats Instance;
		return Instance;
	}
	
	void FPlayerStats::Tick(float DeltaTime)
	{
		RegisterEngineHooks();
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
		if (!GEngine)
		{
			return;
		}

		if (!bHasRegisteredEngineStats)
		{
			return;
		}
		bHasRegisteredEngineStats = true;
		
		UE_LOG(LogMillicastPlayer, Verbose, TEXT("%S"), __FUNCTION__);
		const FName StatName("STAT_Millicast_Player");
		const FName StatCategory("STATCAT_Millicast_Player");
		const FText StatDescription(FText::FromString("Millicast Player streaming stats."));
		UEngine::FEngineStatRender RenderStatFunc = UEngine::FEngineStatRender::CreateRaw(this, &FPlayerStats::OnRenderStats);
		UEngine::FEngineStatToggle ToggleStatFunc = UEngine::FEngineStatToggle::CreateRaw(this, &FPlayerStats::OnToggleStats);
		GEngine->AddEngineStat(StatName, StatCategory, StatDescription, RenderStatFunc, ToggleStatFunc, false);
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
