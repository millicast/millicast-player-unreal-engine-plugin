// Copyright CoSMoSoftware 2021. All Rights Reserved.

#pragma once

#include "modules/audio_device/include/audio_device.h"
#include "AudioMixerDevice.h"
#include "HAL/CriticalSection.h"
#include "GenericPlatform/GenericPlatformMisc.h"
#include "Templates/Function.h"
#include "Templates/SharedPointer.h"



// A custom audio device module for WebRTC.
class FAudioDeviceModule : public webrtc::AudioDeviceModule
{

public:
	FAudioDeviceModule() = default;

	virtual ~FAudioDeviceModule() = default;

private:

	// webrtc::AudioDeviceModule interface
	int32 ActiveAudioLayer(AudioLayer* audioLayer) const override;
	int32 RegisterAudioCallback(webrtc::AudioTransport* audioCallback) override;

	// Main initialization and termination
	int32 Init() override;
	int32 Terminate() override;
	bool Initialized() const override;

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
	bool PlayoutIsInitialized() const override;
	int32 RecordingIsAvailable(bool* available) override;
	int32 InitRecording() override;
	bool RecordingIsInitialized() const override;

	// Audio transport control
	virtual int32 StartPlayout() override;
	virtual int32 StopPlayout() override;

	// True when audio is being pulled by the instance.
	virtual bool Playing() const override;

	virtual int32 StartRecording() override;
	virtual int32 StopRecording() override;
	virtual bool Recording() const override;

	// Audio mixer initialization
	virtual int32 InitSpeaker() override;
	virtual bool SpeakerIsInitialized() const override;
	virtual int32 InitMicrophone() override;
	virtual bool MicrophoneIsInitialized() const override;

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
	virtual int32 SetMicrophoneVolume(uint32 volume) override
	{
		return 0;
	}
	virtual int32 MicrophoneVolume(uint32* volume) const override
	{
		return 0;
	}
	virtual int32 MaxMicrophoneVolume(uint32* maxVolume) const override
	{
		*maxVolume = 0;
		return 0;
	}
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
	virtual int32 SetStereoPlayout(bool enable) override;
	virtual int32 StereoPlayout(bool* enabled) const override;
	virtual int32 StereoRecordingIsAvailable(bool* available) const override;
	virtual int32 SetStereoRecording(bool enable) override;
	virtual int32 StereoRecording(bool* enabled) const override;

	// Playout delay
	virtual int32 PlayoutDelay(uint16* delayMS) const override
	{
		*delayMS = 0;
		return 0;
	}

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

};
