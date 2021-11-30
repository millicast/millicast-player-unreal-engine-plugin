// Copyright Epic Games, Inc. All Rights Reserved.

#include <IPlacementModeModule.h>
#include "CoreMinimal.h"
#include "Brushes/SlateImageBrush.h"
#include "Interfaces/IPluginManager.h"
#include "Modules/ModuleInterface.h"
#include "Modules/ModuleManager.h"
#include "Styling/SlateStyle.h"
#include "Styling/SlateStyleRegistry.h"
#include "Templates/UniquePtr.h"
#include "MillicastPlayerActor.h"

#define LOCTEXT_NAMESPACE "MillicastPlayerEditor"

#define IMAGE_BRUSH(RelativePath, ...) FSlateImageBrush(StyleInstance->RootToContentDir(RelativePath, TEXT(".png")), __VA_ARGS__)
#define PLACEMENT_CATEGORY TEXT("MillicastPlayer")
#define PLACEMENT_LOCTEXT NSLOCTEXT("Millicast", "MillicastPlayer", "MillicastPlayer")
#define PLACEMENT_TEXT TEXT("PMMILLICAST")

/**
 * Implements the MediaEditor module.
 */
class FMillicastPlayerEditorModule : public IModuleInterface
{
public:

	//~ IModuleInterface interface

	virtual void StartupModule() override
	{
		RegisterStyle();
	}

	virtual void ShutdownModule() override
	{
		if (!UObjectInitialized() && !IsEngineExitRequested())
		{
			UnregisterStyle();
		}
	}

private:
	TUniquePtr<FSlateStyleSet> StyleInstance;

private:

	void RegisterStyle()
	{
		const FName& CategoryName = PLACEMENT_CATEGORY;
		IPlacementModeModule& PlacementModeModule = IPlacementModeModule::Get();
		StyleInstance = MakeUnique<FSlateStyleSet>("MillicastStyle");

		TSharedPtr<IPlugin> Plugin = IPluginManager::Get().FindPlugin(TEXT("MillicastPlayer"));
		if (Plugin.IsValid())
		{
			StyleInstance->SetContentRoot(FPaths::Combine(Plugin->GetContentDir(), TEXT("Editor/Icons")));
		}

		const FVector2D Icon20x20(20.0f, 20.0f);
		const FVector2D Icon64x64(64.0f, 64.0f);

		StyleInstance->Set("ClassThumbnail.MillicastMediaSource",
				   new IMAGE_BRUSH("MillicastMediaSource_64x", Icon64x64));
		StyleInstance->Set("ClassIcon.MillicastMediaSource",
				   new IMAGE_BRUSH("MillicastMediaSource_20x", Icon20x20));
		StyleInstance->Set("ClassThumbnail.MillicastPlayerActor",
				   new IMAGE_BRUSH("MillicastMediaSource_64x", Icon64x64));
		StyleInstance->Set("ClassIcon.MillicastPlayerActor",
				   new IMAGE_BRUSH("MillicastMediaSource_20x", Icon20x20));
		StyleInstance->Set("ClassThumbnail.MillicastMediaTexture2D",
				   new IMAGE_BRUSH("MillicastMediaSource_64x", Icon64x64));
		StyleInstance->Set("ClassIcon.MillicastMediaTexture2D",
				   new IMAGE_BRUSH("MillicastMediaSource_20x", Icon20x20));

		FSlateStyleRegistry::RegisterSlateStyle(*StyleInstance.Get());

		PlacementModeModule.RegisterPlacementCategory(
			FPlacementCategoryInfo(
				PLACEMENT_LOCTEXT,
				CategoryName,
				PLACEMENT_TEXT,
				41, // FBuiltInPlacementCategories::Volumes() == 40
				true
			)
		);

		// Get the Registered Placement Category
		if (const FPlacementCategoryInfo* PlacementCategoryInformation = PlacementModeModule.GetRegisteredPlacementCategory(CategoryName))
		{
			// Register the Millicast Player a placeable item within the editor
			PlacementModeModule.RegisterPlaceableItem(PlacementCategoryInformation->UniqueHandle, MakeShareable(
				new FPlaceableItem(
					*UActorFactory::StaticClass(),
					FAssetData(AMillicastPlayerActor::StaticClass()->ClassDefaultObject),
					FName("ClassThumbnail.MillicastPlayerActor"),
					TOptional<FLinearColor>(),
					20,
					NSLOCTEXT("Millicast", "MillicastPlayerActor", "Millicast Player Actor")
				))
			);
		}
	}

	void UnregisterStyle()
	{
		FSlateStyleRegistry::UnRegisterSlateStyle(*StyleInstance.Get());
		StyleInstance.Reset();

		IPlacementModeModule& PlacementModeModule = IPlacementModeModule::Get();
		PlacementModeModule.UnregisterPlacementCategory(PLACEMENT_CATEGORY);
	}
};



IMPLEMENT_MODULE(FMillicastPlayerEditorModule, MillicastPlayerEditor);

#undef LOCTEXT_NAMESPACE
