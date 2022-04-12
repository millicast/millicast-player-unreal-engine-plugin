// Copyright (c) Improbable Worlds Ltd, All Rights Reserved

#pragma once

#include "CoreMinimal.h"
#include "IMillicastExternalAudioConsumer.h"

#include "DefaultAudioConsumer.generated.h"

UCLASS()
class UMillicastDefaultAudioConsumer : public UObject, public IMillicastExternalAudioConsumer
{
    GENERATED_BODY()

public:
    // IMillicastExternalAudioConsumer
    virtual FMillicastAudioParameters GetAudioParameters() const override;
    
    virtual void Initialize() override;
    virtual void Shutdown() override;
    virtual void QueueAudioData(TArray<uint8>& AudioData, int32 NumSamples) override;
    // ~IMillicastExternalAudioConsumer

private:
	void InitSoundWave();

    FMillicastAudioParameters AudioParameters;
    
    UPROPERTY()
    USoundWaveProcedural* SoundStreaming;
    
    UPROPERTY()
    UAudioComponent* AudioComponent;
};
