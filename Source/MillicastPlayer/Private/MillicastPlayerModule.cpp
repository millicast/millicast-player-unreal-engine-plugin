// Copyright CoSMoSoftware 2021. All Rights Reserved.

#include "IMillicastPlayerModule.h"

#include "MillicastPlayerPrivate.h"
#include "MillicastMediaPlayer.h"
#include "Brushes/SlateImageBrush.h"
#include "Interfaces/IPluginManager.h"
#include "Modules/ModuleManager.h"
#include "Styling/SlateStyle.h"

DEFINE_LOG_CATEGORY(LogMillicastPlayer);

#define LOCTEXT_NAMESPACE "MillicastPlayerModule"

/**
 * Implements the NdiMedia module.
 */
class FMillicastPlayerModule : public IMillicastPlayerModule
{
public:

	virtual TSharedPtr<FSlateStyleSet> GetStyle() override
	{
		return StyleSet;
	}

public:

	//~ IModuleInterface interface
	virtual void StartupModule() override
	{
		CreateStyle();
	}

	virtual void ShutdownModule() override {}

private:
	void CreateStyle()
	{
		static FName StyleName(TEXT("MillicastPlayerStyle"));
		StyleSet = MakeShared<FSlateStyleSet>(StyleName);

		const FString ContentDir = IPluginManager::Get().FindPlugin(TEXT("MillicastPlayer"))->GetContentDir();
		const FVector2D Icon16x16(16.0f, 16.0f);
		StyleSet->Set("MillicastPlayerIcon", new FSlateImageBrush((ContentDir / TEXT("Editor/Icons/MillicastMediaSource_64x")) + TEXT(".png"), Icon16x16));
	}

private:
	TSharedPtr<FSlateStyleSet> StyleSet;
};

IMPLEMENT_MODULE(FMillicastPlayerModule, MillicastPlayer);

#undef LOCTEXT_NAMESPACE
