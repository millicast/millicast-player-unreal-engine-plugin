// Copyright CoSMoSOftware 2021. All Rights Reserved.

#include "MillicastMediaTexture2DFactory.h"

#include <AssetTypeCategories.h>
#include "MillicastMediaTexture2D.h"

#define LOCTEXT_NAMESPACE "MillicastPlayerEditorMediaSoundWaveFactory"

UMillicastMediaTexture2DFactory::UMillicastMediaTexture2DFactory(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer) {

	bCreateNew = true;
	bEditAfterNew = true;

	SupportedClass = UMillicastMediaTexture2D::StaticClass();
}

FText UMillicastMediaTexture2DFactory::GetDisplayName() const { return LOCTEXT("MillicastMediaTexture2DFactoryDisplayName", "Millicast Media Texture2D"); }

uint32 UMillicastMediaTexture2DFactory::GetMenuCategories() const { return EAssetTypeCategories::MaterialsAndTextures; }

UObject* UMillicastMediaTexture2DFactory::FactoryCreateNew(UClass* InClass, UObject* InParent, FName InName, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn)
{
	if (UMillicastMediaTexture2D* Resource = NewObject<UMillicastMediaTexture2D>(InParent, InName, Flags | RF_Transactional))
	{
		Resource->UpdateResource();
		return Resource;
	}

	return nullptr;
}

#undef LOCTEXT_NAMESPACE
