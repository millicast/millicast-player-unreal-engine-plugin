// Copyright CoSMoSoftware 2021. All Rights Reserved.

#pragma once

#include <CoreMinimal.h>
#include <ExternalTexture.h>
#include <TextureResource.h>

/**
	A Texture Resource object used by the MillicastMediaTexture2D object
*/
class MILLICASTPLAYER_API FMillicastMediaTextureResource : public FTextureResource
{
public:
	/**
		Constructs a new instance of this object specifying a media texture owner

		@param Owner The media object used as the owner for this object
	*/
	FMillicastMediaTextureResource(class UMillicastMediaTexture2D* Owner = nullptr);

	/** FTextureResource Interface Implementation for 'InitDynamicRHI' */
	virtual void InitDynamicRHI() override;

	/** FTextureResource Interface Implementation for 'ReleaseDynamicRHI' */
	virtual void ReleaseDynamicRHI() override;

	/** FTextureResource Interface Implementation for 'GetResourceSize' */
	SIZE_T GetResourceSize();

	/** FTextureResource Interface Implementation for 'GetSizeX' */
	virtual uint32 GetSizeX() const override;

	/** FTextureResource Interface Implementation for 'GetSizeY' */
	virtual uint32 GetSizeY() const override;

private:
	UMillicastMediaTexture2D* MediaTexture = nullptr;
};
