// Copyright CoSMoSoftware 2021. All Rights Reserved.

#include "MillicastMediaSourceFactoryNew.h"

#include "AssetTypeCategories.h"
#include "MillicastMediaSource.h"

#define LOCTEXT_NAMESPACE "MillicastPlayerEditorMediaSourceFactory"

/* UMillicastMediaSourceFactoryNew structors
 *****************************************************************************/

UMillicastMediaSourceFactoryNew::UMillicastMediaSourceFactoryNew(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	SupportedClass = UMillicastMediaSource::StaticClass();
	bCreateNew = true;
	bEditAfterNew = true;
}


/* UFactory overrides
 *****************************************************************************/

FText UMillicastMediaSourceFactoryNew::GetDisplayName() const
{
	return LOCTEXT("MillicastMediaSourceFactory", "Millicast Media Source");
}

UObject* UMillicastMediaSourceFactoryNew::FactoryCreateNew(UClass* InClass, UObject* InParent, FName InName, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn)
{
	return NewObject<UMillicastMediaSource>(InParent, InClass, InName, Flags);
}


uint32 UMillicastMediaSourceFactoryNew::GetMenuCategories() const
{
	return EAssetTypeCategories::Media;
}


bool UMillicastMediaSourceFactoryNew::ShouldShowInNewMenu() const
{
	return true;
}

#undef LOCTEXT_NAMESPACE
