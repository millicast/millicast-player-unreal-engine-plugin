// Copyright Millicast 2023. All Rights Reserved.

#include "Audio/MillicastAudioActor.h"
#include "Components/AudioComponent.h"

AMillicastAudioActor::AMillicastAudioActor(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("SceneComponent"));
	
	AudioComponent = ObjectInitializer.CreateDefaultSubobject<UAudioComponent>(
		this,
		TEXT("AudioComponent")
		);
	AudioComponent->SetupAttachment(RootComponent);
}
