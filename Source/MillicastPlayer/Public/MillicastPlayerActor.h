// Copyright CoSMoSoftware 2021. All Rights Reserved.

#pragma once

#include <CoreMinimal.h>
#include <Components/AudioComponent.h>
#include <Components/StaticMeshComponent.h>
#include "MillicastSubscriberComponent.h"

#include "MillicastPlayerActor.generated.h"

UCLASS(HideCategories = (Activation, Rendering, AssetUserData, Material, Attachment, Actor, Input, Cooking, LOD, Sound,
						 StaticMesh, Materials),
	   Category = "MillicastPlayer", META = (DisplayName = "Millicast Player Actor"))
class MILLICASTPLAYER_API AMillicastPlayerActor : public AActor
{
	GENERATED_UCLASS_BODY()

private:
	/** The desired height of the frame in cm, represented in the virtual scene */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Interp, BlueprintSetter = "SetFrameHeight", Category = "MillicastPlayer",
			  META = (DisplayName = "Frame Height", AllowPrivateAccess = true))
	float FrameHeight = 100.0f;

	/** The desired width of the frame in cm, represented in the virtual scene */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Interp, BlueprintSetter = "SetFrameWidth", Category = "MillicastPlayer",
			  META = (DisplayName = "Frame Width", AllowPrivateAccess = true))
	float FrameWidth = 178.887;

	/** The Receiver object used to get Audio, Video, and Metadata from on the network */
	UPROPERTY(BlueprintReadWrite, EditInstanceOnly, Category = "MillicastPlayer|Media",
			  META = (DisplayName = "Millicast Media Source", AllowPrivateAccess = true))
	UMillicastMediaSource* MillicastMediaSource = nullptr;

	/** The component used to display the video received from the Media Sender object */
	UPROPERTY(Transient, META = (DisplayName = "Video Mesh Component"))
	UStaticMeshComponent* VideoMeshComponent = nullptr;

private:
	/** The material we are trying to apply to the video mesh */
	class UMaterialInterface* VideoMaterial = nullptr;

	/** The dynamic material to apply to the plane object of this actor */
	UPROPERTY()
	class UMaterialInstanceDynamic* VideoMaterialInstance = nullptr;

public:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	/**
		Attempts to set the desired frame size in cm, represented in the virtual scene
	*/
	void SetFrameSize(FVector2D InFrameSize);

	/**
		Returns the current frame size of the 'VideoMeshComponent' for this object
	*/
	const FVector2D GetFrameSize() const;

private:
	UFUNCTION(BlueprintSetter)
	void SetFrameHeight(const float& InFrameHeight);

	UFUNCTION(BlueprintSetter)
	void SetFrameWidth(const float& InFrameWidth);

#if WITH_EDITORONLY_DATA

	virtual void PreEditChange(FProperty* InProperty) override;
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;

#endif
};
