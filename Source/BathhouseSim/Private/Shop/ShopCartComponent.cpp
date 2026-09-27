#include "Shop/ShopCartComponent.h"

#include "Placement/FacilityPlacementDefinition.h"
#include "Placement/FacilityPlacementTypes.h"
#include "Shop/ShopCatalog.h"
#include "Shop/ShopSettings.h"

UShopCartComponent::UShopCartComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

const FShopCartLine* UShopCartComponent::FindLine(const FName ProductId) const
{
	return Lines.FindByPredicate([ProductId](const FShopCartLine& Line)
	{
		return Line.ProductId == ProductId;
	});
}

EShopFailureCode UShopCartComponent::EvaluateAdd(const FName ProductId) const
{
	const UShopSettings* Settings = GetDefault<UShopSettings>();
	UShopCatalog* Catalog = Settings ? Settings->LoadCatalog() : nullptr;
	if (!Catalog)
	{
		return EShopFailureCode::MissingCatalog;
	}
	const FShopProductEntry* Product = Catalog->FindProduct(ProductId);
	if (!Product || !Product->bForSale)
	{
		return EShopFailureCode::NotForSale;
	}
	FText DefinitionFailure;
	if (Product->ProductId.IsNone() || Product->DisplayName.IsEmpty() || Product->Price <= 0
		|| !Product->PlacementDefinition || Product->PlacementDefinition->LockerSlotCount != 0
		|| !Product->PlacementDefinition->FacilityTags.HasTag(TAG_Facility_Discardable)
		|| !Product->PlacementDefinition->ValidateRuntime(DefinitionFailure))
	{
		return EShopFailureCode::InvalidProduct;
	}
	const FShopCartLine* Existing = FindLine(ProductId);
	const int32 Quantity = Existing ? Existing->Quantity : 0;
	if (Quantity >= Settings->GetPerProductQuantityLimit())
	{
		return EShopFailureCode::ProductLimit;
	}
	if (GetTotalQuantity() >= Settings->GetCartTotalQuantityLimit())
	{
		return EShopFailureCode::CartLimit;
	}
	return EShopFailureCode::None;
}

bool UShopCartComponent::TryAdd(const FName ProductId, EShopFailureCode& OutFailure)
{
	return TryIncrementInternal(ProductId, OutFailure);
}

bool UShopCartComponent::TryIncrement(const FName ProductId, EShopFailureCode& OutFailure)
{
	return TryIncrementInternal(ProductId, OutFailure);
}

bool UShopCartComponent::TryIncrementInternal(const FName ProductId, EShopFailureCode& OutFailure)
{
	OutFailure = EvaluateAdd(ProductId);
	if (OutFailure != EShopFailureCode::None)
	{
		return false;
	}
	if (FShopCartLine* Existing = Lines.FindByPredicate([ProductId](const FShopCartLine& Line)
		{
			return Line.ProductId == ProductId;
		}))
	{
		++Existing->Quantity;
	}
	else
	{
		FShopCartLine& NewLine = Lines.AddDefaulted_GetRef();
		NewLine.ProductId = ProductId;
		NewLine.Quantity = 1;
	}
	BroadcastChanged();
	return true;
}

bool UShopCartComponent::TryDecrement(const FName ProductId)
{
	const int32 Index = Lines.IndexOfByPredicate([ProductId](const FShopCartLine& Line)
	{
		return Line.ProductId == ProductId;
	});
	if (Index == INDEX_NONE || Lines[Index].Quantity <= 0)
	{
		return false;
	}
	if (--Lines[Index].Quantity == 0)
	{
		Lines.RemoveAt(Index);
	}
	BroadcastChanged();
	return true;
}

bool UShopCartComponent::TryRemoveLine(const FName ProductId)
{
	const int32 Removed = Lines.RemoveAll([ProductId](const FShopCartLine& Line)
	{
		return Line.ProductId == ProductId;
	});
	if (Removed == 0)
	{
		return false;
	}
	BroadcastChanged();
	return true;
}

void UShopCartComponent::Clear()
{
	if (Lines.IsEmpty())
	{
		return;
	}
	Lines.Reset();
	BroadcastChanged();
}

int32 UShopCartComponent::GetTotalQuantity() const
{
	int64 Total = 0;
	for (const FShopCartLine& Line : Lines)
	{
		Total += FMath::Max(0, Line.Quantity);
	}
	return static_cast<int32>(FMath::Min<int64>(Total, MAX_int32));
}

bool UShopCartComponent::CalculateTotalPrice(
	const UShopCatalog& Catalog,
	int32& OutTotal,
	EShopFailureCode& OutFailure) const
{
	OutTotal = 0;
	OutFailure = EShopFailureCode::None;
	int64 Total = 0;
	for (const FShopCartLine& Line : Lines)
	{
		const FShopProductEntry* Product = Catalog.FindProduct(Line.ProductId);
		if (!Product || Product->ProductId.IsNone() || Product->Price <= 0 || Line.Quantity <= 0
			|| !Product->PlacementDefinition)
		{
			OutFailure = EShopFailureCode::InvalidProduct;
			return false;
		}
		Total += static_cast<int64>(Product->Price) * static_cast<int64>(Line.Quantity);
		if (Total > MAX_int32)
		{
			OutFailure = EShopFailureCode::InvalidProduct;
			return false;
		}
	}
	OutTotal = static_cast<int32>(Total);
	return true;
}

void UShopCartComponent::BroadcastChanged()
{
	OnCartChanged.Broadcast();
}
