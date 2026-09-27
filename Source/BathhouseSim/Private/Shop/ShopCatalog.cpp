#include "Shop/ShopCatalog.h"

#include "Placement/FacilityPlacementDefinition.h"
#include "Placement/FacilityPlacementTypes.h"
#include "Misc/DataValidation.h"

const FShopProductEntry* UShopCatalog::FindProduct(const FName ProductId) const
{
	return Products.FindByPredicate([ProductId](const FShopProductEntry& Product)
	{
		return Product.ProductId == ProductId;
	});
}

#if WITH_EDITOR
EDataValidationResult UShopCatalog::IsDataValid(FDataValidationContext& Context) const
{
	EDataValidationResult Result = Super::IsDataValid(Context);
	auto Invalidate = [&Context, &Result](const FText& Message)
	{
		Context.AddError(Message);
		Result = EDataValidationResult::Invalid;
	};
	TSet<FName> ProductIds;
	for (const FShopProductEntry& Product : Products)
	{
		if (Product.ProductId.IsNone() || ProductIds.Contains(Product.ProductId))
		{
			Invalidate(NSLOCTEXT("ShopCatalog", "DuplicateProductId", "Shop product identifiers must be unique and non-empty."));
		}
		else
		{
			ProductIds.Add(Product.ProductId);
		}
		if (Product.DisplayName.IsEmpty() || Product.Price <= 0)
		{
			Invalidate(NSLOCTEXT("ShopCatalog", "InvalidProductDisplayOrPrice", "Shop products require a display name and a positive price."));
		}
		const UFacilityPlacementDefinition* Definition = Product.PlacementDefinition;
		FText DefinitionFailure;
		if (!Definition || !Definition->ValidateRuntime(DefinitionFailure))
		{
			Invalidate(NSLOCTEXT("ShopCatalog", "MissingPlacementDefinition", "Every shop product requires a valid placement definition."));
			continue;
		}
		if (Definition->LockerSlotCount != 0
			|| !Definition->FacilityTags.HasTag(TAG_Facility_Discardable))
		{
			Invalidate(NSLOCTEXT("ShopCatalog", "InvalidDiscardableDefinition", "Shop products must be non-locker facilities marked Facility.Discardable."));
		}
	}
	return Result == EDataValidationResult::NotValidated ? EDataValidationResult::Valid : Result;
}
#endif
