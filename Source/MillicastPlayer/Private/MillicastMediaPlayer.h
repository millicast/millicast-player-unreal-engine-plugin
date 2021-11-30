// Copyright CoSMoSOftware 2021. All Rights Reserved.

#pragma once

#include "MediaIOCorePlayerBase.h"
#include "api/media_stream_interface.h"

#include "MediaIOCoreAudioSampleBase.h"
#include "MediaIOCoreTextureSampleBase.h"
#include "MediaObjectPool.h"
#include "MediaShaders.h"

class IMediaEventSink;
class IWebSocket;

namespace millicast
{
class PeerConnection;
}

enum class EMediaTextureSampleFormat;
enum class EMediaIOSampleType;

class FMillicastMediaTextureSample : public FMediaIOCoreTextureSampleBase
{
        virtual const FMatrix& GetYUVToRGBMatrix() const override { return MediaShaders::YuvToRgbRec709Full; }
};

class FMillicastMediaAudioSamplePool : public TMediaObjectPool<FMediaIOCoreAudioSampleBase> { };
class FMillicastMediaTextureSamplePool : public TMediaObjectPool<FMillicastMediaTextureSample> { };

/**
 * Implements a media player for Millicast
 */
class FMillicastMediaPlayer : public FMediaIOCorePlayerBase, public rtc::VideoSinkInterface<webrtc::VideoFrame>
{
private:
        using Super = FMediaIOCorePlayerBase;

public:

	/**
	 * Create and initialize a new instance.
	 *
	 * @param InEventSink The object that receives media events from this player.
	 */
	FMillicastMediaPlayer(IMediaEventSink& InEventSink);

	/** Virtual destructor. */
	virtual ~FMillicastMediaPlayer();

public:

	//~ IMediaPlayer interface

	virtual void Close() override;
	virtual FGuid GetPlayerPluginGUID() const override;

	virtual bool Open(const FString& Url, const IMediaOptions* Options) override;

	virtual void TickInput(FTimespan DeltaTime, FTimespan Timecode) override;
	virtual void TickFetch(FTimespan DeltaTime, FTimespan Timecode) override;

	//~ ITimedDataInput interface
#if WITH_EDITOR
	virtual const FSlateBrush* GetDisplayIcon() const override;
#endif

protected:
  //~ VideoSink interface
  void OnFrame(const webrtc::VideoFrame& frame) override;

public:
	/** Is Hardware initialized */
	virtual bool IsHardwareReady() const override;

protected:
	/** Setup our different channels with the current set of settings */
	virtual void SetupSampleChannels() override;

private:
	/** Director api */
	bool AuthenticateToMillicast();
	/** Websocket Connection */
	bool StartWebSocketConnection(const FString& url, const FString& jwt);
	/** Create the peerconnection and starts subscribing*/
	bool SubscribeToMillicast();

	/** Audio, MetaData, Texture  sample object pool. */
	// FMillicastMediaAudioSamplePool* AudioSamplePool;
	FMillicastMediaTextureSamplePool* TextureSamplePool;

	/** WebSocket Connection */
	TSharedPtr<IWebSocket> WS;
	FDelegateHandle OnConnectedHandle;
	FDelegateHandle OnConnectionErrorHandle;
	FDelegateHandle OnClosedHandle;
	FDelegateHandle OnMessageHandle;

	TUniquePtr<millicast::PeerConnection> PeerConnection;

	uint8_t * Buffer;
	size_t BufferSize;

	void OnConnected();
	void OnConnectionError(const FString& Error);
	void OnClosed(int32 StatusCode, const FString& Reason, bool bWasClean);
	void OnMessage(const FString& Msg);

	/** Used to flag which sample types we advertise as supported for timed data monitoring */
	EMediaIOSampleType SupportedSampleTypes;
};
