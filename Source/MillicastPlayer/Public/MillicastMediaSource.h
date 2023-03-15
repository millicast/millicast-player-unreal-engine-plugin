// Copyright CoSMoSoftware 2021. All Rights Reserved.
#pragma once

#include "UObject/ObjectMacros.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "StreamMediaSource.h"
#include "MillicastMediaTexture2D.h"
#include "MillicastSignalingData.h"
#include "RendererInterface.h"

#include "api/media_stream_interface.h"

#include "MillicastMediaSource.generated.h"


/**
 * Media source description for Millicast Player.
 */
UCLASS(BlueprintType, hideCategories=(Platforms,Object),
       META = (DisplayName = "Millicast Media Source"))
class MILLICASTPLAYER_API UMillicastMediaSource : public UStreamMediaSource
{
	GENERATED_BODY()
public:

	UMillicastMediaSource();

	/** The Millicast Stream name. */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category=Stream, AssetRegistrySearchable)
	FString StreamName;

	/** The Millicast account id. */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category=Stream, AssetRegistrySearchable)
	FString AccountId;

	/** Whether to use the subscribe token or not. */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Stream, AssetRegistrySearchable)
	bool bUseSubscribeToken = false;

	/** Subscribe token (optional). */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Stream, AssetRegistrySearchable)
	FString SubscribeToken;

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
public:
	//~ UObject interface
#if WITH_EDITOR
	virtual bool CanEditChange(const FProperty* InProperty) const override;
	virtual void PostEditChangeChainProperty(struct FPropertyChangedChainEvent& InPropertyChangedEvent) override;
#endif //WITH_EDITOR
	//~ End UObject interface

private:
	uint8_t * Buffer;
	size_t    BufferSize;

	FCriticalSection RenderSyncContext;
	FTexture2DRHIRef SourceTexture;
	FPooledRenderTargetDesc RenderTargetDescriptor;
	TRefCountPtr<IPooledRenderTarget> RenderTarget;
};
