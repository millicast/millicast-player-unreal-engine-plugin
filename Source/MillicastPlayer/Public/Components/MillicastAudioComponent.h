// Copyright Millicast 2023. All Rights Reserved.

#pragma once

#include "Components/SceneComponent.h"
#include "IMillicastExternalAudioConsumer.h"
#include "MillicastAudioComponent.generated.h"

class USoundWaveProcedural;
class UAudioComponent;

UCLASS(BlueprintType, Category = "Millicast Player", META = (DisplayName = "Millicast Audio ActorComponent"))
class MILLICASTPLAYER_API UMillicastAudioComponent : public USceneComponent, public IMillicastExternalAudioConsumer
{
	GENERATED_BODY()

public:
	void InjectDependencies(UAudioComponent* InAudioComponent);
	
	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category = "Properties")
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
