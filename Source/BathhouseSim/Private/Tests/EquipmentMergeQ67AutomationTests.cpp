#if WITH_DEV_AUTOMATION_TESTS
#include "Tests/ServiceAmenityAutomationTestSupport.h"
#include "Tests/CleaningLitterAutomationTestSupport.h"
#include "Towel/CleanTowelStackActor.h"
#include "Towel/TowelProcessingMachineActor.h"
#include "Service/DisplayCueComponent.h"
#include "Interaction/Presentation/OpeningPresentationComponent.h"
#include "Combat/MonkeyWrenchActor.h"
#include "Shop/ShopDeliveryBoxActor.h"
using namespace ServiceAmenityTest;
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEquipmentMergeQ67Test,
								 "BathhouseSim.Interaction.Equipment.Q67ReasonOnlyApplyAndOwnRows",
								 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FEquipmentMergeQ67Test::RunTest(const FString&)
{
	FScopedUtilityLaborWorld Scope(TEXT("Q67World"));
	FFixture F(Scope.Get());
	if (!F.Install(*this, AServiceAmenityChairProbe::StaticClass()))
	{
		return false;
	}
	auto* Chair = CastChecked<AServiceAmenityChairProbe>(F.Facility);
	Chair->ConfigureUse(60, 100);
	F.Occupy();
	TickWorldForDuration(F.World, 60.75);
	auto Query = Chair->QueryInteraction(F.Context());
	TestTrue(TEXT("MASS-012 empty-hand repair hint"),
			 Query.bHeldApplyVisible && !Query.bCanHeldApply &&
				 Query.HeldApplyFailureReason.ToString() == TEXT("몽키스패너가 필요합니다"));
	auto* Tongs = CleaningLitterTest::Tongs(F.World);
	FText Failure;
	TestTrue(TEXT("Tongs held"), F.Player.Carry->TryTakePhysicalObject(Tongs, Failure));
	AddAimShape(*Chair);
	F.Player.Camera->SetWorldLocationAndRotation(Chair->GetActorLocation() - FVector(180, 0, 0), FRotator::ZeroRotator);
	F.Player.Interaction->RefreshInteractionQuery();
	Query = F.Player.Interaction->GetCurrentInteractionQuery();
	TestFalse(TEXT("Tongs has no equipment LMB row on chair"), Query.bEquipmentUseVisible);
	TestTrue(TEXT("MASS-012 Q67 preserves repair hint"),
			 Query.bHeldApplyVisible && !Query.bCanHeldApply && Query.HeldApplyActionName.IsEmpty() &&
				 Query.HeldApplyFailureReason.ToString() == TEXT("몽키스패너가 필요합니다"));
	TArray<FPlayerInteractionResult> Reports;
	auto Handle = F.Player.Interaction->OnInteractionAttemptFinishedNative.AddLambda(
		[&](const FPlayerInteractionResult& R)
		{
			Reports.Add(R);
		});
	auto Result = F.Player.EquipmentUse->BeginEquipmentUse();
	TestFalse(TEXT("Tongs cannot repair"), Result.bSucceeded);
	TestEqual(TEXT("Q67 reports exact hint"), Result.FailureReason.ToString(),
			  FString(TEXT("몽키스패너가 필요합니다")));
	TestTrue(TEXT("EquipmentUse intent reported once"),
			 Reports.Num() == 1 && Reports[0].Intent == EPlayerInteractionIntent::EquipmentUse);
	TestTrue(TEXT("No mutation"), Chair->IsBroken());
	// Even an applicable target signal must be downgraded to reason-only with a non-target equipment query.
	FPlayerInteractionQuery Base;
	Base.bHeldApplyVisible = true;
	Base.bCanHeldApply = true;
	Base.HeldApplyActionName = FText::FromString(TEXT("넣기"));
	Base.HeldApplyFailureReason = FText::FromString(TEXT("Required tool"));
	Base.bHeldTakeVisible = true;
	Base.bCanHeldTake = true;
	auto Merged = F.Player.EquipmentUse->MergeEquipmentQuery(Base);
	TestTrue(TEXT("Apply visibility and reason survive"),
			 Merged.bHeldApplyVisible && Merged.HeldApplyFailureReason.EqualTo(Base.HeldApplyFailureReason));
	TestFalse(TEXT("No apply preview or take highlight possible"),
			  Merged.bCanHeldApply || Merged.bCanHeldTake || Merged.bHeldTakeVisible);
	TestTrue(TEXT("Apply action removed"), Merged.HeldApplyActionName.IsEmpty());
	TestTrue(TEXT("Separate RMB tie preserved"), Merged.bEquipmentSecondaryVisible);
	Chair->SetActorLocation(FVector(12000, 4000, 0));
	auto* Shelf = F.World->SpawnActor<ACleanTowelStackActor>();
	BeginActorPlayIfNeeded(Shelf);
	Shelf->GetFacilityPlacementComponent()->SetPlacedDomainActive(true);
	AddAimShape(*Shelf);
	Shelf->SetActorLocation(F.Player.Camera->GetComponentLocation() + FVector(180, 0, 0));
	F.Player.Interaction->RefreshInteractionQuery();
	Query = F.Player.Interaction->GetCurrentInteractionQuery();
	TestTrue(TEXT("Q67 towel shelf required basket hint"),
			 Query.bHeldApplyVisible && !Query.bCanHeldApply &&
				 Query.HeldApplyFailureReason.ToString().Contains(TEXT("수건 바구니")));
	TestFalse(TEXT("Shelf take highlight stays off"), Shelf->GetDisplayCue()->GetTakeHighlightProxy()->IsVisible());
	Shelf->SetActorLocation(FVector(12000, 4000, 0));
	auto* Washer = F.World->SpawnActor<ATowelProcessingMachineActor>();
	BeginActorPlayIfNeeded(Washer);
	Washer->GetFacilityPlacementComponent()->SetPlacedDomainActive(true);
	AddAimShape(*Washer);
	Washer->SetActorLocation(F.Player.Camera->GetComponentLocation() + FVector(180, 0, 0));
	F.Player.Interaction->RefreshInteractionQuery();
	TestFalse(TEXT("Q67 washer lid does not open"),
			  Washer->FindComponentByClass<UOpeningPresentationComponent>()->GetTargetAlpha() > 0.f);
	Washer->SetActorLocation(FVector(12000, 4500, 0));
	auto* Piece = CleaningLitterTest::Litter(F.World, F.Player.Camera->GetComponentLocation() + FVector(180, 0, 0));
	F.Player.Interaction->RefreshInteractionQuery();
	Query = F.Player.Interaction->GetCurrentInteractionQuery();
	TestTrue(TEXT("Tongs litter uses own pickup row"),
			 Query.bEquipmentUseVisible && Query.EquipmentActionName.ToString() == TEXT("줍기"));
	TestFalse(TEXT("Own row clears Apply"), Query.bHeldApplyVisible);
	Piece->Destroy();
	auto Release = [&](AActor* Actor)
	{
		F.Player.Carry->RecoverHeldPhysicalObject(Actor);
		Actor->SetActorLocation(FVector(0, 5000, 500));
	};
	Release(Tongs);
	auto* Mop = F.World->SpawnActor<AWetMopActor>();
	CleaningLitterTest::Mesh(Mop);
	F.Player.Carry->TryTakePhysicalObject(Mop, Failure);
	Merged = F.Player.EquipmentUse->MergeEquipmentQuery(Base);
	TestTrue(TEXT("Mop visible original row"),
			 Merged.bEquipmentUseVisible && Merged.EquipmentActionName.ToString() == TEXT("물걸레질"));
	TestFalse(TEXT("Mop clears both held directions"), Merged.bHeldApplyVisible || Merged.bHeldTakeVisible);
	Release(Mop);
	auto* Wrench = F.World->SpawnActor<AMonkeyWrenchActor>();
	CleaningLitterTest::Mesh(Wrench);
	F.Player.Carry->TryTakePhysicalObject(Wrench, Failure);
	Merged = F.Player.EquipmentUse->MergeEquipmentQuery(Base);
	TestTrue(TEXT("Wrench visible original row"),
			 Merged.bEquipmentUseVisible && Merged.EquipmentActionName.ToString() == TEXT("휘두르기"));
	Release(Wrench);
	FShopOrderLine Line;
	Line.ProductId = TEXT("Q67Delivery");
	Line.Quantity = 1;
	Line.PlacementDefinition = F.Definition;
	Line.DisplayName = FText::FromString(TEXT("안마의자"));
	const FTransform DeliveryTransform(FVector(0, 0, 300));
	auto* Delivery =
		F.World->SpawnActorDeferred<AShopDeliveryBoxActor>(AShopDeliveryBoxActor::StaticClass(), DeliveryTransform);
	Delivery->InitializeContents(67, {Line});
	Delivery->FinishSpawning(DeliveryTransform);
	Delivery->ActivateFreeWorld(DeliveryTransform, Failure);
	CleaningLitterTest::Mesh(Delivery);
	F.Player.Carry->TryTakePhysicalObject(Delivery, Failure);
	Merged = F.Player.EquipmentUse->MergeEquipmentQuery(Base);
	TestTrue(TEXT("Delivery retains visible equipment row"), Merged.bEquipmentUseVisible);
	TestFalse(TEXT("Delivery clears both held directions"), Merged.bHeldApplyVisible || Merged.bHeldTakeVisible);
	Release(Delivery);
	Chair->SetActorLocation(F.Transform.GetLocation());
	auto* Towel = HoldTowel(*F.World, *F.Player.Carry);
	auto* TowelLitter = CleaningLitterTest::Litter(F.World, FVector(0, 6000, 300));
	auto TowelContext = F.Context();
	TestEqual(TEXT("SCRB-019 towel aiming litter requires tongs"),
			  TowelLitter->QueryInteraction(TowelContext).HeldApplyFailureReason.ToString(),
			  FString(TEXT("집게가 필요합니다")));
	TowelLitter->Destroy();
	Query = Chair->QueryInteraction(F.Context());
	TestTrue(TEXT("MASS-015 towel still receives target hint"), Query.bHeldApplyVisible && !Query.bCanHeldApply);
	Release(Towel);
	auto* Box = SpawnBox(F.World, nullptr, 0);
	F.Player.Carry->TryTakePhysicalObject(Box, Failure);
	Query = Chair->QueryInteraction(F.Context());
	TestTrue(TEXT("Box retains required wrench reason"),
			 Query.HeldApplyFailureReason.ToString() == TEXT("몽키스패너가 필요합니다"));
	Release(Box);
	auto* Item =
		APlaceableFacilityItemActor::SpawnFreshItem(*F.World, *F.Definition, FTransform(FVector(0, 0, 300)), Failure);
	Item->ActivateFreeWorld(Item->GetActorTransform(), Failure);
	F.Player.Carry->TryTakePhysicalObject(Item, Failure);
	TestFalse(TEXT("Facility item LMB belongs to placement without chair hint"),
			  Chair->QueryInteraction(F.Context()).bHeldApplyVisible);
	F.Player.Interaction->OnInteractionAttemptFinishedNative.Remove(Handle);
	return true;
}
#endif
