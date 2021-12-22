// Copyright CoSMoSoftware 2021. All Rights Reserved.

#include "AudioDeviceModule.h"
#include "MillicastPlayerPrivate.h"
#include "AudioMixerDevice.h"
#include "SampleBuffer.h"
#include "Engine/GameEngine.h"

const char FAudioDeviceModule::kTimerQueueName[] = "FAudioDeviceModuleTimer";

FAudioDeviceModule::FAudioDeviceModule(webrtc::TaskQueueFactory * queue_factory) noexcept
  : audio_callback_(nullptr),
	recording_(false),
	playing_(false),
	play_is_initialized_(false),
	rec_is_initialized_(false),
	current_mic_level_(kMaxVolume),
	started_(false),
	next_frame_time_(0),
	task_queue_(queue_factory->CreateTaskQueue(kTimerQueueName,
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
	SoundStreaming->bLooping = false;

	if (AudioComponent == nullptr) {
		auto AudioDevice = GEngine->GetMainAudioDevice();
		AudioComponent = AudioDevice->CreateComponent(SoundStreaming);
		AudioComponent->bIsUISound = false;
		AudioComponent->bAllowSpatialization = false;
		AudioComponent->SetVolumeMultiplier(1.0f);
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
FAudioDeviceModule::Create(webrtc::TaskQueueFactory * queue_factory)
{
  rtc::scoped_refptr<FAudioDeviceModule>
	adm(new rtc::RefCountedObject<FAudioDeviceModule>(queue_factory));

  return adm;
}

int32 FAudioDeviceModule::ActiveAudioLayer(AudioLayer* audioLayer) const
{
	*audioLayer = AudioLayer::kDummyAudio;
	return 0;
}

int32_t FAudioDeviceModule::RegisterAudioCallback(webrtc::AudioTransport* audio_callback)
{
  // rtc::CritScope cs(&crit_callback_);

  audio_callback_ = audio_callback;
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

int32_t FAudioDeviceModule::RecordingDeviceName(uint16_t ,
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
  return ((play_is_initialized_) ? -1 : 0);
}

int32_t FAudioDeviceModule::SetRecordingDevice(uint16_t)
{
  return 0;
}

int32_t FAudioDeviceModule::SetRecordingDevice(WindowsDeviceType)
{
  return ((rec_is_initialized_) ? -1 : 0);
}

int32_t FAudioDeviceModule::InitPlayout()
{
  play_is_initialized_ = true;
  return 0;
}

int32_t FAudioDeviceModule::InitRecording()
{
  rec_is_initialized_ = true;
  return 0;
}

int32_t FAudioDeviceModule::StartPlayout()
{
	UE_LOG(LogMillicastPlayer, Log, TEXT("Start Playout"));
  {
	rtc::CritScope cs(&crit_);
	playing_ = true;
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
	  UpdateProcessing(true);
  });

  return 0;
}

int32_t FAudioDeviceModule::StopPlayout()
{
	UE_LOG(LogMillicastPlayer, Log, TEXT("Stop Playout"));
  bool start = false;
  {
	rtc::CritScope cs(&crit_);
	playing_ = false;
	start = ShouldStartProcessing();
  }

  task_queue_.PostTask([this]() {
	  OnMessage(TaskMessage::MSG_STOP_PROCESS);
  });

  task_queue_.PostTask([this]() {
	  AsyncTask(ENamedThreads::GameThread, [this]() {
		  AudioComponent->Stop();
		  SoundStreaming = nullptr;
	  });
  });

  return 0;
}

bool FAudioDeviceModule::Playing() const
{
  rtc::CritScope cs(&crit_);
  return playing_;
}

int32_t FAudioDeviceModule::StartRecording()
{
  if (!rec_is_initialized_) {
	return -1;
  }
  {
	rtc::CritScope cs(&crit_);
	recording_ = true;
  }

  bool start = true;
  UpdateProcessing(start);
  return 0;
}

int32_t FAudioDeviceModule::StopRecording()
{
  bool start = false;
  {
	rtc::CritScope cs(&crit_);
	recording_ = false;
	start = ShouldStartProcessing();
  }

  UpdateProcessing(start);
  return 0;
}

bool FAudioDeviceModule::Recording() const
{
  rtc::CritScope cs(&crit_);
  return recording_;
}

int32 FAudioDeviceModule::RecordingIsAvailable(bool* available)
{
	*available = false;
	return 0;
}

int32_t FAudioDeviceModule::SetMicrophoneVolume(uint32_t volume)
{
  current_mic_level_ = volume;
  return 0;
}

int32_t FAudioDeviceModule::MicrophoneVolume(uint32_t* volume) const
{
  *volume = current_mic_level_;
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

void FAudioDeviceModule::OnMessage(TaskMessage msg)
{
  switch (msg) {
  case MSG_START_PROCESS:
	StartProcessP();
	break;
  case MSG_RUN_PROCESS:
	ProcessFrameP();
	break;
  case MSG_STOP_PROCESS:
	started_ = false;
	break;
  default:
	// All existing messages should be caught. Getting here should never happen.
	RTC_NOTREACHED();
  }
}

bool FAudioDeviceModule::ShouldStartProcessing()
{
  return recording_ || playing_;
}

void FAudioDeviceModule::UpdateProcessing(bool start)
{
  if (start) {
	task_queue_.PostTask(std::bind(&FAudioDeviceModule::OnMessage,
				   this, MSG_START_PROCESS));
  }
  else {
	task_queue_.PostTask(std::bind(&FAudioDeviceModule::OnMessage,
				   this, MSG_STOP_PROCESS));
  }
}

void FAudioDeviceModule::StartProcessP()
{
  RTC_DCHECK_RUN_ON(&task_queue_);
  if (started_) return;
  ProcessFrameP();
}

void FAudioDeviceModule::ProcessFrameP()
{
  RTC_DCHECK_RUN_ON(&task_queue_);
  if (!started_) {
	next_frame_time_ = rtc::TimeMillis();
	started_ = true;
  }

  {
	rtc::CritScope cs(&crit_);
	// Receive and send frames every kTimePerFrameMs.
	if (playing_) ReceiveFrameP();
  }

  next_frame_time_ += kTimePerFrameMs;
  const int64_t current_time = rtc::TimeMillis();
  const int64_t wait_time =
	(next_frame_time_ > current_time) ? next_frame_time_ - current_time : 0;
  task_queue_.PostDelayedTask(std::bind(&FAudioDeviceModule::OnMessage,
					this,
					MSG_RUN_PROCESS),
				  int32_t(wait_time));
}

void FAudioDeviceModule::ReceiveFrameP()
{
	// constexpr float PI = 3.1415926355;
	RTC_DCHECK_RUN_ON(&task_queue_);

	// rtc::CritScope cs(&_crit_callback);

	int64_t elapsed, ntp;
	size_t  out;

	audio_callback_->NeedMorePlayData(kNumberSamples, sizeof(Sample), kNumberOfChannels,
					kSamplesPerSecond, _rec_buffer, out, &elapsed, &ntp);
	/*static int sini = 0;
	auto buf = reinterpret_cast<Sample*>(_rec_buffer);
	for(int i = 0; i < kNumberSamples * kNumberOfChannels; ++i) {
		if((i & 0x01) == 0) buf[i] = static_cast<Sample>(sin(sini++ * 2.f * PI * 440.f / kSamplesPerSecond) * 10000);
	}*/

	SoundStreaming->QueueAudio((uint8*)_rec_buffer, kNumberSamples * kNumberBytesPerSample);
}

