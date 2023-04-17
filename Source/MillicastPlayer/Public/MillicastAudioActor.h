// Copyright Millicast 2023. All Rights Reserved.

#pragma once

#include "GameFramework/Actor.h"
#include "UObject/ObjectMacros.h"
#include "MillicastAudioActor.generated.h"

class UAudioComponent;
class UMillicastAudioComponent;
class USoundWaveProcedural;

UCLASS(BlueprintType, Blueprintable, Category = "Millicast Player", META = (DisplayName = "Millicast Audio Actor"))
class MILLICASTPLAYER_API AMillicastAudioActor : public AActor
{
    GENERATED_BODY()
        
public:
    AMillicastAudioActor(const FObjectInitializer& ObjectInitializer);

    UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category = "Properties")
    UMillicastAudioComponent* MillicastAudioComponent;

    UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category = "Properties")
    UAudioComponent* AudioComponent;
};
