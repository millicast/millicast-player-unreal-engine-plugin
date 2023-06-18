// Copyright Millicast 2023. All Rights Reserved.

#include "Audio/MillicastSoundWaveProcedural.h"

#include "AudioDevice.h"
#include "Engine/Engine.h"
#include "Settings/MillicastAudioSettings.h"

// NOTE [RW] Tightly coupled to USoundWaveProcedural implementation. Be on the lookout for improvements to that class

UMillicastSoundWaveProcedural::UMillicastSoundWaveProcedural(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	bProcedural = true;

	// If the main audio device has been set up, we can use this to define our callback size.
	// We need to do this for procedural sound waves that we do not process asynchronously,
	// to ensure that we do not underrun.

	CachedMaxQueuedAudio = GetDefault<UMillicastAudioSettings>()->MaxQueuedAudio;
	
	if (GEngine)
	{
		const FAudioDevice* MainAudioDevice = GEngine->GetMainAudioDeviceRaw();
		if (MainAudioDevice && !MainAudioDevice->IsAudioMixerEnabled())
		{
#if PLATFORM_MAC
			// We special case the mac callback on the old audio engine, Since Buffer Length is smaller than the device callback size.
			NumSamplesToGeneratePerCallback = 2048;
#else
			NumSamplesToGeneratePerCallback = MainAudioDevice->GetBufferLength();
#endif
			NumBufferUnderrunSamples = NumSamplesToGeneratePerCallback / 2;
		}
	}

	checkf(NumSamplesToGeneratePerCallback >= NumBufferUnderrunSamples, TEXT("Should generate more samples than this per callback."));
}

void UMillicastSoundWaveProcedural::QueueAudio(const uint8* AudioData, const int32 BufferSize)
{
	const Audio::EAudioMixerStreamDataFormat::Type Format = GetGeneratedPCMDataFormat();
	check(Format == Audio::EAudioMixerStreamDataFormat::Int16); // we do not support float

	if (BufferSize == 0 || !ensure((BufferSize % SampleByteSize) == 0))
	{
		return;
	}

	TArray<uint8> NewAudioBuffer;
	NewAudioBuffer.AddUninitialized(BufferSize);
	FMemory::Memcpy(NewAudioBuffer.GetData(), AudioData, BufferSize);
	QueuedAudio.Enqueue(NewAudioBuffer);

	AvailableByteCount.Add(BufferSize);

	// After Queuing, we check if we do not have too much queued, and if we do, we fade in and out
	FadeInFadeOutAudio();
}

void UMillicastSoundWaveProcedural::FadeInFadeOutAudio(bool bForced)
{
	const int32 AvailableBytes = GetAvailableAudioByteCount();
	if(!bForced && AvailableBytes < CachedMaxQueuedAudio)
	{
		return;
	}
	
	const auto* Settings = GetDefault<UMillicastAudioSettings>();

	TArray<uint8> AllSamples;
	AllSamples.Reserve(AvailableBytes);

	TArray<uint8> Temp;
	while(QueuedAudio.Dequeue(Temp))
	{
		AllSamples.Append(Temp);
	}

	// 48000 SamplesPerSecond. Format is int16, so 2 bytes. That results in 24 int16 SamplesPerMs
	const int32 SamplesForFadeIn = 24 * Settings->FadeInTimeInMs;
	const int32 SamplesForFadeOut = 24 * Settings->FadeOutTimeInMs;

	const int32 NewQueuedAudioSize = (SamplesForFadeIn+SamplesForFadeOut) * 2;
	if(AllSamples.Num() < NewQueuedAudioSize)
	{
		// not enough data in the buffer, can happen when called with force
		// or if the configured MaxSize is smaller than FadeIn and FadeOut allow to be combined with. We only check against the latter
		check((SamplesForFadeIn + SamplesForFadeOut) * 2 <= CachedMaxQueuedAudio);
		return;
	}
	
	TArray<uint8> NewQueuedAudio;
	NewQueuedAudio.AddUninitialized(NewQueuedAudioSize);

	int16* AllSamplesBufferPtr = (int16*)AllSamples.GetData();
	int16* NewQueuedAudioPtr = (int16*)NewQueuedAudio.GetData();
		
	{
		for(int32 i = 0; i < SamplesForFadeIn; ++i)
		{
			NewQueuedAudioPtr[i] = static_cast<int32>(AllSamplesBufferPtr[i]) * (SamplesForFadeIn-i) / SamplesForFadeIn;
		}
	}

	for (int32 i = 0; i < SamplesForFadeOut; ++i)
	{
		const int32 Index = AllSamples.Num() / 2 - SamplesForFadeOut + i;
		const int16 Result = static_cast<int32>(AllSamplesBufferPtr[Index]) * (i + 1) / SamplesForFadeOut;
		NewQueuedAudioPtr[SamplesForFadeIn + i] = Result;
	}

	QueuedAudio.Enqueue(NewQueuedAudio);
	AvailableByteCount.Set(NewQueuedAudioSize);
}


void UMillicastSoundWaveProcedural::PumpQueuedAudio()
{
	// Pump the enqueued audio
	TArray<uint8> NewQueuedBuffer;
	while (QueuedAudio.Dequeue(NewQueuedBuffer))
	{
		AudioBuffer.Append(NewQueuedBuffer);
	}
}

int32 UMillicastSoundWaveProcedural::GeneratePCMData(uint8* PCMData, const int32 SamplesNeeded)
{
	// Check if we've been told to reset our audio buffer
	if (bReset)
	{
		bReset = false;
		AudioBuffer.Reset();
		AvailableByteCount.Reset();
	}

	const Audio::EAudioMixerStreamDataFormat::Type Format = GetGeneratedPCMDataFormat();
	SampleByteSize = (Format == Audio::EAudioMixerStreamDataFormat::Int16) ? 2 : 4;

	int32 SamplesAvailable = AudioBuffer.Num() / SampleByteSize;
	const int32 SamplesToGenerate = FMath::Min(NumSamplesToGeneratePerCallback, SamplesNeeded);

	check(SamplesToGenerate >= NumBufferUnderrunSamples);

	bool bPumpQueuedAudio = true;

	if (SamplesAvailable < SamplesToGenerate)
	{
		// First try to use the virtual function which assumes we're writing directly into our audio buffer
		// since we're calling from the audio render thread.
		const int32 NumSamplesGenerated = OnGeneratePCMAudio(AudioBuffer, SamplesToGenerate);
		if (NumSamplesGenerated > 0)
		{
			// Shrink the audio buffer size to the actual number of samples generated
			const int32 BytesGenerated = NumSamplesGenerated * SampleByteSize;
			ensureAlwaysMsgf(BytesGenerated <= AudioBuffer.Num(), TEXT("Soundwave Procedural generated more bytes than expected (%d generated, %d expected)"), BytesGenerated, AudioBuffer.Num());
			if (BytesGenerated < AudioBuffer.Num())
			{
				AudioBuffer.SetNum(BytesGenerated, false);
			}
			bPumpQueuedAudio = false;
		}
	}

	if (bPumpQueuedAudio)
	{
		PumpQueuedAudio();
	}

	SamplesAvailable = AudioBuffer.Num() / SampleByteSize;

	// Wait until we have enough samples that are requested before starting.
	if (SamplesAvailable > 0)
	{
		const int32 SamplesToCopy = FMath::Min<int32>(SamplesToGenerate, SamplesAvailable);
		const int32 BytesToCopy = SamplesToCopy * SampleByteSize;

		FMemory::Memcpy((void*)PCMData, &AudioBuffer[0], BytesToCopy);
		AudioBuffer.RemoveAt(0, BytesToCopy, false);

		// Decrease the available by count
		if (bPumpQueuedAudio)
		{
			AvailableByteCount.Subtract(BytesToCopy);
		}

		return BytesToCopy;
	}

	// There wasn't enough data ready, write out zeros
	const int32 BytesCopied = NumBufferUnderrunSamples * SampleByteSize;
	FMemory::Memzero(PCMData, BytesCopied);
	return BytesCopied;
}

void UMillicastSoundWaveProcedural::ResetAudio()
{
	// Empty out any enqueued audio buffers
	QueuedAudio.Empty();

	// Flag that we need to reset our audio buffer (on the audio thread)
	bReset = true;
}

int32 UMillicastSoundWaveProcedural::GetAvailableAudioByteCount() const
{
	return AvailableByteCount.GetValue();
}

int32 UMillicastSoundWaveProcedural::GetResourceSizeForFormat(FName Format)
{
	return 0;
}

void UMillicastSoundWaveProcedural::GetAssetRegistryTags(TArray<FAssetRegistryTag>& OutTags) const
{
	Super::GetAssetRegistryTags(OutTags);
}

bool UMillicastSoundWaveProcedural::HasCompressedData(FName Format, ITargetPlatform* TargetPlatform) const
{
	return false;
}

void UMillicastSoundWaveProcedural::BeginGetCompressedData(FName Format, const FPlatformAudioCookOverrides* CompressionOverrides)
{
	// SoundWaveProcedural does not have compressed data and should generally not be asked about it
}

FByteBulkData* UMillicastSoundWaveProcedural::GetCompressedData(FName Format, const FPlatformAudioCookOverrides* CompressionOverrides)
{
	// SoundWaveProcedural does not have compressed data and should generally not be asked about it
	return nullptr;
}

void UMillicastSoundWaveProcedural::Serialize(FArchive& Ar)
{
	// Do not call the USoundWave version of serialize
	USoundBase::Serialize(Ar);

#if WITH_EDITORONLY_DATA
	// Due to "skipping" USoundWave::Serialize above, modulation
	// versioning is required to be called explicitly here.
	if (Ar.IsLoading())
	{
		ModulationSettings.VersionModulators();
	}
#endif
}

void UMillicastSoundWaveProcedural::InitAudioResource(FByteBulkData& CompressedData)
{
	// Should never be pushing compressed data to a SoundWaveProcedural
	check(false);
}

bool UMillicastSoundWaveProcedural::InitAudioResource(FName Format)
{
	// Nothing to be done to initialize a USoundWaveProcedural
	return true;
}
