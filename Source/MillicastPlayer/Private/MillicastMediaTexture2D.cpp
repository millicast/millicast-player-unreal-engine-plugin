/*
	All rights reserved. Copyright(c) 2018-2021, NewTek Inc.

	This file and it's use within a Product is bound by the terms of NDI SDK license that was provided
	as part of the NDI SDK. For more information, please review the license and the NDI SDK documentation.
*/

#include "MillicastMediaTexture2D.h"
#include "MillicastMediaTextureResource.h"

UMillicastMediaTexture2D::UMillicastMediaTexture2D(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{

	this->Resource = nullptr;
}

void UMillicastMediaTexture2D::UpdateTextureReference(FRHICommandList& RHICmdList, FTexture2DRHIRef Reference)
{
	if (Resource != nullptr)
	{
		static int32 DefaultWidth = 1280;
		static int32 DefaultHeight = 720;

		if (Reference.IsValid() && Resource->TextureRHI != Reference)
		{
			Resource->TextureRHI = (FTexture2DRHIRef&)Reference;
			RHIUpdateTextureReference(TextureReference.TextureReferenceRHI, Resource->TextureRHI);
		}
		else if (!Reference.IsValid())
		{
			if (FMillicastMediaTextureResource* TextureResource = static_cast<FMillicastMediaTextureResource*>(this->Resource))
			{
				// Set the default video texture to reference nothing
				TRefCountPtr<FRHITexture2D> ShaderTexture2D;
				TRefCountPtr<FRHITexture2D> RenderableTexture;
				FRHIResourceCreateInfo CreateInfo = {FClearValueBinding(FLinearColor(0.0f, 0.0f, 0.0f))};

				RHICreateTargetableShaderResource2D(DefaultWidth, DefaultHeight, EPixelFormat::PF_B8G8R8A8, 1,
													TexCreate_Dynamic, TexCreate_RenderTargetable, false, CreateInfo,
													RenderableTexture, ShaderTexture2D);

				TextureResource->TextureRHI = (FTextureRHIRef&)RenderableTexture;

				ENQUEUE_RENDER_COMMAND(FMillicastMediaTexture2DUpdateTextureReference)
				([this](FRHICommandListImmediate& RHICmdList) {
					RHIUpdateTextureReference(TextureReference.TextureReferenceRHI, Resource->TextureRHI);
				});

				// Make sure _RenderThread is executed before continuing
				FlushRenderingCommands();
			}
		}
	}
}

FTextureResource* UMillicastMediaTexture2D::CreateResource()
{
	if (this->Resource != nullptr)
	{
		delete Resource;
		Resource = nullptr;
	}

	if (FMillicastMediaTextureResource* TextureResource = new FMillicastMediaTextureResource(this))
	{
		this->Resource = TextureResource;

		static int32 DefaultWidth = 1280;
		static int32 DefaultHeight = 720;

		// Set the default video texture to reference nothing
		TRefCountPtr<FRHITexture2D> ShaderTexture2D;
		TRefCountPtr<FRHITexture2D> RenderableTexture;
		FRHIResourceCreateInfo CreateInfo = {FClearValueBinding(FLinearColor(0.0f, 0.0f, 0.0f))};

		RHICreateTargetableShaderResource2D(DefaultWidth, DefaultHeight, EPixelFormat::PF_B8G8R8A8, 1,
											TexCreate_Dynamic, TexCreate_RenderTargetable, false, CreateInfo,
											RenderableTexture, ShaderTexture2D);

		Resource->TextureRHI = (FTextureRHIRef&)RenderableTexture;

		ENQUEUE_RENDER_COMMAND(FMillicastMediaTexture2DUpdateTextureReference)
		([this](FRHICommandListImmediate& RHICmdList) {
			RHIUpdateTextureReference(TextureReference.TextureReferenceRHI, Resource->TextureRHI);
		});
	}

	return this->Resource;
}

void UMillicastMediaTexture2D::GetResourceSizeEx(FResourceSizeEx& CumulativeResourceSize)
{
	Super::GetResourceSizeEx(CumulativeResourceSize);

	if (FMillicastMediaTextureResource* CurrentResource = static_cast<FMillicastMediaTextureResource*>(this->Resource))
	{
		CumulativeResourceSize.AddUnknownMemoryBytes(CurrentResource->GetResourceSize());
	}
}

float UMillicastMediaTexture2D::GetSurfaceHeight() const
{
	return Resource != nullptr ? Resource->GetSizeY() : 0.0f;
}

float UMillicastMediaTexture2D::GetSurfaceWidth() const
{
	return Resource != nullptr ? Resource->GetSizeX() : 0.0f;
}

EMaterialValueType UMillicastMediaTexture2D::GetMaterialType() const
{
	return MCT_Texture2D;
}
