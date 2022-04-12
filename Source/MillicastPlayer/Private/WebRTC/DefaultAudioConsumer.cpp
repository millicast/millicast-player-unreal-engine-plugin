// Copyright (c) Improbable Worlds Ltd, All Rights Reserved

#include "DefaultAudioConsumer.h"


FMillicastAudioParameters UMillicastDefaultAudioConsumer::GetAudioParameters() const
{
    return AudioParameters;
}

void UMillicastDefaultAudioConsumer::Initialize()
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

void UMillicastDefaultAudioConsumer::Shutdown()
{
    if (AudioComponent && AudioComponent->IsPlaying())
    {
        AudioComponent->Stop();
    }
    AudioComponent = nullptr;
    SoundStreaming = nullptr;    
}

void UMillicastDefaultAudioConsumer::QueueAudioData(TArray<uint8>& AudioData, int32 NumSamples)
{
    SoundStreaming->QueueAudio(AudioData.GetData(), AudioParameters.GetNumberSamples() * AudioParameters.GetNumberBytesPerSample());    
}

void UMillicastDefaultAudioConsumer::InitSoundWave()
{
    SoundStreaming = NewObject<USoundWaveProcedural>(this);
    SoundStreaming->SetSampleRate(AudioParameters.SamplesPerSecond);
    SoundStreaming->NumChannels = AudioParameters.NumberOfChannels;
    SoundStreaming->SampleByteSize = AudioParameters.GetNumberBytesPerSample();
    SoundStreaming->Duration = INDEFINITELY_LOOPING_DURATION;
    SoundStreaming->SoundGroup = SOUNDGROUP_Voice;
    SoundStreaming->bLooping = true;

    if (AudioComponent == nullptr)
    {
        auto AudioDevice = GEngine->GetMainAudioDevice();
        if (AudioDevice)
        {
            AudioComponent = AudioDevice->CreateComponent(SoundStreaming);
            AudioComponent->bIsUISound = false;
            AudioComponent->bAllowSpatialization = false;
            AudioComponent->SetVolumeMultiplier(1.0f);
            AudioComponent->AddToRoot();
        }
    }
    else
    {
        AudioComponent->Sound = SoundStreaming;
    }

    const FSoftObjectPath VoiPSoundClassName = GetDefault<UAudioSettings>()->VoiPSoundClass;
    if (AudioComponent && VoiPSoundClassName.IsValid())
    {
        AudioComponent->SoundClassOverride = LoadObject<USoundClass>(nullptr, *VoiPSoundClassName.ToString());
    }
}
