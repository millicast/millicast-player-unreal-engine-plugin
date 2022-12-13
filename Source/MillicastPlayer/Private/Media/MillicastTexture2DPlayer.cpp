// Copyright Millciast 2022. All Rights Reserved.

#include "MillicastTexture2DPlayer.h"
#include "MillicastPlayerPrivate.h"

#include <RenderTargetPool.h>

UMillicastTexture2DPlayer::UMillicastTexture2DPlayer(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{}

void UMillicastTexture2DPlayer::OnFrame(TArray<uint8>& VideoData, int Width, int Height)
{
	constexpr auto TEXTURE_PIXEL_FORMAT = PF_B8G8R8A8;

	AsyncTask(ENamedThreads::ActualRenderingThread, [=]() {
		FScopeLock Lock(&RenderSyncContext);

		FIntPoint FrameSize = FIntPoint(Width, Height);
		FRHICommandListImmediate& RHICmdList = FRHICommandListExecutor::GetImmediateCommandList();

		if (!RenderTargetDescriptor.IsValid() ||
			RenderTargetDescriptor.GetSize() != FIntVector(FrameSize.X, FrameSize.Y, 0))
		{
			// Create the RenderTarget descriptor
			RenderTargetDescriptor = FPooledRenderTargetDesc::Create2DDesc(FrameSize,
				TEXTURE_PIXEL_FORMAT,
				FClearValueBinding::None,
				TexCreate_None,
				TexCreate_RenderTargetable,
				false);

			// Update the shader resource for the 'SourceTexture'
			FRHITextureCreateDesc CreateDesc = FRHITextureCreateDesc::Create2D(TEXT("MillicastTexture2d"),
				FrameSize.X, FrameSize.Y, EPixelFormat::PF_B8G8R8A8);

			CreateDesc.SetFlags(TexCreate_SRGB | TexCreate_Dynamic);

			SourceTexture = RHICreateTexture(CreateDesc);

			// Find a free target-able texture from the render pool
			GRenderTargetPool.FindFreeElement(RHICmdList,
				RenderTargetDescriptor,
				RenderTarget,
				TEXT("MILLICASTPLAYER"));
		}

		// Create the update region structure
		FUpdateTextureRegion2D Region(0, 0, 0, 0, FrameSize.X, FrameSize.Y);

		// Set the Pixel data of the webrtc Frame to the SourceTexture
		RHIUpdateTexture2D(SourceTexture, 0, Region, Width * 4, VideoData.GetData());

		VideoTexture->UpdateTextureReference(RHICmdList, (FTexture2DRHIRef&)SourceTexture);
		});
}

void UMillicastTexture2DPlayer::BeginDestroy()
{
	AsyncTask(ENamedThreads::ActualRenderingThread, [this]() {
		FScopeLock Lock(&RenderSyncContext);
		RenderTarget = nullptr;
		});

	Super::BeginDestroy();
}

void UMillicastTexture2DPlayer::ChangeVideoTexture(UMillicastMediaTexture2D* InVideoTexture)
{
	UE_LOG(LogMillicastPlayer, Verbose, TEXT("%S"), __FUNCTION__);
	FScopeLock Lock(&RenderSyncContext);

	if (IsValid(VideoTexture))
	{
		VideoTexture->UpdateTextureReference(FRHICommandListExecutor::GetImmediateCommandList(), nullptr);
	}

	VideoTexture = InVideoTexture;
}