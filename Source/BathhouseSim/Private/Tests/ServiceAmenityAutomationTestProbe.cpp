#if WITH_DEV_AUTOMATION_TESTS
#include "Tests/ServiceAmenityAutomationTestProbe.h"
#include "Blueprint/WidgetTree.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "Components/BoxComponent.h"
#include "Economy/BathhouseCashPaymentActor.h"
#include "Facility/BathhouseFacilitySlotComponent.h"

namespace
{
	void MakeSlots(AActor& Actor, int32 Count)
	{
		if (Actor.FindComponentByClass<UBathhouseFacilitySlotComponent>())
		{
			return;
		}
		for (int32 I = 0; I < Count; ++I)
		{
			auto* Slot =
				NewObject<UBathhouseFacilitySlotComponent>(&Actor, *FString::Printf(TEXT("ConstructedSlot%d"), I));
			Slot->SetupAttachment(Actor.GetRootComponent());
			Actor.AddInstanceComponent(Slot);
			Slot->RegisterComponent();
		}
	}
} // namespace

AServiceAmenityChairProbe::AServiceAmenityChairProbe()
{
	BreakChancePercent = 0;
	PlacementFootprint->SetBoxExtent(FVector(30, 30, 50));
	PlacementFootprint->SetRelativeLocation(FVector(0, 0, 50));
}

void AServiceAmenityChairProbe::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	MakeSlots(*this, 1);
}

AServiceAmenityBenchProbe::AServiceAmenityBenchProbe()
{
	FacilityType = EBathhouseFacilityType::RestBench;
	PlacementFootprint->SetBoxExtent(FVector(30, 30, 50));
	PlacementFootprint->SetRelativeLocation(FVector(0, 0, 50));
}

void AServiceAmenityBenchProbe::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	MakeSlots(*this, 3);
}

AServiceAmenityTableProbe::AServiceAmenityTableProbe()
{
	CashOfferClass = ABathhouseCashPaymentActor::StaticClass();
	FocusBlendInSeconds = 0;
	FocusBlendOutSeconds = 0;
	PlacementFootprint->SetBoxExtent(FVector(30, 30, 50));
	PlacementFootprint->SetRelativeLocation(FVector(0, 0, 50));
	ScrubArea->SetBoxExtent(FVector(10, 20, 1));
	ScrubExitPoint->SetRelativeLocation(FVector(250, 0, 0));
	CashOfferPoint->SetRelativeLocation(FVector(250, 100, 100));
	CashStandPoint->SetRelativeLocation(FVector(250, 100, 0));
}

void AServiceAmenityTableProbe::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	MakeSlots(*this, 1);
}

void UServiceAmenityHudProbe::BuildForTest()
{
	WidgetTree = NewObject<UWidgetTree>(this);
	ScrubGaugeBar = WidgetTree->ConstructWidget<UProgressBar>();
	ScrubWaitText = WidgetTree->ConstructWidget<UTextBlock>();
}

float UServiceAmenityHudProbe::GetRatioForTest() const
{
	return ScrubGaugeBar->GetPercent();
}

FString UServiceAmenityHudProbe::GetWaitForTest() const
{
	return ScrubWaitText->GetText().ToString();
}
#endif
