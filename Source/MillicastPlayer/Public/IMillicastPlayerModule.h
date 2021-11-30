// Copyright CoSMoSoftware 2021. All Rights Reserved.

#pragma once

#include "Modules/ModuleInterface.h"
#include "Modules/ModuleManager.h"
#include "Templates/SharedPointer.h"

class FSlateStyleSet;
class IMediaEventSink;
class IMediaPlayer;

/**
 * Interface for the Media module.
 */
class IMillicastPlayerModule : public IModuleInterface
{
public:

	static inline IMillicastPlayerModule& Get()
	{
		static const FName ModuleName = "MillicastPlayer";
		return FModuleManager::LoadModuleChecked<IMillicastPlayerModule>(ModuleName);
	}

	/** @return SlateStyleSet to be used across the MillicastPlayer module */
	virtual TSharedPtr<FSlateStyleSet> GetStyle() = 0;
};

