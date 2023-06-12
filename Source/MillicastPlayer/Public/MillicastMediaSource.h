// Copyright CoSMoSoftware 2021. All Rights Reserved.
#pragma once

#include "RendererInterface.h"
#include "StreamMediaSource.h"
#include "MillicastMediaSource.generated.h"

/**
 * Media source description for Millicast Player.
 */
UCLASS(BlueprintType, hideCategories=(Platforms,Object), META = (DisplayName = "Millicast Media Source"))
class MILLICASTPLAYER_API UMillicastMediaSource : public UStreamMediaSource
{
	GENERATED_BODY()

public:
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
	UMillicastMediaSource();

	//~ IMediaOptions interface
	FString GetMediaOption(const FName& Key, const FString& DefaultValue) const override;
	bool HasMediaOption(const FName& Key) const override;
	//~ UMediaSource interface

	FString GetUrl() const override { return StreamUrl; }
	bool Validate() const override;
	
	//~ UObject interface
#if WITH_EDITOR
	virtual bool CanEditChange(const FProperty* InProperty) const override;
#endif //WITH_EDITOR
	//~ End UObject interface

private:
	FCriticalSection RenderSyncContext;
	FTexture2DRHIRef SourceTexture;
	FPooledRenderTargetDesc RenderTargetDescriptor;
};
