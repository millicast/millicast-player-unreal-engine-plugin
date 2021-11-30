// Copyright CoSMoSOftware 2021. All Rights Reserved.

#pragma once

#include <CoreMinimal.h>
#include <Factories/Factory.h>
#include <UObject/Object.h>

#include "MillicastMediaTexture2DFactory.generated.h"

/**
	Factory Class used to create assets via content browser for NDI Texture2D objects
*/
UCLASS()
class UMillicastMediaTexture2DFactory : public UFactory
{
	GENERATED_UCLASS_BODY()

	public:
		virtual FText GetDisplayName() const override;
		virtual uint32 GetMenuCategories() const override;

		virtual UObject* FactoryCreateNew(UClass* InClass, UObject* InParent, FName InName, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn) override;
};
