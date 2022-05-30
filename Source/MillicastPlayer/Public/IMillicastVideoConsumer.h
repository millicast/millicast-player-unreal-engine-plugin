// Copyright Millicast 2022. All Rights Reserved.

#pragma once

#include <CoreMinimal.h>
#include <UObject/Interface.h>

#include "IMillicastVideoConsumer.generated.h"

UINTERFACE()
class MILLICASTPLAYER_API UMillicastVideoConsumer : public UInterface
{
	GENERATED_BODY()
};

class IMillicastVideoConsumer
{
	GENERATED_BODY()

public:

	/**
	* Called from a WebRTC thread when a new frame is received
	* The pixel format is ARGB
	* Width and height are the Width and height of the video frame
	*/
	virtual void OnFrame(TArray<uint8>& VideoData, int Width, int Height) = 0;
};