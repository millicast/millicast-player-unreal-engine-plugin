#include "MillicastMediaUtil.h"

#include "Runtime/Launch/Resources/Version.h"

void NMillicastMedia::CreateTexture( FTexture2DRHIRef& TargetRef, int32 Width, int32 Height)
{
#if ENGINE_MAJOR_VERSION >= 5 && ENGINE_MINOR_VERSION > 0
	FRHITextureCreateDesc CreateDesc = FRHITextureCreateDesc::Create2D(TEXT("MillicastTexture2d"), Width, Height, EPixelFormat::PF_B8G8R8A8);

	CreateDesc.SetFlags(NMillicastMedia::TextureCreateFlags);
	TargetRef = RHICreateTexture(CreateDesc);
#else
	TRefCountPtr<FRHITexture2D> ShaderTexture2D;
	FRHIResourceCreateInfo CreateInfo = {
	#if ENGINE_MAJOR_VERSION >= 5
		TEXT("ResourceCreateInfo"), 
	#endif
		FClearValueBinding(FLinearColor(0.0f, 0.0f, 0.0f))
	};

	RHICreateTargetableShaderResource2D(Width, Height, EPixelFormat::PF_B8G8R8A8, 1,
										NMillicastMedia::TextureCreateFlags, TexCreate_RenderTargetable, false, CreateInfo,
										TargetRef, ShaderTexture2D);
#endif
}
