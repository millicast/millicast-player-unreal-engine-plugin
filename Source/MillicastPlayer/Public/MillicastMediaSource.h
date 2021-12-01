// Copyright CoSMoSoftware 2021. All Rights Reserved.
#pragma once

#include "UObject/ObjectMacros.h"
#include "StreamMediaSource.h"
#include "MillicastMediaTexture2D.h"
#include "MillicastSignalingData.h"

#include "api/media_stream_interface.h"

#include "MillicastMediaSource.generated.h"


/**
 * Media source description for Millicast Player.
 */
UCLASS(BlueprintType, hideCategories=(Platforms,Object),
       META = (DisplayName = "Millicast Media Source"))
class MILLICASTPLAYER_API UMillicastMediaSource : public UStreamMediaSource,
                                                  public rtc::VideoSinkInterface<webrtc::VideoFrame>
{
        GENERATED_BODY()

public:

	/** Default constructor. */
	UMillicastMediaSource();

public:

	/** The Millicast Stream name. */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category=Stream, AssetRegistrySearchable)
	FString StreamName;

	/** The Millicast account id. */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category=Stream, AssetRegistrySearchable)
	FString AccountId;

	/**
		Provides an Millicast Video Texture object to render videos frames.
	*/
	UPROPERTY(BlueprintReadWrite, EditAnywhere, BlueprintSetter = "ChangeVideoTexture", Category = "Content",
			  META = (DisplayName = "Video Texture", AllowPrivateAccess = true))
	UMillicastMediaTexture2D* VideoTexture = nullptr;

	/**
		Attempts to change the Video Texture object used as the video frame capture object
	*/
	UFUNCTION(BlueprintSetter)
	void ChangeVideoTexture(UMillicastMediaTexture2D* InVideoTexture = nullptr);

public:
	//~ IMediaOptions interface

	FString GetMediaOption(const FName& Key, const FString& DefaultValue) const override;
	bool HasMediaOption(const FName& Key) const override;


public:
	//~ UMediaSource interface

	FString GetUrl() const override;
	bool Validate() const override;

public:
	/**
	   Called before destroying the object.  This is called immediately upon deciding to destroy the object,
	   to allow the object to begin an asynchronous cleanup process.
	 */
	void BeginDestroy() override;

	/**
	    Create websocket connection and attempts to subscribe to the feed
	*/
	bool Initialize(const FMillicastSignalingData& data);

	/**
		Updates the DynamicMaterial with the VideoTexture of this object
	*/
	void UpdateMaterialTexture(class UMaterialInstanceDynamic* MaterialInstance, FString ParameterName);

public:
	//~ UObject interface
#if WITH_EDITOR
	virtual bool CanEditChange(const FProperty* InProperty) const override;
	virtual void PostEditChangeChainProperty(struct FPropertyChangedChainEvent& InPropertyChangedEvent) override;
#endif //WITH_EDITOR
	//~ End UObject interface

protected:
	//~ VideoSink interface
	void OnFrame(const webrtc::VideoFrame& frame) override;

private:
	uint8_t * Buffer;
	size_t    BufferSize;

	FCriticalSection RenderSyncContext;
	FTexture2DRHIRef SourceTexture;
	FPooledRenderTargetDesc RenderTargetDescriptor;
	TRefCountPtr<IPooledRenderTarget> RenderTarget;
};
