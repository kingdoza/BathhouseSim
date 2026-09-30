#include "Shop/ShopUnboxItemShape.h"

#include "Components/PrimitiveComponent.h"
#include "Placement/FacilityPlacementDefinition.h"
#include "Placement/PlaceableFacilityItemActor.h"
#include "Service/ItemBoxActor.h"

#define LOCTEXT_NAMESPACE "ShopUnboxItemShape"

bool FShopUnboxItemShape::FromFacilityDefinition(
	UFacilityPlacementDefinition& Definition,
	FShopUnboxItemShape& OutShape,
	FText& OutFailureReason)
{
	OutShape = FShopUnboxItemShape();
	if (!APlaceableFacilityItemActor::GetDefinitionItemScale(Definition, OutShape.ItemScale, OutFailureReason))
	{
		return false;
	}
	OutShape.DebugName = Definition.GetPathName();
	const TWeakObjectPtr<UFacilityPlacementDefinition> WeakDefinition(&Definition);
	OutShape.BuildCollisionQuery = [WeakDefinition](
		const FTransform& ProbeWorldTransform,
		FVector& OutLocation,
		FQuat& OutRotation,
		FCollisionShape& OutCollisionShape,
		const UPrimitiveComponent*& OutCollisionTemplate,
		FText& OutFailure)
	{
		const UFacilityPlacementDefinition* Live = WeakDefinition.Get();
		if (!Live)
		{
			OutFailure = LOCTEXT("MissingPlacementDefinition", "주문 상품 배치 정의를 찾을 수 없습니다.");
			return false;
		}
		return APlaceableFacilityItemActor::BuildDefinitionCollisionQuery(
			*Live,
			ProbeWorldTransform,
			OutLocation,
			OutRotation,
			OutCollisionShape,
			OutCollisionTemplate,
			OutFailure);
	};
	return true;
}

bool FShopUnboxItemShape::FromItemBoxClass(
	const TSubclassOf<AItemBoxActor> BoxClass,
	FShopUnboxItemShape& OutShape,
	FText& OutFailureReason)
{
	OutShape = FShopUnboxItemShape();
	const AItemBoxActor* BoxCDO = BoxClass ? BoxClass->GetDefaultObject<AItemBoxActor>() : nullptr;
	const USceneComponent* Root = BoxCDO ? BoxCDO->GetRootComponent() : nullptr;
	if (!Root)
	{
		OutFailureReason = LOCTEXT("InvalidItemBoxClass", "품목 박스 클래스의 기본 루트를 확인할 수 없습니다.");
		return false;
	}
	OutShape.ItemScale = Root->GetRelativeScale3D();
	OutShape.DebugName = BoxClass->GetPathName();
	OutShape.BuildCollisionQuery = [BoxClass](
		const FTransform& ProbeWorldTransform,
		FVector& OutLocation,
		FQuat& OutRotation,
		FCollisionShape& OutCollisionShape,
		const UPrimitiveComponent*& OutCollisionTemplate,
		FText& OutFailure)
	{
		return AItemBoxActor::BuildClassCollisionQuery(
			BoxClass,
			ProbeWorldTransform,
			OutLocation,
			OutRotation,
			OutCollisionShape,
			OutCollisionTemplate,
			OutFailure);
	};
	return true;
}

#undef LOCTEXT_NAMESPACE
