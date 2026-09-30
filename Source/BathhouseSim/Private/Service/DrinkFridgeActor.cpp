#include "Service/DrinkFridgeActor.h"
#include "Facility/BathhouseFacilitySlotComponent.h"
#include "Placement/FacilityPlacementComponent.h"
#include "Service/DisplaySpaceComponent.h"
#include "Service/ServiceDisplayManagerComponent.h"
#include "Service/DrinkSalesSubsystem.h"
#include "Service/ServiceItemDefinition.h"
#define LOCTEXT_NAMESPACE "DrinkFridgeActor"

ADrinkFridgeActor::ADrinkFridgeActor()
{
	FacilityType = EBathhouseFacilityType::DrinkFridge;
	DisplayManager = CreateDefaultSubobject<UServiceDisplayManagerComponent>(TEXT("DisplayManager"));
	DisplayManager->RequiredCustomerSlotCount = 1;
}

bool ADrinkFridgeActor::CollectSpaces(TArray<UDisplaySpaceComponent*>& Out, FText& Failure) const
{
	return DisplayManager && DisplayManager->CollectSpaces(Out, Failure);
}

bool ADrinkFridgeActor::ValidateSpaceLayout(TConstArrayView<const UDisplaySpaceComponent*> Spaces, int32 Slots,
											FText& Failure)
{
	return UServiceDisplayManagerComponent::ValidateSpaceLayout(Spaces, Slots, 1, 0, Failure);
}

int32 ADrinkFridgeActor::GetTotalStock() const
{
	return DisplayManager ? DisplayManager->GetTotalStock() : 0;
}

bool ADrinkFridgeActor::IsAvailableForReservation() const
{
	return Super::IsAvailableForReservation() && GetTotalStock() > 0;
}

UBathhouseFacilitySlotComponent* ADrinkFridgeActor::GetCustomerSlot() const
{
	const auto& Slots = GetFacilitySlots();
	return Slots.Num() == 1 ? Slots[0].Get() : nullptr;
}

bool ADrinkFridgeActor::IsPlacedDomainOperational() const
{
	const UFacilityPlacementComponent* Placement = GetFacilityPlacementComponent();
	return Placement && Placement->GetMode() == EPlaceableFacilityMode::Placed && !Placement->IsStagedPlacement() &&
		   Placement->IsPlacedDomainActive();
}

bool ADrinkFridgeActor::TryTakeDrinkForCustomer(AActor& Customer, UServiceItemDefinition*& OutKind,
												FText& OutFailureReason)
{
	OutKind = nullptr;
	const UBathhouseFacilitySlotComponent* Slot = GetCustomerSlot();
	UDrinkSalesSubsystem* Sales = GetWorld() ? GetWorld()->GetSubsystem<UDrinkSalesSubsystem>() : nullptr;
	if (!Slot || !Sales || !IsPlacedDomainOperational() || IsRecoveryHoldActive() ||
		Slot->GetCurrentUser() != &Customer)
	{
		OutFailureReason = LOCTEXT("TakeUnavailable", "음료를 가져갈 수 없는 상태입니다.");
		return false;
	}
	TArray<UDisplaySpaceComponent*> Spaces;
	GetComponents<UDisplaySpaceComponent>(Spaces);
	Spaces.Sort(
		[](const UDisplaySpaceComponent& A, const UDisplaySpaceComponent& B)
		{
			return A.GetSpaceIndex() < B.GetSpaceIndex();
		});
	UDisplaySpaceComponent* Source = nullptr;
	for (UDisplaySpaceComponent* Space : Spaces)
	{
		if (Space->GetStock().Count > 0)
		{
			Source = Space;
			break;
		}
	}
	if (!Source)
	{
		OutFailureReason = LOCTEXT("FridgeEmpty", "냉장고가 비어 있습니다.");
		return false;
	}
	UServiceItemDefinition* Kind = nullptr;
	if (!Source->RemoveOneForCustomer(Kind, OutFailureReason))
	{
		return false;
	}
	// Removal and sale are one uninterruptible step; the stock broadcast happens only after both.
	Sales->AddSale(Kind->SaleValue);
	OutKind = Kind;
	Source->PublishStockChanged();
	return true;
}

#undef LOCTEXT_NAMESPACE
