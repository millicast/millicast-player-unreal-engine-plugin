// Copyright (c) Improbable Worlds Ltd, All Rights Reserved

#include "MillicastAudioActor.h"

#include "AudioDevice.h"
#include "AudioDeviceManager.h"
#include "Components/AudioComponent.h"

#include "MillicastPlayerPrivate.h"

AMillicastAudioActor::AMillicastAudioActor(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer), SoundStreaming(nullptr)
{
	AudioComponent = ObjectInitializer.CreateDefaultSubobject<UAudioComponent>(
		this,
		TEXT("UAudioComponent")
		);

	AudioComponent->SetSound(SoundStreaming);
	AudioComponent->AttachToComponent(RootComponent, FAttachmentTransformRules::KeepRelativeTransform);
}

AMillicastAudioActor::~AMillicastAudioActor() noexcept
{}

FMillicastAudioParameters AMillicastAudioActor::GetAudioParameters() const
{
	return AudioParameters;
}

void AMillicastAudioActor::UpdateAudioParameters(FMillicastAudioParameters Parameters) noexcept
{
	AudioParameters = MoveTemp(Parameters);

	if (SoundStreaming)
	{
		SoundStreaming->SetSampleRate(AudioParameters.SamplesPerSecond);
		SoundStreaming->NumChannels = AudioParameters.NumberOfChannels;
		SoundStreaming->SampleByteSize = AudioParameters.GetNumberBytesPerSample();
	}
}

void AMillicastAudioActor::Initialize()
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

void AMillicastAudioActor::Shutdown()
{
	if (AudioComponent && AudioComponent->IsPlaying())
	{
		AudioComponent->Stop();
	}

	SoundStreaming = nullptr;
}

void AMillicastAudioActor::QueueAudioData(const uint8* AudioData, int32 NumSamples)
{
	/* Don't queue if IsVirtualized is true because the buffer is not actually playing, this will desync with video*/
	if (AudioComponent->IsPlaying() && !AudioComponent->IsVirtualized())
	{
		SoundStreaming->QueueAudio(AudioData, NumSamples * AudioParameters.GetNumberBytesPerSample());
	}
}

void AMillicastAudioActor::InitSoundWave()
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
	if (!AudioComponent->SoundClassOverride.IsNull())
	{
		const auto& VoiPSoundClassName = GetDefault<UAudioSettings>()->VoiPSoundClass;
		if (VoiPSoundClassName.IsValid())
		{
			AudioComponent->SoundClassOverride = LoadObject<USoundClass>(nullptr, *VoiPSoundClassName.ToString());
		}
	}
}
