// Copyright Millicast 2023. All Rights Reserved.

#pragma once

#include "Engine/DeveloperSettings.h"
#include "MillicastAudioSettings.generated.h"

UCLASS(config = Game, defaultconfig, meta = (DisplayName = "Millicast Audio Settings"))
class MILLICASTPLAYER_API UMillicastAudioSettings : public UDeveloperSettings
{
    GENERATED_BODY()

public:
    UPROPERTY(config, EditAnywhere, Category = "General")
    int32 FadeInTimeInMs = 500;

    UPROPERTY(config, EditAnywhere, Category = "General")
    int32 FadeOutTimeInMs = 500;

    UPROPERTY(config, EditAnywhere, Category = "General")
    int32 MaxQueuedAudio = 24000;
};
