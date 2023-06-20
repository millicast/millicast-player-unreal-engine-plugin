// Copyright Millicast 2023. All Rights Reserved.

#pragma once

#include "IMillicastExternalAudioConsumer.h"
#include "MillicastAudioInstance.generated.h"

class USoundWaveProcedural;
class UAudioComponent;

UCLASS()
class MILLICASTPLAYER_API UMillicastAudioInstance : public UObject, public IMillicastExternalAudioConsumer
{
	GENERATED_BODY()

public:
	void InjectDependencies(UAudioComponent* InAudioComponent);

	const UAudioComponent* GetAudioComponent() const { return AudioComponent; }
	
	// IMillicastExternalAudioConsumer
	virtual FMillicastAudioParameters GetAudioParameters() const override { return AudioParameters; }
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
	UAudioComponent* AudioComponent;
	
	UPROPERTY()
	USoundWaveProcedural* SoundStreaming;

	FMillicastAudioParameters AudioParameters;
};
