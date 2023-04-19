// Copyright CoSMoSoftware 2021. All Rights Reserved.

#pragma once

#include "Engine/Texture.h"

#include "Runtime/Launch/Resources/Version.h"

#include "MillicastMediaTexture2D.generated.h"

class FMillicastMediaTextureResource;

/**
	A Texture Object to render a webrtc frame
*/
UCLASS(NotBlueprintType, NotBlueprintable, HideDropdown,
	   HideCategories = (ImportSettings, Compression, Texture, Adjustments, Compositing, LevelOfDetail, Object),
	   META = (DisplayName = "Millicast Media Texture 2D"))
class MILLICASTPLAYER_API UMillicastMediaTexture2D : public UTexture
{
	GENERATED_UCLASS_BODY()

public:
	virtual float GetSurfaceHeight() const override;
	virtual float GetSurfaceWidth() const override;

#if ENGINE_MAJOR_VERSION >= 5
	virtual float GetSurfaceDepth() const override;
	virtual uint32 GetSurfaceArraySize() const override { return 0; }

#if ENGINE_MINOR_VERSION > 0
	ETextureClass GetTextureClass() const override;
#endif

#endif
	
	virtual void GetResourceSizeEx(FResourceSizeEx& CumulativeResourceSize) override;
	virtual EMaterialValueType GetMaterialType() const override;

	virtual void UpdateTextureReference(FRHICommandList& RHICmdList, FTexture2DRHIRef Reference) final;

private:
	virtual class FTextureResource* CreateResource() override;
	void CreateRenderableTexture(FMillicastMediaTextureResource* TextureResource);
};
