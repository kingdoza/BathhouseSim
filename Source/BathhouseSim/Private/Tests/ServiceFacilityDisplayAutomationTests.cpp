#if WITH_DEV_AUTOMATION_TESTS
#include "Tests/ServiceFacilityAutomationTestSupport.h"
#include "Service/ServiceItemTransfer.h"
#include "Components/InstancedStaticMeshComponent.h"

using namespace ServiceFacilityTest;
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FBathhouseServiceFacilityGroupsTest,
								 "BathhouseSim.Service.Facility.GroupsAndConsumption",
								 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FBathhouseServiceFacilityGroupsTest::RunTest(const FString& Parameters)
{
	FScopedUtilityLaborWorld Scope(TEXT("FacilityGroupsWorld"));
	ServiceFacilityTest::FFixture Fixture(Scope.Get());
	if (!TestTrue(TEXT("Vanity installs through conversion"), Fixture.Install(*this)))
	{
		return false;
	}
	auto* Facility = Fixture.Facility;
	auto Spaces = Facility->GetSpaces();
	auto* Router = Facility->GetRouter();
	FText Failure;
	TestEqual(TEXT("Vanity has four groups"), Spaces.Num(), 4);
	TestEqual(TEXT("Vanity exposes twenty-second use"), Facility->GetManager()->GetCustomerUseSeconds(), 20.f);
	TestTrue(TEXT("Fresh routed stock is empty"), Facility->GetManager()->GetTotalStock() == 0);
	auto* Box = SpawnBox(Fixture.World, Fixture.Kinds[1], 6);
	if (!TestTrue(TEXT("Lotion box is held"), Fixture.Player.Carry->TryTakePhysicalObject(Box, Failure)))
	{
		return false;
	}
	auto Context = Fixture.Context(3);
	auto Query = Router->QueryInteraction(Context);
	TestEqual(TEXT("Nonempty box selects kind despite comb aim"), Query.HeldUseTargetKey, 1);
	for (int32 Index = 0; Index < 4; ++Index)
	{
		TestTrue(TEXT("Apply fills only lotion"),
				 Router->ExecuteHeldTargetUse(Context, EPlayerHeldTargetUseDirection::Apply).bSucceeded);
	}
	TestEqual(TEXT("Lotion capacity denial"), Router->QueryInteraction(Context).HeldApplyFailureReason.ToString(),
			  FString(TEXT("가득 참")));
	TestEqual(TEXT("Comb remains empty"), Spaces[3]->GetStock().Count, 0);
	TestFalse(TEXT("E has no action"), Router->ExecuteInteraction(Context).bSucceeded);
	if (!TestEqual(TEXT("Vanity BeginPlay binds one slot"), Facility->GetFacilitySlots().Num(), 1))
	{
		return false;
	}
	auto* Slot = Facility->GetFacilitySlots()[0].Get();
	auto* User = Fixture.World->SpawnActor<AActor>();
	BeginActorPlayIfNeeded(User);
	TestTrue(TEXT("Reservation succeeds without stock consumption"), Slot->TryReserve(User));
	TestEqual(TEXT("Reservation has not consumed"), Spaces[1]->GetStock().InUseRemaining, 0);
	TestTrue(TEXT("Occupied starts a consumable"), Slot->BeginUse(User));
	TestEqual(TEXT("One use deducted"), Spaces[1]->GetStock().InUseRemaining, 9);
	TestTrue(TEXT("Duplicate BeginUse is idempotent"), Slot->BeginUse(User));
	TestEqual(TEXT("No duplicate deduction"), Spaces[1]->GetStock().InUseRemaining, 9);
	TestFalse(TEXT("Occupied facility cannot recover"), Facility->QueryFacilityRecovery().bSucceeded);
	Query = Router->QueryInteraction(Context);
	TestTrue(TEXT("HUD shows fresh and used portions"),
			 Query.TargetName.ToString().Contains(TEXT("새것 3 + 사용 중 9/10회")));
	TestTrue(TEXT("Player can take fresh stock during customer use"),
			 Router->ExecuteHeldTargetUse(Context, EPlayerHeldTargetUseDirection::Take).bSucceeded);
	TestEqual(TEXT("Fresh LIFO take preserves front use"), Spaces[1]->GetStock().InUseRemaining, 9);
	TestTrue(TEXT("New stock can be inserted during use"),
			 Router->ExecuteHeldTargetUse(Context, EPlayerHeldTargetUseDirection::Apply).bSucceeded);
	for (int32 Index = 0; Index < 9; ++Index)
	{
		Slot->EndUse(User);
		Slot->Release(User);
		Slot->TryReserve(User);
		Slot->BeginUse(User);
	}
	TestEqual(TEXT("Ten starts remove front item"), Spaces[1]->GetStock().Count, 3);
	TestEqual(TEXT("Depletion clears remaining"), Spaces[1]->GetStock().InUseRemaining, 0);
	Slot->EndUse(User);
	Slot->Release(User);
	TestTrue(TEXT("Reusable stock imports"), Spaces[0]->ImportStock(Fixture.Kinds[0], 2, Failure));
	TestTrue(TEXT("Only-used stock imports"), Spaces[1]->ImportStock(Fixture.Kinds[1], 1, Failure, 7));
	// EBT-004: an empty box skips groups with only in-use stock and targets the takeable dryer group.
	StowBox(Fixture.Player, Box);
	auto* EmptyBox = SpawnBox(Fixture.World, nullptr, 0);
	BeginActorPlayIfNeeded(EmptyBox);
	TestTrue(TEXT("Empty box held"), Fixture.Player.Carry->TryTakePhysicalObject(EmptyBox, Failure));
	for (const int32 AimIndex : {1, 2})
	{
		Query = Router->QueryInteraction(Fixture.Context(AimIndex));
		TestEqual(TEXT("Takeable dryer group is targeted"), Query.HeldUseTargetKey, 0);
		TestTrue(TEXT("Take is possible"), Query.bCanHeldTake);
		TestTrue(TEXT("No take denial"), Query.HeldTakeFailureReason.IsEmpty());
	}
	Slot->TryReserve(User);
	Slot->BeginUse(User);
	TestEqual(TEXT("Reusable dryer remains"), Spaces[0]->GetStock().Count, 2);
	TestEqual(TEXT("Empty swab remains empty"), Spaces[2]->GetStock().Count, 0);
	User->Destroy();
	TestEqual(TEXT("User EndPlay frees occupied slot"), Slot->GetSlotState(), EBathhouseFacilitySlotState::Available);
	TestEqual(TEXT("EndPlay does not refund consumption"), Spaces[1]->GetStock().InUseRemaining, 6);
	StowBox(Fixture.Player, EmptyBox);
	auto* Foreign = SpawnBox(Fixture.World, MakeKind(TEXT("Foreign"), TEXT("외부"), 2, 0), 1);
	Fixture.Player.Carry->TryTakePhysicalObject(Foreign, Failure);
	Query = Router->QueryInteraction(Fixture.Context());
	TestEqual(TEXT("Foreign apply rejected"), Query.HeldApplyFailureReason.ToString(),
			  FString(TEXT("여기에 넣을 수 없는 물건")));
	TestEqual(TEXT("Foreign take rejected"), Query.HeldTakeFailureReason.ToString(),
			  FString(TEXT("여기에 넣을 수 없는 물건")));
	TArray<const UDisplaySpaceComponent*> Layout;
	for (auto* Space : Spaces)
	{
		Layout.Add(Space);
	}
	TestFalse(TEXT("Routed groups need exactly one router"),
			  UServiceDisplayManagerComponent::ValidateSpaceLayout(Layout, 1, 1, 0, Failure));
	static_cast<UServiceAutomationDisplaySpace*>(Spaces[3])->ConfigureRoutedForTest(3, Fixture.Kinds[0], 6);
	TestFalse(TEXT("Duplicate fixed kind rejected"),
			  UServiceDisplayManagerComponent::ValidateSpaceLayout(Layout, 1, 1, 1, Failure));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FBathhouseServiceRouterRepeatTest,
								 "BathhouseSim.Service.Facility.RouterRepeatAndKeyGuard",
								 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FBathhouseServiceRouterRepeatTest::RunTest(const FString& Parameters)
{
	FScopedUtilityLaborWorld Scope(TEXT("RouterRepeatWorld"));
	ServiceFacilityTest::FFixture Fixture(Scope.Get());
	if (!Fixture.Install(*this))
	{
		return false;
	}
	auto Spaces = Fixture.Facility->GetSpaces();
	FText Failure;
	Spaces[0]->ImportStock(Fixture.Kinds[0], 2, Failure);
	Spaces[3]->ImportStock(Fixture.Kinds[3], 3, Failure);
	auto* Box = SpawnBox(Fixture.World, nullptr, 0);
	BeginActorPlayIfNeeded(Box);
	if (!TestTrue(TEXT("Empty box held"), Fixture.Player.Carry->TryTakePhysicalObject(Box, Failure)))
	{
		return false;
	}
	Fixture.AimAt(0);
	FHitResult Hit;
	if (!TestTrue(TEXT("Actual trace hits router"), Fixture.Player.Interaction->GetCurrentFocusHit(Hit) &&
														Hit.GetComponent() == Fixture.Facility->GetRouter()))
	{
		return false;
	}
	Fixture.Player.HeldUse->BeginUse(EPlayerHeldTargetUseDirection::Take);
	TestEqual(TEXT("First RMB takes dryer"), Box->GetContents().Count, 1);
	Fixture.AimAt(3);
	TestEqual(TEXT("Changing aim keeps dryer key"),
			  Fixture.Facility->GetRouter()->QueryInteraction(Fixture.Context(3)).HeldUseTargetKey, 0);
	TickWorldForDuration(Fixture.World, 0.3, 0.05f);
	TestEqual(TEXT("DISP-024 continues dryer only"), Box->GetContents().Count, 2);
	TestEqual(TEXT("Comb unchanged"), Spaces[3]->GetStock().Count, 3);
	TestEqual(TEXT("Dryer group emptied"), Spaces[0]->GetStock().Count, 0);
	Spaces[0]->ImportStock(Fixture.Kinds[0], 2, Failure);
	TickWorldForDuration(Fixture.World, 0.3, 0.05f);
	TestEqual(TEXT("Stopped repetition cannot resume on refill"), Box->GetContents().Count, 2);
	Fixture.Player.HeldUse->EndUse();
	// A full box stops before removing anything.
	Fixture.Player.HeldUse->BeginUse(EPlayerHeldTargetUseDirection::Take);
	TickWorldForDuration(Fixture.World, 0.3, 0.05f);
	TestEqual(TEXT("Full box leaves stock"), Spaces[0]->GetStock().Count, 2);
	Fixture.Player.HeldUse->EndUse();
	StowBox(Fixture.Player, Box);
	auto* Empty = SpawnBox(Fixture.World, nullptr, 0);
	BeginActorPlayIfNeeded(Empty);
	Fixture.Player.Carry->TryTakePhysicalObject(Empty, Failure);
	Fixture.AimAt(0);
	Fixture.Player.HeldUse->BeginUse(EPlayerHeldTargetUseDirection::Take);
	Fixture.Player.Camera->SetWorldRotation(FRotator(0, 180, 0));
	Fixture.Player.Interaction->RefreshInteractionQuery();
	TickWorldForDuration(Fixture.World, 0.3, 0.05f);
	Fixture.AimAt(0);
	TickWorldForDuration(Fixture.World, 0.3, 0.05f);
	TestEqual(TEXT("Leaving router stops until release"), Empty->GetContents().Count, 1);
	Fixture.Player.HeldUse->EndUse();
	// Independent guard: actual focused target UObject stays the same while only its query key changes.
	auto* TargetOwner = Fixture.World->SpawnActor<AActor>();
	BeginActorPlayIfNeeded(TargetOwner);
	auto* Target = NewObject<UServiceAutomationKeyTarget>(TargetOwner);
	TargetOwner->SetRootComponent(Target);
	TargetOwner->AddInstanceComponent(Target);
	Target->RegisterComponent();
	Fixture.Player.Camera->SetWorldLocationAndRotation(FVector::ZeroVector, FRotator::ZeroRotator);
	TargetOwner->SetActorLocation(FVector(120, 0, 0));
	Fixture.Player.Interaction->RefreshInteractionQuery();
	Fixture.Player.HeldUse->BeginUse(EPlayerHeldTargetUseDirection::Take);
	TestEqual(TEXT("Guard fixture starts"), Target->Executions, 1);
	Target->Key = 1;
	TickWorldForDuration(Fixture.World, 0.3, 0.05f);
	TestEqual(TEXT("Key change stops same object"), Target->Executions, 1);
	Target->Key = 0;
	Fixture.Player.HeldUse->BeginUse(EPlayerHeldTargetUseDirection::Take);
	TickWorldForDuration(Fixture.World, 0.3, 0.05f);
	TestEqual(TEXT("Same press cannot resume"), Target->Executions, 1);
	Fixture.Player.HeldUse->EndUse();
	Fixture.Player.HeldUse->BeginUse(EPlayerHeldTargetUseDirection::Take);
	TestEqual(TEXT("Release permits next press"), Target->Executions, 2);
	Fixture.Player.HeldUse->EndUse();
	FPlayerInteractionQuery A, B;
	B.HeldUseTargetKey = 1;
	TestFalse(TEXT("Query equality includes key"), A.Equals(B));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FBathhouseServiceShowerUseTest,
								 "BathhouseSim.Service.Facility.ShowerSharedConsumption",
								 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FBathhouseServiceShowerUseTest::RunTest(const FString& Parameters)
{
	FScopedUtilityLaborWorld Scope(TEXT("ShowerConsumptionWorld"));
	ServiceFacilityTest::FFixture Fixture(Scope.Get());
	if (!Fixture.Install(*this, true))
	{
		return false;
	}
	auto Spaces = Fixture.Facility->GetSpaces();
	auto Slots = Fixture.Facility->GetFacilitySlots();
	if (!TestEqual(TEXT("Shower BeginPlay binds two slots"), Slots.Num(), 2))
	{
		return false;
	}
	FText Failure;
	Spaces[0]->ImportStock(Fixture.Kinds[0], 2, Failure, 2);
	Spaces[1]->ImportStock(Fixture.Kinds[1], 1, Failure);
	auto* A = Fixture.World->SpawnActor<AActor>();
	BeginActorPlayIfNeeded(A);
	auto* B = Fixture.World->SpawnActor<AActor>();
	BeginActorPlayIfNeeded(B);
	Slots[0]->TryReserve(A);
	Slots[1]->TryReserve(B);
	TestEqual(TEXT("Two reservations do not consume"), Spaces[0]->GetStock().InUseRemaining, 2);
	Slots[0]->BeginUse(A);
	Slots[1]->BeginUse(B);
	TestEqual(TEXT("Two users deplete the same front shampoo"), Spaces[0]->GetStock().Count, 1);
	TestEqual(TEXT("Depleted shampoo shifts and clears remaining"), Spaces[0]->GetStock().InUseRemaining, 0);
	TestEqual(TEXT("Bodywash deducts twice"), Spaces[1]->GetStock().InUseRemaining, 28);
	A->Destroy();
	B->Destroy();
	TestTrue(TEXT("Both user slots release"), Slots[0]->IsAvailable() && Slots[1]->IsAvailable());
	TestTrue(TEXT("Supplies absent is valid"),
			 Spaces[0]->ImportStock(nullptr, 0, Failure) && Spaces[1]->ImportStock(nullptr, 0, Failure));
	auto* C = Fixture.World->SpawnActor<AActor>();
	BeginActorPlayIfNeeded(C);
	TestTrue(TEXT("Empty shower can be used"), Slots[0]->TryReserve(C) && Slots[0]->BeginUse(C));
	TestEqual(TEXT("Empty use keeps zero stock"), Fixture.Facility->GetManager()->GetTotalStock(), 0);
	return true;
}
#endif
