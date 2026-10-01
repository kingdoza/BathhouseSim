#if WITH_DEV_AUTOMATION_TESTS
#include "Tests/ServiceAmenityAutomationTestSupport.h"
#include "Tests/CleaningLitterAutomationTestSupport.h"
#include "Computer/PlayerComputerUseComponent.h"
#include "Economy/BathhouseCashPaymentActor.h"
#include "Interaction/PhysicalCarryDiscardable.h"
#include "InputActionValue.h"
#include "Placement/PlayerFacilityPlacementComponent.h"
using namespace ServiceAmenityTest;
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FServiceAmenityScrubFocusTest,
								 "BathhouseSim.Service.Amenity.Scrub.FocusClampReentryCompletionCashAndHud",
								 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FServiceAmenityScrubFocusTest::RunTest(const FString&)
{
	FScopedUtilityLaborWorld Scope(TEXT("ScrubFocusWorld"));
	FFixture F(Scope.Get());
	if (!F.Install(*this, AServiceAmenityTableProbe::StaticClass()))
	{
		return false;
	}
	auto* Table = CastChecked<AServiceAmenityTableProbe>(F.Facility);
	Table->ConfigureScrub(100, 90);
	auto* Pawn = F.Character();
	auto* Focus = Pawn->GetPlayerScrubFocus();
	FPlayerInteractionContext C;
	C.Interactor = Pawn;
	C.CarryComponent = Pawn->GetPlayerCarry();
	TestEqual(TEXT("SCRB-003 empty hand reason"), Table->QueryInteraction(C).FailureReason.ToString(),
			  FString(TEXT("때수건이 필요합니다")));
	TestFalse(TEXT("No towel begin rejected"), Focus->BeginScrubFocus(Table));
	auto* Towel = HoldTowel(*F.World, *C.CarryComponent);
	TestNotNull(TEXT("Towel held"), Towel);
	TestEqual(TEXT("SCRB-004 no user reason"), Table->QueryInteraction(C).FailureReason.ToString(),
			  FString(TEXT("세신할 손님 없음")));
	auto* User = F.Occupy();
	if (!TestNotNull(TEXT("User occupied"), User))
	{
		return false;
	}
	TestTrue(TEXT("SCRB-001 query includes wait and gauge"),
			 Table->QueryInteraction(C).TargetName.ToString().Contains(TEXT("대기 90초 · 0%")));
	TestFalse(TEXT("SCRB-012 occupied cannot recover"), Table->QueryFacilityRecovery().bSucceeded);
	TestTrue(TEXT("SCRB-002 E execution enters focus"), Table->ExecuteInteraction(C).bSucceeded);
	TestEqual(TEXT("Active"), Focus->GetPhase(), EPlayerScrubFocusPhase::Active);
	TestTrue(TEXT("Suppressed"), Pawn->GetPlayerInteraction()->IsInteractionSuppressed());
	TestEqual(TEXT("MOVE_None"), Pawn->GetFirstPersonMovement()->MovementMode.GetValue(), MOVE_None);
	TestFalse(TEXT("Second scrubber denied"),
			  Table->TryBeginScrubSession(*NewObject<UPlayerScrubFocusComponent>(Pawn)));
	auto* Hud = NewObject<UServiceAmenityHudProbe>(F.Player.Controller);
	Hud->BuildForTest();
	Hud->SetFocusComponent(Focus);
	Hud->PollForTest();
	TestEqual(TEXT("HUD visible active"), Hud->GetRenderOpacity(), 1.f);
	TestEqual(TEXT("HUD wait text"), Hud->GetWaitForTest(), FString(TEXT("대기 90초")));
	Focus->AddRubInput(FVector2D(3, 4));
	TestEqual(TEXT("SCRB-005 no LMB gauge unchanged"), Table->GetScrubProgress(), 0.f);
	TestTrue(TEXT("Input axes map Y->X X->Y"), Focus->GetCursorLocal().Equals(FVector2D(4, 3)));
	Focus->SetRubbing(true);
	Focus->AddRubInput(FVector2D(-3, -4));
	TestEqual(TEXT("SCRB-006 actual five cm"), Table->GetScrubProgress(), .05f);
	Focus->AddRubInput(FVector2D(100, 100));
	const float AtEdge = Table->GetScrubProgress();
	TestTrue(TEXT("Clamped diagonal distance"), FMath::IsNearlyEqual(AtEdge, (5 + FMath::Sqrt(500.f)) / 100));
	Focus->AddRubInput(FVector2D(100, 100));
	TestEqual(TEXT("Boundary pushing cannot add progress"), Table->GetScrubProgress(), AtEdge);
	Hud->PollForTest();
	TestEqual(TEXT("HUD reads domain ratio"), Hud->GetRatioForTest(), AtEdge);
	Focus->RequestEndScrubFocus();
	Focus->RequestEndScrubFocus();
	TestEqual(TEXT("SCRB-007 manual exit inactive"), Focus->GetPhase(), EPlayerScrubFocusPhase::Inactive);
	TestEqual(TEXT("Manual exit preserves gauge"), Table->GetScrubProgress(), AtEdge);
	TestEqual(TEXT("Towel remains held"), C.CarryComponent->GetHeldObject(), static_cast<AActor*>(Towel));
	Hud->PollForTest();
	TestEqual(TEXT("HUD transparent outside focus"), Hud->GetRenderOpacity(), 0.f);
	TestTrue(TEXT("Reentry succeeds"), Focus->BeginScrubFocus(Table));
	TestEqual(TEXT("Reentry progress retained"), Table->GetScrubProgress(), AtEdge);
	Focus->SetRubbing(true);
	Focus->AddRubInput(FVector2D(100, 100));
	Focus->AddRubInput(FVector2D(-100, -100));
	Focus->AddRubInput(FVector2D(100, 100));
	TestEqual(TEXT("SCRB-008 completion auto exit"), Focus->GetPhase(), EPlayerScrubFocusPhase::Inactive);
	TestTrue(TEXT("SCRB-014 slot immediately empty"), Table->GetFacilitySlots()[0]->IsAvailable());
	TestTrue(TEXT("User remains to offer cash"), IsValid(User));
	TestTrue(TEXT("User at stand point"),
			 User->GetActorTransform().Equals(Table->GetStandPoint()->GetComponentTransform()));
	ABathhouseCashPaymentActor* Cash = nullptr;
	int32 CashCount = 0;
	for (TActorIterator<ABathhouseCashPaymentActor> It(F.World); It; ++It)
	{
		Cash = *It;
		++CashCount;
	}
	if (!TestNotNull(TEXT("Independent cash actor"), Cash))
	{
		return false;
	}
	TestEqual(TEXT("Exactly one cash"), CashCount, 1);
	auto* Next = F.Occupy();
	if (!TestNotNull(TEXT("SCRB-014 next user can lie down"), Next))
	{
		return false;
	}
	Next->Destroy();
	TestTrue(TEXT("User loss releases slot"), Table->GetFacilitySlots()[0]->IsAvailable());
	Table->Destroy();
	TestTrue(TEXT("Cash survives table removal"), IsValid(Cash));
	auto* Wallet = F.Player.PlayerState->GetWallet();
	int32 Before = Wallet->GetCurrentMoney();
	TestTrue(TEXT("SCRB-015 cash E succeeds with towel"), Cash->ExecuteInteraction(C).bSucceeded);
	TestEqual(TEXT("Fee paid once"), Wallet->GetCurrentMoney(), Before + 20000);
	TestTrue(TEXT("Test user removed after claimed"), !IsValid(User));
	TestFalse(TEXT("Repeated cash E rejected"), Cash->ExecuteInteraction(C).bSucceeded);
	return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FServiceAmenityScrubExpiryTest,
								 "BathhouseSim.Service.Amenity.Scrub.ExpiryKnockdownAndUserLoss",
								 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FServiceAmenityScrubExpiryTest::RunTest(const FString&)
{
	FScopedUtilityLaborWorld Scope(TEXT("ScrubExpiryWorld"));
	FFixture F(Scope.Get());
	if (!F.Install(*this, AServiceAmenityTableProbe::StaticClass()))
	{
		return false;
	}
	auto* Table = CastChecked<AServiceAmenityTableProbe>(F.Facility);
	auto* Pawn = F.Character();
	auto* Focus = Pawn->GetPlayerScrubFocus();
	HoldTowel(*F.World, *Pawn->GetPlayerCarry());
	auto* User = F.Occupy();
	auto* Slot = Table->GetFacilitySlots()[0].Get();
	Focus->BeginScrubFocus(Table);
	Focus->SetRubbing(true);
	Focus->AddRubInput(FVector2D(10, 10));
	TestTrue(TEXT("Progress starts"), Table->GetScrubProgress() > 0);
	TickWorldForDuration(F.World, 30);
	Slot->EndUse(User);
	TestEqual(TEXT("SCRB-020 knockdown reserves"), Slot->GetSlotState(), EBathhouseFacilitySlotState::Reserved);
	TestEqual(TEXT("Knockdown clears gauge"), Table->GetScrubProgress(), 0.f);
	TestEqual(TEXT("Knockdown exits focus"), Focus->GetPhase(), EPlayerScrubFocusPhase::Inactive);
	TickWorldForDuration(F.World, 91);
	TestTrue(TEXT("Interrupted wait cannot expire user"), IsValid(User));
	Slot->BeginUse(User);
	TestEqual(TEXT("Restart waits full ninety seconds"), Table->GetRemainingWaitSeconds(), 90.f);
	Focus->BeginScrubFocus(Table);
	TickWorldForDuration(F.World, 89);
	TestTrue(TEXT("Before expiry still occupied"), Table->HasOccupiedUser());
	TickWorldForDuration(F.World, 1.25);
	TestTrue(TEXT("SCRB-009/018 expired dummy removed"), !IsValid(User));
	TestTrue(TEXT("Expiry clears slot"), Slot->IsAvailable());
	TestEqual(TEXT("Expiry exits focus"), Focus->GetPhase(), EPlayerScrubFocusPhase::Inactive);
	int32 CashCount = 0;
	for (TActorIterator<ABathhouseCashPaymentActor> It(F.World); It; ++It)
	{
		++CashCount;
	}
	TestEqual(TEXT("Expiry no cash"), CashCount, 0);
	User = F.Occupy();
	Focus->BeginScrubFocus(Table);
	User->Destroy();
	TestTrue(TEXT("Destroy user frees slot"), Slot->IsAvailable());
	TestEqual(TEXT("Destroy user exits"), Focus->GetPhase(), EPlayerScrubFocusPhase::Inactive);
	User = F.Occupy();
	Slot->EndUse(User);
	User->Destroy();
	TestTrue(TEXT("Destroy reserved user frees slot"), Slot->IsAvailable());
	return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FServiceAmenityScrubFailureTest,
								 "BathhouseSim.Service.Amenity.Scrub.CashSpawnFailureAndTableDestroyCleanup",
								 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FServiceAmenityScrubFailureTest::RunTest(const FString&)
{
	FScopedUtilityLaborWorld Scope(TEXT("ScrubFailureWorld"));
	FFixture F(Scope.Get());
	if (!F.Install(*this, AServiceAmenityTableProbe::StaticClass()))
	{
		return false;
	}
	auto* Table = CastChecked<AServiceAmenityTableProbe>(F.Facility);
	auto* Pawn = F.Character();
	auto* Focus = Pawn->GetPlayerScrubFocus();
	HoldTowel(*F.World, *Pawn->GetPlayerCarry());
	auto* User = F.Occupy();
	Table->ConfigureScrub(1, 90);
	Table->ClearCashClass();
	Focus->BeginScrubFocus(Table);
	Focus->SetRubbing(true);
	AddExpectedError(TEXT("Scrub cash spawn failed"), EAutomationExpectedErrorFlags::Contains, 1);
	Focus->AddRubInput(FVector2D(2, 2));
	TestTrue(TEXT("Failed spawn keeps session and user"), Table->HasOccupiedUser() && IsValid(User));
	TestTrue(TEXT("Failed spawn ratio below one"), Table->GetScrubProgress() < 1 && Table->GetScrubProgress() > 0);
	TestEqual(TEXT("Failed spawn does not complete focus"), Focus->GetPhase(), EPlayerScrubFocusPhase::Active);
	const FVector Before = Pawn->GetActorLocation();
	Table->Destroy();
	TestEqual(TEXT("Destroy table immediate cleanup"), Focus->GetPhase(), EPlayerScrubFocusPhase::Inactive);
	TestFalse(TEXT("Suppression cleared"), Pawn->GetPlayerInteraction()->IsInteractionSuppressed());
	TestTrue(TEXT("Destroy does not teleport pawn"), Pawn->GetActorLocation().Equals(Before));
	TestEqual(TEXT("View restored"), F.Player.Controller->GetViewTarget(), static_cast<AActor*>(Pawn));
	TestTrue(TEXT("Movement restored"), Pawn->GetFirstPersonMovement()->MovementMode != MOVE_None);
	Focus->RequestEndScrubFocus();
	User->Destroy();
	return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FServiceAmenityInputTest, "BathhouseSim.Service.Amenity.Scrub.InputOwnershipEntryReleaseExitSearchAndTransitions",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FServiceAmenityInputTest::RunTest(const FString&)
{
	FScopedUtilityLaborWorld Scope(TEXT("ScrubInputWorld"));
	FFixture F(Scope.Get());
	if (!F.Install(*this, AServiceAmenityTableProbe::StaticClass()))
	{
		return false;
	}
	auto* Table = CastChecked<AServiceAmenityTableProbe>(F.Facility);
	Table->ConfigureScrub(100, 90, .2f, .2f);
	auto* Pawn = F.Character();
	auto* Focus = Pawn->GetPlayerScrubFocus();
	auto* Towel = HoldTowel(*F.World, *Pawn->GetPlayerCarry());
	F.Occupy();
	AddAimShape(*Table);
	Pawn->GetFirstPersonCamera()->SetWorldLocationAndRotation(Table->GetActorLocation() - FVector(180, 0, 0),
															  FRotator::ZeroRotator);
	Pawn->GetPlayerInteraction()->RefreshInteractionQuery();
	Pawn->InteractStartInput();
	TestEqual(TEXT("Entry E starts blend"), Focus->GetPhase(), EPlayerScrubFocusPhase::FocusingIn);
	Pawn->InteractEndInput();
	TestEqual(TEXT("SCRB-011 entry E release cannot exit"), Focus->GetPhase(), EPlayerScrubFocusPhase::FocusingIn);
	Focus->AddRubInput(FVector2D(10, 10));
	TestEqual(TEXT("Focus-in blocks rubbing"), Table->GetScrubProgress(), 0.f);
	TickWorldForDuration(F.World, .75);
	TestEqual(TEXT("Blend completes"), Focus->GetPhase(), EPlayerScrubFocusPhase::Active);
	const auto Rotation = F.Player.Controller->GetControlRotation();
	Pawn->DoMove(1, 1);
	Pawn->MoveInput(FInputActionValue(FVector2D(1, 1)));
	Pawn->DoJumpStart();
	Pawn->SprintStartInput();
	Pawn->DropCarryInput();
	Pawn->RecoverFacilityStartInput();
	Pawn->SecondaryUseStartInput();
	Pawn->SecondaryUseEndInput();
	Pawn->SecondaryInteractInput();
	TestTrue(TEXT("SCRB-016 movement input blocked"), Pawn->GetPendingMovementInputVector().IsZero());
	TestFalse(TEXT("Jump blocked"), Pawn->bPressedJump);
	TestEqual(TEXT("G keeps towel"), Pawn->GetPlayerCarry()->GetHeldObject(), static_cast<AActor*>(Towel));
	TestFalse(TEXT("Q placement remains inactive"), Pawn->GetPlayerFacilityPlacement()->IsPlacementActive());
	TestFalse(TEXT("RMB held-use remains inactive"), Pawn->GetPlayerHeldTargetUse()->IsUseActive());
	Pawn->LookInput(FInputActionValue(FVector2D(1, 2)));
	TestTrue(TEXT("Look routes cursor without camera rotation"),
			 F.Player.Controller->GetControlRotation().Equals(Rotation));
	TestTrue(TEXT("Look cursor maps input"), Focus->GetCursorLocal().Equals(FVector2D(2, 1)));
	Pawn->PrimaryUseStartInput();
	TestEqual(TEXT("LMB Scrub owner"), Pawn->PrimaryUsePressOwner, AFirstPersonCharacter::EPrimaryUsePressOwner::Scrub);
	Pawn->LookInput(FInputActionValue(FVector2D(3, 4)));
	TestEqual(TEXT("LMB rub distance"), Table->GetScrubProgress(), .05f);
	Pawn->PrimaryUseEndInput();
	auto Foot = Table->GetExitFootTransform();
	const float HalfHeight = Pawn->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
	auto* Obstacle =
		CleaningLitterTest::Box(F.World, Foot.GetLocation() + FVector(0, 0, HalfHeight), FVector(5, 5, 80));
	Pawn->CancelInput();
	TestEqual(TEXT("ESC starts focus-out"), Focus->GetPhase(), EPlayerScrubFocusPhase::FocusingOut);
	Pawn->InteractStartInput();
	Pawn->InteractEndInput();
	TestEqual(TEXT("Duplicate E does not disturb focus-out"), Focus->GetPhase(), EPlayerScrubFocusPhase::FocusingOut);
	TestTrue(TEXT("SCRB-010 blocked exit searches nearby"),
			 FVector::Dist2D(Pawn->GetActorLocation(), Foot.GetLocation()) > 1 &&
				 FVector::Dist2D(Pawn->GetActorLocation(), Foot.GetLocation()) <= 100);
	TestTrue(TEXT("Exit at authored foot height plus capsule"),
			 FMath::IsNearlyEqual(Pawn->GetActorLocation().Z, Foot.GetLocation().Z + HalfHeight));
	Obstacle->GetOwner()->Destroy();
	TickWorldForDuration(F.World, .75);
	TestEqual(TEXT("Exit completes"), Focus->GetPhase(), EPlayerScrubFocusPhase::Inactive);
	TestFalse(TEXT("Input restored after blend"), Pawn->GetPlayerInteraction()->IsInteractionSuppressed());
	TestTrue(TEXT("Gauge retained after ESC"), Table->GetScrubProgress() > 0);
	return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FServiceAmenityTowelTest,
								 "BathhouseSim.Service.Amenity.Scrub.TowelExactSlotDropRecoveryAndNoDiscard",
								 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FServiceAmenityTowelTest::RunTest(const FString&)
{
	FScopedUtilityLaborWorld Scope(TEXT("ScrubTowelWorld"));
	UWorld* World = Scope.Get();
	FPlayer P;
	if (!BuildPlayer(*this, World, P))
	{
		return false;
	}
	CleaningLitterTest::AddCapsule(P);
	auto* Towel =
		World->SpawnActorDeferred<AScrubTowelActor>(AScrubTowelActor::StaticClass(), FTransform(FVector(400, 0, 160)));
	CleaningLitterTest::Mesh(Towel, FVector(.1));
	Towel->FinishSpawning(Towel->GetActorTransform());
	BeginActorPlayIfNeeded(Towel);
	auto SpawnSlot = [&](AActor* Item, const FVector& Location)
	{
		auto* Slot = World->SpawnActorDeferred<APhysicalCarryFixedSlotActor>(
			APhysicalCarryFixedSlotActor::StaticClass(), FTransform(Location));
		auto* Property = FindFProperty<FObjectProperty>(Slot->GetClass(), TEXT("AssignedItem"));
		Property->SetObjectPropertyValue_InContainer(Slot, Item);
		Slot->FinishSpawning(FTransform(Location));
		BeginActorPlayIfNeeded(Slot);
		return Slot;
	};
	auto* Slot = SpawnSlot(Towel, FVector(400, 0, 160));
	TestTrue(TEXT("SCRB-013 starts in exact slot"), Slot->GetStoredPhysicalCarryItem() == Towel);
	FText Failure;
	TestTrue(TEXT("Take exact slot"), P.Carry->TryTakeFromFixedSlot(Slot).bSucceeded);
	TestEqual(TEXT("ScrubTowel kind"), P.Carry->GetHeldKind(), EPhysicalCarryKind::ScrubTowel);
	TestNull(TEXT("Not equipment"), Cast<IHeldEquipmentUsable>(Towel));
	TestNull(TEXT("SCRB-017 not discardable"), Cast<IPhysicalCarryDiscardable>(Towel));
	auto* Other =
		World->SpawnActorDeferred<AScrubTowelActor>(AScrubTowelActor::StaticClass(), FTransform(FVector(600, 0, 160)));
	CleaningLitterTest::Mesh(Other, FVector(.1));
	Other->FinishSpawning(Other->GetActorTransform());
	BeginActorPlayIfNeeded(Other);
	auto* OtherSlot = SpawnSlot(Other, FVector(600, 0, 160));
	TestFalse(TEXT("Wrong identical slot rejects store"),
			  OtherSlot->QueryStorePhysicalCarry(*P.Carry, *Towel, Failure));
	const auto Held = Towel->GetActorTransform();
	TestTrue(TEXT("SCRB-013 free drop"), P.Carry->TryFreeDropHeldObject(FVector::ForwardVector).bSucceeded);
	TestTrue(TEXT("Held-position drop origin"), Towel->GetActorLocation().Equals(Held.GetLocation()));
	auto* Mesh = Towel->FindComponentByClass<UStaticMeshComponent>();
	TestTrue(TEXT("World physics CCD pawn ignore"), Mesh->IsSimulatingPhysics() && Mesh->BodyInstance.bUseCCD &&
														Mesh->GetCollisionResponseToChannel(ECC_Pawn) == ECR_Ignore);
	Towel->RecoverPhysicalCarryable(nullptr);
	TestTrue(TEXT("Recovery prefers exact slot"), Slot->GetStoredPhysicalCarryItem() == Towel);
	TestTrue(TEXT("Retake exact"), P.Carry->TryTakeFromFixedSlot(Slot).bSucceeded);
	P.Carry->RecoverHeldPhysicalObject(Towel);
	TestTrue(TEXT("Held cleanup returns to slot"), Slot->GetStoredPhysicalCarryItem() == Towel);
	Slot->Destroy();
	TestTrue(TEXT("Slot destruction frees towel"),
			 IsValid(Towel) && !Towel->IsStoredInAssignedPhysicalCarryFixedSlot());
	return true;
}
#endif
