// Copyright CoSMoSoftware 2021. All Rights Reserved.

#include "AudioDeviceModule.h"
#include "MillicastPlayerPrivate.h"
#include "AudioMixerDevice.h"
#include "SampleBuffer.h"
#include "Engine/GameEngine.h"

int32 FAudioDeviceModule::ActiveAudioLayer(AudioLayer* audioLayer) const
{
	*audioLayer = AudioDeviceModule::kDummyAudio;
	return 0;
}

int32 FAudioDeviceModule::RegisterAudioCallback(webrtc::AudioTransport*)
{
	return 0;
}

int32 FAudioDeviceModule::Init()
{
	return 0;
}

int32 FAudioDeviceModule::Terminate()
{
	return 0;
}

bool FAudioDeviceModule::Initialized() const
{
	return true;
}

int16 FAudioDeviceModule::PlayoutDevices()
{
	return -1;
}

int16 FAudioDeviceModule::RecordingDevices()
{
	return -1;
}

int32 FAudioDeviceModule::PlayoutDeviceName(uint16, char name[webrtc::kAdmMaxDeviceNameSize], char guid[webrtc::kAdmMaxGuidSize])
{
	return -1;
}

int32 FAudioDeviceModule::RecordingDeviceName(uint16, char name[webrtc::kAdmMaxDeviceNameSize], char guid[webrtc::kAdmMaxGuidSize])
{
	return -1;
}

int32 FAudioDeviceModule::SetPlayoutDevice(uint16)
{
	return 0;
}

int32 FAudioDeviceModule::SetPlayoutDevice(WindowsDeviceType)
{
	return 0;
}

int32 FAudioDeviceModule::SetRecordingDevice(uint16)
{
	return 0;
}

int32 FAudioDeviceModule::SetRecordingDevice(WindowsDeviceType)
{
	return 0;
}

int32 FAudioDeviceModule::PlayoutIsAvailable(bool* available)
{
	*available = true;
	return 0;
}

int32 FAudioDeviceModule::InitPlayout()
{
	return 0;
}

bool FAudioDeviceModule::PlayoutIsInitialized() const
{
	return true;
}

int32 FAudioDeviceModule::StartPlayout()
{
	return 0;
}

int32 FAudioDeviceModule::StopPlayout()
{
	return 0;
}

bool FAudioDeviceModule::Playing() const
{
	return false;
}

int32 FAudioDeviceModule::RecordingIsAvailable(bool* available)
{
	*available = false;
	return 0;
}

int32 FAudioDeviceModule::InitRecording()
{
	return 0;
}

bool FAudioDeviceModule::RecordingIsInitialized() const
{
	return false;
}

int32 FAudioDeviceModule::StartRecording()
{
	return 0;
}

int32 FAudioDeviceModule::StopRecording()
{
	return 0;
}

bool FAudioDeviceModule::Recording() const
{
	return false;
}

int32 FAudioDeviceModule::InitSpeaker()
{
	return -1;
}

bool FAudioDeviceModule::SpeakerIsInitialized() const
{
	return false;
}

int32 FAudioDeviceModule::InitMicrophone()
{
	return 0;
}

bool FAudioDeviceModule::MicrophoneIsInitialized() const
{
	return true;
}

int32 FAudioDeviceModule::StereoPlayoutIsAvailable(bool* available) const
{
	*available = true;
	return 0;
}

int32 FAudioDeviceModule::SetStereoPlayout(bool enable)
{
	FString AudioChannelStr = enable ? TEXT("stereo") : TEXT("mono");
	UE_LOG(LogMillicastPlayer, Verbose, TEXT("WebRTC has requested browser audio playout in UE be: %s"), *AudioChannelStr);
	return 0;
}

int32 FAudioDeviceModule::StereoPlayout(bool* enabled) const
{
	*enabled = true;
	return 0;
}

int32 FAudioDeviceModule::StereoRecordingIsAvailable(bool* available) const
{
	*available = true;
	return 0;
}

int32 FAudioDeviceModule::SetStereoRecording(bool enable)
{
	return 0;
}

int32 FAudioDeviceModule::StereoRecording(bool* enabled) const
{
	*enabled = true;
	return 0;
}
