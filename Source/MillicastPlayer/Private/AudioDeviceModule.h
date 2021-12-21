// Copyright CoSMoSoftware 2021. All Rights Reserved.

#pragma once

#include "AudioMixerDevice.h"
#include "HAL/CriticalSection.h"
#include "GenericPlatform/GenericPlatformMisc.h"
#include "Templates/Function.h"
#include "Templates/SharedPointer.h"

#include <WebRTCInc.h>

#include "Sound/SoundWaveProcedural.h"

// A custom audio device module for WebRTC.
class FAudioDeviceModule : public webrtc::AudioDeviceModule
{
	typedef uint16_t Sample;

	static constexpr int kTimePerFrameMs = 10;
	static constexpr uint8_t kNumberOfChannels = 2;
	static constexpr int kSamplesPerSecond = 48000;
	static constexpr int kTotalDelayMs = 0;
	static constexpr int kClockDriftMs = 0;
	static constexpr uint32_t kMaxVolume = 14392;
	static constexpr size_t kNumberSamples = kTimePerFrameMs * kSamplesPerSecond / 1000;
	static constexpr size_t kNumberBytesPerSample = sizeof(Sample) * kNumberOfChannels;

	static const char kTimerQueueName[];

public:
	enum TaskMessage {
		MSG_START_PROCESS,
		MSG_RUN_PROCESS,
		MSG_STOP_PROCESS
	};

	explicit FAudioDeviceModule(webrtc::TaskQueueFactory * queue_factory) noexcept;

	~FAudioDeviceModule() = default;

	static rtc::scoped_refptr<FAudioDeviceModule> Create(webrtc::TaskQueueFactory * queue_factory);

private:

	// webrtc::AudioDeviceModule interface
	int32 ActiveAudioLayer(AudioLayer* audioLayer) const override;
	int32 RegisterAudioCallback(webrtc::AudioTransport* audioCallback) override;

	// Main initialization and termination
	int32 Init() override { return 0; }
	int32 Terminate() override { return 0; }
	bool Initialized() const override { return true; }

	// Device enumeration
	int16 PlayoutDevices() override;
	int16 RecordingDevices() override;
	int32 PlayoutDeviceName(uint16 index, char name[webrtc::kAdmMaxDeviceNameSize], char guid[webrtc::kAdmMaxGuidSize]) override;
	int32 RecordingDeviceName(uint16 index, char name[webrtc::kAdmMaxDeviceNameSize], char guid[webrtc::kAdmMaxGuidSize]) override;

	// Device selection
	int32 SetPlayoutDevice(uint16 index) override;
	int32 SetPlayoutDevice(WindowsDeviceType device) override;
	int32 SetRecordingDevice(uint16 index) override;
	int32 SetRecordingDevice(WindowsDeviceType device) override;

	// Audio transport initialization
	int32 PlayoutIsAvailable(bool* available) override;
	int32 InitPlayout() override;
	bool PlayoutIsInitialized() const override { return true; }
	int32 RecordingIsAvailable(bool* available) override;
	int32 InitRecording() override;
	bool RecordingIsInitialized() const override { return true; }

	// Audio transport control
	virtual int32 StartPlayout() override;
	virtual int32 StopPlayout() override;

	// True when audio is being pulled by the instance.
	virtual bool Playing() const override;

	virtual int32 StartRecording() override;
	virtual int32 StopRecording() override;
	virtual bool Recording() const override;

	// Audio mixer initialization
	virtual int32 InitSpeaker() override { return 0; }
	virtual bool SpeakerIsInitialized() const override { return true; }
	virtual int32 InitMicrophone() override { return 0; }
	virtual bool MicrophoneIsInitialized() const override { return true; }

	// Speaker volume controls
	virtual int32 SpeakerVolumeIsAvailable(bool* available) override
	{
		return -1;
	}
	virtual int32 SetSpeakerVolume(uint32 volume) override
	{
		return -1;
	}
	virtual int32 SpeakerVolume(uint32* volume) const override
	{
		return -1;
	}
	virtual int32 MaxSpeakerVolume(uint32* maxVolume) const override
	{
		return -1;
	}
	virtual int32 MinSpeakerVolume(uint32* minVolume) const override
	{
		return -1;
	}

	// Microphone volume controls
	virtual int32 MicrophoneVolumeIsAvailable(bool* available) override
	{
		return 0;
	}
	virtual int32 SetMicrophoneVolume(uint32 volume) override;
	virtual int32 MicrophoneVolume(uint32* volume) const override;
	virtual int32 MaxMicrophoneVolume(uint32* maxVolume) const override;
	virtual int32 MinMicrophoneVolume(uint32* minVolume) const override
	{
		return 0;
	}

	// Speaker mute control
	virtual int32 SpeakerMuteIsAvailable(bool* available) override
	{
		return -1;
	}
	virtual int32 SetSpeakerMute(bool enable) override
	{
		return -1;
	}
	virtual int32 SpeakerMute(bool* enabled) const override
	{
		return -1;
	}

	// Microphone mute control
	virtual int32 MicrophoneMuteIsAvailable(bool* available) override
	{
		*available = false;
		return -1;
	}
	virtual int32 SetMicrophoneMute(bool enable) override
	{
		return -1;
	}
	virtual int32 MicrophoneMute(bool* enabled) const override
	{
		return -1;
	}

	// Stereo support
	virtual int32 StereoPlayoutIsAvailable(bool* available) const override;
	virtual int32 SetStereoPlayout(bool enable) override { return 0; }
	virtual int32 StereoPlayout(bool* enabled) const override;
	virtual int32 StereoRecordingIsAvailable(bool* available) const override;
	virtual int32 SetStereoRecording(bool enable) override;
	virtual int32 StereoRecording(bool* enabled) const override;

	// Playout delay
	virtual int32 PlayoutDelay(uint16* delayMS) const override;

	virtual bool BuiltInAECIsAvailable() const override
	{
		return false;
	}
	virtual bool BuiltInAGCIsAvailable() const override
	{
		return false;
	}
	virtual bool BuiltInNSIsAvailable() const override
	{
		return false;
	}

	// Enables the built-in audio effects. Only supported on Android.
	virtual int32 EnableBuiltInAEC(bool enable) override
	{
		return -1;
	}
	virtual int32 EnableBuiltInAGC(bool enable) override
	{
		return -1;
	}
	virtual int32 EnableBuiltInNS(bool enable) override
	{
		return -1;
	}

private:
	void ReceiveFrameP();
	void ProcessFrameP();
	void StartProcessP();
	void UpdateProcessing(bool);
	bool ShouldStartProcessing();
	void OnMessage(TaskMessage msg);

	// Callback for playout and recording.
	webrtc::AudioTransport* audio_callback_;

	bool recording_;  // True when audio is being pushed from the instance.
	bool playing_;    // True when audio is being pulled by the instance.

	bool play_is_initialized_;  // True when the instance is ready to pull audio.
	bool rec_is_initialized_;   // True when the instance is ready to push audio.

	// Input to and output from RecordedDataIsAvailable(..) makes it possible to
	// modify the current mic level. The implementation does not care about the
	// mic level so it just feeds back what it receives.
	uint32_t current_mic_level_;

	// next_frame_time_ is updated in a non-drifting manner to indicate the next
	// wall clock time the next frame should be generated and received. started_
	// ensures that next_frame_time_ can be initialized properly on first call.
	bool started_;
	int64_t next_frame_time_;

	rtc::TaskQueue task_queue_;

	// Buffer for samples to send to the webrtc::AudioTransport.
	char _rec_buffer[kNumberSamples * kNumberBytesPerSample];


	// Protects variables that are accessed from process_thread_ and
	// the main thread.
	mutable rtc::CriticalSection crit_;
	// Protects |audio_callback_| that is accessed from process_thread_ and
	// the main thread.
	// mutable webrtc::Mutex crit_callback_;

	USoundWaveProcedural* SoundStreaming;
	UAudioComponent * AudioComponent;
};
