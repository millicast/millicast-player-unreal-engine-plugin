// Copyright CoSMoSoftware 2021. All Rights Reserved.

#include "MillicastMediaSource.h"
#include "MillicastPlayerPrivate.h"

UMillicastMediaSource::UMillicastMediaSource()
{

}

bool UMillicastMediaSource::Initialize(const FMillicastSignalingData& /*data*/)
{
  UE_LOG(LogMillicastPlayer, Log, TEXT("Initialize Media Source"));
  return true;
}

void UMillicastMediaSource::BeginDestroy()
{
  Super::BeginDestroy();
}

/*
 * IMediaOptions interface
 */

FString UMillicastMediaSource::GetMediaOption(const FName& Key, const FString& DefaultValue) const
{
	if (Key == MillicastPlayerOption::StreamName)
	{
		return StreamName;
	}
	if (Key == MillicastPlayerOption::AccountId)
	{
		return AccountId;
	}
	return Super::GetMediaOption(Key, DefaultValue);
}

bool UMillicastMediaSource::HasMediaOption(const FName& Key) const
{

	if (   Key == MillicastPlayerOption::StreamName
		|| Key == MillicastPlayerOption::AccountId)
	{
		return true;
	}

	return Super::HasMediaOption(Key);
}

/*
 * UMediaSource interface
 */

FString UMillicastMediaSource::GetUrl() const
{
        return StreamUrl;
}

bool UMillicastMediaSource::Validate() const
{
	/*FString FailureReason;

	UE_LOG(LogMillicastPlayer, Warning, TEXT("Not yet implemented but good to be there"));
	return false;*/

	// TODO : check if stream name and account id are not empty

	return true;
}

/**
	Attempts to change the Video Texture object used as the video frame capture object
*/
void UMillicastMediaSource::ChangeVideoTexture(UMillicastMediaTexture2D* InVideoTexture)
{
	// FScopeLock Lock(&RenderSyncContext);

	if (IsValid(VideoTexture))
	{
		// make sure that the old texture is not referencing the rendering of this texture
		VideoTexture->UpdateTextureReference(FRHICommandListExecutor::GetImmediateCommandList(), nullptr);
	}

	// Just copy the new texture here.
	VideoTexture = InVideoTexture;
}

/**
	Updates the DynamicMaterial with the VideoTexture of this object
*/
void UMillicastMediaSource::UpdateMaterialTexture(UMaterialInstanceDynamic* MaterialInstance, FString ParameterName)
{
	// Ensure that both the material instance and the video texture are valid
	/*if (IsValid(MaterialInstance) && IsValid(this->VideoTexture))
	{
		// Call the function to set the texture parameter with the proper texture
		MaterialInstance->SetTextureParameterValue(FName(*ParameterName), this->VideoTexture);
	}*/
}

#if WITH_EDITOR
bool UMillicastMediaSource::CanEditChange(const FProperty* InProperty) const
{
	if (!Super::CanEditChange(InProperty))
	{
		return false;
	}

	return true;
}

void UMillicastMediaSource::PostEditChangeChainProperty(struct FPropertyChangedChainEvent& InPropertyChangedEvent)
{
	Super::PostEditChangeChainProperty(InPropertyChangedEvent);
}
#endif //WITH_EDITOR
