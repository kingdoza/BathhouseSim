#include "Shop/ShopCatalog.h"

#include "Misc/DataValidation.h"
#include "Shop/ShopProductRules.h"

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
		FText DefinitionFailure;
		if (!FShopProductRules::ValidateDefinitions(Product.PlacementDefinition, Product.ItemBoxDefinition, DefinitionFailure))
		{
			Invalidate(FText::Format(
				NSLOCTEXT("ShopCatalog", "InvalidProductDefinition", "Shop product '{0}': {1}"),
				FText::FromName(Product.ProductId),
				DefinitionFailure));
		}
	}
	return Result == EDataValidationResult::NotValidated ? EDataValidationResult::Valid : Result;
}
#endif
