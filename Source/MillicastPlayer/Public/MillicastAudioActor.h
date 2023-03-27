// Copyright (c) Improbable Worlds Ltd, All Rights Reserved

#pragma once

#include "GameFramework/Actor.h"
#include "UObject/ObjectMacros.h"
#include "IMillicastExternalAudioConsumer.h"
#include "StreamMediaSource.h"
#include "MillicastAudioActor.generated.h"

class USoundWaveProcedural;
class UAudioComponent;

UCLASS(BlueprintType, Blueprintable, Category = "Millicast Player", META = (DisplayName = "Millicast Audio Actor"),
    hidecategories = Object)
class MILLICASTPLAYER_API AMillicastAudioActor : public AActor, public IMillicastExternalAudioConsumer
{
    GENERATED_BODY()
        
public:
    AMillicastAudioActor(const FObjectInitializer& ObjectInitializer);

    UPROPERTY(BlueprintReadWrite, EditAnywhere, AssetRegistrySearchable, Category = "Properties")
    UAudioComponent* AudioComponent;

    // IMillicastExternalAudioConsumer
    virtual FMillicastAudioParameters GetAudioParameters() const override;
    void UpdateAudioParameters(FMillicastAudioParameters Parameters) noexcept override;
    
    /**
    * Initialize this actor and create the SoundWaveProcedural object.
    */
    virtual void Initialize() override;

    /**
    * Call this to stop the playing and remove the SoundWaveProcedural
    */
    virtual void Shutdown() override;

    /**
    * Add audio data to the unreal audio buffer.
    */
    virtual void QueueAudioData(const uint8* AudioData, int32 NumSamples) override;
    // ~IMillicastExternalAudioConsumer

private:
	void InitSoundWave();

    UPROPERTY()
    USoundWaveProcedural* SoundStreaming;

    FMillicastAudioParameters AudioParameters;
};