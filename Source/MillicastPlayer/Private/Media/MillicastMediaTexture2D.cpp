// Copyright CoSMoSoftware 2021. All Rights Reserved.

#include "MillicastMediaTexture2D.h"
#include "MillicastMediaTextureResource.h"
#include "MillicastMediaUtil.h"
#include "MillicastPlayerPrivate.h"

UMillicastMediaTexture2D::UMillicastMediaTexture2D(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	SetResource(nullptr);
}

void UMillicastMediaTexture2D::UpdateTextureReference(FRHICommandList& RHICmdList, FTexture2DRHIRef Reference)
{
	auto* CurrentResource = GetResource();
	if (!CurrentResource)
	{
		return;
	}

	if (!Reference.IsValid())
	{
		CreateRenderableTexture(static_cast<FMillicastMediaTextureResource*>(CurrentResource));
		return;
	}

	if( CurrentResource->TextureRHI != Reference)
	{
		CurrentResource->TextureRHI = (FTexture2DRHIRef&)Reference;
		RHIUpdateTextureReference(TextureReference.TextureReferenceRHI, CurrentResource->TextureRHI);
	}
}

void UMillicastMediaTexture2D::CreateRenderableTexture(FMillicastMediaTextureResource* TextureResource)
{
	if( !TextureResource )
	{
		return;
	}

	ENQUEUE_RENDER_COMMAND(FMillicastMediaTexture2DUpdateTextureReference)
	([this, TextureResource](FRHICommandListImmediate& RHICmdList)
	{
		TRefCountPtr<FRHITexture2D> RenderableTexture;
		NMillicastMedia::CreateTexture(RenderableTexture, 1920, 1080);
		TextureResource->TextureRHI = (FTextureRHIRef&)RenderableTexture;
		RHIUpdateTextureReference(TextureReference.TextureReferenceRHI, GetResource()->TextureRHI);
	});

	// Make sure _RenderThread is executed before continuing
	FlushRenderingCommands();
}

FTextureResource* UMillicastMediaTexture2D::CreateResource()
{
	UE_LOG(LogMillicastPlayer, Verbose, TEXT("%S"), __FUNCTION__);

	{
		auto CurrentResource = GetResource();
		if (CurrentResource)
		{
			delete CurrentResource;
			SetResource(nullptr);
		}
	}

	if (FMillicastMediaTextureResource* TextureResource = new FMillicastMediaTextureResource(this))
	{
		SetResource(TextureResource);
		CreateRenderableTexture(TextureResource);
	}

	return GetResource();
}

void UMillicastMediaTexture2D::GetResourceSizeEx(FResourceSizeEx& CumulativeResourceSize)
{
	Super::GetResourceSizeEx(CumulativeResourceSize);

	if (FMillicastMediaTextureResource* CurrentResource = static_cast<FMillicastMediaTextureResource*>(GetResource()))
	{
		CumulativeResourceSize.AddUnknownMemoryBytes(CurrentResource->GetResourceSize());
	}
}

float UMillicastMediaTexture2D::GetSurfaceHeight() const
{
	return GetResource() != nullptr ? GetResource()->GetSizeY() : 0.0f;
}

float UMillicastMediaTexture2D::GetSurfaceWidth() const
{
	return GetResource() != nullptr ? GetResource()->GetSizeX() : 0.0f;
}

#if ENGINE_MAJOR_VERSION >= 5
float UMillicastMediaTexture2D::GetSurfaceDepth() const
{
	return GetResource() != nullptr ? GetResource()->GetSizeZ() : 0.0f;
}

#if ENGINE_MINOR_VERSION > 0

ETextureClass UMillicastMediaTexture2D::GetTextureClass() const
{
	return ETextureClass::RenderTarget;
}

#endif
#endif

EMaterialValueType UMillicastMediaTexture2D::GetMaterialType() const
{
	return MCT_Texture2D;
}
