// Copyright Millicast 2022. All Rights Reserved.

#pragma once

#include "IMillicastVideoConsumer.h"
#include "MillicastMediaTexture2D.h"
#include "RendererInterface.h"
#include "Engine/DataAsset.h"
#include "MillicastTexture2DPlayer.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FMillicastVideoResolutionChangedPlayer, int32, Width, int32, Height);

UCLASS(BlueprintType, editinlinenew, hideCategories = (Object), META = (DisplayName = "Millicast Texture2D Player"))
class MILLICASTPLAYER_API UMillicastTexture2DPlayer : public UDataAsset , public IMillicastVideoConsumer
{
    GENERATED_BODY()
	
public:
	/**
		Provides an Millicast Video Texture object to render videos frames.
	*/
	UPROPERTY(BlueprintReadWrite, EditAnywhere, BlueprintSetter = "ChangeVideoTexture", Category = "Content",
		META = (DisplayName = "Video Texture"))
	UMillicastMediaTexture2D* VideoTexture = nullptr;

	/**
		Attempts to change the Video Texture object used as the video frame capture object
	*/
	UFUNCTION(BlueprintSetter)
	void ChangeVideoTexture(UMillicastMediaTexture2D* InVideoTexture = nullptr);

	void BeginDestroy() override;
	void OnFrame(TArray<uint8>& VideoData, int Width, int Height) override;

	UPROPERTY(BlueprintAssignable, Category="MillicastPlayer")
	FMillicastVideoResolutionChangedPlayer OnVideoResolutionChanged;
	
private:
	FCriticalSection RenderSyncContext;
	FTexture2DRHIRef SourceTexture;
	FPooledRenderTargetDesc RenderTargetDescriptor;
	TRefCountPtr<IPooledRenderTarget> RenderTarget;

	FIntPoint CachedResolution;
};