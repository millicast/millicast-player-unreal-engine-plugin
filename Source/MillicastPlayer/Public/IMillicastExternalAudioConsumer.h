// Copyright (c) Improbable Worlds Ltd, All Rights Reserved

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"

#include "IMillicastExternalAudioConsumer.generated.h"

struct FMillicastAudioParameters
{
    int32 SampleSize = sizeof(int16_t);
    int32 SamplesPerSecond = 48000;
    int32 NumberOfChannels = 2;
    int32 TimePerFrameMs = 10;

    int32 GetNumberSamples() const { return TimePerFrameMs * SamplesPerSecond / 1000; };
    int32 GetNumberBytesPerSample() const { return SampleSize * NumberOfChannels; };
};

UINTERFACE()
class MILLICASTPLAYER_API UMillicastExternalAudioConsumer : public UInterface
{
    GENERATED_BODY()
};

class IMillicastExternalAudioConsumer
{
    GENERATED_BODY()

public:
    virtual FMillicastAudioParameters GetAudioParameters() const = 0;

    // Called when an audio track is added and playback is about to start
    virtual void Initialize() = 0;
    // Called when the subscription to a stream ends
    virtual void Shutdown() = 0;

    // Called from a WebRTC thread when new audio samples are available.
    // The consumer is encouraged to move the data out of this array
    virtual void QueueAudioData(TArray<uint8>& AudioData, int32 NumSamples) = 0;
};
