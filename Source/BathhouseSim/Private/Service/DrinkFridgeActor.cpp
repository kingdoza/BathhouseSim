#include "Service/DrinkFridgeActor.h"

#include "Facility/BathhouseFacilitySlotComponent.h"
#include "Facility/BathhouseFacilitySubsystem.h"
#include "Placement/FacilityPlacementComponent.h"
#include "Service/DisplaySpaceComponent.h"
#include "Service/DrinkFridgePlacementInstanceData.h"
#include "Service/DrinkSalesSubsystem.h"
#include "Service/ServiceItemDefinition.h"
#if WITH_EDITOR
#include "Engine/BlueprintGeneratedClass.h"
#include "Engine/SCS_Node.h"
#include "Engine/SimpleConstructionScript.h"
#include "Misc/DataValidation.h"
#endif

#define LOCTEXT_NAMESPACE "DrinkFridgeActor"

ADrinkFridgeActor::ADrinkFridgeActor()
{
	FacilityType = EBathhouseFacilityType::DrinkFridge;
}

void ADrinkFridgeActor::BeginPlay()
{
	Super::BeginPlay();
	for (UBathhouseFacilitySlotComponent* Slot : GetFacilitySlots())
	{
		if (Slot)
		{
			Slot->OnSlotStateChanged.AddDynamic(this, &ADrinkFridgeActor::HandleFridgeSlotStateChanged);
		}
	}
	BindSpaces();
}

void ADrinkFridgeActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	for (UBathhouseFacilitySlotComponent* Slot : GetFacilitySlots())
	{
		if (Slot)
		{
			Slot->OnSlotStateChanged.RemoveDynamic(this, &ADrinkFridgeActor::HandleFridgeSlotStateChanged);
		}
	}
	UnbindSpaces();
	TrackUser(nullptr);
	bRecoveryHoldActive = false;
	Super::EndPlay(EndPlayReason);
}

#if WITH_EDITOR
EDataValidationResult ADrinkFridgeActor::IsDataValid(FDataValidationContext& Context) const
{
	EDataValidationResult Result = Super::IsDataValid(Context);
	FText Failure;
	if (!IsTemplate())
	{
		TArray<UDisplaySpaceComponent*> Spaces;
		if (!CollectSpaces(Spaces, Failure))
		{
			Context.AddError(Failure);
			Result = EDataValidationResult::Invalid;
		}
	}
	else
	{
		// A Blueprint CDO has no SCS component instances, so read native subobjects plus every SCS template
		// in the Blueprint generated-class chain and apply the same layout rule as an installed instance.
		TArray<const UDisplaySpaceComponent*> Spaces;
		int32 SlotCount = 0;
		TSet<const UActorComponent*> Seen;
		auto Collect = [&](const UActorComponent* Component)
		{
			if (!Component || Seen.Contains(Component))
			{
				return;
			}
			Seen.Add(Component);
			if (const UDisplaySpaceComponent* Space = Cast<UDisplaySpaceComponent>(Component))
			{
				Spaces.Add(Space);
			}
			else if (Component->IsA<UBathhouseFacilitySlotComponent>())
			{
				++SlotCount;
			}
		};
		TInlineComponentArray<UActorComponent*> NativeComponents;
		GetComponents(NativeComponents);
		for (const UActorComponent* Component : NativeComponents)
		{
			Collect(Component);
		}
		for (const UClass* Class = GetClass(); Class; Class = Class->GetSuperClass())
		{
			const UBlueprintGeneratedClass* Generated = Cast<UBlueprintGeneratedClass>(Class);
			if (!Generated || !Generated->SimpleConstructionScript)
			{
				continue;
			}
			for (const USCS_Node* Node : Generated->SimpleConstructionScript->GetAllNodes())
			{
				Collect(Node ? Node->ComponentTemplate.Get() : nullptr);
			}
		}
		if (!ValidateSpaceLayout(Spaces, SlotCount, Failure))
		{
			Context.AddError(Failure);
			Result = EDataValidationResult::Invalid;
		}
	}
	return Result == EDataValidationResult::NotValidated ? EDataValidationResult::Valid : Result;
}
#endif

UBathhouseFacilitySlotComponent* ADrinkFridgeActor::GetCustomerSlot() const
{
	const TArray<TObjectPtr<UBathhouseFacilitySlotComponent>>& Slots = GetFacilitySlots();
	return Slots.Num() == 1 ? Slots[0].Get() : nullptr;
}

bool ADrinkFridgeActor::ValidateSpaceLayout(
	const TConstArrayView<const UDisplaySpaceComponent*> Spaces,
	const int32 CustomerSlotCount,
	FText& OutFailureReason)
{
	OutFailureReason = FText::GetEmpty();
	TArray<const UDisplaySpaceComponent*> Sorted(Spaces);
	Sorted.Sort([](const UDisplaySpaceComponent& A, const UDisplaySpaceComponent& B)
	{
		return A.GetSpaceIndex() < B.GetSpaceIndex();
	});
	if (Sorted.IsEmpty())
	{
		OutFailureReason = LOCTEXT("NoSpaces", "냉장고에는 진열 공간이 1개 이상 필요합니다.");
		return false;
	}
	for (int32 Index = 0; Index < Sorted.Num(); ++Index)
	{
		if (Sorted[Index]->GetSpaceIndex() != Index)
		{
			OutFailureReason = LOCTEXT("SpaceIndexGap", "진열 공간 SpaceIndex는 0부터 중복 없이 연속이어야 합니다.");
			return false;
		}
		if (!Sorted[Index]->ValidateAuthoring(OutFailureReason))
		{
			return false;
		}
	}
	if (CustomerSlotCount != 1)
	{
		OutFailureReason = LOCTEXT("SlotCount", "냉장고에는 손님 자리 slot이 정확히 1개 필요합니다.");
		return false;
	}
	return true;
}

bool ADrinkFridgeActor::CollectSpaces(
	TArray<UDisplaySpaceComponent*>& OutSpaces,
	FText& OutFailureReason) const
{
	OutSpaces.Reset();
	GetComponents<UDisplaySpaceComponent>(OutSpaces);
	OutSpaces.Sort([](const UDisplaySpaceComponent& A, const UDisplaySpaceComponent& B)
	{
		return A.GetSpaceIndex() < B.GetSpaceIndex();
	});
	TArray<UBathhouseFacilitySlotComponent*> Slots;
	GetComponents<UBathhouseFacilitySlotComponent>(Slots);
	TArray<const UDisplaySpaceComponent*> ConstSpaces(OutSpaces);
	return ValidateSpaceLayout(ConstSpaces, Slots.Num(), OutFailureReason);
}

int32 ADrinkFridgeActor::GetTotalStock() const
{
	TArray<UDisplaySpaceComponent*> Spaces;
	GetComponents<UDisplaySpaceComponent>(Spaces);
	int32 Total = 0;
	for (const UDisplaySpaceComponent* Space : Spaces)
	{
		Total += Space->GetStock().Count;
	}
	return Total;
}

bool ADrinkFridgeActor::IsPlacedDomainOperational() const
{
	const UFacilityPlacementComponent* Placement = GetFacilityPlacementComponent();
	return Placement && Placement->GetMode() == EPlaceableFacilityMode::Placed
		&& !Placement->IsStagedPlacement() && Placement->IsPlacedDomainActive();
}

bool ADrinkFridgeActor::IsAvailableForReservation() const
{
	return Super::IsAvailableForReservation() && !bRecoveryHoldActive && GetTotalStock() > 0;
}

bool ADrinkFridgeActor::TryBeginFacilityRecoveryHold(FText& OutFailureReason)
{
	if (bRecoveryHoldActive)
	{
		OutFailureReason = LOCTEXT("FridgeRecoveryAlreadyHeld", "설비 회수가 이미 진행 중입니다.");
		return false;
	}
	if (!Super::TryBeginFacilityRecoveryHold(OutFailureReason))
	{
		return false;
	}
	const FFacilityPlacementTransactionResult Query = QueryFacilityRecovery();
	if (!Query.bSucceeded)
	{
		OutFailureReason = Query.FailureReason;
		return false;
	}
	bRecoveryHoldActive = true;
	if (UBathhouseFacilitySubsystem* Facilities = GetWorld() ? GetWorld()->GetSubsystem<UBathhouseFacilitySubsystem>() : nullptr)
	{
		Facilities->NotifyFacilityAvailabilityChanged(FacilityType);
	}
	return true;
}

void ADrinkFridgeActor::CancelFacilityRecoveryHold()
{
	Super::CancelFacilityRecoveryHold();
	if (!bRecoveryHoldActive)
	{
		return;
	}
	bRecoveryHoldActive = false;
	if (UBathhouseFacilitySubsystem* Facilities = GetWorld() ? GetWorld()->GetSubsystem<UBathhouseFacilitySubsystem>() : nullptr)
	{
		Facilities->NotifyFacilityAvailabilityChanged(FacilityType);
	}
}

bool ADrinkFridgeActor::TryTakeDrinkForCustomer(
	AActor& Customer,
	UServiceItemDefinition*& OutKind,
	FText& OutFailureReason)
{
	OutKind = nullptr;
	const UBathhouseFacilitySlotComponent* Slot = GetCustomerSlot();
	UDrinkSalesSubsystem* Sales = GetWorld() ? GetWorld()->GetSubsystem<UDrinkSalesSubsystem>() : nullptr;
	if (!Slot || !Sales || !IsPlacedDomainOperational() || bRecoveryHoldActive
		|| Slot->GetCurrentUser() != &Customer)
	{
		OutFailureReason = LOCTEXT("TakeUnavailable", "음료를 가져갈 수 없는 상태입니다.");
		return false;
	}
	TArray<UDisplaySpaceComponent*> Spaces;
	GetComponents<UDisplaySpaceComponent>(Spaces);
	Spaces.Sort([](const UDisplaySpaceComponent& A, const UDisplaySpaceComponent& B)
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

UBathhouseFacilityPlacementInstanceData* ADrinkFridgeActor::CreateFacilityPlacementInstanceData(UObject* Outer) const
{
	return NewObject<UDrinkFridgePlacementInstanceData>(Outer);
}

bool ADrinkFridgeActor::ExportFacilityExtension(
	UBathhouseFacilityPlacementInstanceData& Data,
	FText& OutFailureReason) const
{
	UDrinkFridgePlacementInstanceData* FridgeData = Cast<UDrinkFridgePlacementInstanceData>(&Data);
	TArray<UDisplaySpaceComponent*> Spaces;
	if (!FridgeData || !CollectSpaces(Spaces, OutFailureReason))
	{
		if (OutFailureReason.IsEmpty())
		{
			OutFailureReason = LOCTEXT("InvalidExportData", "냉장고 회수 데이터를 만들 수 없습니다.");
		}
		return false;
	}
	FridgeData->Spaces.Reset();
	for (const UDisplaySpaceComponent* Space : Spaces)
	{
		FridgeData->Spaces.Add(Space->BuildSnapshot());
	}
	return true;
}

bool ADrinkFridgeActor::ImportFacilityExtension(
	const UBathhouseFacilityPlacementInstanceData* Data,
	FText& OutFailureReason)
{
	TArray<UDisplaySpaceComponent*> Spaces;
	if (!CollectSpaces(Spaces, OutFailureReason))
	{
		return false;
	}
	if (!Data)
	{
		for (UDisplaySpaceComponent* Space : Spaces)
		{
			if (!Space->ImportStock(nullptr, 0, OutFailureReason))
			{
				return false;
			}
		}
		return true;
	}
	const UDrinkFridgePlacementInstanceData* FridgeData = Cast<UDrinkFridgePlacementInstanceData>(Data);
	if (!FridgeData || FridgeData->Spaces.Num() != Spaces.Num())
	{
		OutFailureReason = LOCTEXT("InvalidImportData", "냉장고 진열 공간 데이터가 올바르지 않습니다.");
		return false;
	}
	// Validate everything first so a bad payload leaves every space untouched.
	for (int32 Index = 0; Index < Spaces.Num(); ++Index)
	{
		const FDisplaySpaceSnapshot& Snapshot = FridgeData->Spaces[Index];
		if (Snapshot.SpaceIndex != Spaces[Index]->GetSpaceIndex()
			|| !Spaces[Index]->ValidateStockImport(Snapshot.Kind, Snapshot.Count, OutFailureReason))
		{
			if (OutFailureReason.IsEmpty())
			{
				OutFailureReason = LOCTEXT("SpaceIndexMismatch", "냉장고 진열 공간 번호가 일치하지 않습니다.");
			}
			return false;
		}
	}
	for (int32 Index = 0; Index < Spaces.Num(); ++Index)
	{
		const FDisplaySpaceSnapshot& Snapshot = FridgeData->Spaces[Index];
		if (!Spaces[Index]->ImportStock(Snapshot.Kind, Snapshot.Count, OutFailureReason))
		{
			return false;
		}
	}
	return true;
}

void ADrinkFridgeActor::BindSpaces()
{
	UnbindSpaces();
	TArray<UDisplaySpaceComponent*> Spaces;
	GetComponents<UDisplaySpaceComponent>(Spaces);
	for (UDisplaySpaceComponent* Space : Spaces)
	{
		Space->OnStockChanged.AddDynamic(this, &ADrinkFridgeActor::HandleSpaceStockChanged);
		BoundSpaces.Add(Space);
	}
}

void ADrinkFridgeActor::UnbindSpaces()
{
	for (UDisplaySpaceComponent* Space : BoundSpaces)
	{
		if (IsValid(Space))
		{
			Space->OnStockChanged.RemoveDynamic(this, &ADrinkFridgeActor::HandleSpaceStockChanged);
		}
	}
	BoundSpaces.Reset();
}

void ADrinkFridgeActor::TrackUser(AActor* User)
{
	AActor* Previous = TrackedUser.Get();
	if (Previous == User)
	{
		return;
	}
	if (Previous)
	{
		Previous->OnEndPlay.RemoveDynamic(this, &ADrinkFridgeActor::HandleTrackedUserEndPlay);
	}
	TrackedUser = User;
	if (User)
	{
		User->OnEndPlay.AddUniqueDynamic(this, &ADrinkFridgeActor::HandleTrackedUserEndPlay);
	}
}

void ADrinkFridgeActor::HandleFridgeSlotStateChanged(
	UBathhouseFacilitySlotComponent* Slot,
	const EBathhouseFacilitySlotState PreviousState,
	const EBathhouseFacilitySlotState NewState)
{
	(void)PreviousState;
	if (NewState == EBathhouseFacilitySlotState::Available || !Slot)
	{
		TrackUser(nullptr);
		return;
	}
	TrackUser(Slot->GetCurrentUser());
}

void ADrinkFridgeActor::HandleTrackedUserEndPlay(AActor* Actor, const EEndPlayReason::Type EndPlayReason)
{
	(void)EndPlayReason;
	UBathhouseFacilitySlotComponent* Slot = GetCustomerSlot();
	if (Actor && Actor == TrackedUser.Get() && Slot && Slot->GetCurrentUser() == Actor)
	{
		// Bottles already taken and sales already recorded are intentionally not reverted.
		Slot->ForceRelease();
	}
	else if (Actor == TrackedUser.Get())
	{
		TrackUser(nullptr);
	}
}

void ADrinkFridgeActor::HandleSpaceStockChanged(const FDisplaySpaceSnapshot& Snapshot)
{
	(void)Snapshot;
	if (UBathhouseFacilitySubsystem* Facilities = GetWorld() ? GetWorld()->GetSubsystem<UBathhouseFacilitySubsystem>() : nullptr)
	{
		Facilities->NotifyFacilityAvailabilityChanged(FacilityType);
	}
}

FText UDrinkFridgePlacementInstanceData::GetPlacementContentsSummary() const
{
	TArray<TPair<const UServiceItemDefinition*, int32>> Totals;
	for (const FDisplaySpaceSnapshot& Snapshot : Spaces)
	{
		if (!Snapshot.Kind || Snapshot.Count <= 0)
		{
			continue;
		}
		TPair<const UServiceItemDefinition*, int32>* Existing = Totals.FindByPredicate(
			[&Snapshot](const TPair<const UServiceItemDefinition*, int32>& Entry)
			{
				return Entry.Key == Snapshot.Kind;
			});
		if (Existing)
		{
			Existing->Value += Snapshot.Count;
		}
		else
		{
			Totals.Emplace(Snapshot.Kind, Snapshot.Count);
		}
	}
	TArray<FString> Parts;
	for (const TPair<const UServiceItemDefinition*, int32>& Entry : Totals)
	{
		Parts.Add(FText::Format(
			LOCTEXT("SummaryLine", "{0} {1}병"),
			Entry.Key->DisplayName,
			FText::AsNumber(Entry.Value)).ToString());
	}
	return Parts.IsEmpty() ? FText::GetEmpty() : FText::FromString(FString::Join(Parts, TEXT(", ")));
}

#undef LOCTEXT_NAMESPACE
