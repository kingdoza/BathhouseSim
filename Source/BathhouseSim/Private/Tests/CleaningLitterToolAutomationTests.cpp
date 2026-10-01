#if WITH_DEV_AUTOMATION_TESTS
#include "Tests/CleaningLitterAutomationTestSupport.h"
#include "Character/FirstPersonCharacter.h"
#include "Cleaning/TrashBagDropPlacement.h"
#include "Engine/LocalPlayer.h"
#include "Interaction/BathhouseKeyActor.h"
#include "Combat/MonkeyWrenchActor.h"
#include "Shop/ShopDeliveryBoxActor.h"
#include "Towel/CleanTowelStackActor.h"
#include "Misc/DataValidation.h"
using namespace CleaningLitterTest;
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCleaningLitterToolTest, "BathhouseSim.Cleaning.Litter.TongsAndBag",
								 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCleaningLitterToolTest::RunTest(const FString&)
{
	FScopedUtilityLaborWorld Scope(TEXT("LitterTool"));
	auto* World = Scope.Get();
	if (!World)
	{
		return false;
	}
	FPlayer Player;
	if (!BuildPlayer(*this, World, Player))
	{
		return false;
	}
	AddCapsule(Player);
	auto* T = Tongs(World);
	FText Failure;
	TestTrue(TEXT("E takes tongs"), Player.Carry->TryTakePhysicalObject(T, Failure));
	auto C = Context(Player, T);
	TestFalse(TEXT("TRSH-024: no focus LMB invisible"), T->QueryEquipmentUse(C).bVisible);
	int32 Reports = 0;
	auto Handle = Player.Interaction->OnInteractionAttemptFinishedNative.AddLambda(
		[&](const FPlayerInteractionResult&)
		{
			++Reports;
		});
	Player.Interaction->ClearInteractionQuery();
	Player.EquipmentUse->BeginEquipmentUse();
	TestEqual(TEXT("Empty-space LMB reports no result"), Reports, 0);
	auto Query = Player.Interaction->GetCurrentInteractionQuery();
	Query = Player.EquipmentUse->MergeEquipmentQuery(Query);
	TestTrue(TEXT("RMB always offers tie"),
			 Query.bEquipmentSecondaryVisible && Query.EquipmentSecondaryActionName.ToString() == TEXT("봉투 묶기"));
	TestFalse(TEXT("Tie is not a held-use Take row"), Query.bHeldTakeVisible || Query.bCanHeldTake);
	TestEqual(TEXT("Empty bag reason"), Query.EquipmentSecondaryFailureReason.ToString(),
			  FString(TEXT("봉투가 비어 있음")));
	for (int32 I = 0; I < 20; ++I)
	{
		auto* L = Litter(World, FVector(300 + I * 30, 0, 0));
		C = Context(Player, T, L);
		TestTrue(TEXT("TRSH-007: one press collects one"), T->BeginEquipmentUse(C).bSucceeded);
		TestFalse(TEXT("TRSH-008: stale focus cannot collect twice"), T->BeginEquipmentUse(C).bSucceeded);
	}
	TestEqual(TEXT("TRSH-009: capacity reaches 20"), T->GetBagCount(), 20);
	auto* Extra = Litter(World, FVector(1000, 0, 0));
	C = Context(Player, T, Extra);
	TestEqual(TEXT("Full bag reason"), T->QueryEquipmentUse(C).FailureReason.ToString(), FString(TEXT("봉투 가득 참")));
	TestFalse(TEXT("Full bag does not consume litter"), T->BeginEquipmentUse(C).bSucceeded);
	TestTrue(TEXT("Rejected litter stays"), IsValid(Extra));
	TestEqual(TEXT("TRSH-015: held summary"), T->GetHeldSummaryText().ToString(), FString(TEXT("봉투 20/20")));
	auto* Slot = World->SpawnActor<APhysicalCarryFixedSlotActor>(APhysicalCarryFixedSlotActor::StaticClass(),
																 FTransform(FVector(500, 500, 100)));
	Set<FObjectProperty>(Slot, TEXT("AssignedItem"), T);
	Set<FBoolProperty>(Slot, TEXT("bStartOccupied"), false);
	BeginActorPlayIfNeeded(Slot);
	TestTrue(TEXT("Exact fixed slot stores tongs"), Player.Carry->TryStoreHeldObjectInFixedSlot(Slot).bSucceeded);
	TestEqual(TEXT("TRSH-013: stored count persists"), T->GetBagCount(), 20);
	TestTrue(TEXT("Take back from exact fixed slot"), Player.Carry->TryTakeFromFixedSlot(Slot).bSucceeded);
	TestTrue(TEXT("G free drops tongs"), Player.Carry->TryFreeDropHeldObject(FVector::ForwardVector).bSucceeded);
	TestEqual(TEXT("TRSH-023: drop count persists"), T->GetBagCount(), 20);
	T->SetActorLocation(FVector(0, 0, -10000));
	T->RecoverPhysicalCarryable(nullptr);
	TestTrue(TEXT("Fall recovery prefers fixed slot"), Slot->IsOccupied());
	TestEqual(TEXT("Recovered count persists"), T->GetBagCount(), 20);
	TestTrue(TEXT("Retake after fall"), Player.Carry->TryTakeFromFixedSlot(Slot).bSucceeded);
	C = Context(Player, T);
	// Full-height wall: blocks the view-front stage (eye level) as well as the floor-front stage.
	auto* Wall = Box(World, FVector(50, 0, 170), FVector(70, 100, 170));
	TestFalse(TEXT("TRSH-021: blocked tie fails"), T->ExecuteSecondaryEquipmentUse(C).bSucceeded);
	TestEqual(TEXT("Failed tie preserves count"), T->GetBagCount(), 20);
	Wall->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Set<FClassProperty>(T, TEXT("TiedBagClass"), static_cast<UClass*>(nullptr));
	TestFalse(TEXT("Invalid bag class fails safely"), T->ExecuteSecondaryEquipmentUse(C).bSucceeded);
	TestEqual(TEXT("Spawn failure preserves count"), T->GetBagCount(), 20);
	Set<FClassProperty>(T, TEXT("TiedBagClass"), ATrashBagActor::StaticClass());
	TestTrue(TEXT("TRSH-010/012: tie succeeds without focus"), T->ExecuteSecondaryEquipmentUse(C).bSucceeded);
	TestEqual(TEXT("Count clears after success"), T->GetBagCount(), 0);
	TestTrue(TEXT("Tongs remain held"), Player.Carry->GetHeldObject() == T);
	ATrashBagActor* Bag = nullptr;
	for (TActorIterator<ATrashBagActor> I(World); I; ++I)
	{
		Bag = *I;
	}
	if (!TestNotNull(TEXT("Tied bag exists"), Bag))
	{
		return false;
	}
	TestEqual(TEXT("TRSH-011: immutable bag contents"), Bag->GetLitterCount(), 20);
	TestFalse(TEXT("Count cannot be reinitialized"), Bag->InitializeCount(1));
	TestEqual(TEXT("Bag target name"), Bag->GetBagSummary().ToString(), FString(TEXT("쓰레기봉투 (20개)")));
	TestTrue(TEXT("Bag spawns with no extra velocity"),
			 Bag->GetPhysicalCarryPrimitive()->GetPhysicsLinearVelocity().IsNearlyZero());
	Player.Carry->TryStoreHeldObjectInFixedSlot(Slot);
	TestTrue(TEXT("TRSH-014: E takes tied bag"), Player.Carry->TryTakePhysicalObject(Bag, Failure));
	TestFalse(TEXT("Held bag cannot be world discarded"), Bag->CanDiscardFromWorld(Failure));
	TestTrue(TEXT("G drops bag"), Player.Carry->TryFreeDropHeldObject(FVector::ForwardVector).bSucceeded);
	TestEqual(TEXT("Dropped bag contents preserved"), Bag->GetLitterCount(), 20);
	TestTrue(TEXT("Free bag can be world discarded"), Bag->CanDiscardFromWorld(Failure));
	Player.Interaction->OnInteractionAttemptFinishedNative.Remove(Handle);
	return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCleaningLitterQueryAndOcclusionTest,
								 "BathhouseSim.Cleaning.Litter.QueryAndMopOcclusion",
								 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCleaningLitterQueryAndOcclusionTest::RunTest(const FString&)
{
	FScopedServiceGrid Grid;
	FScopedUtilityLaborWorld Scope(TEXT("LitterOcclusion"));
	auto* World = Scope.Get();
	if (!World)
	{
		return false;
	}
	FPlayer Player;
	BuildPlayer(*this, World, Player);
	Player.Camera->SetWorldLocation(FVector(0, 0, 0));
	auto* Stain = World->SpawnActor<AWaterStainActor>(AWaterStainActor::StaticClass(), FTransform(FVector(150, 0, 0)));
	BeginActorPlayIfNeeded(Stain);
	auto* L = Litter(World, FVector(90, 0, 0));
	FPlayerInteractionContext IC;
	IC.Interactor = Player.Pawn;
	IC.CarryComponent = Player.Carry;
	auto Q = L->QueryInteraction(IC);
	TestTrue(TEXT("TRSH-006: litter target without E"),
			 Q.bVisible && Q.TargetName.ToString() == TEXT("쓰레기") && Q.ActionName.IsEmpty());
	TestEqual(TEXT("TRSH-020: empty hand Apply failure"), Q.HeldApplyFailureReason.ToString(),
			  FString(TEXT("집게가 필요합니다")));
	auto* Kind = MakeKind(TEXT("LitterQueryBox"), TEXT("Box"), 2, 0);
	auto* Item = SpawnBox(World, Kind, 1, FVector(1000, 0, 100));
	FText Failure;
	Player.Carry->TryTakePhysicalObject(Item, Failure);
	TestTrue(TEXT("Non-equipment box still requires tongs"), L->QueryInteraction(IC).bHeldApplyVisible);
	Player.Carry->RecoverHeldPhysicalObject(Item);
	auto* Def = MakeFridgeDefinition();
	auto* FacilityItem =
		APlaceableFacilityItemActor::SpawnFreshItem(*World, *Def, FTransform(FVector(1000, 1000, 100)), Failure);
	if (!TestNotNull(Failure.ToString(), FacilityItem))
	{
		return false;
	}
	FacilityItem->ActivateFreeWorld(FacilityItem->GetActorTransform(), Failure);
	Player.Carry->TryTakePhysicalObject(FacilityItem, Failure);
	TestFalse(TEXT("Facility owns LMB and has no litter Apply hint"), L->QueryInteraction(IC).bHeldApplyVisible);
	Player.Carry->RecoverHeldPhysicalObject(FacilityItem);
	auto* Mop = World->SpawnActor<AWetMopActor>();
	Mesh(Mop);
	BeginActorPlayIfNeeded(Mop);
	Player.Carry->TryTakePhysicalObject(Mop, Failure);
	Player.Interaction->RefreshInteractionQuery();
	FPlayerInteractionContext Focus;
	IPlayerInteractable* Target = nullptr;
	UObject* Object = nullptr;
	TestTrue(TEXT("Aim resolves focused litter"), Player.Interaction->ResolveFocusedInteraction(Focus, Target, Object));
	TestTrue(TEXT("TRSH-026: litter blocks stain trace"), Object == L);
	auto MC = Context(Player, Mop, L);
	TestTrue(TEXT("Mop begins own action over litter"), Mop->BeginEquipmentUse(MC).bSucceeded);
	Mop->UpdateEquipmentUse(MC, .5f);
	TestEqual(TEXT("Blocked stain makes no progress"), Stain->GetCleaningProgress(), 0.f);
	TestTrue(TEXT("TRSH-028: equipment merge keeps mopping own action"),
			 Player.Interaction->GetCurrentInteractionQuery().EquipmentActionName.ToString() == TEXT("물걸레질"));
	L->CommitCollected();
	Player.Interaction->RefreshInteractionQuery();
	Player.Interaction->ResolveFocusedInteraction(Focus, Target, Object);
	TestTrue(TEXT("After litter removal aim reaches stain"), Object == Stain);
	MC.FocusHit = Focus.HitResult;
	Mop->UpdateEquipmentUse(MC, .5f);
	TestTrue(TEXT("Stain advances after occluder removal"), Stain->GetCleaningProgress() > 0);
	return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCleaningLitterInputRoutingTest, "BathhouseSim.Cleaning.Litter.RmbInputRouting",
								 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCleaningLitterInputRoutingTest::RunTest(const FString&)
{
	FScopedUtilityLaborWorld Scope(TEXT("LitterInput"));
	auto* World = Scope.Get();
	if (!World)
	{
		return false;
	}
	auto* Character =
		World->SpawnActor<AFirstPersonCharacter>(AFirstPersonCharacter::StaticClass(), FTransform(FVector(0, 0, 88)));
	BeginActorPlayIfNeeded(Character);
	auto* Controller = World->SpawnActor<APlayerController>();
	BeginActorPlayIfNeeded(Controller);
	Controller->SetPlayer(NewObject<ULocalPlayer>(GEngine));
	Controller->Possess(Character);
	FPlayer Player;
	Player.Pawn = Character;
	Player.Carry = Character->GetPlayerCarry();
	Player.Interaction = Character->GetPlayerInteraction();
	Player.EquipmentUse = Character->GetPlayerEquipmentUse();
	Player.Camera = Character->GetFirstPersonCamera();
	auto* T = Tongs(World);
	FText Failure;
	Player.Carry->TryTakePhysicalObject(T, Failure);
	auto Fill = [&]
	{
		auto* L = Litter(World, FVector(2000, 0, 0));
		T->BeginEquipmentUse(Context(Player, T, L));
	};
	auto* Shelf = World->SpawnActor<ACleanTowelStackActor>();
	BeginActorPlayIfNeeded(Shelf);
	auto* Collision = Box(World, Player.Camera->GetComponentLocation() + FVector(140, 0, 0), FVector(20), Shelf);
	Fill();
	Player.Interaction->RefreshInteractionQuery();
	Character->SecondaryUseStartInput();
	TestEqual(TEXT("RMB owner is equipment secondary over towel shelf"), Character->SecondaryUsePressOwner,
			  AFirstPersonCharacter::ESecondaryUsePressOwner::EquipmentSecondary);
	TestEqual(TEXT("TRSH-022: RMB ties instead of towel Take"), T->GetBagCount(), 0);
	Character->SecondaryUseEndInput();
	int32 Bags = 0;
	for (TActorIterator<ATrashBagActor> I(World); I; ++I)
	{
		++Bags;
	}
	TestEqual(TEXT("Release never creates another bag"), Bags, 1);
	for (TActorIterator<ATrashBagActor> I(World); I; ++I)
	{
		I->Destroy();
	}
	Collision->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	auto* Fridge = World->SpawnActor<AServiceAutomationFridge>();
	Fridge->SetActorLocation(FVector(140, 0, 0));
	auto* FC = Box(World, Player.Camera->GetComponentLocation() + FVector(140, 0, 0), FVector(20), Fridge);
	Fill();
	Player.Interaction->RefreshInteractionQuery();
	Character->SecondaryUseStartInput();
	TestEqual(TEXT("RMB ties regardless of fridge focus"), T->GetBagCount(), 0);
	Character->SecondaryUseEndInput();
	for (TActorIterator<ATrashBagActor> I(World); I; ++I)
	{
		I->Destroy();
	}
	FC->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	auto* L = Litter(World, Player.Camera->GetComponentLocation() + FVector(90, 0, 0));
	Player.Interaction->RefreshInteractionQuery();
	Character->PrimaryUseStartInput();
	TestEqual(TEXT("LMB input collects exactly one litter"), T->GetBagCount(), 1);
	Character->SecondaryUseStartInput();
	TestEqual(TEXT("RMB ignored during LMB equipment input"), Character->SecondaryUsePressOwner,
			  AFirstPersonCharacter::ESecondaryUsePressOwner::Ignored);
	TestEqual(TEXT("RMB does not clear bag during LMB"), T->GetBagCount(), 1);
	Character->SecondaryUseEndInput();
	Character->PrimaryUseTriggeredInput();
	TestEqual(TEXT("Holding LMB collects no repeat"), T->GetBagCount(), 1);
	Character->PrimaryUseEndInput();
	Player.Carry->RecoverHeldPhysicalObject(T);
	auto* Wrench = World->SpawnActor<AMonkeyWrenchActor>();
	Mesh(Wrench);
	BeginActorPlayIfNeeded(Wrench);
	Player.Carry->TryTakePhysicalObject(Wrench, Failure);
	Character->SecondaryUseStartInput();
	TestEqual(TEXT("Wrench RMB keeps ignored behavior"), Character->SecondaryUsePressOwner,
			  AFirstPersonCharacter::ESecondaryUsePressOwner::Ignored);
	Character->SecondaryUseEndInput();
	Player.Carry->RecoverHeldPhysicalObject(Wrench);
	auto* Mop = World->SpawnActor<AWetMopActor>();
	Mesh(Mop);
	BeginActorPlayIfNeeded(Mop);
	Player.Carry->TryTakePhysicalObject(Mop, Failure);
	Character->SecondaryUseStartInput();
	TestEqual(TEXT("Mop RMB keeps ignored behavior"), Character->SecondaryUsePressOwner,
			  AFirstPersonCharacter::ESecondaryUsePressOwner::Ignored);
	Character->SecondaryUseEndInput();
	Player.Carry->RecoverHeldPhysicalObject(Mop);
	auto* Delivery = World->SpawnActor<AShopDeliveryBoxActor>();
	Mesh(Delivery);
	Set<FBoolProperty>(Delivery, TEXT("bContentsInitialized"), true);
	Delivery->ActivateFreeWorld(FTransform(FVector(1500, 1500, 1500)), Failure);
	BeginActorPlayIfNeeded(Delivery);
	Player.Carry->TryTakePhysicalObject(Delivery, Failure);
	Character->SecondaryUseStartInput();
	TestEqual(TEXT("Delivery box RMB keeps ignored behavior"), Character->SecondaryUsePressOwner,
			  AFirstPersonCharacter::ESecondaryUsePressOwner::Ignored);
	Character->SecondaryUseEndInput();
	Player.Carry->RecoverHeldPhysicalObject(Delivery);
	Character->SecondaryUseStartInput();
	TestEqual(TEXT("Empty hand RMB retains held Take owner"), Character->SecondaryUsePressOwner,
			  AFirstPersonCharacter::ESecondaryUsePressOwner::HeldTargetUse);
	Character->SecondaryUseEndInput();
	auto* Kind = MakeKind(TEXT("InputBox"), TEXT("Box"), 2, 0);
	auto* Item = SpawnBox(World, Kind, 1, FVector(2000, 0, 500));
	Player.Carry->TryTakePhysicalObject(Item, Failure);
	Character->SecondaryUseStartInput();
	TestEqual(TEXT("Item box RMB retains Take owner"), Character->SecondaryUsePressOwner,
			  AFirstPersonCharacter::ESecondaryUsePressOwner::HeldTargetUse);
	Character->SecondaryUseEndInput();
	return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCleaningLitterBagScaleTest, "BathhouseSim.Cleaning.Litter.BagScaleAndFrontDrop",
								 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCleaningLitterBagScaleTest::RunTest(const FString&)
{
	FScopedUtilityLaborWorld Scope(TEXT("BagScale"));
	auto* World = Scope.Get();
	if (!World)
	{
		return false;
	}
	FPlayer Player;
	BuildPlayer(*this, World, Player);
	AddCapsule(Player);
	auto* CDO = GetMutableDefault<ATrashBagActor>();
	auto* Root = CDO->GetPhysicalCarryPrimitive();
	const FVector SavedScale = Root->GetRelativeScale3D();
	Root->SetRelativeScale3D(FVector(.3, .4, .5));
	FTransform Transform;
	const TArray<AActor*> Ignored = {Player.Pawn};
	TestTrue(
		TEXT("Front placement uses class physical bounds"),
		FTrashBagDropPlacement::Find(*World, Player.Pawn, Ignored, ATrashBagActor::StaticClass(), TieFloorOnlyRequest(World, Player), Transform));
	TestTrue(TEXT("Feet plus half height and clearance"), FMath::IsNearlyEqual(Transform.GetLocation().Z, 30.0, 0.1));
	auto* Bag = ATrashBagActor::SpawnTiedBag(*World, ATrashBagActor::StaticClass(), 3, Transform);
	Root->SetRelativeScale3D(SavedScale);
	if (!TestNotNull(TEXT("Scaled bag factory succeeds"), Bag))
	{
		return false;
	}
	TestTrue(TEXT("Factory preserves nonuniform CDO root scale"), Bag->GetActorScale3D().Equals(FVector(.3, .4, .5)));
	FText Failure;
	TestTrue(TEXT("Take scaled bag"), Player.Carry->TryTakePhysicalObject(Bag, Failure));
	TestTrue(TEXT("Held bag retains class scale"), Bag->GetActorScale3D().Equals(FVector(.3, .4, .5)));
	TestTrue(TEXT("Drop scaled bag"), Player.Carry->TryFreeDropHeldObject(FVector::ForwardVector).bSucceeded);
	TestTrue(TEXT("Dropped bag retains scale"), Bag->GetActorScale3D().Equals(FVector(.3, .4, .5)));
	Bag->RecoverPhysicalCarryable(nullptr);
	TestEqual(TEXT("Recovery keeps contents"), Bag->GetLitterCount(), 3);
	Bag->Destroy();

	auto* FarWall = Box(World, FVector(109, 0, 55), FVector(5, 200, 55));
	TestTrue(
		TEXT("Blocked 60cm candidate retries a nearer clear drop"),
		FTrashBagDropPlacement::Find(*World, Player.Pawn, Ignored, ATrashBagActor::StaticClass(), TieFloorOnlyRequest(World, Player), Transform));
	TestTrue(TEXT("Nearer search uses a 10cm step"), FMath::IsNearlyEqual(Transform.GetLocation().X, 50.0, .1));
	FarWall->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	auto* Wall = Box(World, FVector(55, 0, 55), FVector(5, 200, 55));

	TestFalse(
		TEXT("Large native bag cannot be placed through wall"),
		FTrashBagDropPlacement::Find(*World, Player.Pawn, Ignored, ATrashBagActor::StaticClass(), TieFloorOnlyRequest(World, Player), Transform));
	Wall->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	const UStaticMesh* SavedMesh = CastChecked<UStaticMeshComponent>(Root)->GetStaticMesh();
	auto* OffsetMesh = DuplicateObject<UStaticMesh>(SavedMesh, GetTransientPackage());
	OffsetMesh->SetExtendedBounds(FBoxSphereBounds(FVector(25, 0, 20), FVector(50), FVector(50).Length()));
	CastChecked<UStaticMeshComponent>(Root)->SetStaticMesh(OffsetMesh);
	TestTrue(
		TEXT("Off-center bag mesh has a valid front drop"),
		FTrashBagDropPlacement::Find(*World, Player.Pawn, Ignored, ATrashBagActor::StaticClass(), TieFloorOnlyRequest(World, Player), Transform));
	FVector QueryCenter;
	FQuat QueryRotation;
	FCollisionShape QueryShape;
	const UPrimitiveComponent* QueryTemplate = nullptr;
	TestTrue(TEXT("Off-center class collision query is valid"),
			 ATrashBagActor::BuildClassCollisionQuery(ATrashBagActor::StaticClass(), Transform, QueryCenter,
													  QueryRotation, QueryShape, QueryTemplate, Failure));
	TestTrue(TEXT("Front drop positions bounds center at forward/half-height clearance"),
			 QueryCenter.Equals(FVector(60, 0, 55), .1));
	CastChecked<UStaticMeshComponent>(Root)->SetStaticMesh(const_cast<UStaticMesh*>(SavedMesh));
	Player.Camera->SetWorldRotation(FRotator(90, 0, 0));

	ETrashBagDropStage VerticalStage = ETrashBagDropStage::None;
	TestTrue(
		TEXT("Vertical camera places in front of the camera (no horizontal floor stage needed)"),
		FTrashBagDropPlacement::Find(*World, Player.Pawn, Ignored, ATrashBagActor::StaticClass(),
									 TieRequest(World, Player), Transform, &VerticalStage));
	TestTrue(TEXT("Vertical camera uses the view-front stage"), VerticalStage == ETrashBagDropStage::ViewFront);

	FDataValidationContext ValidBagContext;
	TestTrue(TEXT("Native bag validates like item box"),
			 CDO->IsDataValid(ValidBagContext) == EDataValidationResult::Valid);
	Root->SetRelativeScale3D(FVector(-1, 1, 1));
	FDataValidationContext InvalidBagContext;
	TestTrue(TEXT("Invalid bag root scale rejected"),
			 CDO->IsDataValid(InvalidBagContext) == EDataValidationResult::Invalid);
	Root->SetRelativeScale3D(SavedScale);
	auto* L = Litter(World, FVector(1000, 0, 0));
	FDataValidationContext ValidLitterContext;
	TestTrue(TEXT("Litter with valid mesh variants validates"),
			 L->IsDataValid(ValidLitterContext) == EDataValidationResult::Valid);
	Set<FFloatProperty>(L, TEXT("FloorRadiusCm"), 0.f);
	FDataValidationContext InvalidRadiusContext;
	TestTrue(TEXT("Litter nonpositive radius rejected"),
			 L->IsDataValid(InvalidRadiusContext) == EDataValidationResult::Invalid);
	auto* Empty = World->SpawnActor<ALitterActor>();
	FDataValidationContext InvalidMeshContext;
	TestTrue(TEXT("Missing litter mesh variants rejected"),
			 Empty->IsDataValid(InvalidMeshContext) == EDataValidationResult::Invalid);
	return true;
}
#endif
