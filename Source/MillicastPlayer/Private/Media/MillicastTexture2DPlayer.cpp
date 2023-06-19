// Copyright Millicast 2022. All Rights Reserved.

#include "MillicastTexture2DPlayer.h"
#include "MillicastMediaUtil.h"
#include "MillicastPlayerPrivate.h"
#include "RenderTargetPool.h"
#include "Async/Async.h"
#include "Util.h"

void UMillicastTexture2DPlayer::OnFrame(TArray<uint8>& VideoData, int Width, int Height)
{
	if(CachedResolution.X != Width || CachedResolution.Y != Height)
	{
		CachedResolution.X = Width;
		CachedResolution.Y = Height;
		
#if MILLICAST_HAS_CXX20
		AsyncTask(ENamedThreads::GameThread, [=, this]()
#else
		AsyncTask(ENamedThreads::GameThread, [=]()
#endif
		{
			OnVideoResolutionChanged.Broadcast(Width, Height);
		});
	}
	
#if MILLICAST_HAS_CXX20
	AsyncTask(ENamedThreads::ActualRenderingThread, [=, this]()
#else
	AsyncTask(ENamedThreads::ActualRenderingThread, [=]()
#endif
	{
		FScopeLock Lock(&RenderSyncContext);

		FIntPoint FrameSize = FIntPoint(Width, Height);
		FRHICommandListImmediate& RHICmdList = FRHICommandListExecutor::GetImmediateCommandList();

		if (!RenderTargetDescriptor.IsValid() ||
			RenderTargetDescriptor.GetSize() != FIntVector(FrameSize.X, FrameSize.Y, 0))
		{
			// Create the RenderTarget descriptor
			RenderTargetDescriptor = FPooledRenderTargetDesc::Create2DDesc(FrameSize,
				PF_B8G8R8A8,
				FClearValueBinding::None,
				NMillicastMedia::TextureCreateFlags,
				TexCreate_RenderTargetable,
				false);

			NMillicastMedia::CreateTexture( SourceTexture, FrameSize.X, FrameSize.Y);

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

FIntPoint UMillicastTexture2DPlayer::GetCurrentResolution()
{
	return CachedResolution;
}

void UMillicastTexture2DPlayer::BeginDestroy()
{
	TWeakObjectPtr<UMillicastTexture2DPlayer> WeakThis(this);

	AsyncTask(ENamedThreads::ActualRenderingThread, [WeakThis]() {
		if (WeakThis.IsValid())
		{
			FScopeLock Lock(&WeakThis->RenderSyncContext);
			WeakThis->RenderTarget = nullptr;
		}
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