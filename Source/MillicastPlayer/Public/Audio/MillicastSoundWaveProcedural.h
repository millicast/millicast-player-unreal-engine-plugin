// Copyright Millicast 2023. All Rights Reserved.

#pragma once

#include "Containers/Queue.h"
#include "HAL/ThreadSafeCounter.h"
#include "HAL/ThreadSafeBool.h"
#include "Sound/SoundWave.h"
#include "MillicastSoundWaveProcedural.generated.h"

UCLASS()
class MILLICASTPLAYER_API UMillicastSoundWaveProcedural : public USoundWave
{
	GENERATED_BODY()

private:
	// A thread safe queue for queuing audio to be consumed on audio thread
	TQueue<TArray<uint8>> QueuedAudio;

	// The amount of bytes queued and not yet consumed
	FThreadSafeCounter AvailableByteCount;

	// The actual audio buffer that can be consumed. QueuedAudio is fed to this buffer. Accessed only audio thread.
	TArray<uint8> AudioBuffer;

	// Flag to reset the audio buffer
	FThreadSafeBool bReset;

	// Pumps audio queued from game thread
	void PumpQueuedAudio();

protected:
	// Number of samples to pad with 0 if there isn't enough audio available
	int32 NumBufferUnderrunSamples = 512;

	// The number of PCM samples we want to generate. This can't be larger than SamplesNeeded in GeneratePCMData callback, but can be less.
#if PLATFORM_IOS
	int32 NumSamplesToGeneratePerCallback = 8192;
#else
	int32 NumSamplesToGeneratePerCallback = 1024;
#endif
	
public:
	UMillicastSoundWaveProcedural(const FObjectInitializer& ObjectInitializer);

	//~ Begin UObject Interface. 
	virtual void Serialize( FArchive& Ar ) override;
	virtual void GetAssetRegistryTags(TArray<FAssetRegistryTag>& OutTags) const override;
	//~ End UObject Interface. 

	//~ Begin USoundWave Interface.
	virtual int32 GeneratePCMData(uint8* PCMData, const int32 SamplesNeeded) override;
	virtual bool HasCompressedData(FName Format, ITargetPlatform* TargetPlatform) const override;
	virtual void BeginGetCompressedData(FName Format, const FPlatformAudioCookOverrides* CompressionOverrides) override;
	virtual FByteBulkData* GetCompressedData(FName Format, const FPlatformAudioCookOverrides* CompressionOverrides = nullptr) override;
	virtual void InitAudioResource( FByteBulkData& CompressedData ) override;
	virtual bool InitAudioResource(FName Format) override;
	virtual int32 GetResourceSizeForFormat(FName Format) override;
	//~ End USoundWave Interface.

	// Virtual function to generate PCM audio from the audio render thread. 
	// Returns number of samples generated
	virtual int32 OnGeneratePCMAudio(TArray<uint8>& OutAudio, int32 NumSamples) { return 0; }

	/** Add data to the FIFO that feeds the audio device. */
	void QueueAudio(const uint8* AudioData, const int32 BufferSize);

	void FadeInFadeOutAudio(bool bForced = false);
	
	/** Remove all queued data from the FIFO. This is only necessary if you want to start over, or GeneratePCMData() isn't going to be called, since that will eventually drain it. */
	void ResetAudio();

	/** Query bytes queued for playback */
	int32 GetAvailableAudioByteCount() const;

	/** Size in bytes of a single sample of audio in the procedural audio buffer. */
	int32 SampleByteSize = 2;

private:
	int32 CachedMaxQueuedAudio;
};
