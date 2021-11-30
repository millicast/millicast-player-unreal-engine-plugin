#include "peerconnection.h"

#include <sstream>
#include <thread>

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
#include <api/task_queue/default_task_queue_factory.h>

#include <rtc_base/ssl_adapter.h>

#include "PixelStreamingAudioDeviceModule.h"

using millicast::PeerConnection;

rtc::scoped_refptr<webrtc::PeerConnectionFactoryInterface> PeerConnection::_pcf = nullptr;
std::shared_ptr<rtc::Thread> PeerConnection::_working_thread = nullptr;
std::shared_ptr<rtc::Thread> PeerConnection::_signaling_thread = nullptr;
rtc::scoped_refptr<webrtc::AudioDeviceModule> PeerConnection::_adm = nullptr;

void PeerConnection::create_peerconnection_factory()
{
  rtc::InitializeSSL();

  UE_LOG(LogMillicastPlayer, Log, TEXT("Creating PeerConnectionFactory"));
  _signaling_thread  = rtc::Thread::Create();
  _signaling_thread->SetName("WebRtcSignallingThread", nullptr);
  _signaling_thread->Start();
  /*_working_thread = rtc::Thread::Create();
  _working_thread->SetName("_working_thread", NULL);
  if(!_working_thread->Start()){
    RTC_LOG(LS_ERROR) << "Working thread failed to start";
    return;
  }*/

  //Initialize things on working thread thread
  /*auto ret = _working_thread->Invoke<bool>(RTC_FROM_HERE, []() {
      //Create audio module
      _adm = new rtc::RefCountedObject<FPixelStreamingAudioDeviceModule>();

      if(!_adm) {
        RTC_LOG(LS_ERROR) << "AudioDeviceModule creation failed";
        return false;
      }

      RTC_LOG(LS_WARNING) << "AudioDeviceModule created with success";

      return true;
  });*/

  UE_LOG(LogMillicastPlayer, Log, TEXT("Creating AudioDeviceModule"));
  _adm = new rtc::RefCountedObject<FPixelStreamingAudioDeviceModule>();

  UE_LOG(LogMillicastPlayer, Log, TEXT("Creating PCF"));
  _pcf = webrtc::CreatePeerConnectionFactory(
        nullptr, nullptr, _signaling_thread.get(), _adm,
        webrtc::CreateBuiltinAudioEncoderFactory(),
        webrtc::CreateBuiltinAudioDecoderFactory(),
        webrtc::CreateBuiltinVideoEncoderFactory(),
        webrtc::CreateBuiltinVideoDecoderFactory(),
        nullptr, nullptr
      ).release();

  // Check
  if (!_pcf){
      UE_LOG(LogMillicastPlayer, Error, TEXT("Creating PeerConnectionFactory | Failed"));
      return;
  }

  UE_LOG(LogMillicastPlayer, Log, TEXT("Creating PeerConnectionFactory | OK"));

  webrtc::PeerConnectionFactoryInterface::Options options;
  options.crypto_options.srtp.enable_gcm_crypto_suites = true;
  _pcf->SetOptions(options);
}

webrtc::PeerConnectionInterface::RTCConfiguration PeerConnection::get_default_config()
{ 
  RTCConfig config(webrtc::PeerConnectionInterface::RTCConfigurationType::kAggressive);

  config.set_cpu_adaptation(true);
  config.combined_audio_video_bwe.emplace(true);
  config.sdp_semantics = webrtc::SdpSemantics::kUnifiedPlan;

  return config;
}
  
PeerConnection* PeerConnection::create(const RTCConfig& config)
{
  if(_pcf == nullptr) create_peerconnection_factory();

  PeerConnection * pc = new PeerConnection();
  webrtc::PeerConnectionDependencies deps(pc);

  pc->_pc =_pcf->CreatePeerConnection(config, nullptr, nullptr, pc);
  
  pc->_create_sdo      = std::make_unique<CreateSdo>();
  pc->_local_desc_sdo  = std::make_unique<SetSdo>();
  pc->_remote_desc_sdo = std::make_unique<SetSdo>();
  
  return pc;
}

void PeerConnection::create_offer()
{
  _signaling_thread->PostTask(RTC_FROM_HERE, [this]() {
      _pc->CreateOffer(_create_sdo.release(), oa_options);
    });
}

template<typename Callback>
webrtc::SessionDescriptionInterface* PeerConnection::create_description(const std::string& type,
									const std::string& sdp,
									Callback&& failed)
{
  if (type.empty() || sdp.empty()) {
    std::string msg = "Wrong input parameter, type or sdp missing";
    failed(msg);
    return nullptr;
  }

  webrtc::SdpParseError parse_err;
  webrtc::SessionDescriptionInterface* session_description(webrtc::CreateSessionDescription(type, sdp, &parse_err));

  if (!session_description) {
    std::ostringstream oss;
    oss << "Can't parse received session description message. SdpParseError line "
	<< parse_err.line <<  " : " + parse_err.description;
    
    failed(oss.str());
	  
    return nullptr;
  }

  return session_description;
}

void PeerConnection::set_local_desc(const std::string& sdp, const std::string& type)
{
  auto * session_description = create_description(type,
						  sdp,
						  std::ref(_local_desc_sdo->_on_failure));

  if(!session_description) return;
  
  _pc->SetLocalDescription(_local_desc_sdo.release(), session_description);
}

void PeerConnection::set_remote_desc(const std::string& sdp, const std::string& type)
{
   auto * session_description = create_description(type,
						   sdp,
						   std::ref(_remote_desc_sdo->_on_failure));
   
   if(!session_description) return;
  
   _pc->SetRemoteDescription(_remote_desc_sdo.release(), session_description);
}

void PeerConnection::OnSignalingChange(webrtc::PeerConnectionInterface::SignalingState)
{}

void PeerConnection::OnAddStream(rtc::scoped_refptr<webrtc::MediaStreamInterface> m)
{}

void PeerConnection::OnRemoveStream(rtc::scoped_refptr<webrtc::MediaStreamInterface> m)
{}

void PeerConnection::OnAddTrack(rtc::scoped_refptr<webrtc::RtpReceiverInterface>,
				const MediaStreamVector&)
{}

void PeerConnection::OnTrack(rtc::scoped_refptr<webrtc::RtpTransceiverInterface> transceiver)
{
  UE_LOG(LogMillicastPlayer, Log, TEXT("OnTrack !"));
  if(_video_sink && transceiver->media_type() == cricket::MediaType::MEDIA_TYPE_VIDEO) {
      UE_LOG(LogMillicastPlayer, Log, TEXT("Add Video Sink"));
      auto * track = static_cast<webrtc::VideoTrackInterface*>(transceiver->receiver()->track().get());
      track->AddOrUpdateSink(_video_sink, rtc::VideoSinkWants());
  }
}

void PeerConnection::OnRemoveTrack(rtc::scoped_refptr<webrtc::RtpReceiverInterface> rec)
{}

void PeerConnection::OnDataChannel(rtc::scoped_refptr<webrtc::DataChannelInterface>)
{}

void PeerConnection::OnRenegotiationNeeded()
{}

void PeerConnection::OnIceConnectionChange(webrtc::PeerConnectionInterface::IceConnectionState)
{}

void PeerConnection::OnIceGatheringChange(webrtc::PeerConnectionInterface::IceGatheringState)
{}

void PeerConnection::OnIceCandidate(const webrtc::IceCandidateInterface* c)
{}

void PeerConnection::OnIceConnectionReceivingChange(bool)
{}
