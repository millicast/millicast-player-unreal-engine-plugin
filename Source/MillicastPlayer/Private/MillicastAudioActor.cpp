// Copyright Millicast 2023. All Rights Reserved.

#include "MillicastAudioActor.h"
#include "Components/AudioComponent.h"
#include "Components/MillicastAudioComponent.h"

AMillicastAudioActor::AMillicastAudioActor(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("SceneComponent"));
	
	AudioComponent = ObjectInitializer.CreateDefaultSubobject<UAudioComponent>(
		this,
		TEXT("AudioComponent")
		);
	AudioComponent->SetupAttachment(RootComponent);
	
	MillicastAudioComponent = ObjectInitializer.CreateDefaultSubobject<UMillicastAudioComponent>(
		this,
		TEXT("MillicastAudioComponent")
		);
	MillicastAudioComponent->SetupAttachment(RootComponent);

	MillicastAudioComponent->InjectDependencies(AudioComponent);
}
