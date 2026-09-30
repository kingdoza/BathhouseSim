#include "Service/ServiceItemDefinition.h"

#include "Engine/StaticMesh.h"
#include "Service/ServiceItemTypes.h"

UE_DEFINE_GAMEPLAY_TAG(TAG_Display_Fridge, "Display.Fridge");

namespace
{
bool IsValidTransform(const FTransform& Transform)
{
	const FVector Scale = Transform.GetScale3D();
	return !Transform.ContainsNaN()
		&& FMath::IsFinite(Scale.X) && FMath::IsFinite(Scale.Y) && FMath::IsFinite(Scale.Z)
		&& Scale.X > KINDA_SMALL_NUMBER && Scale.Y > KINDA_SMALL_NUMBER && Scale.Z > KINDA_SMALL_NUMBER;
}
}

UStaticMesh* UServiceItemDefinition::ResolveDisplayMesh() const
{
	return DisplayMesh ? DisplayMesh.Get() : ItemMesh.Get();
}

bool UServiceItemDefinition::ValidateRuntime(FText& OutFailureReason) const
{
	if (ItemId.IsNone() || DisplayName.IsEmpty() || !ItemMesh || BoxCapacity < 1 || SaleValue < 0
		|| ConsumableUses < 0 || !IsValidTransform(BoxItemOffset)
		|| BoxSlotTransforms.Num() != BoxCapacity || !IsValidTransform(DisplayOffset))
	{
		OutFailureReason = NSLOCTEXT("ServiceItemDefinition", "InvalidDefinition", "품목 정의가 올바르지 않습니다.");
		return false;
	}
	for (const FTransform& SlotTransform : BoxSlotTransforms)
	{
		if (!IsValidTransform(SlotTransform))
		{
			OutFailureReason = NSLOCTEXT("ServiceItemDefinition", "InvalidSlotTransform", "품목 박스 자리 transform이 올바르지 않습니다.");
			return false;
		}
	}
	OutFailureReason = FText::GetEmpty();
	return true;
}

#if WITH_EDITOR
EDataValidationResult UServiceItemDefinition::IsDataValid(FDataValidationContext& Context) const
{
	EDataValidationResult Result = Super::IsDataValid(Context);
	FText Failure;
	if (!ValidateRuntime(Failure))
	{
		Context.AddError(Failure);
		Result = EDataValidationResult::Invalid;
	}
	return Result == EDataValidationResult::NotValidated ? EDataValidationResult::Valid : Result;
}
#endif
