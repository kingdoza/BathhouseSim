#pragma once

#include "CoreMinimal.h"
#include "NativeGameplayTags.h"
#include "ServiceItemTypes.generated.h"

class UServiceItemDefinition;

UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Display_Fridge);

/** Homogeneous item stack shared by item boxes and display spaces. Count 0 always means Kind is null. */
USTRUCT()
struct BATHHOUSESIM_API FServiceItemStack
{
	GENERATED_BODY()

	UPROPERTY(Transient)
	TObjectPtr<UServiceItemDefinition> Kind = nullptr;

	UPROPERTY(Transient)
	int32 Count = 0;

	UPROPERTY(Transient)
	int32 Revision = 0;

	bool IsEmpty() const { return Count <= 0; }
};

USTRUCT(BlueprintType)
struct BATHHOUSESIM_API FDisplaySpaceSnapshot
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Service")
	int32 SpaceIndex = INDEX_NONE;

	UPROPERTY(BlueprintReadOnly, Category = "Service")
	TObjectPtr<UServiceItemDefinition> Kind = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = "Service")
	int32 Count = 0;
};
