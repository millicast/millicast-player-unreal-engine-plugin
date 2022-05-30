// Copyright Millicast 2022. All Rights Reserved.

#include "MillicastTexture2DPlayerFactoryNew.h"

#include "AssetTypeCategories.h"
#include "MillicastTexture2DPlayer.h"

#define LOCTEXT_NAMESPACE "MillicastPlayerEditorTexture2DPlayerFactory"

/* UMillicastMediaSourceFactoryNew structors
 *****************************************************************************/

UMillicastTexture2DPlayerNew::UMillicastTexture2DPlayerNew(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	SupportedClass = UMillicastTexture2DPlayer::StaticClass();
	bCreateNew = true;
	bEditAfterNew = true;
}

/* UFactory overrides
 *****************************************************************************/

FText UMillicastTexture2DPlayerNew::GetDisplayName() const
{
	return LOCTEXT("MillicastTexture2DPlayerFactory", "Millicast Texture 2D Player");
}

UObject* UMillicastTexture2DPlayerNew::FactoryCreateNew(UClass* InClass, UObject* InParent, FName InName, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn)
{
	return NewObject<UMillicastTexture2DPlayer>(InParent, UMillicastTexture2DPlayer::StaticClass(), InName, Flags);
}


uint32 UMillicastTexture2DPlayerNew::GetMenuCategories() const
{
	return EAssetTypeCategories::Media;
}


bool UMillicastTexture2DPlayerNew::ShouldShowInNewMenu() const
{
	return true;
}

#undef LOCTEXT_NAMESPACE
