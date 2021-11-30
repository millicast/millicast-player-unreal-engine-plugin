/*
	All rights reserved. Copyright(c) 2018-2021, NewTek Inc.

	This file and it's use within a Product is bound by the terms of NDI SDK license that was provided
	as part of the NDI SDK. For more information, please review the license and the NDI SDK documentation.
*/

#include "MillicastPlayerActor.h"

#include <Async/Async.h>
#include <Engine/StaticMesh.h>
#include <Kismet/GameplayStatics.h>
#include <Materials/MaterialInstanceDynamic.h>

#include "MillicastMediaTexture2D.h"
#include <UObject/ConstructorHelpers.h>

AMillicastPlayerActor::AMillicastPlayerActor(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{

	// Get the Engine's 'Plane' static mesh
	static ConstructorHelpers::FObjectFinder<UStaticMesh> MeshObject(
		TEXT("StaticMesh'/Engine/BasicShapes/Plane.Plane'"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> MaterialObject(
		TEXT("Material'/MillicastPlayer/Materials/Millicast_Unlit_SourceMaterial.NDI_Unlit_SourceMaterial'"));

	// Ensure that the object is valid
	if (MeshObject.Object)
	{
		// Create the static mesh component visual
		VideoMeshComponent =
			ObjectInitializer.CreateDefaultSubobject<UStaticMeshComponent>(this, TEXT("VideoMeshComponent"));

		// setup the attachment and modify the position, rotation, and mesh properties
		VideoMeshComponent->SetupAttachment(RootComponent);
		VideoMeshComponent->SetStaticMesh(MeshObject.Object);
		VideoMeshComponent->SetRelativeRotation(FQuat::MakeFromEuler(FVector(90.0f, 0.0f, 90.0f)));
		VideoMeshComponent->SetRelativeScale3D(FVector(FrameWidth / 100.0f, FrameHeight / 100.0f, 1.0f));

		VideoMeshComponent->SetCollisionResponseToAllChannels(ECR_Ignore);
		VideoMeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		VideoMeshComponent->SetCollisionObjectType(ECC_WorldDynamic);

		// This is object is mainly used for simple tests and things that don't require
		// additional material shading support, store the an unlit source material to display
		VideoMaterial = MaterialObject.Object;

		// If the material is valid
		if (VideoMaterial)
		{
			// Set the Mesh Material to the Video Material
			VideoMeshComponent->SetMaterial(0, VideoMaterial);
		}
	}

	bAllowTickBeforeBeginPlay = false;
}

void AMillicastPlayerActor::BeginPlay()
{
	// call the base implementation for 'BeginPlay'
	Super::BeginPlay();

	// We need to validate that we have media source, so we can set the texture in the material instance
	if (IsValid(MillicastMediaSource))
	{
		// Validate the Video Material Instance so we can set the texture used in the Millicast Media source
		if (IsValid(VideoMaterial))
		{
			// create and set the instance material from the MaterialObject
			VideoMaterialInstance =
				VideoMeshComponent->CreateAndSetMaterialInstanceDynamicFromMaterial(0, VideoMaterial);

			// Ensure we have a valid material instance
			if (IsValid(VideoMaterialInstance))
			{
				// alright ensure that the video texture is always enabled
				VideoMaterialInstance->SetScalarParameterValue("Enable Video Alpha", 0.0f);
				VideoMaterialInstance->SetScalarParameterValue("Enable Video Texture", 1.0f);

				MillicastMediaSource->UpdateMaterialTexture(VideoMaterialInstance, "Video Texture");
			}
		}
	}
}

void AMillicastPlayerActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);

	// Ensure we have a valid material instance
	if (EndPlayReason == EEndPlayReason::EndPlayInEditor && IsValid(VideoMaterialInstance))
	{
		// alright ensure that the video texture is always enabled
		VideoMaterialInstance->SetScalarParameterValue("Enable Video Texture", 0.0f);
	}
}

/**
	Attempts to set the desired frame size in cm, represented in the virtual scene
*/
void AMillicastPlayerActor::SetFrameSize(FVector2D InFrameSize)
{
	// clamp the values to the lowest we'll allow
	const float frame_height = FMath::Max(InFrameSize.Y, 0.00001f);
	const float frame_width = FMath::Max(InFrameSize.X, 0.00001f);

	// validate the static mesh component
	if (IsValid(VideoMeshComponent))
	{
		// change the scale of the video
		VideoMeshComponent->SetRelativeScale3D(FVector(frame_width / 100.0f, frame_height / 100.0f, 1.0f));
	}
}

void AMillicastPlayerActor::SetFrameHeight(const float& InFrameHeight)
{
	// Clamp the Frame Height to a minimal value
	FrameHeight = FMath::Max(InFrameHeight, 0.00001f);

	// Call the function to set the frame size with the newly clamped value
	SetFrameSize(FVector2D(FrameWidth, FrameHeight));
}

void AMillicastPlayerActor::SetFrameWidth(const float& InFrameWidth)
{
	// Clamp the Frame Width to a minimal value
	FrameWidth = FMath::Max(InFrameWidth, 0.00001f);

	// Call the function to set the frame size with the newly clamped value
	SetFrameSize(FVector2D(FrameWidth, FrameHeight));
}

/**
	Returns the current frame size of the 'VideoMeshComponent' for this object
*/
const FVector2D AMillicastPlayerActor::GetFrameSize() const
{
	return FVector2D(FrameWidth, FrameHeight);
}

#if WITH_EDITORONLY_DATA

void AMillicastPlayerActor::PreEditChange(FProperty* InProperty)
{
	// call the base class 'PreEditChange'
	Super::PreEditChange(InProperty);
}

void AMillicastPlayerActor::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	// get the name of the property which changed
	FName PropertyName =
		(PropertyChangedEvent.Property != nullptr) ? PropertyChangedEvent.Property->GetFName() : NAME_None;

	// compare against the 'FrameHeight' property
	if (PropertyName == GET_MEMBER_NAME_CHECKED(AMillicastPlayerActor, FrameHeight))
	{
		// resize the frame
		SetFrameSize(FVector2D(FrameWidth / 100.0f, FrameHeight / 100.0f));
	}

	// compare against the 'FrameWidth' property
	else if (PropertyName == GET_MEMBER_NAME_CHECKED(AMillicastPlayerActor, FrameWidth))
	{
		// resize the frame
		SetFrameSize(FVector2D(FrameWidth / 100.0f, FrameHeight / 100.0f));
	}

	// call the base class 'PostEditChangeProperty'
	Super::PostEditChangeProperty(PropertyChangedEvent);
}

#endif
