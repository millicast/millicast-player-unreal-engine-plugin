// Copyright CoSMoSoftware 2021. All Rights Reserved.

#include "MillicastMediaTexture2D.h"
#include "MillicastMediaTextureResource.h"

constexpr int32 DEFAULT_WIDTH = 1920;
constexpr int32 DEFAULT_HEIGHT = 1080;

UMillicastMediaTexture2D::UMillicastMediaTexture2D(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	SetResource(nullptr);
}

void UMillicastMediaTexture2D::UpdateTextureReference(FRHICommandList& RHICmdList, FTexture2DRHIRef Reference)
{
	auto resource = GetResource();
	if (resource != nullptr)
	{
		if (Reference.IsValid() && resource->TextureRHI != Reference)
		{
			resource->TextureRHI = (FTexture2DRHIRef&)Reference;
			RHIUpdateTextureReference(TextureReference.TextureReferenceRHI, resource->TextureRHI);
		}
		else if (!Reference.IsValid())
		{
			if (FMillicastMediaTextureResource* TextureResource = static_cast<FMillicastMediaTextureResource*>(resource))
			{
				TRefCountPtr<FRHITexture2D> RenderableTexture;

				FRHITextureCreateDesc CreateDesc = FRHITextureCreateDesc::Create2D(TEXT("MillicastTexture2d"), 
					DEFAULT_WIDTH, DEFAULT_HEIGHT, EPixelFormat::PF_B8G8R8A8);

				CreateDesc.SetFlags(TexCreate_SRGB | TexCreate_Dynamic);

				RenderableTexture = RHICreateTexture(CreateDesc);

				TextureResource->TextureRHI = (FTextureRHIRef&)RenderableTexture;

				ENQUEUE_RENDER_COMMAND(FMillicastMediaTexture2DUpdateTextureReference)
				([this](FRHICommandListImmediate& RHICmdList) {
					RHIUpdateTextureReference(TextureReference.TextureReferenceRHI, GetResource()->TextureRHI);
				});

				// Make sure _RenderThread is executed before continuing
				FlushRenderingCommands();
			}
		}
	}
}

FTextureResource* UMillicastMediaTexture2D::CreateResource()
{
	auto resource = GetResource();
	if (resource != nullptr)
	{
		delete resource;
		SetResource(nullptr);
	}

	if (FMillicastMediaTextureResource* TextureResource = new FMillicastMediaTextureResource(this))
	{
		SetResource(TextureResource);

		// Set the default video texture to reference nothing
		TRefCountPtr<FRHITexture2D> RenderableTexture;
		FRHITextureCreateDesc CreateDesc = FRHITextureCreateDesc::Create2D(TEXT("MillicastTexture2d"),
			DEFAULT_WIDTH, DEFAULT_HEIGHT, EPixelFormat::PF_B8G8R8A8);

		RenderableTexture = RHICreateTexture(CreateDesc);
		CreateDesc.SetFlags(TexCreate_SRGB | TexCreate_Dynamic);

		TextureResource->TextureRHI = (FTextureRHIRef&)RenderableTexture;

		ENQUEUE_RENDER_COMMAND(FMillicastMediaTexture2DUpdateTextureReference)
		([this](FRHICommandListImmediate& RHICmdList) {
			RHIUpdateTextureReference(TextureReference.TextureReferenceRHI, GetResource()->TextureRHI);
		});
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

float UMillicastMediaTexture2D::GetSurfaceDepth() const
{
	return GetResource() != nullptr ? GetResource()->GetSizeZ() : 0.0f;
}

EMaterialValueType UMillicastMediaTexture2D::GetMaterialType() const
{
	return MCT_Texture2D;
}

ETextureClass UMillicastMediaTexture2D::GetTextureClass() const
{
	return ETextureClass::RenderTarget;
}
