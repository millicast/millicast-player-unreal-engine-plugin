// Copyright CoSMoSoftware 2021. All Rights Reserved.

#include "PeerConnection.h"

#include <sstream>

#include "api/video_codecs/builtin_video_decoder_factory.h"
#include "api/video_codecs/builtin_video_encoder_factory.h"
#include "api/video_codecs/video_decoder_factory.h"
#include "api/video_codecs/video_encoder_factory.h"
#include "api/audio_codecs/audio_decoder_factory.h"
#include "api/audio_codecs/audio_encoder_factory.h"
#include "api/audio_codecs/builtin_audio_decoder_factory.h"
#include "api/audio_codecs/builtin_audio_encoder_factory.h"
#include "api/create_peerconnection_factory.h"
#include "api/rtp_receiver_interface.h"
#include "api/task_queue/default_task_queue_factory.h"

#include <rtc_base/ssl_adapter.h>

#include "AudioDeviceModule.h"

rtc::scoped_refptr<webrtc::PeerConnectionFactoryInterface> FWebRTCPeerConnection::PeerConnectionFactory = nullptr;
TUniquePtr<rtc::Thread> FWebRTCPeerConnection::SignalingThread = nullptr;
rtc::scoped_refptr<webrtc::AudioDeviceModule> FWebRTCPeerConnection::AudioDeviceModule = nullptr;
std::unique_ptr<webrtc::TaskQueueFactory> FWebRTCPeerConnection::TaskQueueFactory = nullptr;

void FWebRTCPeerConnection::CreatePeerConnectionFactory()
{
	UE_LOG(LogMillicastPlayer, Log, TEXT("Creating FWebRTCPeerConnectionFactory"));

	rtc::InitializeSSL();

	SignalingThread  = TUniquePtr<rtc::Thread>(rtc::Thread::Create().release());
	SignalingThread->SetName("WebRTCSignalingThread", nullptr);
	SignalingThread->Start();

	TaskQueueFactory = webrtc::CreateDefaultTaskQueueFactory();
	AudioDeviceModule = FAudioDeviceModule::Create(TaskQueueFactory.get());

	PeerConnectionFactory = webrtc::CreatePeerConnectionFactory(
				nullptr, nullptr, SignalingThread.Get(), AudioDeviceModule,
				webrtc::CreateBuiltinAudioEncoderFactory(),
				webrtc::CreateBuiltinAudioDecoderFactory(),
				webrtc::CreateBuiltinVideoEncoderFactory(),
				webrtc::CreateBuiltinVideoDecoderFactory(),
				nullptr, nullptr
	  ).release();

	// Check
	if (!PeerConnectionFactory)
	{
		UE_LOG(LogMillicastPlayer, Error, TEXT("Creating PeerConnectionFactory | Failed"));
		return;
	}

	webrtc::PeerConnectionFactoryInterface::Options Options;
	Options.crypto_options.srtp.enable_gcm_crypto_suites = true;
	PeerConnectionFactory->SetOptions(Options);
}

webrtc::PeerConnectionInterface::RTCConfiguration FWebRTCPeerConnection::GetDefaultConfig()
{
	FRTCConfig Config(webrtc::PeerConnectionInterface::RTCConfigurationType::kAggressive);

	Config.set_cpu_adaptation(true);
	Config.combined_audio_video_bwe.emplace(true);
	Config.sdp_semantics = webrtc::SdpSemantics::kUnifiedPlan;

	return Config;
}
  
FWebRTCPeerConnection* FWebRTCPeerConnection::Create(const FRTCConfig& Config)
{
	if(PeerConnectionFactory == nullptr)
	{
		CreatePeerConnectionFactory();
	}

	FWebRTCPeerConnection * PeerConnectionInstance = new FWebRTCPeerConnection();
	webrtc::PeerConnectionDependencies deps(PeerConnectionInstance);

	PeerConnectionInstance->PeerConnection =
			PeerConnectionFactory->CreatePeerConnection(Config,
														nullptr,
														nullptr,
														PeerConnectionInstance);

	PeerConnectionInstance->CreateSessionDescription =
			MakeUnique<FCreateSessionDescriptionObserver>();
	PeerConnectionInstance->LocalSessionDescription  =
			MakeUnique<FSetSessionDescriptionObserver>();
	PeerConnectionInstance->RemoteSessionDescription =
			MakeUnique<FSetSessionDescriptionObserver>();

	return PeerConnectionInstance;
}

FWebRTCPeerConnection::FSetSessionDescriptionObserver*
FWebRTCPeerConnection::GetLocalDescriptionObserver()
{
	return LocalSessionDescription.Get();
}

FWebRTCPeerConnection::FSetSessionDescriptionObserver*
FWebRTCPeerConnection::GetRemoteDescriptionObserver()
{
	return RemoteSessionDescription.Get();
}

FWebRTCPeerConnection::FCreateSessionDescriptionObserver*
FWebRTCPeerConnection::GetCreateDescriptionObserver()
{
	return CreateSessionDescription.Get();
}

const FWebRTCPeerConnection::FSetSessionDescriptionObserver*
FWebRTCPeerConnection::GetLocalDescriptionObserver() const
{
	return LocalSessionDescription.Get();
}

const FWebRTCPeerConnection::FSetSessionDescriptionObserver*
FWebRTCPeerConnection::GetRemoteDescriptionObserver() const
{
	return RemoteSessionDescription.Get();
}

const FWebRTCPeerConnection::FCreateSessionDescriptionObserver*
FWebRTCPeerConnection::GetCreateDescriptionObserver() const
{
	return CreateSessionDescription.Get();
}


void FWebRTCPeerConnection::CreateOffer()
{
	SignalingThread->PostTask(RTC_FROM_HERE, [this]() {
		PeerConnection->CreateOffer(CreateSessionDescription.Release(),
									OaOptions);
	});
}

template<typename Callback>
webrtc::SessionDescriptionInterface* FWebRTCPeerConnection::CreateDescription(const std::string& Type,
									const std::string& Sdp,
									Callback&& Failed)
{
	if (Type.empty() || Sdp.empty())
	{
		std::string Msg = "Wrong input parameter, type or sdp missing";
		Failed(Msg);
		return nullptr;
	}

	webrtc::SdpParseError ParseError;
	webrtc::SessionDescriptionInterface* SessionDescription(webrtc::CreateSessionDescription(Type, Sdp, &ParseError));

	if (!SessionDescription)
	{
		std::ostringstream oss;
		oss << "Can't parse received session description message. SdpParseError line "
			<< ParseError.line <<  " : " + ParseError.description;

		Failed(oss.str());

		return nullptr;
	}

	return SessionDescription;
}

void FWebRTCPeerConnection::SetLocalDescription(const std::string& Sdp,
												const std::string& Type)
{
	  auto * SessionDescription = CreateDescription(Type,
													 Sdp,
													 std::ref(LocalSessionDescription->OnFailureCallback));

	  if(!SessionDescription) return;

	  PeerConnection->SetLocalDescription(LocalSessionDescription.Release(),
										  SessionDescription);
}

void FWebRTCPeerConnection::SetRemoteDescription(const std::string& Sdp,
												 const std::string& Type)
{
	auto * SessionDescription = CreateDescription(Type,
												  Sdp,
												  std::ref(RemoteSessionDescription->OnFailureCallback));

	if(!SessionDescription) return;

	PeerConnection->SetRemoteDescription(RemoteSessionDescription.Release(), SessionDescription);
}

void FWebRTCPeerConnection::SetVideoSink(rtc::VideoSinkInterface<webrtc::VideoFrame>* Sink)
{
	VideoSink = Sink;
}

void FWebRTCPeerConnection::SetAudioSink(webrtc::AudioTrackSinkInterface* Sink)
{
	AudioSink = Sink;
}

void FWebRTCPeerConnection::OnSignalingChange(webrtc::PeerConnectionInterface::SignalingState)
{}

void FWebRTCPeerConnection::OnAddStream(rtc::scoped_refptr<webrtc::MediaStreamInterface>)
{}

void FWebRTCPeerConnection::OnRemoveStream(rtc::scoped_refptr<webrtc::MediaStreamInterface>)
{}

void FWebRTCPeerConnection::OnAddTrack(rtc::scoped_refptr<webrtc::RtpReceiverInterface>,
				const FMediaStreamVector&)
{}

void FWebRTCPeerConnection::OnTrack(rtc::scoped_refptr<webrtc::RtpTransceiverInterface> Transceiver)
{
	if(VideoSink && Transceiver->media_type() == cricket::MediaType::MEDIA_TYPE_VIDEO)
	{
		auto * VideoTrack = static_cast<webrtc::VideoTrackInterface*>(Transceiver->receiver()->track().get());
		VideoTrack->AddOrUpdateSink(VideoSink, rtc::VideoSinkWants());
	}
	else if(AudioSink && Transceiver->media_type() == cricket::MediaType::MEDIA_TYPE_AUDIO)
	{
		auto * AudioTrack = static_cast<webrtc::AudioTrackInterface*>(Transceiver->receiver()->track().get());
		AudioTrack->AddSink(AudioSink);
	}
}

void FWebRTCPeerConnection::OnRemoveTrack(rtc::scoped_refptr<webrtc::RtpReceiverInterface>)
{}

void FWebRTCPeerConnection::OnDataChannel(rtc::scoped_refptr<webrtc::DataChannelInterface>)
{}

void FWebRTCPeerConnection::OnRenegotiationNeeded()
{}

void FWebRTCPeerConnection::OnIceConnectionChange(webrtc::PeerConnectionInterface::IceConnectionState)
{}

void FWebRTCPeerConnection::OnIceGatheringChange(webrtc::PeerConnectionInterface::IceGatheringState)
{}

void FWebRTCPeerConnection::OnIceCandidate(const webrtc::IceCandidateInterface*)
{}

void FWebRTCPeerConnection::OnIceConnectionReceivingChange(bool)
{}
