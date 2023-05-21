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

	void FPlayerStats::SetRendering(bool bEnabled)
	{
		if(bRendering == bEnabled)
		{
			return;
		}

		if(!bHasRegisteredEngineStats)
		{
			UE_LOG(LogMillicastPlayerStats,Error,TEXT("[FPlayerStats::SetRendering] called before stat command was registered"));
			return;
		}
		
		DirectStatsCommand( TEXT( "stat Millicast_Player" ) );
	}
	
	int32 FPlayerStats::OnRenderStats(UWorld* World, FViewport* Viewport, FCanvas* Canvas, int32 X, int32 Y, const FVector* ViewLocation, const FRotator* ViewRotation)
	{
		int MessageKey = 100;
		int i = 0;
		
		for (FPlayerStatsCollector* Collector : StatsCollectors)
		{
			Collector->Poll();
			const auto& Data = Collector->GetData();

			GEngine->AddOnScreenDebugMessage(MessageKey++, 0.0f, FColor::Green, FString::Printf(TEXT("RTT = %.2f ms"), Data.Rtt), true);
			GEngine->AddOnScreenDebugMessage(MessageKey++, 0.0f, FColor::Green, FString::Printf(TEXT("Video resolution = %dx%d"), Data.Width, Data.Height), true);
			GEngine->AddOnScreenDebugMessage(MessageKey++, 0.0f, FColor::Green, FString::Printf(TEXT("FPS = %d"), Data.FramePerSecond), true);

			auto [VideoBitrate, VideoBitrateUnit] = GetInUnit(Data.VideoBitrate, TEXT("bps"));
			auto [AudioBitrate, AudioBitrateUnit] = GetInUnit(Data.AudioBitrate, TEXT("bps"));

			auto [VideoBytes, VideoBytesUnit] = GetInUnit(Data.VideoTotalReceived, TEXT("B"));
			auto [AudioBytes, AudioBytesUnit] = GetInUnit(Data.AudioTotalReceived, TEXT("B"));

			GEngine->AddOnScreenDebugMessage(MessageKey++, 0.0f, FColor::Green, FString::Printf(TEXT("Video Bitrate = %.2f %s"), VideoBitrate, *VideoBitrateUnit), true);
			GEngine->AddOnScreenDebugMessage(MessageKey++, 0.0f, FColor::Green, FString::Printf(TEXT("Audio Bitrate = %.2f %s"), AudioBitrate, *AudioBitrateUnit), true);
			GEngine->AddOnScreenDebugMessage(MessageKey++, 0.0f, FColor::Green, FString::Printf(TEXT("Video Total Received = %lld %s"), VideoBytes, *VideoBytesUnit), true);
			GEngine->AddOnScreenDebugMessage(MessageKey++, 0.0f, FColor::Green, FString::Printf(TEXT("Audio Total Received = %lld %s"), AudioBytes, *AudioBytesUnit), true);
			GEngine->AddOnScreenDebugMessage(MessageKey++, 0.0f, FColor::Green, FString::Printf(TEXT("Average Video decode time = %.2f ms"), Data.VideoDecodeTimeAverage), true);
			GEngine->AddOnScreenDebugMessage(MessageKey++, 0.0f, FColor::Green, FString::Printf(TEXT("Video Packet Loss = %d"), Data.VideoPacketLoss), true);
			GEngine->AddOnScreenDebugMessage(MessageKey++, 0.0f, FColor::Green, FString::Printf(TEXT("Audio Packet Loss = %d"), Data.AudioPacketLoss), true);
			GEngine->AddOnScreenDebugMessage(MessageKey++, 0.0f, FColor::Green, FString::Printf(TEXT("Video Jitter Delay = %.2f ms"), Data.VideoJitterAverageDelay), true);
			GEngine->AddOnScreenDebugMessage(MessageKey++, 0.0f, FColor::Green, FString::Printf(TEXT("Audio Jitter Delay = %.2f ms"), Data.AudioJitterAverageDelay), true);
			GEngine->AddOnScreenDebugMessage(MessageKey++, 0.0f, FColor::Green, FString::Printf(TEXT("Video Jitter = %.2f ms"), Data.VideoJitter), true);
			GEngine->AddOnScreenDebugMessage(MessageKey++, 0.0f, FColor::Green, FString::Printf(TEXT("Audio Jitter = %.2f ms"), Data.AudioJitter), true);
			GEngine->AddOnScreenDebugMessage(MessageKey++, 0.0f, FColor::Green, FString::Printf(TEXT("Codecs = %s,%s"), *Data.VideoCodec, *Data.AudioCodec), true);
			GEngine->AddOnScreenDebugMessage(MessageKey++, 0.0f, FColor::Green, FString::Printf(TEXT("Cluster = %s"), *Collector->GetClusterId()), true);
			GEngine->AddOnScreenDebugMessage(MessageKey++, 0.0f, FColor::Green, FString::Printf(TEXT("Server = %s"), *Collector->GetServerId()), true);

			GEngine->AddOnScreenDebugMessage(MessageKey++, 0.0f, FColor::Green, FString::Printf(TEXT("Stats Collector %d"), i), true);

			CSV_CUSTOM_STAT(Millicast_Player, Rtt, Data.Rtt, ECsvCustomStatOp::Set);
			CSV_CUSTOM_STAT(Millicast_Player, Width, (int)Data.Width, ECsvCustomStatOp::Set);
			CSV_CUSTOM_STAT(Millicast_Player, Height, (int)Data.Height, ECsvCustomStatOp::Set);
			CSV_CUSTOM_STAT(Millicast_Player, FramePerSecond, (int)Data.FramePerSecond, ECsvCustomStatOp::Set);
			CSV_CUSTOM_STAT(Millicast_Player, VideoBitrate, Data.VideoBitrate, ECsvCustomStatOp::Set);
			CSV_CUSTOM_STAT(Millicast_Player, AudioBitrate, Data.AudioBitrate, ECsvCustomStatOp::Set);
			CSV_CUSTOM_STAT(Millicast_Player, VideoTotalReceived, (int)Data.VideoTotalReceived, ECsvCustomStatOp::Set);
			CSV_CUSTOM_STAT(Millicast_Player, AudioTotalReceived, (int)Data.AudioTotalReceived, ECsvCustomStatOp::Set);
			CSV_CUSTOM_STAT(Millicast_Player, VideoPacketLoss, Data.VideoPacketLoss, ECsvCustomStatOp::Set);
			CSV_CUSTOM_STAT(Millicast_Player, AudioPacketLoss, Data.AudioPacketLoss, ECsvCustomStatOp::Set);
			CSV_CUSTOM_STAT(Millicast_Player, VideoJitter, Data.VideoJitter, ECsvCustomStatOp::Set);
			CSV_CUSTOM_STAT(Millicast_Player, AudioJitter, Data.AudioJitter, ECsvCustomStatOp::Set);
			CSV_CUSTOM_STAT(Millicast_Player, VideoJitterAverageDelay, Data.VideoJitterAverageDelay, ECsvCustomStatOp::Set);
			CSV_CUSTOM_STAT(Millicast_Player, AudioJitterAverageDelay, Data.AudioJitterAverageDelay, ECsvCustomStatOp::Set);
			// CSV_CUSTOM_STAT(Millicast_Player, VideoCodec, *Data.VideoCodec, ECsvCustomStatOp::Set);
			// CSV_CUSTOM_STAT(Millicast_Player, AudioCodec, *Data.AudioCodec, ECsvCustomStatOp::Set);
			CSV_CUSTOM_STAT(Millicast_Player, LastVideoReceivedTimestamp, Data.LastVideoReceivedTimestamp, ECsvCustomStatOp::Set);
			CSV_CUSTOM_STAT(Millicast_Player, LastAudioReceivedTimestamp, Data.LastAudioReceivedTimestamp, ECsvCustomStatOp::Set);
			CSV_CUSTOM_STAT(Millicast_Player, VideoDecodeTime, Data.VideoDecodeTime, ECsvCustomStatOp::Set);
			CSV_CUSTOM_STAT(Millicast_Player, VideoDecodeTimeAverage, Data.VideoDecodeTimeAverage, ECsvCustomStatOp::Set);
			CSV_CUSTOM_STAT(Millicast_Player, Timestamp, Data.Timestamp, ECsvCustomStatOp::Set);
			CSV_CUSTOM_STAT(Millicast_Player, FramesDropped, Data.FramesDropped, ECsvCustomStatOp::Set);
			CSV_CUSTOM_STAT(Millicast_Player, SilentConcealedSamples, Data.SilentConcealedSamples, ECsvCustomStatOp::Set);
			CSV_CUSTOM_STAT(Millicast_Player, ConcealedSamples, Data.ConcealedSamples, ECsvCustomStatOp::Set);
			CSV_CUSTOM_STAT(Millicast_Player, AudioPacketDiscarded, Data.AudioPacketDiscarded, ECsvCustomStatOp::Set);
			CSV_CUSTOM_STAT(Millicast_Player, VideoPacketDiscarded, Data.VideoPacketDiscarded, ECsvCustomStatOp::Set);
			CSV_CUSTOM_STAT(Millicast_Player, AudioNackCount, Data.AudioNackCount, ECsvCustomStatOp::Set);
			CSV_CUSTOM_STAT(Millicast_Player, VideoNackCount, Data.VideoNackCount, ECsvCustomStatOp::Set);

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

		if (bHasRegisteredEngineStats)
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
		bRendering = !bRendering;
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
