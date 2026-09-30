#if WITH_DEV_AUTOMATION_TESTS

#include "Tests/ServiceAutomationTestProbe.h"

#include "Components/StaticMeshComponent.h"
#include "Facility/BathhouseFacilitySlotComponent.h"
#include "Placement/FacilityPlacementComponent.h"
#include "Service/ServiceItemTypes.h"

void UServiceAutomationDisplaySpace::ConfigureForTest(
	const int32 InSpaceIndex,
	const FGameplayTag& InCategory,
	const int32 SlotCount)
{
	SpaceIndex = InSpaceIndex;
	AcceptedCategory = InCategory;
	SlotTransforms.Reset();
	for (int32 Index = 0; Index < SlotCount; ++Index)
	{
		SlotTransforms.Add(FTransform(FVector(Index * 10.0f, 0.0f, 0.0f)));
	}
}

AServiceAutomationFridge::AServiceAutomationFridge()
{
	SpaceA = CreateDefaultSubobject<UServiceAutomationDisplaySpace>(TEXT("SpaceA"));
	SpaceA->SetupAttachment(GetRootComponent());
	SpaceA->SetRelativeLocation(FVector(0.0f, 0.0f, 60.0f));
	SpaceA->InitBoxExtent(FVector(20.0f));
	SpaceA->ConfigureForTest(0, TAG_Display_Fridge, 3);
	SpaceB = CreateDefaultSubobject<UServiceAutomationDisplaySpace>(TEXT("SpaceB"));
	SpaceB->SetupAttachment(GetRootComponent());
	SpaceB->SetRelativeLocation(FVector(0.0f, 0.0f, 120.0f));
	SpaceB->InitBoxExtent(FVector(20.0f));
	SpaceB->ConfigureForTest(1, TAG_Display_Fridge, 3);
	CustomerSlot = CreateDefaultSubobject<UBathhouseFacilitySlotComponent>(TEXT("CustomerSlot"));
	CustomerSlot->SetupAttachment(GetRootComponent());
}

AServiceAutomationConstructedFridge::AServiceAutomationConstructedFridge() = default;

void AServiceAutomationConstructedFridge::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	TArray<UDisplaySpaceComponent*> Existing;
	GetComponents<UDisplaySpaceComponent>(Existing);
	if (!Existing.IsEmpty())
	{
		return;
	}
	for (int32 Index = 0; Index < 4; ++Index)
	{
		UServiceAutomationDisplaySpace* Space = NewObject<UServiceAutomationDisplaySpace>(
			this, *FString::Printf(TEXT("ConstructedSpace%d"), Index));
		Space->SetupAttachment(GetRootComponent());
		Space->SetRelativeLocation(FVector(0.0f, 0.0f, 40.0f * Index));
		Space->InitBoxExtent(FVector(15.0f));
		Space->ConfigureForTest(Index, TAG_Display_Fridge, 3);
		AddInstanceComponent(Space);
		Space->RegisterComponent();
	}
	UBathhouseFacilitySlotComponent* Slot = NewObject<UBathhouseFacilitySlotComponent>(this, TEXT("ConstructedSlot"));
	Slot->SetupAttachment(GetRootComponent());
	AddInstanceComponent(Slot);
	Slot->RegisterComponent();
}

UBathhouseFacilitySlotComponent* AServiceAutomationConstructedFridge::GetSlotForTest() const
{
	return FindComponentByClass<UBathhouseFacilitySlotComponent>();
}

#endif
