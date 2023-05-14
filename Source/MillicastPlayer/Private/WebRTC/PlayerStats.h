// Copyright Millicast 2022. All Rights Reserved.

#pragma once

#include "Runtime/Launch/Resources/Version.h"

#if ENGINE_MAJOR_VERSION >= 5 && ENGINE_MINOR_VERSION > 0

#include "Tickable.h"
#include "WebRTC/WebRTCInc.h"

class FCanvas;
class FCommonViewportClient;
class FPlayerStatsCollector;
class FViewport;

namespace Millicast::Player
{
	/*
	 * Some basic performance stats about how the publisher is running, e.g. how long capture/encode takes.
	 * Stats are drawn to screen for now as it is useful to observe them in realtime.
	 */
	class FPlayerStats : FTickableGameObject
	{
	public:
		TArray<FPlayerStatsCollector*> StatsCollectors;

		static FPlayerStats& Get();

		void SetRendering(bool bEnabled);

		bool IsAllowedToTick() const override { return !bHasRegisteredEngineStats; }
		void Tick(float DeltaTime) override;
		FORCEINLINE TStatId GetStatId() const override { RETURN_QUICK_DECLARE_CYCLE_STAT(MillicastPlayerProducerStats, STATGROUP_Tickables); }

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
	
	private:
		bool bRendering = false;
		bool bHasRegisteredEngineStats = false;
		bool LogStatsEnabled = true;
	};

}

#endif
