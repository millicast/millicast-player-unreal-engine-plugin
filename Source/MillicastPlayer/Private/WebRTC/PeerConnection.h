// Copyright CoSMoSoftware 2021. All Rights Reserved.

#pragma once

#include "Runtime/Launch/Resources/Version.h"
#include "SessionDescriptionObserver.h"
#include "WebRTC/WebRTCInc.h"

namespace webrtc 
{
	class AudioDeviceModule;
	class TaskQueueFactory;
}  // webrtc


namespace Millicast::Player
{
	class FAudioDeviceModule;
	class FPlayerStatsCollector;
	
/*
 * Small wrapper for the WebRTC peerconnection
 */
	class FWebRTCPeerConnection : public webrtc::PeerConnectionObserver
	{
		using FMediaStreamVector = std::vector<rtc::scoped_refptr<webrtc::MediaStreamInterface>>;
		using FRTCConfig = webrtc::PeerConnectionInterface::RTCConfiguration;

		rtc::scoped_refptr<webrtc::PeerConnectionInterface> PeerConnection;

		TUniquePtr<rtc::Thread>                SignalingThread;
		TUniquePtr<rtc::Thread>                WorkingThread;
		TUniquePtr<rtc::Thread>                NetworkingThread;
		rtc::scoped_refptr<FAudioDeviceModule> AudioDeviceModule;
		std::unique_ptr<webrtc::TaskQueueFactory> TaskQueueFactory;

		using FCreateSessionDescriptionObserver = TSessionDescriptionObserver<webrtc::CreateSessionDescriptionObserver>;
		using FSetSessionDescriptionObserver = TSessionDescriptionObserver<webrtc::SetSessionDescriptionObserver>;

		TUniquePtr<FCreateSessionDescriptionObserver> CreateSessionDescription;
		TUniquePtr<FSetSessionDescriptionObserver>    LocalSessionDescription;
		TUniquePtr<FSetSessionDescriptionObserver>    RemoteSessionDescription;

		bool bUseFrameTransformer{ false };

#if ENGINE_MAJOR_VERSION >= 5 && ENGINE_MINOR_VERSION > 0
		TUniquePtr<FPlayerStatsCollector>             RTCStatsCollector;
#endif

		template<typename Callback>
		webrtc::SessionDescriptionInterface* CreateDescription(const std::string&,
			const std::string&,
			Callback&&);

		rtc::scoped_refptr<webrtc::PeerConnectionFactoryInterface> PeerConnectionFactory;
		void CreatePeerConnectionFactory();

		void Renegociate(const webrtc::SessionDescriptionInterface* local_sdp,
			const webrtc::SessionDescriptionInterface* remote_sdp);

		static TAtomic<int> RefCounter; // Number of Peerconnection factory created

	public:
		FString ClusterId;
		FString ServerId;

		std::function<void(const std::string& mid, rtc::scoped_refptr<webrtc::MediaStreamTrackInterface>)> OnVideoTrack = nullptr;
		std::function<void(const std::string& mid, rtc::scoped_refptr<webrtc::MediaStreamTrackInterface>)> OnAudioTrack = nullptr;
		std::function<void(uint32 Ssrc, uint32 Timestamp, const TArray<uint8>& Data)> OnFrameMetadata = nullptr;

		webrtc::PeerConnectionInterface::RTCOfferAnswerOptions OaOptions;
		
		~FWebRTCPeerConnection() noexcept;
		void Init(const FRTCConfig& Config);

#if ENGINE_MAJOR_VERSION >= 5 && ENGINE_MINOR_VERSION > 0
		FPlayerStatsCollector* GetStatsCollector();
#endif
		
		static FRTCConfig GetDefaultConfig();
		static FWebRTCPeerConnection* Create(const FRTCConfig& Config);

		FSetSessionDescriptionObserver* GetLocalDescriptionObserver();
		FSetSessionDescriptionObserver* GetRemoteDescriptionObserver();
		FCreateSessionDescriptionObserver* GetCreateDescriptionObserver();

		const FSetSessionDescriptionObserver* GetLocalDescriptionObserver()     const;
		const FSetSessionDescriptionObserver* GetRemoteDescriptionObserver()    const;
		const FCreateSessionDescriptionObserver* GetCreateDescriptionObserver() const;

		void CreateOffer();
		void SetLocalDescription(const std::string& Sdp, const std::string& Type);
		void SetRemoteDescription(const std::string& Sdp, const std::string& Type = std::string("answer"));

		// PeerConnection Observer interface
		void OnSignalingChange(webrtc::PeerConnectionInterface::SignalingState new_state) override;
		void OnAddStream(rtc::scoped_refptr<webrtc::MediaStreamInterface> stream) override;
		void OnRemoveStream(rtc::scoped_refptr<webrtc::MediaStreamInterface> stream) override;
		void OnAddTrack(rtc::scoped_refptr<webrtc::RtpReceiverInterface> receiver,
			const FMediaStreamVector& streams) override;
		void OnTrack(rtc::scoped_refptr<webrtc::RtpTransceiverInterface> transceiver) override;
		void OnRemoveTrack(rtc::scoped_refptr<webrtc::RtpReceiverInterface> receiver) override;
		void OnDataChannel(rtc::scoped_refptr<webrtc::DataChannelInterface> channel)  override;
		void OnRenegotiationNeeded() override;
		void OnIceConnectionChange(webrtc::PeerConnectionInterface::IceConnectionState new_state) override;
		void OnIceGatheringChange(webrtc::PeerConnectionInterface::IceGatheringState new_state) override;
		void OnIceCandidate(const webrtc::IceCandidateInterface* candidate) override;
		void OnIceConnectionReceivingChange(bool receiving) override;

		void EnableFrameTransformer(bool Enable);

#if ENGINE_MAJOR_VERSION >= 5 && ENGINE_MINOR_VERSION > 0
		void EnableStats(bool Enable);
		void PollStats();
#endif

		webrtc::PeerConnectionInterface* operator->()
		{
			return PeerConnection.get();
		}
	};
}
