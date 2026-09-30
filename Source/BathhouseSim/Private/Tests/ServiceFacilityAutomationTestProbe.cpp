#if WITH_DEV_AUTOMATION_TESTS
#include "Tests/ServiceFacilityAutomationTestProbe.h"
#include "Tests/ServiceAutomationTestProbe.h"
#include "Facility/BathhouseFacilitySlotComponent.h"
#include "Service/ServiceDisplayManagerComponent.h"

#define LOCTEXT_NAMESPACE "ServiceFacilityAutomationTestProbe"

AServiceAutomationDisplayFacility::AServiceAutomationDisplayFacility()
{
	FacilityType = EBathhouseFacilityType::Vanity;
}

AServiceAutomationShower::AServiceAutomationShower()
{
	FacilityType = EBathhouseFacilityType::Shower;
}

void AServiceAutomationDisplayFacility::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	FixtureKinds = CastChecked<AServiceAutomationDisplayFacility>(GetClass()->GetDefaultObject())->FixtureKinds;
	if (FindComponentByClass<UServiceDisplayManagerComponent>())
	{
		return;
	}
	auto* Manager = NewObject<UServiceDisplayManagerComponent>(this, TEXT("ConstructedManager"));
	Manager->RequiredCustomerSlotCount = FacilityType == EBathhouseFacilityType::Shower ? 2 : 1;
	Manager->bConsumeOnCustomerUseStart = true;
	Manager->CustomerUseSeconds = FacilityType == EBathhouseFacilityType::Vanity ? 20 : 0;
	AddInstanceComponent(Manager);
	Manager->RegisterComponent();
	auto* Router = NewObject<UDisplayFacilityTargetComponent>(this, TEXT("ConstructedRouter"));
	Router->FacilityDisplayName =
		FacilityType == EBathhouseFacilityType::Shower ? LOCTEXT("Shower", "샤워기") : LOCTEXT("Vanity", "화장대");
	Router->SetupAttachment(GetRootComponent());
	Router->InitBoxExtent(FVector(35, 120, 100));
	AddInstanceComponent(Router);
	Router->RegisterComponent();
	const int32 Capacities[] = {2, 4, 4, 6};
	for (int32 Index = 0; Index < FixtureKinds.Num(); ++Index)
	{
		auto* Space =
			NewObject<UServiceAutomationDisplaySpace>(this, *FString::Printf(TEXT("ConstructedGroup%d"), Index));
		Space->SetupAttachment(GetRootComponent());
		Space->SetRelativeLocation(FVector(0, Index * 50 - 75, 0));
		Space->ConfigureRoutedForTest(Index, FixtureKinds[Index],
									  FacilityType == EBathhouseFacilityType::Shower ? 2 : Capacities[Index]);
		AddInstanceComponent(Space);
		Space->RegisterComponent();
	}
	for (int32 Index = 0; Index < Manager->RequiredCustomerSlotCount; ++Index)
	{
		auto* Slot =
			NewObject<UBathhouseFacilitySlotComponent>(this, *FString::Printf(TEXT("ConstructedSlot%d"), Index));
		Slot->SetupAttachment(GetRootComponent());
		AddInstanceComponent(Slot);
		Slot->RegisterComponent();
	}
}

UServiceDisplayManagerComponent* AServiceAutomationDisplayFacility::GetManager() const
{
	return FindComponentByClass<UServiceDisplayManagerComponent>();
}

UDisplayFacilityTargetComponent* AServiceAutomationDisplayFacility::GetRouter() const
{
	return FindComponentByClass<UDisplayFacilityTargetComponent>();
}

TArray<UDisplaySpaceComponent*> AServiceAutomationDisplayFacility::GetSpaces() const
{
	TArray<UDisplaySpaceComponent*> Spaces;
	GetComponents(Spaces);
	Spaces.Sort(
		[](const UDisplaySpaceComponent& A, const UDisplaySpaceComponent& B)
		{
			return A.GetSpaceIndex() < B.GetSpaceIndex();
		});
	return Spaces;
}

UFacilityPlacementExtensionData* UServiceAutomationExtension::ExportPlacementExtension(UObject* Outer) const
{
	auto* Data = NewObject<UServiceAutomationExtensionData>(Outer);
	Data->Key = FixtureKey;
	Data->Value = Value;
	return Data;
}

bool UServiceAutomationExtension::ValidatePlacementExtension(const UFacilityPlacementExtensionData* Data,
															 FText& Failure) const
{
	if (bReject || (Data && (!Cast<UServiceAutomationExtensionData>(Data) || Data->Key != FixtureKey)))
	{
		Failure = FText::FromString(TEXT("Injected independent extension rejection"));
		return false;
	}
	return true;
}

void UServiceAutomationExtension::ApplyPlacementExtension(const UFacilityPlacementExtensionData* Data)
{
	++ApplyCalls;
	Value = Data ? CastChecked<UServiceAutomationExtensionData>(Data)->Value : 0;
}

FPlayerInteractionQuery UServiceAutomationKeyTarget::QueryInteraction(const FPlayerInteractionContext& Context) const
{
	FPlayerInteractionQuery Query;
	Query.HeldUseTargetKey = Key;
	Query.bHeldTakeVisible = true;
	Query.bCanHeldTake = true;
	Query.HeldTakeActivationMode = EPlayerInteractionActivationMode::Repeat;
	return Query;
}

FPlayerInteractionResult UServiceAutomationKeyTarget::ExecuteHeldTargetUse(const FPlayerInteractionContext& Context,
																		   EPlayerHeldTargetUseDirection Direction)
{
	++Executions;
	return FPlayerInteractionResult::Succeeded(EPlayerInteractionIntent::HeldTake);
}

#undef LOCTEXT_NAMESPACE

#endif
