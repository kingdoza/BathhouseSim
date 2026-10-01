#if WITH_DEV_AUTOMATION_TESTS
#include "Tests/ServiceAmenityAutomationTestSupport.h"
#include "Combat/MonkeyWrenchActor.h"
#include "Combat/MeleeAttackComponent.h"
#include "Combat/WrenchRepairSession.h"
#include "Service/MassageChairPlacementInstanceData.h"
using namespace ServiceAmenityTest;
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FServiceAmenityChairTimerTest,
								 "BathhouseSim.Service.Amenity.Chair.TimerCollectionAndKnockdown",
								 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FServiceAmenityChairTimerTest::RunTest(const FString&)
{
	FScopedUtilityLaborWorld Scope(TEXT("ChairTimerWorld"));
	FFixture F(Scope.Get());
	if (!F.Install(*this, AServiceAmenityChairProbe::StaticClass()))
	{
		return false;
	}
	auto* Chair = CastChecked<AServiceAmenityChairProbe>(F.Facility);
	auto* Slot = Chair->GetFacilitySlots()[0].Get();
	auto* Wallet = F.Player.PlayerState->GetWallet();
	const int32 Before = Wallet->GetCurrentMoney();
	TestEqual(TEXT("MASS-010 empty coin reason"), Chair->QueryInteraction(F.Context()).FailureReason.ToString(),
			  FString(TEXT("모인 돈 없음")));
	auto* User = F.Occupy();
	if (!TestNotNull(TEXT("Reserved user"), User))
	{
		return false;
	}
	TestFalse(TEXT("MASS-009 occupied recovery rejected"), Chair->QueryFacilityRecovery().bSucceeded);
	TickWorldForDuration(F.World, 30);
	Slot->EndUse(User);
	TestEqual(TEXT("MASS-011 knockdown keeps Reserved"), Slot->GetSlotState(), EBathhouseFacilitySlotState::Reserved);
	TickWorldForDuration(F.World, 65);
	TestEqual(TEXT("Interrupted timer has no charge"), Chair->GetCoinBalance(), 0);
	TestFalse(TEXT("Interrupted timer has no break"), Chair->IsBroken());
	Slot->BeginUse(User);
	TickWorldForDuration(F.World, 59);
	TestEqual(TEXT("Restart requires all sixty seconds"), Chair->GetCoinBalance(), 0);
	TickWorldForDuration(F.World, 1.25);
	TestEqual(TEXT("MASS-001 completion adds fee"), Chair->GetCoinBalance(), 3000);
	TestTrue(TEXT("Test user completed and removed"), !IsValid(User));
	TestTrue(TEXT("Zero percent never breaks"), !Chair->IsBroken() && Chair->IsAvailableForReservation());
	auto* Towel = HoldTowel(*F.World, *F.Player.Carry);
	TestNotNull(TEXT("Held object for money collection"), Towel);
	auto* ConcurrentUser = F.Occupy();
	TestTrue(TEXT("MASS-009 collection is possible during current use"),
			 Chair->QueryInteraction(F.Context()).bCanInteract);
	TestFalse(TEXT("MASS-009 recovery remains denied during current use"), Chair->QueryFacilityRecovery().bSucceeded);
	TestTrue(TEXT("MASS-002 held-object collection succeeds"), Chair->ExecuteInteraction(F.Context()).bSucceeded);
	TestFalse(TEXT("MASS-014 second E cannot pay twice"), Chair->ExecuteInteraction(F.Context()).bSucceeded);
	TestEqual(TEXT("Collection pays exact fee"), Wallet->GetCurrentMoney(), Before + 3000);
	ConcurrentUser->Destroy();
	TestTrue(TEXT("MASS-011 occupied user destruction releases slot"), Slot->IsAvailable());
	TestEqual(TEXT("Disappearing user has no charge"), Chair->GetCoinBalance(), 0);
	Chair->ConfigureUse(60, 100);
	F.Occupy();
	TickWorldForDuration(F.World, 60.75);
	TestTrue(TEXT("MASS-003/004 hundred percent breaks and blocks reservation"),
			 Chair->IsBroken() && !Chair->IsAvailableForReservation());
	TestEqual(TEXT("Broken chair has second fee"), Chair->GetCoinBalance(), 3000);
	auto Context = F.Context();
	Context.Interactor = F.Player.Controller;
	TestTrue(TEXT("Controller resolves wallet"), Chair->ExecuteInteraction(Context).bSucceeded);
	TestTrue(TEXT("Intermediate chance lower roll succeeds"), AMassageChairActor::ShouldBreak(9.99f, 10));
	TestFalse(TEXT("Intermediate threshold is exclusive"), AMassageChairActor::ShouldBreak(10, 10));
	TestFalse(TEXT("MASS-013 zero chance with zero roll"), AMassageChairActor::ShouldBreak(0, 0));
	FText RepairFailure;
	Chair->CommitWrenchRepair(RepairFailure);
	return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FServiceAmenityChairRecoveryTest,
								 "BathhouseSim.Service.Amenity.Chair.RecoveryPaymentRollbackAndBrokenPayload",
								 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FServiceAmenityChairRecoveryTest::RunTest(const FString&)
{
	FScopedUtilityLaborWorld Scope(TEXT("ChairRecoveryWorld"));
	FFixture F(Scope.Get());
	if (!F.Install(*this, AServiceAmenityChairProbe::StaticClass()))
	{
		return false;
	}
	auto* Chair = CastChecked<AServiceAmenityChairProbe>(F.Facility);
	Chair->ConfigureUse(60, 100);
	F.Occupy();
	TickWorldForDuration(F.World, 60.75);
	auto* Wallet = F.Player.PlayerState->GetWallet();
	int32 Before = Wallet->GetCurrentMoney();
	FText Failure;
	Chair->SetFacilityRecoveryInstigator(F.Player.Pawn);
	TestTrue(TEXT("Q hold begins"), Chair->TryBeginFacilityRecoveryHold(Failure));
	Chair->CancelFacilityRecoveryHold();
	Chair->SetFacilityRecoveryInstigator(nullptr);
	TestEqual(TEXT("MASS-008 Q cancel preserves money"), Chair->GetCoinBalance(), 3000);
	TestEqual(TEXT("Q cancel does not pay"), Wallet->GetCurrentMoney(), Before);
	auto* Failed = FFacilityActorConversionTransaction::RecoverFacilityToItem(*Chair, Failure);
	TestNull(TEXT("No wallet fails stage"), Failed);
	TestEqual(TEXT("Failed stage has no payout"), Wallet->GetCurrentMoney(), Before);
	TestEqual(TEXT("Failed stage keeps coins"), Chair->GetCoinBalance(), 3000);
	TestTrue(TEXT("Failed stage restores domain and collision"),
			 Chair->GetFacilityPlacementComponent()->IsPlacedDomainActive() && Chair->GetActorEnableCollision());
	Chair->SetFacilityRecoveryInstigator(F.Player.Pawn);
	FFacilityActorConversionTransaction::SetTestFault(
		FFacilityActorConversionTransaction::ETestFault::RecoverySourceDestroy);
	Failed = FFacilityActorConversionTransaction::RecoverFacilityToItem(*Chair, Failure);
	FFacilityActorConversionTransaction::ClearTestFault();
	TestNull(TEXT("Source destroy failure rolls back"), Failed);
	TestEqual(TEXT("Rollback coins retained"), Chair->GetCoinBalance(), 3000);
	TestEqual(TEXT("Rollback no payment"), Wallet->GetCurrentMoney(), Before);
	auto* Item = FFacilityActorConversionTransaction::RecoverFacilityToItem(*Chair, Failure);
	if (!TestNotNull(TEXT("MASS-008 actual recovery succeeds"), Item))
	{
		return false;
	}
	TestEqual(TEXT("Recovery pays performer"), Wallet->GetCurrentMoney(), Before + 3000);
	TestEqual(TEXT("Destroyed chair coin balance cleared"), Chair->GetCoinBalance(), 0);
	const auto* Data = Cast<UMassageChairPlacementInstanceData>(Item->GetPlacementPayload().InstanceData);
	TestTrue(TEXT("Recovered broken state"), Data && Data->bBroken);
	TestTrue(TEXT("Item summary says broken"), Item->GetHeldSummaryText().ToString().Contains(TEXT("고장")));
	TestTrue(TEXT("Recovered item held"), F.Player.Carry->TryTakePhysicalObject(Item, Failure));
	auto* Installed = Cast<AMassageChairActor>(FFacilityActorConversionTransaction::PlaceItemAsFacility(
		*Item, F.Transform, *F.Zone, *F.Player.Carry, Failure));
	if (!TestNotNull(TEXT("Actual reinstall succeeds"), Installed))
	{
		return false;
	}
	TestTrue(TEXT("Reinstall retains break"), Installed->IsBroken());
	TestEqual(TEXT("Reinstall has no transferred money"), Installed->GetCoinBalance(), 0);
	Installed->SetFacilityRecoveryInstigator(F.Player.Pawn);
	auto* ZeroItem = FFacilityActorConversionTransaction::RecoverFacilityToItem(*Installed, Failure);
	TestNotNull(TEXT("Zero-balance chair can recover with performer wallet"), ZeroItem);
	TestEqual(TEXT("Zero-balance recovery does not pay again"), Wallet->GetCurrentMoney(), Before + 3000);
	return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FServiceAmenityChairRepairTest,
								 "BathhouseSim.Service.Amenity.Chair.WrenchRepairCancelAimLossAndAttack",
								 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FServiceAmenityChairRepairTest::RunTest(const FString&)
{
	FScopedUtilityLaborWorld Scope(TEXT("ChairRepairWorld"));
	FFixture F(Scope.Get());
	if (!F.Install(*this, AServiceAmenityChairProbe::StaticClass()))
	{
		return false;
	}
	auto* Chair = CastChecked<AServiceAmenityChairProbe>(F.Facility);
	Chair->ConfigureUse(60, 100);
	F.Occupy();
	TickWorldForDuration(F.World, 60.75);
	auto* Wrench = F.World->SpawnActor<AMonkeyWrenchActor>();
	auto* Mesh = Wrench->FindComponentByClass<UStaticMeshComponent>();
	Mesh->SetStaticMesh(LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube")));
	FText Failure;
	if (!TestTrue(TEXT("Wrench held"), F.Player.Carry->TryTakePhysicalObject(Wrench, Failure)))
	{
		return false;
	}
	FHeldEquipmentUseContext C;
	C.User = F.Player.Pawn;
	C.Equipment = Wrench;
	C.CarryComponent = F.Player.Carry;
	C.Camera = F.Player.Camera;
	C.FocusHit =
		FHitResult(Chair, Chair->GetRootComponent() ? Cast<UPrimitiveComponent>(Chair->GetRootComponent()) : nullptr,
				   Chair->GetActorLocation(), FVector::UpVector);
	auto Query = Wrench->QueryEquipmentUse(C);
	TestEqual(TEXT("MASS-005 action"), Query.ActionName.ToString(), FString(TEXT("수리")));
	TestEqual(TEXT("Repair Hold"), Query.ActivationMode, EPlayerInteractionActivationMode::Hold);
	TestTrue(TEXT("Repair begins"), Wrench->BeginEquipmentUse(C).bSucceeded);
	TestFalse(TEXT("No melee attack during repair"), Wrench->GetMeleeAttack()->IsAttacking());
	auto Progress = Wrench->UpdateEquipmentUse(C, 1.5f);
	TestEqual(TEXT("Half repair"), Progress.Progress, .5f);
	Wrench->EndEquipmentUse(C);
	TestEqual(TEXT("MASS-006 released progress reset"), Wrench->QueryEquipmentUse(C).Progress, 0.f);
	Wrench->BeginEquipmentUse(C);
	Wrench->UpdateEquipmentUse(C, 1);
	auto Away = C;
	Away.FocusHit = FHitResult();
	TestEqual(TEXT("Aim loss fails silently"), Wrench->UpdateEquipmentUse(Away, 1).State,
			  EPlayerHoldInteractionState::Failed);
	TestTrue(TEXT("Still broken"), Chair->IsBroken());
	Wrench->BeginEquipmentUse(C);
	Wrench->CancelEquipmentUse(C);
	TestEqual(TEXT("Cancel resets progress"), Wrench->QueryEquipmentUse(C).Progress, 0.f);
	Wrench->BeginEquipmentUse(C);
	TestEqual(TEXT("Full three second repair"), Wrench->UpdateEquipmentUse(C, 3).State,
			  EPlayerHoldInteractionState::Succeeded);
	TestTrue(TEXT("Normal and reservable"), !Chair->IsBroken() && Chair->IsAvailableForReservation());
	TestEqual(TEXT("MASS-007 normal chair swings"), Wrench->QueryEquipmentUse(C).ActionName.ToString(),
			  FString(TEXT("휘두르기")));
	TestTrue(TEXT("Normal chair starts attack"), Wrench->BeginEquipmentUse(C).bSucceeded);
	TestTrue(TEXT("Attack active"), Wrench->GetMeleeAttack()->IsAttacking());
	Wrench->CancelEquipmentUse(C);
	auto* User = F.Occupy();
	if (!TestNotNull(TEXT("User after repair"), User))
	{
		return false;
	}
	Chair->GetFacilitySlots()[0]->EndUse(User);
	User->Destroy();
	TestTrue(TEXT("Destroy reserved user releases slot"), Chair->GetFacilitySlots()[0]->IsAvailable());
	return true;
}
#endif
