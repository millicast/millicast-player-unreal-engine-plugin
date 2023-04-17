// Copyright Millicast 2023. All Rights Reserved.

#include "Components/MillicastAudioComponent.h"

#include "AudioDevice.h"
#include "MillicastPlayerPrivate.h"

#include "Components/AudioComponent.h"

FMillicastAudioParameters UMillicastAudioComponent::GetAudioParameters() const
{
	return AudioParameters;
}

void UMillicastAudioComponent::UpdateAudioParameters(FMillicastAudioParameters Parameters) noexcept
{
	AudioParameters = MoveTemp(Parameters);

	if (SoundStreaming)
	{
		SoundStreaming->SetSampleRate(AudioParameters.SamplesPerSecond);
		SoundStreaming->NumChannels = AudioParameters.NumberOfChannels;
		SoundStreaming->SampleByteSize = AudioParameters.GetNumberBytesPerSample();
	}
}

void UMillicastAudioComponent::InjectDependencies(UAudioComponent* InAudioComponent)
{
	AudioComponent = InAudioComponent;
}

void UMillicastAudioComponent::Initialize()
{
	if (SoundStreaming == nullptr)
	{
		InitSoundWave();
	}
	else
	{
		SoundStreaming->ResetAudio();
	}

	if (AudioComponent)
	{
		AudioComponent->Play(0.0f);
	}
}

void UMillicastAudioComponent::Shutdown()
{
	if (AudioComponent && AudioComponent->IsPlaying())
	{
		AudioComponent->Stop();
	}

	SoundStreaming = nullptr;
}

void UMillicastAudioComponent::QueueAudioData(const uint8* AudioData, int32 NumSamples)
{
	if(!AudioComponent)
	{
		return;
	}
	
	/* Don't queue if IsVirtualized is true because the buffer is not actually playing, this will desync with video*/
	if (AudioComponent->IsPlaying() && !AudioComponent->IsVirtualized())
	{
		SoundStreaming->QueueAudio(AudioData, NumSamples * AudioParameters.GetNumberBytesPerSample());
	}
}

void UMillicastAudioComponent::InitSoundWave()
{
	if (SoundStreaming)
	{
		UE_LOG(LogMillicastPlayer, Error, TEXT("AMillicastAudioActor::InitSoundWave called whilst already initialized"));
		return;
	}

	UE_LOG(LogMillicastPlayer, Log, TEXT("InitSoundWave"));

	SoundStreaming = NewObject<USoundWaveProcedural>(this);
	SoundStreaming->SetSampleRate(AudioParameters.SamplesPerSecond);
	SoundStreaming->NumChannels = AudioParameters.NumberOfChannels;
	SoundStreaming->SampleByteSize = AudioParameters.GetNumberBytesPerSample();
	SoundStreaming->Duration = INDEFINITELY_LOOPING_DURATION;
	SoundStreaming->SoundGroup = SOUNDGROUP_Voice;
	SoundStreaming->bLooping = true;
	SoundStreaming->VirtualizationMode = EVirtualizationMode::PlayWhenSilent;

	if (AudioComponent)
	{
		AudioComponent->Sound = SoundStreaming;
	}
	else
	{
		auto AudioDevice = GEngine->GetMainAudioDevice();
		if (!AudioDevice)
		{
			UE_LOG(LogMillicastPlayer, Error, TEXT("AMillicastAudioActor::InitSoundWave has failed at the last step to produce an audiodevice"));
			return;
		}

		AudioComponent = AudioDevice->CreateComponent(SoundStreaming);
		AudioComponent->bIsUISound = false;
		AudioComponent->bAllowSpatialization = false;
		AudioComponent->SetVolumeMultiplier(1.0f);
	}

	// Provide VoiP sound class override if none provided by the user
	if (!AudioComponent->SoundClassOverride)
	{
		const auto& VoiPSoundClassName = GetDefault<UAudioSettings>()->VoiPSoundClass;
		if (VoiPSoundClassName.IsValid())
		{
			AudioComponent->SoundClassOverride = LoadObject<USoundClass>(nullptr, *VoiPSoundClassName.ToString());
		}
	}
}
