#pragma once

#include "RHIResources.h"

namespace NMillicastMedia
{
	static constexpr ETextureCreateFlags TextureCreateFlags = TexCreate_SRGB | TexCreate_Dynamic; 
	
	void CreateTexture( FTexture2DRHIRef& Target, int32 Width, int32 Height);
}
