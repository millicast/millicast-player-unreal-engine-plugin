// Copyright CoSMoSoftware 2021. All Rights Reserved.

#include "AudioDeviceModule.h"
#include "MillicastPlayerPrivate.h"

const char FAudioDeviceModule::kTimerQueueName[] = "FAudioDeviceModuleTimer";

FAudioDeviceModule::FAudioDeviceModule(webrtc::TaskQueueFactory* queue_factory) noexcept
	: AudioCallback(nullptr),
	bIsPlaying(false),
	bIsPlayInitialized(false),
	bIsStarted(false),
	NextFrameTime(0),
	TaskQueue(queue_factory->CreateTaskQueue(kTimerQueueName,
		webrtc::TaskQueueFactory::Priority::NORMAL)),
	SoundStreaming(nullptr),
	AudioComponent(nullptr)
{

}

void FAudioDeviceModule::InitSoundWave()
{
	SoundStreaming = NewObject<USoundWaveProcedural>();
	SoundStreaming->SetSampleRate(kSamplesPerSecond);
	SoundStreaming->NumChannels = kNumberOfChannels;
	SoundStreaming->SampleByteSize = kNumberBytesPerSample;
	SoundStreaming->Duration = INDEFINITELY_LOOPING_DURATION;
	SoundStreaming->SoundGroup = SOUNDGROUP_Voice;
	SoundStreaming->bLooping = true;

	if (AudioComponent == nullptr) 
	{
		auto AudioDevice = GEngine->GetMainAudioDevice();
		AudioComponent = AudioDevice->CreateComponent(SoundStreaming);
		AudioComponent->bIsUISound = false;
		AudioComponent->bAllowSpatialization = false;
		AudioComponent->SetVolumeMultiplier(1.0f);
		AudioComponent->AddToRoot();
	}
	else
	{
		AudioComponent->Sound = SoundStreaming;
	}

	const FSoftObjectPath VoiPSoundClassName = GetDefault<UAudioSettings>()->VoiPSoundClass;
	if (VoiPSoundClassName.IsValid())
	{
		AudioComponent->SoundClassOverride = LoadObject<USoundClass>(nullptr, *VoiPSoundClassName.ToString());
	}
}

rtc::scoped_refptr<FAudioDeviceModule>
FAudioDeviceModule::Create(webrtc::TaskQueueFactory* queue_factory)
{
	rtc::scoped_refptr<FAudioDeviceModule>
		AudioDeviceModule(new rtc::RefCountedObject<FAudioDeviceModule>(queue_factory));

	return AudioDeviceModule;
}

int32 FAudioDeviceModule::ActiveAudioLayer(AudioLayer* audioLayer) const
{
	*audioLayer = AudioLayer::kDummyAudio;
	return 0;
}

int32_t FAudioDeviceModule::RegisterAudioCallback(webrtc::AudioTransport* audio_callback)
{
	AudioCallback = audio_callback;
	return 0;
}

int16_t FAudioDeviceModule::PlayoutDevices()
{
	return 0;
}

int16_t FAudioDeviceModule::RecordingDevices()
{
	return 0;
}

int32_t FAudioDeviceModule::PlayoutDeviceName(uint16_t,
	char[webrtc::kAdmMaxDeviceNameSize],
	char[webrtc::kAdmMaxGuidSize])
{
	return 0;
}

int32_t FAudioDeviceModule::RecordingDeviceName(uint16_t,
	char[webrtc::kAdmMaxDeviceNameSize],
	char[webrtc::kAdmMaxGuidSize])
{
	return 0;
}

int32_t FAudioDeviceModule::SetPlayoutDevice(uint16_t)
{
	return 0;
}

int32_t FAudioDeviceModule::SetPlayoutDevice(WindowsDeviceType)
{
	return 0;
}

int32_t FAudioDeviceModule::SetRecordingDevice(uint16_t)
{
	return 0;
}

int32_t FAudioDeviceModule::SetRecordingDevice(WindowsDeviceType)
{
	return 0;
}

int32_t FAudioDeviceModule::InitPlayout()
{
	bIsPlayInitialized = true;
	return 0;
}

int32_t FAudioDeviceModule::InitRecording()
{
	return 0;
}

int32_t FAudioDeviceModule::StartPlayout()
{
	UE_LOG(LogMillicastPlayer, Log, TEXT("Start Playout"));
	{
		rtc::CritScope cs(&CriticalSection);
		bIsPlaying = true;
	}

	AsyncTask(ENamedThreads::GameThread, [this]() {
		if (SoundStreaming == nullptr)
		{
			InitSoundWave();
		}
		else
		{
			SoundStreaming->ResetAudio();
		}

		AudioComponent->Play(0.0f);
		TaskQueue.PostTask([this]() { Process(); });
	});

	return 0;
}

int32_t FAudioDeviceModule::StopPlayout()
{
	UE_LOG(LogMillicastPlayer, Log, TEXT("Stop Playout"));
	bool start = false;
	{
		rtc::CritScope cs(&CriticalSection);
		bIsPlaying = false;
		bIsStarted = false;
	}

	TaskQueue.PostTask([this]() {
		AsyncTask(ENamedThreads::GameThread, [this]() {
			if (AudioComponent->IsPlaying())
			{
				AudioComponent->Stop();
			}
			AudioComponent = nullptr;
			SoundStreaming = nullptr;
		});
	});

	return 0;
}

bool FAudioDeviceModule::Playing() const
{
	rtc::CritScope cs(&CriticalSection);
	return bIsPlaying;
}

int32_t FAudioDeviceModule::StartRecording()
{
	return 0;
}

int32_t FAudioDeviceModule::StopRecording()
{
	return 0;
}

bool FAudioDeviceModule::Recording() const
{
	return false;
}

int32 FAudioDeviceModule::RecordingIsAvailable(bool* available)
{
	*available = false;
	return 0;
}

int32_t FAudioDeviceModule::SetMicrophoneVolume(uint32_t volume)
{
	return 0;
}

int32_t FAudioDeviceModule::MicrophoneVolume(uint32_t* volume) const
{
	*volume = 0;
	return 0;
}

int32_t FAudioDeviceModule::MaxMicrophoneVolume(
	uint32_t* max_volume) const
{
	*max_volume = kMaxVolume;
	return 0;
}

int32 RecordingIsAvailable(bool* available)
{
	*available = false;
	return 0;
}

int32 FAudioDeviceModule::PlayoutIsAvailable(bool* available)
{
	*available = true;
	return 0;
}

int32 FAudioDeviceModule::StereoPlayoutIsAvailable(bool* available) const
{
	*available = true;
	return 0;
}

int32 FAudioDeviceModule::StereoPlayout(bool* available) const
{
	*available = true;
	return 0;
}

int32 FAudioDeviceModule::StereoRecordingIsAvailable(bool* available) const
{
	*available = false;
	return 0;
}

int32 FAudioDeviceModule::SetStereoRecording(bool enable)
{
	return 0;
}

int32 FAudioDeviceModule::StereoRecording(bool* enabled) const
{
	*enabled = false;
	return 0;
}

int32_t FAudioDeviceModule::PlayoutDelay(uint16_t* delay_ms) const
{
	*delay_ms = 0;
	return 0;
}

void FAudioDeviceModule::Process()
{
	RTC_DCHECK_RUN_ON(&TaskQueue);
	{
		rtc::CritScope cs(&CriticalSection);

		if (!bIsStarted)
		{
			NextFrameTime = rtc::TimeMillis();
			bIsStarted = true;
		}

		if (bIsPlaying)
		{
			PullAudioData();
			NextFrameTime += kTimePerFrameMs;
			const int64_t current_time = rtc::TimeMillis();
			const int64_t wait_time =
				(NextFrameTime > current_time) ? NextFrameTime - current_time : 0;
			TaskQueue.PostDelayedTask([this]() { Process(); }, int32_t(wait_time));
		}
	}
}

void FAudioDeviceModule::PullAudioData()
{
	int64_t elapsed, ntp;
	size_t  out;

	AudioCallback->NeedMorePlayData(kNumberSamples, sizeof(Sample), kNumberOfChannels,
		kSamplesPerSecond, AudioBuffer, out, &elapsed, &ntp);

	SoundStreaming->QueueAudio(AudioBuffer, kNumberSamples * kNumberBytesPerSample);
}

