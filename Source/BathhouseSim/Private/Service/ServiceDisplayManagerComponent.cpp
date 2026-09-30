#include "Service/ServiceDisplayManagerComponent.h"
#include "Service/ServiceDisplayPlacementData.h"
#include "Service/DisplaySpaceComponent.h"
#include "Service/DisplayFacilityTargetComponent.h"
#include "Service/ServiceItemDefinition.h"
#include "Facility/BathhouseFacilityActor.h"
#include "Facility/BathhouseFacilitySlotComponent.h"
#include "Facility/BathhouseFacilitySubsystem.h"
#include "Engine/World.h"
#define LOCTEXT_NAMESPACE "ServiceDisplayManager"

UServiceDisplayManagerComponent::UServiceDisplayManagerComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

bool UServiceDisplayManagerComponent::ValidateSpaceLayout(TConstArrayView<const UDisplaySpaceComponent*> Spaces,
														  int32 SlotCount, int32 RequiredSlots, int32 RouterCount,
														  FText& Failure)
{
	TArray<const UDisplaySpaceComponent*> Sorted(Spaces);
	Sorted.Sort(
		[](const UDisplaySpaceComponent& A, const UDisplaySpaceComponent& B)
		{
			return A.GetSpaceIndex() < B.GetSpaceIndex();
		});
	if (Sorted.IsEmpty() || RequiredSlots < 1 || SlotCount != RequiredSlots)
	{
		Failure = LOCTEXT("InvalidLayout", "진열 공간과 필요한 손님 자리 수를 확인하십시오.");
		return false;
	}
	TSet<const UServiceItemDefinition*> FixedKinds;
	bool bRouted = false;
	for (int32 Index = 0; Index < Sorted.Num(); ++Index)
	{
		const auto* Space = Sorted[Index];
		if (Space->GetSpaceIndex() != Index || !Space->ValidateAuthoring(Failure))
		{
			if (Failure.IsEmpty())
			{
				Failure = LOCTEXT("Index", "진열 공간 SpaceIndex는 0부터 중복 없이 연속이어야 합니다.");
			}
			return false;
		}
		if (Space->GetTargetMode() == EDisplaySpaceTargetMode::FacilityRouted)
		{
			bRouted = true;
			if (FixedKinds.Contains(Space->GetFixedKind()))
			{
				Failure = LOCTEXT("DuplicateKind", "고정 품목 묶음은 중복될 수 없습니다.");
				return false;
			}
			FixedKinds.Add(Space->GetFixedKind());
		}
	}
	if (bRouted && RouterCount != 1)
	{
		Failure = LOCTEXT("RouterCount", "설비 전체 조준 진열에는 router가 정확히 1개 필요합니다.");
		return false;
	}
	Failure = FText::GetEmpty();
	return true;
}

bool UServiceDisplayManagerComponent::ValidateExtensionAuthoring(TConstArrayView<UActorComponent*> Components,
																 FText& Failure) const
{
	TArray<const UDisplaySpaceComponent*> Spaces;
	int32 Slots = 0, Routers = 0;
	for (const auto* Component : Components)
	{
		if (const auto* Space = Cast<UDisplaySpaceComponent>(Component))
		{
			Spaces.Add(Space);
		}
		if (Cast<UBathhouseFacilitySlotComponent>(Component))
		{
			++Slots;
		}
		if (const auto* Router = Cast<UDisplayFacilityTargetComponent>(Component))
		{
			++Routers;
			if (!Router->ValidateAuthoring(Failure))
			{
				return false;
			}
		}
	}
	if (!FMath::IsFinite(CustomerUseSeconds) || CustomerUseSeconds < 0)
	{
		Failure = LOCTEXT("UseSeconds", "이용 시간은 유한한 0 이상의 값이어야 합니다.");
		return false;
	}
	return ValidateSpaceLayout(Spaces, Slots, RequiredCustomerSlotCount, Routers, Failure);
}

bool UServiceDisplayManagerComponent::CollectSpaces(TArray<UDisplaySpaceComponent*>& Out, FText& Failure) const
{
	Out.Reset();
	if (!GetOwner())
	{
		return false;
	}
	TArray<UActorComponent*> Components;
	GetOwner()->GetComponents(Components);
	if (!ValidateExtensionAuthoring(Components, Failure))
	{
		return false;
	}
	for (auto* Component : Components)
	{
		if (auto* Space = Cast<UDisplaySpaceComponent>(Component))
		{
			Out.Add(Space);
		}
	}
	Out.Sort(
		[](const UDisplaySpaceComponent& A, const UDisplaySpaceComponent& B)
		{
			return A.GetSpaceIndex() < B.GetSpaceIndex();
		});
	return true;
}

UFacilityPlacementExtensionData* UServiceDisplayManagerComponent::ExportPlacementExtension(UObject* Outer) const
{
	auto* Data = NewObject<UServiceDisplayPlacementData>(Outer);
	Data->Key = GetPlacementExtensionKey();
	const auto* Facility = Cast<ABathhouseFacilityActor>(GetOwner());
	Data->bBottleSummary = Facility && Facility->GetFacilityType() == EBathhouseFacilityType::DrinkFridge;
	TArray<UDisplaySpaceComponent*> Spaces;
	FText Failure;
	if (!CollectSpaces(Spaces, Failure))
	{
		return nullptr;
	}
	for (const auto* Space : Spaces)
	{
		Data->Spaces.Add(Space->BuildSnapshot());
	}
	return Data;
}

bool UServiceDisplayManagerComponent::ValidatePlacementExtension(const UFacilityPlacementExtensionData* Data,
																 FText& Failure) const
{
	TArray<UDisplaySpaceComponent*> Spaces;
	if (!CollectSpaces(Spaces, Failure))
	{
		return false;
	}
	if (!Data)
	{
		return true;
	}
	const auto* DisplayData = Cast<UServiceDisplayPlacementData>(Data);
	if (!DisplayData || Data->Key != GetPlacementExtensionKey() || DisplayData->Spaces.Num() != Spaces.Num())
	{
		Failure = LOCTEXT("Payload", "설비 진열 데이터가 올바르지 않습니다.");
		return false;
	}
	for (int32 Index = 0; Index < Spaces.Num(); ++Index)
	{
		const auto& Snapshot = DisplayData->Spaces[Index];
		if (Snapshot.SpaceIndex != Spaces[Index]->GetSpaceIndex() ||
			!Spaces[Index]->ValidateStockImport(Snapshot.Kind, Snapshot.Count, Failure, Snapshot.InUseRemaining))
		{
			if (Failure.IsEmpty())
			{
				Failure = LOCTEXT("PayloadIndex", "진열 공간 번호가 일치하지 않습니다.");
			}
			return false;
		}
	}
	Failure = FText::GetEmpty();
	return true;
}

void UServiceDisplayManagerComponent::ApplyPlacementExtension(const UFacilityPlacementExtensionData* Data)
{
	TArray<UDisplaySpaceComponent*> Spaces;
	FText Failure;
	if (!CollectSpaces(Spaces, Failure))
	{
		return;
	}
	const auto* DisplayData = Cast<UServiceDisplayPlacementData>(Data);
	for (int32 Index = 0; Index < Spaces.Num(); ++Index)
	{
		const FDisplaySpaceSnapshot Snapshot = DisplayData ? DisplayData->Spaces[Index] : FDisplaySpaceSnapshot();
		Spaces[Index]->ImportStock(Snapshot.Kind, Snapshot.Count, Failure, Snapshot.InUseRemaining);
	}
}

int32 UServiceDisplayManagerComponent::GetTotalStock() const
{
	TArray<UDisplaySpaceComponent*> Spaces;
	if (GetOwner())
	{
		GetOwner()->GetComponents(Spaces);
	}
	int32 Count = 0;
	for (const auto* Space : Spaces)
	{
		Count += Space->GetStock().Count;
	}
	return Count;
}

void UServiceDisplayManagerComponent::BeginPlay()
{
	Super::BeginPlay();
	if (!GetOwner())
	{
		return;
	}
	GetOwner()->GetComponents(BoundSpaces);
	GetOwner()->GetComponents(BoundSlots);
	for (UDisplaySpaceComponent* Space : BoundSpaces)
	{
		Space->OnStockChanged.AddDynamic(this, &UServiceDisplayManagerComponent::HandleStockChanged);
	}
	for (UBathhouseFacilitySlotComponent* Slot : BoundSlots)
	{
		Slot->OnSlotStateChanged.AddDynamic(this, &UServiceDisplayManagerComponent::HandleSlotChanged);
	}
	RefreshTrackedUsers();
}

void UServiceDisplayManagerComponent::EndPlay(const EEndPlayReason::Type Reason)
{
	for (UDisplaySpaceComponent* Space : BoundSpaces)
	{
		if (IsValid(Space))
		{
			Space->OnStockChanged.RemoveDynamic(this, &UServiceDisplayManagerComponent::HandleStockChanged);
		}
	}
	for (UBathhouseFacilitySlotComponent* Slot : BoundSlots)
	{
		if (IsValid(Slot))
		{
			Slot->OnSlotStateChanged.RemoveDynamic(this, &UServiceDisplayManagerComponent::HandleSlotChanged);
		}
	}
	for (const auto& User : TrackedUsers)
	{
		if (User.IsValid())
		{
			User->OnEndPlay.RemoveDynamic(this, &UServiceDisplayManagerComponent::HandleUserEndPlay);
		}
	}
	TrackedUsers.Reset();
	ConsumedReservationSlots.Reset();
	BoundSlots.Reset();
	BoundSpaces.Reset();
	Super::EndPlay(Reason);
}

void UServiceDisplayManagerComponent::RefreshTrackedUsers()
{
	TArray<TWeakObjectPtr<AActor>> Current;
	for (UBathhouseFacilitySlotComponent* Slot : BoundSlots)
	{
		if (IsValid(Slot) && Slot->GetCurrentUser())
		{
			Current.AddUnique(Slot->GetCurrentUser());
		}
	}
	for (const auto& User : TrackedUsers)
	{
		if (User.IsValid() && !Current.Contains(User))
		{
			User->OnEndPlay.RemoveDynamic(this, &UServiceDisplayManagerComponent::HandleUserEndPlay);
		}
	}
	for (const auto& User : Current)
	{
		if (User.IsValid())
		{
			User->OnEndPlay.AddUniqueDynamic(this, &UServiceDisplayManagerComponent::HandleUserEndPlay);
		}
	}
	TrackedUsers = MoveTemp(Current);
}

void UServiceDisplayManagerComponent::HandleSlotChanged(UBathhouseFacilitySlotComponent* Slot,
														EBathhouseFacilitySlotState Previous,
														EBathhouseFacilitySlotState Current)
{
	if (Current == EBathhouseFacilitySlotState::Available)
	{
		ConsumedReservationSlots.Remove(Slot);
	}
	RefreshTrackedUsers();
	if (!bConsumeOnCustomerUseStart || !Slot || Current != EBathhouseFacilitySlotState::Occupied ||
		ConsumedReservationSlots.Contains(Slot))
	{
		return;
	}
	// Mark before broadcasting stock changes so reentrant notifications cannot consume twice.
	ConsumedReservationSlots.Add(Slot);
	for (UDisplaySpaceComponent* Space : BoundSpaces)
	{
		if (IsValid(Space))
		{
			bool bDepleted = false;
			Space->ConsumeOneUse(bDepleted);
		}
	}
}

void UServiceDisplayManagerComponent::HandleUserEndPlay(AActor* Actor, EEndPlayReason::Type Reason)
{
	for (UBathhouseFacilitySlotComponent* Slot : BoundSlots)
	{
		if (IsValid(Slot) && Slot->GetCurrentUser() == Actor)
		{
			Slot->ForceRelease();
		}
	}
	RefreshTrackedUsers();
}

void UServiceDisplayManagerComponent::HandleStockChanged(const FDisplaySpaceSnapshot& Snapshot)
{
	auto* Facility = Cast<ABathhouseFacilityActor>(GetOwner());
	if (Facility && GetWorld())
	{
		if (auto* Facilities = GetWorld()->GetSubsystem<UBathhouseFacilitySubsystem>())
		{
			Facilities->NotifyFacilityAvailabilityChanged(Facility->GetFacilityType());
		}
	}
}

#undef LOCTEXT_NAMESPACE
