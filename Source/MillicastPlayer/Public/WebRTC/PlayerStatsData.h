// Copyright Millicast 2022. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "PlayerStatsData.generated.h"


USTRUCT(BlueprintType)
struct MILLICASTPLAYER_API FPlayerStatsData
{
	GENERATED_BODY()
	
	UPROPERTY(BlueprintReadOnly, Category="MillicastPlayer")
	float Rtt = 0.0f; // ms

	UPROPERTY(BlueprintReadOnly, Category="MillicastPlayer")
	int32 Width = 0; // px

	UPROPERTY(BlueprintReadOnly, Category="MillicastPlayer")
	int32 Height = 0; // px

	UPROPERTY(BlueprintReadOnly, Category="MillicastPlayer")
	float FramesPerSecond = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category="MillicastPlayer")
	float VideoBitrate = 0.0f; // bps

	UPROPERTY(BlueprintReadOnly, Category="MillicastPlayer")
	float AudioBitrate = 0.0f; // bps
	
	UPROPERTY(BlueprintReadOnly, Category="MillicastPlayer")
	int32 VideoTotalReceived = 0; // bytes
	
	UPROPERTY(BlueprintReadOnly, Category="MillicastPlayer")
	int32 AudioTotalReceived = 0; // bytes

	UPROPERTY(BlueprintReadOnly, Category="MillicastPlayer")
	int32 VideoPacketLoss = 0; // num packets

	UPROPERTY(BlueprintReadOnly, Category="MillicastPlayer")
	int32 AudioPacketLoss = 0; // num packets

	UPROPERTY(BlueprintReadOnly, Category="MillicastPlayer")
	float VideoJitter = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category="MillicastPlayer")
	float AudioJitter = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category="MillicastPlayer")
	float VideoJitterAverageDelay = 0.0f; // ms

	UPROPERTY(BlueprintReadOnly, Category="MillicastPlayer")
	float AudioJitterAverageDelay = 0.0f; // ms

	UPROPERTY(BlueprintReadOnly, Category="MillicastPlayer")
	FString VideoCodec; // mimetype

	UPROPERTY(BlueprintReadOnly, Category="MillicastPlayer")
	FString AudioCodec; // mimetype

	UPROPERTY(BlueprintReadOnly, Category="MillicastPlayer")
	float LastVideoReceivedTimestamp = 0.0f; // us

	UPROPERTY(BlueprintReadOnly, Category="MillicastPlayer")
	float LastAudioReceivedTimestamp = 0.0f; // us

	UPROPERTY(BlueprintReadOnly, Category="MillicastPlayer")
	float VideoDecodeTime = 0.0f; // s
	
	UPROPERTY(BlueprintReadOnly, Category="MillicastPlayer")
	float VideoDecodeTimeAverage = 0.0f; // ms

	// int PauseCount; // Not yet in unreal libwebrtc
	// int freezeCount; // Not yet in unreal libwebrtc

	UPROPERTY(BlueprintReadOnly, Category="MillicastPlayer")
	int32 FramesDropped = 0;

	UPROPERTY(BlueprintReadOnly, Category="MillicastPlayer")
	int32 SilentConcealedSamples = 0;

	UPROPERTY(BlueprintReadOnly, Category="MillicastPlayer")
	int32 ConcealedSamples = 0;

	UPROPERTY(BlueprintReadOnly, Category="MillicastPlayer")
	int32 AudioPacketDiscarded = 0;

	UPROPERTY(BlueprintReadOnly, Category="MillicastPlayer")
	int32 VideoPacketDiscarded = 0;
	
	// double AudioProcessingDelay; Not yet in this libwebrtc
	// double VideoProcessingDelay; Not yet in this libwebrtc

	UPROPERTY(BlueprintReadOnly, Category="MillicastPlayer")
	int32 AudioNackCount = 0;

	UPROPERTY(BlueprintReadOnly, Category="MillicastPlayer")
	int32 VideoNackCount = 0;

	UPROPERTY(BlueprintReadOnly, Category = "MillicastPlayer")
	float AudioLevel = 0.0f; // ms

	UPROPERTY(BlueprintReadOnly, Category="MillicastPlayer")
	float Timestamp = 0.0f; // us

	// TODO [RW] Why was this not made accessible by David?
	double LastVideoStatTimestamp = 0.0f;
	double LastAudioStatTimestamp = 0.0f;
};
