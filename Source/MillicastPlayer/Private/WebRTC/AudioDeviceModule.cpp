// Copyright CoSMoSoftware 2021. All Rights Reserved.

#include "AudioDeviceModule.h"
#include "MillicastPlayerPrivate.h"

const char FAudioDeviceModule::kTimerQueueName[] = "FAudioDeviceModuleTimer";
TAtomic<bool> FAudioDeviceModule::ReadDataAvailable = false;


FAudioDeviceModule::FAudioDeviceModule(webrtc::TaskQueueFactory* queue_factory) noexcept
	: AudioCallback(nullptr),
	bIsPlaying(false),
	bIsPlayInitialized(false),
	bIsStarted(false),
	NextFrameTime(0),
	TaskQueue(queue_factory->CreateTaskQueue(kTimerQueueName,
		webrtc::TaskQueueFactory::Priority::NORMAL))
{}

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
		FScopeLock cs(&CriticalSection);
		bIsPlaying = true;
	}

	AsyncTask(ENamedThreads::GameThread, [this]() {
		if (IMillicastExternalAudioConsumer* Consumer = AudioConsumer.Get())
		{
			AudioParameters = Consumer->GetAudioParameters();
			Consumer->Initialize();
		}

		ReadDataAvailable = false;
		AudioBuffer.SetNumUninitialized(AudioParameters.GetNumberSamples() * AudioParameters.GetNumberBytesPerSample());
		TaskQueue.PostTask([this]() { Process(); });
	});

	return 0;
}

int32_t FAudioDeviceModule::StopPlayout()
{
	UE_LOG(LogMillicastPlayer, Log, TEXT("Stop Playout"));
	bool start = false;
	{
		FScopeLock cs(&CriticalSection);
		bIsPlaying = false;
		bIsStarted = false;
		ReadDataAvailable = false;
	}

	TaskQueue.PostTask([this]() {
		AsyncTask(ENamedThreads::GameThread, [this]() {
			if (IMillicastExternalAudioConsumer* Consumer = AudioConsumer.Get())
			{
				Consumer->Shutdown();
			}
		});
	});

	return 0;
}

bool FAudioDeviceModule::Playing() const
{
	FScopeLock cs(&CriticalSection);
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

void FAudioDeviceModule::SetAudioConsumer(TWeakInterfacePtr<IMillicastExternalAudioConsumer> Consumer)
{
	AudioConsumer = Consumer;
}

void FAudioDeviceModule::Process()
{
	RTC_DCHECK_RUN_ON(&TaskQueue);
	{
		FScopeLock cs(&CriticalSection);

		if (!bIsStarted)
		{
			NextFrameTime = rtc::TimeMillis();
			bIsStarted = true;
		}

		if (bIsPlaying)
		{
			AsyncTask(ENamedThreads::GameThread, [this]() { PullAudioData(); });
			// PullAudioData();

			NextFrameTime += AudioParameters.TimePerFrameMs;
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

	AudioCallback->NeedMorePlayData(AudioParameters.GetNumberSamples(), AudioParameters.SampleSize,
		AudioParameters.NumberOfChannels, AudioParameters.SamplesPerSecond, AudioBuffer.GetData(),
		out, &elapsed, &ntp);

	// Before the stream actually started playing, elapsed == -1 and all samples are silent. Don't queue those
	if (elapsed >= 0)
	{
		ReadDataAvailable = true;
		if (IMillicastExternalAudioConsumer* Consumer = AudioConsumer.Get())
		{
			Consumer->QueueAudioData(AudioBuffer.GetData(), out);
		}
	}
	else
	{
		ReadDataAvailable = false;
	}
}

