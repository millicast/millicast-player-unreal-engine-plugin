// Copyright CoSMoSoftware 2021. All Rights Reserved.

#include "MillicastMediaTextureResource.h"
#include "MillicastMediaTexture2D.h"
#include "RenderUtils.h"
#include "RHI.h"
#include "DeviceProfiles/DeviceProfile.h"
#include "DeviceProfiles/DeviceProfileManager.h"

FMillicastMediaTextureResource::FMillicastMediaTextureResource(UMillicastMediaTexture2D* Owner)
{
	MediaTexture = Owner;
}

#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 3
void FMillicastMediaTextureResource::InitRHI(FRHICommandListBase& Rhi)
#else
void FMillicastMediaTextureResource::InitDynamicRHI()
#endif
{
	if (MediaTexture != nullptr)
	{
		FSamplerStateInitializerRHI SamplerStateInitializer(
			(ESamplerFilter)UDeviceProfileManager::Get().GetActiveProfile()->GetTextureLODSettings()->GetSamplerFilter(
				MediaTexture),
			AM_Border, AM_Border, AM_Wrap);

		SamplerStateRHI = RHICreateSamplerState(SamplerStateInitializer);
	}
}

#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 3
void FMillicastMediaTextureResource::ReleaseRHI()
#else
void FMillicastMediaTextureResource::ReleaseDynamicRHI()
#endif
{
	TextureRHI.SafeRelease();

	if (MediaTexture != nullptr)
	{
		RHIUpdateTextureReference(MediaTexture->TextureReference.TextureReferenceRHI, nullptr);
	}
}

SIZE_T FMillicastMediaTextureResource::GetResourceSize()
{
	return CalcTextureSize(GetSizeX(), GetSizeY(), EPixelFormat::PF_A8R8G8B8, 1);
}

uint32 FMillicastMediaTextureResource::GetSizeX() const
{
	return TextureRHI.IsValid() ? TextureRHI->GetSizeXYZ().X : 0;
}

uint32 FMillicastMediaTextureResource::GetSizeY() const
{
	return TextureRHI.IsValid() ? TextureRHI->GetSizeXYZ().Y : 0;
}
