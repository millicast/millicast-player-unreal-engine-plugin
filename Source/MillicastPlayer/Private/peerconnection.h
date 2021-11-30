#ifndef MILLICAST_PEERCONNECTION_H
#define MILLICAST_PEERCONNECTION_H

#include <absl/base/config.h>
#undef ABSL_USES_STD_OPTIONAL

#include <api/peer_connection_interface.h>

#include "session_description_observer.h"

namespace webrtc {

class TaskQueueFactory;
class AudioDeviceModule;

}  // webrtc

namespace millicast {

/*
 * Small wrapper not to expose the peerconnection header
 */

class PeerConnection : public webrtc::PeerConnectionObserver
{
  using MediaStreamVector = std::vector<rtc::scoped_refptr<webrtc::MediaStreamInterface>>;
  using RTCConfig = webrtc::PeerConnectionInterface::RTCConfiguration;

  rtc::scoped_refptr<webrtc::PeerConnectionInterface> _pc;

  static std::shared_ptr<rtc::Thread>                        _working_thread;
  static std::shared_ptr<rtc::Thread>                        _signaling_thread;
  static rtc::scoped_refptr<webrtc::AudioDeviceModule>       _adm;

  using CreateSdo = SessionDescriptionObserver<webrtc::CreateSessionDescriptionObserver>;
  using SetSdo = SessionDescriptionObserver<webrtc::SetSessionDescriptionObserver>;

  std::unique_ptr<CreateSdo> _create_sdo;
  std::unique_ptr<SetSdo>    _local_desc_sdo;
  std::unique_ptr<SetSdo>    _remote_desc_sdo;

  rtc::VideoSinkInterface<webrtc::VideoFrame>* _video_sink;

  template<typename Callback>
  webrtc::SessionDescriptionInterface* create_description(const std::string&,
							  const std::string&,
							  Callback&&);

  static rtc::scoped_refptr<webrtc::PeerConnectionFactoryInterface> _pcf;
  static void create_peerconnection_factory();
  
public:

  webrtc::PeerConnectionInterface::RTCOfferAnswerOptions oa_options;
  
  PeerConnection() = default;

  static RTCConfig get_default_config();
  static PeerConnection* create(const RTCConfig& config);

  SetSdo* local_desc_observer()     { return _local_desc_sdo.get(); }
  SetSdo* remote_desc_observer()    { return _remote_desc_sdo.get(); }
  CreateSdo* create_desc_observer() { return _create_sdo.get(); }

  const SetSdo* local_desc_observer()     const { return _local_desc_sdo.get(); }
  const SetSdo* remote_desc_observer()    const { return _remote_desc_sdo.get(); }
  const CreateSdo* create_desc_observer() const { return _create_sdo.get(); }

  void set_video_sink(rtc::VideoSinkInterface<webrtc::VideoFrame>* sink) {
    _video_sink = sink;
  }
  void create_offer();
  void set_local_desc(const std::string& sdp, const std::string& type);
  void set_remote_desc(const std::string& sdp, const std::string& type=std::string("answer"));
  
  void OnSignalingChange(webrtc::PeerConnectionInterface::SignalingState new_state) override;
  void OnAddStream(rtc::scoped_refptr<webrtc::MediaStreamInterface> stream) override;
  void OnRemoveStream(rtc::scoped_refptr<webrtc::MediaStreamInterface> stream) override;
  void OnAddTrack(rtc::scoped_refptr<webrtc::RtpReceiverInterface> receiver,
      const MediaStreamVector& streams) override;
  void OnTrack(rtc::scoped_refptr<webrtc::RtpTransceiverInterface> transceiver) override;
  void OnRemoveTrack(rtc::scoped_refptr<webrtc::RtpReceiverInterface> receiver) override;
  void OnDataChannel(rtc::scoped_refptr<webrtc::DataChannelInterface> channel)  override;
  void OnRenegotiationNeeded() override;
  void OnIceConnectionChange(webrtc::PeerConnectionInterface::IceConnectionState new_state) override;
  void OnIceGatheringChange(webrtc::PeerConnectionInterface::IceGatheringState new_state) override;
  void OnIceCandidate(const webrtc::IceCandidateInterface* candidate) override;
  void OnIceConnectionReceivingChange(bool receiving) override;

  webrtc::PeerConnectionInterface* operator->() { return _pc.get(); }
};


}  // millicast

#endif /* MILLICAST_PEERCONNECTION_H */
