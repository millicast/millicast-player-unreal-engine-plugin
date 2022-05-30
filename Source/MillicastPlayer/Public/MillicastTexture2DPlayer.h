// Copyright Millicast 2022. All Rights Reserved.

#pragma once


#include <Engine/DataAsset.h>

#include "IMillicastVideoConsumer.h"
#include "MillicastMediaTexture2D.h"

#include "MillicastTexture2DPlayer.generated.h"

UCLASS(BlueprintType, editinlinenew, hideCategories = (Object),
    META = (DisplayName = "Millicast Texture2D Player"))
class MILLICASTPLAYER_API UMillicastTexture2DPlayer : public UDataAsset , public IMillicastVideoConsumer
{
    GENERATED_UCLASS_BODY()
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

public:
	
	void OnFrame(TArray<uint8>& VideoData, int Width, int Height) override;

public:

	void BeginDestroy() override;

private:
	FCriticalSection RenderSyncContext;
	FTexture2DRHIRef SourceTexture;
	FPooledRenderTargetDesc RenderTargetDescriptor;
	TRefCountPtr<IPooledRenderTarget> RenderTarget;
};