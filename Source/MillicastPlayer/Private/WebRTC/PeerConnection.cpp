// Copyright CoSMoSoftware 2021. All Rights Reserved.

#include "PeerConnection.h"

#include <sstream>
#include <pc/session_description.h>
#include <api/jsep_session_description.h>

#include "AudioDeviceModule.h"
#include "MillicastAudioActor.h"
#include "MillicastPlayerPrivate.h"

TAtomic<int> FWebRTCPeerConnection::RefCounter = 0;

void FWebRTCPeerConnection::CreatePeerConnectionFactory()
{
	UE_LOG(LogMillicastPlayer, Log, TEXT("Creating FWebRTCPeerConnectionFactory"));

	if (RefCounter == 0)
	{
		++RefCounter; // increase ref count
		rtc::InitializeSSL();
		rtc::InitRandom((int)rtc::Time());
	}
	else
	{
		++RefCounter;
	}

	SignalingThread = TUniquePtr<rtc::Thread>(rtc::Thread::Create().release());
	SignalingThread->SetName("WebRTCSignalingThread", nullptr);
	SignalingThread->Start();

	WorkingThread = TUniquePtr<rtc::Thread>(rtc::Thread::Create().release());
	WorkingThread->SetName("WebRTCWorkerThread", nullptr);
	WorkingThread->Start();

	NetworkingThread = TUniquePtr<rtc::Thread>(rtc::Thread::CreateWithSocketServer().release());
	NetworkingThread->SetName("WebRTCNetworkThread", nullptr);
	NetworkingThread->Start();

	TaskQueueFactory = webrtc::CreateDefaultTaskQueueFactory();
	AudioDeviceModule = FAudioDeviceModule::Create(TaskQueueFactory.get());

	PeerConnectionFactory = webrtc::CreatePeerConnectionFactory(
		NetworkingThread.Get(), WorkingThread.Get(), SignalingThread.Get(), AudioDeviceModule,
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

FWebRTCPeerConnection::~FWebRTCPeerConnection() noexcept
{
	PeerConnection = nullptr;

	WorkingThread->Invoke<void>(RTC_FROM_HERE, [this]() {
		AudioDeviceModule->StopPlayout();
		AudioDeviceModule->Terminate();
		});

	PeerConnectionFactory = nullptr;
	AudioDeviceModule = nullptr;

	SignalingThread->Stop();
	NetworkingThread->Stop();
	WorkingThread->Stop();

	--RefCounter;

	if (RefCounter == 0)
	{
		rtc::CleanupSSL();
	}
}

webrtc::PeerConnectionInterface::RTCConfiguration FWebRTCPeerConnection::GetDefaultConfig()
{
	FRTCConfig Config(webrtc::PeerConnectionInterface::RTCConfigurationType::kAggressive);

	Config.set_cpu_adaptation(false);
	Config.combined_audio_video_bwe.emplace(true);
	Config.sdp_semantics = webrtc::SdpSemantics::kUnifiedPlan;

	return Config;
}

FWebRTCPeerConnection* FWebRTCPeerConnection::Create(const FRTCConfig& Config, TWeakInterfacePtr<IMillicastExternalAudioConsumer> ExternalAudioConsumer)
{
	FWebRTCPeerConnection* PeerConnectionInstance = new FWebRTCPeerConnection();

	PeerConnectionInstance->Init(Config, ExternalAudioConsumer);

	return PeerConnectionInstance;
}

void FWebRTCPeerConnection::Init(const FRTCConfig& Config, TWeakInterfacePtr<IMillicastExternalAudioConsumer> ExternalAudioConsumer)
{
	CreatePeerConnectionFactory();

	if (ExternalAudioConsumer.IsValid())
	{
		AudioDeviceModule->SetAudioConsumer(ExternalAudioConsumer);
	}

	webrtc::PeerConnectionDependencies deps(this);

	auto result = PeerConnectionFactory->CreatePeerConnectionOrError(Config, std::move(deps));

	if (!result.ok())
	{
		UE_LOG(LogMillicastPlayer, Error, TEXT("Could not create peerconnection : %S"), result.error().message());
		PeerConnection = nullptr;
		return;
	}

	PeerConnection = result.value();

	CreateSessionDescription = MakeUnique<FCreateSessionDescriptionObserver>();
	LocalSessionDescription = MakeUnique<FSetSessionDescriptionObserver>();
	RemoteSessionDescription = MakeUnique<FSetSessionDescriptionObserver>();
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
	if(OnVideoTrack && Transceiver->media_type() == cricket::MediaType::MEDIA_TYPE_VIDEO)
	{
		OnVideoTrack(*Transceiver->mid(), Transceiver->receiver()->track());
	}
	else if (OnAudioTrack && Transceiver->media_type() == cricket::MediaType::MEDIA_TYPE_AUDIO)
	{
		OnAudioTrack(*Transceiver->mid(), Transceiver->receiver()->track());
	}
}

void FWebRTCPeerConnection::OnRemoveTrack(rtc::scoped_refptr<webrtc::RtpReceiverInterface>)
{}

void FWebRTCPeerConnection::OnDataChannel(rtc::scoped_refptr<webrtc::DataChannelInterface>)
{}

void FWebRTCPeerConnection::OnRenegotiationNeeded()
{
	UE_LOG(LogMillicastPlayer, Log, TEXT("On renegociation needed"));

	std::string sdp;
	auto remote_sdp = PeerConnection->remote_description();

	if (!remote_sdp) return;

	remote_sdp->ToString(&sdp);
	if (sdp.empty()) return;

	CreateSessionDescription = MakeUnique<FCreateSessionDescriptionObserver>();
	LocalSessionDescription = MakeUnique<FSetSessionDescriptionObserver>();
	RemoteSessionDescription = MakeUnique<FSetSessionDescriptionObserver>();

	CreateSessionDescription->SetOnSuccessCallback([this](const std::string& type, const std::string& sdp) {
		UE_LOG(LogMillicastPlayer, Log, TEXT("[renegociation] pc.createOffer() | Success"));
		SetLocalDescription(sdp, type);
		});

	CreateSessionDescription->SetOnFailureCallback([](const std::string& err) {
		UE_LOG(LogMillicastPlayer, Error, TEXT("[renegociation] pc.createOffer() | Error: %s"), err.c_str());
		});

	LocalSessionDescription->SetOnSuccessCallback([this]() {
		UE_LOG(LogMillicastPlayer, Log, TEXT("[renegociation] pc.setLocalDescription() | success"));
		Renegociate(PeerConnection->local_description(), PeerConnection->remote_description());
		});

	LocalSessionDescription->SetOnFailureCallback([](const std::string& err) {
		UE_LOG(LogMillicastPlayer, Error, TEXT("[renegociation]  Set local description failed | Error: %s"), err.c_str());
		});

	RemoteSessionDescription->SetOnSuccessCallback([]() {
		UE_LOG(LogMillicastPlayer, Log, TEXT("[renegociation] Set remote description | success"));
		});
	RemoteSessionDescription->SetOnFailureCallback([](const std::string& err) {
		UE_LOG(LogMillicastPlayer, Error, TEXT("[renegociation]  Set remote description failed | Error: %s"), err.c_str());
		});

	UE_LOG(LogMillicastPlayer, Log, TEXT("Starting renegociation"));

	CreateOffer();
}

void FWebRTCPeerConnection::OnIceConnectionChange(webrtc::PeerConnectionInterface::IceConnectionState)
{}

void FWebRTCPeerConnection::OnIceGatheringChange(webrtc::PeerConnectionInterface::IceGatheringState)
{}

void FWebRTCPeerConnection::OnIceCandidate(const webrtc::IceCandidateInterface*)
{}

void FWebRTCPeerConnection::OnIceConnectionReceivingChange(bool)
{}

static inline webrtc::RtpTransceiverDirection reverse_direction(webrtc::RtpTransceiverDirection direction)
{
	switch (direction) {
	case webrtc::RtpTransceiverDirection::kSendOnly:
		return webrtc::RtpTransceiverDirection::kRecvOnly;
	case webrtc::RtpTransceiverDirection::kRecvOnly:
		return webrtc::RtpTransceiverDirection::kSendOnly;
	default: return direction;
	}
}

void FWebRTCPeerConnection::Renegociate(const webrtc::SessionDescriptionInterface* local_sdp,
	const webrtc::SessionDescriptionInterface* remote_sdp)
{
	// Clone the remote sdp to have a setup a new one
	auto new_remote = remote_sdp->Clone();

	if (!new_remote) {
		UE_LOG(LogMillicastPlayer, Error, TEXT("Could not clone remote sdp"));
		return;
	}

	auto local_desc = local_sdp->description();

	int mline_index = 0; // Keep track of the mline index to add ice candidates
	for (const auto& offer_content : local_desc->contents()) {
		auto remote_desc = new_remote->description();

		// Find the corresponding mid in the answer
		auto answered_media = remote_desc->GetContentDescriptionByName(offer_content.mid());

		// If it does not exists create it
		if (!answered_media) {
			// Get offered media description
			auto offered_media = offer_content.media_description();

			// Copy the offer media into the answered media
			auto answered_media_new = offered_media->Clone();
			// Invert the transceiver direction
			answered_media_new->set_direction(reverse_direction(offered_media->direction()));

			// Add the media description for the answer
			remote_desc->AddContent(offer_content.mid(),
				cricket::MediaProtocolType::kRtp,
				std::move(answered_media_new));

			// Copy the transport info from the first mid of the remote desc
			auto transport_info = remote_desc->GetTransportInfoByName(remote_desc->FirstContent()->name);
			cricket::TransportInfo new_transport_info{ offer_content.mid(), transport_info->description };

			remote_desc->AddTransportInfo(new_transport_info);

			// Add mid to the BUNDLE group
			cricket::ContentGroup bundle = remote_desc->groups().front();
			bundle.AddContentName(offer_content.mid());
			remote_desc->RemoveGroupByName(bundle.semantics());
			remote_desc->AddGroup(bundle);

			// reinit to update the number of mediasections
			auto new_remote_jsep = static_cast<webrtc::JsepSessionDescription*>(new_remote.get());
			new_remote_jsep->Initialize(remote_desc->Clone(),
				new_remote->session_id(),
				new_remote->session_version());

			// Copy ice candidates for the new mline_index
			auto candidates = new_remote->candidates(0);
			for (size_t i = 0; i < candidates->count(); ++i) {
				auto c = candidates->at(i);
				auto new_candidate = webrtc::CreateIceCandidate(offer_content.mid(),
					mline_index,
					c->candidate());
				new_remote->AddCandidate(new_candidate.release());
			}
		}

		++mline_index;
	}

	std::string sdp;
	new_remote->ToString(&sdp);

	UE_LOG(LogMillicastPlayer, Log, TEXT("[renegociation] remote sdp : %s"), sdp.c_str());
	SetRemoteDescription(sdp);
}