#include "Shop/ShopProductRules.h"

#include "Placement/FacilityPlacementDefinition.h"
#include "Placement/FacilityPlacementTypes.h"
#include "Service/ServiceItemDefinition.h"

#define LOCTEXT_NAMESPACE "ShopProductRules"

bool FShopProductRules::ValidateDefinitions(
	const UFacilityPlacementDefinition* Facility,
	const UServiceItemDefinition* ItemBox,
	FText& OutFailureReason)
{
	OutFailureReason = FText::GetEmpty();
	const bool bHasFacility = IsValid(Facility);
	const bool bHasBox = IsValid(ItemBox);
	if (bHasFacility == bHasBox)
	{
		OutFailureReason = LOCTEXT("ExactlyOneDefinition", "상품에는 설비 배치 정의 또는 품목 박스 정의 중 정확히 하나가 필요합니다.");
		return false;
	}
	if (bHasFacility)
	{
		const bool bDiscardable = Facility->FacilityTags.HasTag(TAG_Facility_Discardable);
		if (Facility->LockerSlotCount == 0 && !bDiscardable)
		{
			OutFailureReason = LOCTEXT("InvalidDiscardableDefinition", "설비 상품은 Facility.Discardable 설비여야 합니다.");
			return false;
		}
		if (Facility->LockerSlotCount > 0 && bDiscardable)
		{
			OutFailureReason = LOCTEXT("InvalidLockerDefinition", "락커 상품에는 Facility.Discardable 태그를 둘 수 없습니다.");
			return false;
		}
		return Facility->ValidateRuntime(OutFailureReason);
	}
	return ItemBox->ValidateRuntime(OutFailureReason);
}

bool FShopProductRules::ValidateProduct(const FShopProductEntry& Product, FText& OutFailureReason)
{
	if (Product.ProductId.IsNone() || Product.DisplayName.IsEmpty() || Product.Price <= 0)
	{
		OutFailureReason = LOCTEXT("InvalidProductFields", "상품에는 ID, 표시 이름과 양수 가격이 필요합니다.");
		return false;
	}
	return ValidateDefinitions(Product.PlacementDefinition, Product.ItemBoxDefinition, OutFailureReason);
}

bool FShopProductRules::ValidateOrderLine(const FShopOrderLine& Line, FText& OutFailureReason)
{
	if (Line.ProductId.IsNone() || Line.DisplayName.IsEmpty() || Line.Quantity <= 0)
	{
		OutFailureReason = LOCTEXT("InvalidLineFields", "주문 줄에는 ID, 표시 이름과 양수 수량이 필요합니다.");
		return false;
	}
	return ValidateDefinitions(Line.PlacementDefinition, Line.ItemBoxDefinition, OutFailureReason);
}

#undef LOCTEXT_NAMESPACE
