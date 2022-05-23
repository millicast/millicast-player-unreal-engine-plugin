// Copyright (c) Improbable Worlds Ltd, All Rights Reserved

#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "IMillicastExternalAudioConsumer.h"

#include "StreamMediaSource.h"

class USoundWaveProcedural;
class UAudioComponent;

#include "MillicastAudioActor.generated.h"

UCLASS(BlueprintType, Blueprintable, Category = "Millicast Player", META = (DisplayName = "Millicast Audio Actor"),
    hidecategories = Object)
class MILLICASTPLAYER_API AMillicastAudioActor : public AActor, public IMillicastExternalAudioConsumer
{
    GENERATED_BODY()
        
public:
    AMillicastAudioActor(const FObjectInitializer& ObjectInitializer);

    UPROPERTY(BlueprintReadWrite, EditAnywhere, AssetRegistrySearchable)
    UAudioComponent* AudioComponent;
public:
    // IMillicastExternalAudioConsumer
    virtual FMillicastAudioParameters GetAudioParameters() const override;
    
    virtual void Initialize() override;
    virtual void Shutdown() override;
    virtual void QueueAudioData(TArray<uint8>& AudioData, int32 NumSamples) override;
    // ~IMillicastExternalAudioConsumer

private:
	void InitSoundWave();

    USoundWaveProcedural* SoundStreaming;

    FMillicastAudioParameters AudioParameters;
};