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
    ~AMillicastAudioActor() noexcept;

<<<<<<< HEAD
    UPROPERTY(BlueprintReadWrite, EditAnywhere, AssetRegistrySearchable, Category = "Properties")
=======
    UPROPERTY(BlueprintReadWrite, Category = "Millicast Player", EditAnywhere, AssetRegistrySearchable)
>>>>>>> 090551bab85fc37c422480fe0c4617dd43c8d7e4
    UAudioComponent* AudioComponent;

public:
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
