#ifndef MILLICAST_SESSION_DESCRIPTION_OBSERVER_H
#define MILLICAST_SESSION_DESCRIPTION_OBSERVER_H

#include <functional>

#include <api/peer_connection_interface.h>

class FWebRTCPeerConnection;

namespace detail
{

template<typename T>
class SessionDescriptionObserver : public rtc::RefCountedObject<T>
{
  friend class ::FWebRTCPeerConnection;
  // friend class PeerConnection;
  std::function<void(const std::string&)> _on_failure;
public:
  SessionDescriptionObserver() : _on_failure(nullptr) {}
  
  void OnFailure(webrtc::RTCError error) override {
    if(_on_failure) _on_failure(error.message());
  }

  template<typename Callback>
  void on_failure(Callback&& c) { _on_failure = std::forward<Callback>(c); }
};

}

template<typename T>
class SessionDescriptionObserver;

template<>
class SessionDescriptionObserver<webrtc::CreateSessionDescriptionObserver> :
    public detail::SessionDescriptionObserver<webrtc::CreateSessionDescriptionObserver>
{
  friend class FWebRTCPeerConnection;
  // type, sdp
  std::function<void(const std::string&, const std::string&)> _on_success;
  
public:

  void OnSuccess(webrtc::SessionDescriptionInterface* desc) override {
    std::string sdp;
    desc->ToString(&sdp);
    
    if(_on_success) _on_success(desc->type(), sdp);
  }

  template<typename Callback>
  void on_success(Callback&& c) { _on_success = std::forward<Callback>(c); }
};

template<typename T>
class SessionDescriptionObserver;

template<>
class SessionDescriptionObserver<webrtc::SetSessionDescriptionObserver> :
    public detail::SessionDescriptionObserver<webrtc::SetSessionDescriptionObserver>
{
  friend class FWebRTCPeerConnection;
  // type, sdp
  std::function<void()> _on_success;
  
public:

  void OnSuccess() override { 
    if(_on_success) _on_success();
  }

  template<typename Callback>
  void on_success(Callback&& c) { _on_success = std::forward<Callback>(c); }
};

#endif /* MILLICAST_SESSION_DESCRIPTION_OBSERVER_H */
