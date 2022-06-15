// Copyright (c) Improbable Worlds Ltd, All Rights Reserved

#include "MillicastAudioActor.h"

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
    // AudioComponent = nullptr;
    SoundStreaming = nullptr;
}

void AMillicastAudioActor::QueueAudioData(const uint8* AudioData, int32 NumSamples)
{
    SoundStreaming->QueueAudio(AudioData, NumSamples * AudioParameters.GetNumberBytesPerSample());
}

void AMillicastAudioActor::InitSoundWave()
{
    UE_LOG(LogMillicastPlayer, Log, TEXT("InitSoundWave"));

    SoundStreaming = NewObject<USoundWaveProcedural>(this);
    SoundStreaming->SetSampleRate(AudioParameters.SamplesPerSecond);
    SoundStreaming->NumChannels = AudioParameters.NumberOfChannels;
    SoundStreaming->SampleByteSize = AudioParameters.GetNumberBytesPerSample();
    SoundStreaming->Duration = INDEFINITELY_LOOPING_DURATION;
    SoundStreaming->SoundGroup = SOUNDGROUP_Voice;
    SoundStreaming->bLooping = true;
    // SoundStreaming->AddToRoot();

    if (AudioComponent == nullptr)
    {
        auto AudioDevice = GEngine->GetMainAudioDevice();
        if (AudioDevice)
        {
            AudioComponent = AudioDevice->CreateComponent(SoundStreaming);
            AudioComponent->bIsUISound = false;
            AudioComponent->bAllowSpatialization = false;
            AudioComponent->SetVolumeMultiplier(1.0f);
            // AudioComponent->AddToRoot();
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
