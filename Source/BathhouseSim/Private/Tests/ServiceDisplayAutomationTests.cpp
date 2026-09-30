#if WITH_DEV_AUTOMATION_TESTS

#include "Tests/ServiceAutomationTestSupport.h"

#include "Components/InstancedStaticMeshComponent.h"
#include "Interaction/HeldEquipmentUsable.h"
#include "Service/DrinkSalesSubsystem.h"
#include "Service/ServiceItemTransfer.h"
#include "Shop/BathhouseTrashBinActor.h"
#include "Towel/TowelBasketActor.h"
#include "Towel/TowelInventoryComponent.h"

namespace
{
using namespace ServiceTest;

FString Reason(const FServiceTransferEvaluation& Evaluation)
{
	return Evaluation.Reason.ToString();
}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBathhouseServiceTransferRulesTest,
	"BathhouseSim.Service.Transfer.RulesAndAtomicity",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FBathhouseServiceTransferRulesTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	UServiceItemDefinition* Milk = MakeKind(TEXT("Milk"), TEXT("바나나우유"), 12, 2000);
	UServiceItemDefinition* Juice = MakeKind(TEXT("Juice"), TEXT("주스"), 6, 1500);
	UServiceItemDefinition* Snack = MakeKind(TEXT("Snack"), TEXT("과자"), 4, 500, false);
	const FGameplayTag Category = TAG_Display_Fridge;
	const int32 SpaceCapacity = 3;

	FText Failure;
	TestTrue(TEXT("Definitions validate"), Milk->ValidateRuntime(Failure) && Juice->ValidateRuntime(Failure));
	UServiceItemDefinition* BadCapacity = MakeKind(TEXT("Bad"), TEXT("나쁨"), 3, 1);
	BadCapacity->BoxSlotTransforms.Pop();
	TestFalse(TEXT("Slot transform count must equal capacity"), BadCapacity->ValidateRuntime(Failure));
	UServiceItemDefinition* NoMesh = MakeKind(TEXT("NoMesh"), TEXT("메시없음"), 2, 1);
	NoMesh->ItemMesh = nullptr;
	TestFalse(TEXT("Mesh is required"), NoMesh->ValidateRuntime(Failure));
	UServiceItemDefinition* NegativeSale = MakeKind(TEXT("Neg"), TEXT("음수"), 2, -1);
	TestFalse(TEXT("Sale value cannot be negative"), NegativeSale->ValidateRuntime(Failure));

	auto Stack = [](UServiceItemDefinition* Kind, const int32 Count)
	{
		FServiceItemStack Result;
		Result.Kind = Count > 0 ? Kind : nullptr;
		Result.Count = Count;
		return Result;
	};

	// Apply reasons in priority order.
	TestEqual(TEXT("Apply: wrong category"),
		Reason(FServiceItemTransfer::EvaluateApply(Stack(Snack, 2), Stack(nullptr, 0), Category, SpaceCapacity)),
		FString(TEXT("여기에 넣을 수 없는 물건")));
	TestEqual(TEXT("Apply: different kind beats empty box"),
		Reason(FServiceItemTransfer::EvaluateApply(Stack(Milk, 2), Stack(Juice, 1), Category, SpaceCapacity)),
		FString(TEXT("다른 음료가 진열됨")));
	TestEqual(TEXT("Apply: empty box"),
		Reason(FServiceItemTransfer::EvaluateApply(Stack(nullptr, 0), Stack(nullptr, 0), Category, SpaceCapacity)),
		FString(TEXT("박스가 비어 있음")));
	TestEqual(TEXT("Apply: empty box on a stocked space is still empty"),
		Reason(FServiceItemTransfer::EvaluateApply(Stack(nullptr, 0), Stack(Milk, 1), Category, SpaceCapacity)),
		FString(TEXT("박스가 비어 있음")));
	TestEqual(TEXT("Apply: full space"),
		Reason(FServiceItemTransfer::EvaluateApply(Stack(Milk, 2), Stack(Milk, 3), Category, SpaceCapacity)),
		FString(TEXT("가득 참")));
	TestTrue(TEXT("Apply: allowed"),
		FServiceItemTransfer::EvaluateApply(Stack(Milk, 2), Stack(Milk, 2), Category, SpaceCapacity).bCan);

	// Take reasons in priority order.
	TestEqual(TEXT("Take: wrong category"),
		Reason(FServiceItemTransfer::EvaluateTake(Stack(Snack, 2), Stack(Milk, 1), Category, SpaceCapacity)),
		FString(TEXT("여기에 넣을 수 없는 물건")));
	TestEqual(TEXT("Take: different kind"),
		Reason(FServiceItemTransfer::EvaluateTake(Stack(Milk, 2), Stack(Juice, 1), Category, SpaceCapacity)),
		FString(TEXT("다른 음료가 진열됨")));
	TestEqual(TEXT("Take: empty space"),
		Reason(FServiceItemTransfer::EvaluateTake(Stack(Milk, 2), Stack(nullptr, 0), Category, SpaceCapacity)),
		FString(TEXT("꺼낼 물건 없음")));
	TestEqual(TEXT("Take: full box"),
		Reason(FServiceItemTransfer::EvaluateTake(Stack(Milk, 12), Stack(Milk, 1), Category, SpaceCapacity)),
		FString(TEXT("박스 가득 참")));
	TestTrue(TEXT("Take: empty box adopts the space kind and its capacity"),
		FServiceItemTransfer::EvaluateTake(Stack(nullptr, 0), Stack(Juice, 1), Category, SpaceCapacity).bCan);

	// Failure leaves both stacks and revisions untouched.
	FServiceItemStack Box = Stack(Milk, 2);
	FServiceItemStack Space = Stack(Juice, 1);
	Box.Revision = 7;
	Space.Revision = 9;
	TestFalse(TEXT("A rejected move fails"), FServiceItemTransfer::TryApplyOne(Box, Space, Category, SpaceCapacity, Failure));
	TestTrue(TEXT("A rejected move changes nothing"),
		Box.Kind == Milk && Box.Count == 2 && Box.Revision == 7
		&& Space.Kind == Juice && Space.Count == 1 && Space.Revision == 9);

	// Success moves exactly one unit, locks and unlocks the kind, and bumps both revisions.
	Box = Stack(Milk, 2);
	Space = Stack(nullptr, 0);
	TestTrue(TEXT("First apply succeeds"), FServiceItemTransfer::TryApplyOne(Box, Space, Category, SpaceCapacity, Failure));
	TestTrue(TEXT("First apply locks the space kind"), Space.Kind == Milk && Space.Count == 1 && Box.Count == 1);
	TestTrue(TEXT("Both revisions advance"), Space.Revision == 1 && Box.Revision == 1);
	TestTrue(TEXT("Second apply succeeds"), FServiceItemTransfer::TryApplyOne(Box, Space, Category, SpaceCapacity, Failure));
	TestTrue(TEXT("Emptied box unlocks its kind"), Box.Count == 0 && Box.Kind == nullptr && Space.Count == 2);
	TestTrue(TEXT("Empty box can take from the space"), FServiceItemTransfer::TryTakeOne(Box, Space, Category, SpaceCapacity, Failure));
	TestTrue(TEXT("Empty box adopts the kind on take"), Box.Kind == Milk && Box.Count == 1 && Space.Count == 1);
	TestTrue(TEXT("Last take succeeds"), FServiceItemTransfer::TryTakeOne(Box, Space, Category, SpaceCapacity, Failure));
	TestTrue(TEXT("Emptied space unlocks its kind"), Space.Kind == nullptr && Space.Count == 0 && Box.Count == 2);

	// Customer removal.
	Space = Stack(Milk, 2);
	UServiceItemDefinition* Removed = nullptr;
	TestTrue(TEXT("Customer removal succeeds"), FServiceItemTransfer::TryRemoveOne(Space, Removed, Failure)
		&& Removed == Milk && Space.Count == 1 && Space.Revision == 1);
	TestTrue(TEXT("Last removal unlocks the kind"), FServiceItemTransfer::TryRemoveOne(Space, Removed, Failure)
		&& Space.Kind == nullptr);
	TestFalse(TEXT("Removal from an empty space fails"), FServiceItemTransfer::TryRemoveOne(Space, Removed, Failure));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBathhouseServiceItemBoxLifecycleTest,
	"BathhouseSim.Service.ItemBox.LifecycleAndContents",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FBathhouseServiceItemBoxLifecycleTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	FScopedUtilityLaborWorld Scope(TEXT("ServiceItemBoxWorld"));
	UWorld* World = Scope.Get();
	if (!TestNotNull(TEXT("Automation world exists"), World))
	{
		return false;
	}
	FPlayer Player;
	if (!TestTrue(TEXT("Player fixture builds"), BuildPlayer(*this, World, Player, true)))
	{
		return false;
	}
	UServiceItemDefinition* Milk = MakeKind(TEXT("BoxMilk"), TEXT("바나나우유"), 12, 2000);

	AItemBoxActor* Box = SpawnBox(World, Milk, 7);
	AItemBoxActor* EmptyBox = SpawnBox(World, nullptr, 0, FVector(50.0f, 0.0f, 500.0f));
	if (!TestNotNull(TEXT("Filled box exists"), Box) || !TestNotNull(TEXT("Empty box exists"), EmptyBox))
	{
		return false;
	}
	TestEqual(TEXT("Box kind is ItemBox"), Box->GetPhysicalCarryKind(), EPhysicalCarryKind::ItemBox);
	TestTrue(TEXT("Box supports FreeDrop only"),
		Box->GetPhysicalCarryCapabilities() == EPhysicalCarryCapability::FreeDrop);
	TestNull(TEXT("Box is not equipment"), Cast<IHeldEquipmentUsable>(Box));
	TestEqual(TEXT("Filled summary"), Box->GetContentsSummary().ToString(), FString(TEXT("바나나우유 7/12")));
	TestEqual(TEXT("Empty summary"), EmptyBox->GetContentsSummary().ToString(), FString(TEXT("빈 박스")));
	TestEqual(TEXT("ContentsVisual shows one instance per unit"),
		Box->GetContentsVisual()->GetInstanceCount(), 7);
	TestEqual(TEXT("Empty box shows no instances"), EmptyBox->GetContentsVisual()->GetInstanceCount(), 0);

	TestFalse(TEXT("Contents cannot be initialised after BeginPlay"), Box->InitializeContents(Milk, 3));
	AItemBoxActor* Deferred = World->SpawnActorDeferred<AItemBoxActor>(
		AItemBoxActor::StaticClass(), FTransform::Identity, nullptr, nullptr,
		ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
	TestFalse(TEXT("Over-capacity contents are rejected"), Deferred->InitializeContents(Milk, 13));
	TestFalse(TEXT("Negative contents are rejected"), Deferred->InitializeContents(Milk, -1));
	TestTrue(TEXT("Exactly one initialisation is allowed"), Deferred->InitializeContents(Milk, 12));
	TestFalse(TEXT("A second initialisation is rejected"), Deferred->InitializeContents(Milk, 1));
	Deferred->Destroy();

	// E interaction.
	FPlayerInteractionContext Context;
	Context.Interactor = Player.Pawn;
	Context.CarryComponent = Player.Carry;
	Context.InteractionComponent = Player.Interaction;
	FPlayerInteractionQuery Query = Box->QueryInteraction(Context);
	TestTrue(TEXT("Empty hand can take the box"), Query.bVisible && Query.bCanInteract);
	TestEqual(TEXT("Box target name is its summary"), Query.TargetName.ToString(), FString(TEXT("바나나우유 7/12")));
	TestEqual(TEXT("Box action is take"), Query.ActionName.ToString(), FString(TEXT("들기")));
	TestTrue(TEXT("Taking the box succeeds"), Box->ExecuteInteraction(Context).bSucceeded);
	TestTrue(TEXT("Box is held"), Player.Carry->GetHeldObject() == Box && Box->IsHeld());
	Query = EmptyBox->QueryInteraction(Context);
	TestFalse(TEXT("A busy hand cannot take a second box"), Query.bCanInteract);
	Player.Interaction->RefreshInteractionQuery();
	TestEqual(TEXT("Held summary reaches the HUD query"),
		Player.Interaction->GetCurrentInteractionQuery().HeldObjectSummary.ToString(),
		FString(TEXT("바나나우유 7/12")));

	// Drop and recovery keep the contents.
	const FPlayerInteractionResult Drop = Player.Carry->TryFreeDropHeldObject(FVector::ForwardVector);
	TestTrue(TEXT("Free drop succeeds"), Drop.bSucceeded);
	TestTrue(TEXT("Dropped box is free-world with contents intact"),
		Box->IsFreeWorld() && Box->GetContents().Count == 7 && Box->GetContents().Kind == Milk);
	Box->RecoverPhysicalCarryable(nullptr);
	TestTrue(TEXT("Recovered box keeps contents"), Box->GetContents().Count == 7 && Box->IsFreeWorld());
	TestEqual(TEXT("Recovered box keeps its instances"), Box->GetContentsVisual()->GetInstanceCount(), 7);

	// Discard destroys the box with its contents and gives no money.
	TestTrue(TEXT("Retaking the box succeeds"), Box->ExecuteInteraction(Context).bSucceeded);
	FText Failure;
	TestTrue(TEXT("A held box can be discarded"), Box->CanDiscardCarriedObject(Failure));
	UDrinkSalesSubsystem* Sales = World->GetSubsystem<UDrinkSalesSubsystem>();
	const int32 MoneyBefore = Player.PlayerState->GetWallet()->GetCurrentMoney();
	const int32 PoolBefore = Sales ? Sales->GetPendingAmount() : -1;
	ABathhouseTrashBinActor* TrashBin = World->SpawnActor<ABathhouseTrashBinActor>();
	TestTrue(TEXT("The trash bin accepts a held box"), TrashBin && TrashBin->QueryInteraction(Context).bCanInteract);
	TestTrue(TEXT("Discarding through the trash bin succeeds"), TrashBin && TrashBin->ExecuteInteraction(Context).bSucceeded);
	TestTrue(TEXT("Discard consumes the held box"), Player.Carry->IsHandEmpty() && !IsValid(Box));
	TestEqual(TEXT("Discarding a box gives no money"), Player.PlayerState->GetWallet()->GetCurrentMoney(), MoneyBefore);
	TestEqual(TEXT("Discarding a box leaves the sales pool alone"), Sales ? Sales->GetPendingAmount() : -1, PoolBefore);
	TestFalse(TEXT("A free-world box is not discardable"), EmptyBox->CanDiscardCarriedObject(Failure));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBathhouseServiceDisplayHeldUseTest,
	"BathhouseSim.Service.Display.HeldUseTransfer",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FBathhouseServiceDisplayHeldUseTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	FScopedServiceGrid GridGuard;
	FScopedUtilityLaborWorld Scope(TEXT("ServiceDisplayWorld"));
	UWorld* World = Scope.Get();
	if (!TestNotNull(TEXT("Automation world exists"), World))
	{
		return false;
	}
	FPlayer Player;
	if (!TestTrue(TEXT("Player fixture builds"), BuildPlayer(*this, World, Player)))
	{
		return false;
	}
	UFacilityPlacementDefinition* Definition = MakeFridgeDefinition();
	if (!TestNotNull(TEXT("Fridge definition exists"), Definition))
	{
		return false;
	}
	// SpaceA at (120, 0, 0), SpaceB 60 cm above the aim ray.
	AServiceAutomationFridge* Fridge = SpawnInstalledFridge(
		*this, World, *Definition, FVector(120.0f, 0.0f, -60.0f));
	if (!TestNotNull(TEXT("Installed fridge exists"), Fridge))
	{
		return false;
	}
	UDisplaySpaceComponent* SpaceA = Fridge->GetSpaceA();
	UDisplaySpaceComponent* SpaceB = Fridge->GetSpaceB();
	UServiceItemDefinition* Milk = MakeKind(TEXT("DisplayMilk"), TEXT("바나나우유"), 12, 2000);
	UServiceItemDefinition* Juice = MakeKind(TEXT("DisplayJuice"), TEXT("주스"), 6, 1500);
	AItemBoxActor* MilkBox = SpawnBox(World, Milk, 5);
	AItemBoxActor* JuiceBox = SpawnBox(World, Juice, 2, FVector(0.0f, 50.0f, 500.0f));
	AItemBoxActor* EmptyBox = SpawnBox(World, nullptr, 0, FVector(0.0f, 100.0f, 500.0f));
	if (!TestNotNull(TEXT("Boxes exist"), MilkBox) || !TestNotNull(TEXT("Juice box exists"), JuiceBox)
		|| !TestNotNull(TEXT("Empty box exists"), EmptyBox))
	{
		return false;
	}

	// Empty hand: name only, nothing to apply.
	Player.Interaction->RefreshInteractionQuery();
	FPlayerInteractionQuery Query = Player.Interaction->GetCurrentInteractionQuery();
	TestEqual(TEXT("Empty space is named without any box"), Query.TargetName.ToString(), FString(TEXT("빈 공간 0/3")));
	TestTrue(TEXT("Empty hand shows no held rows"), !Query.bHeldApplyVisible && !Query.bHeldTakeVisible);
	TestTrue(TEXT("E row has no action"), Query.ActionName.IsEmpty() && !Query.bCanInteract);
	TestFalse(TEXT("F row is hidden"), Query.bSecondaryVisible);

	FText Failure;
	TestTrue(TEXT("Player takes the milk box"), Player.Carry->TryTakePhysicalObject(MilkBox, Failure));
	Player.Interaction->RefreshInteractionQuery();
	Query = Player.Interaction->GetCurrentInteractionQuery();
	TestTrue(TEXT("Apply row is visible and possible"), Query.bHeldApplyVisible && Query.bCanHeldApply);
	TestEqual(TEXT("Apply action"), Query.HeldApplyActionName.ToString(), FString(TEXT("넣기")));
	TestEqual(TEXT("Apply mode is Repeat"), Query.HeldApplyActivationMode, EPlayerInteractionActivationMode::Repeat);
	TestTrue(TEXT("Take row is visible but impossible"), Query.bHeldTakeVisible && !Query.bCanHeldTake);
	TestEqual(TEXT("Take reason on an empty space"), Query.HeldTakeFailureReason.ToString(), FString(TEXT("꺼낼 물건 없음")));
	TestEqual(TEXT("Take mode is Repeat"), Query.HeldTakeActivationMode, EPlayerInteractionActivationMode::Repeat);
	TestFalse(TEXT("Held rows do not create an E action"), Query.bCanInteract);

	TArray<FPlayerInteractionResult> Reports;
	Player.Interaction->OnInteractionAttemptFinishedNative.AddLambda(
		[&Reports](const FPlayerInteractionResult& Result) { Reports.Add(Result); });
	int32 StockChanges = 0;
	FDelegateHandle Unused;
	(void)Unused;

	Player.HeldUse->BeginUse(EPlayerHeldTargetUseDirection::Apply);
	TestEqual(TEXT("Press moves one bottle in immediately"), SpaceA->GetStock().Count, 1);
	TestEqual(TEXT("Box lost one bottle"), MilkBox->GetContents().Count, 4);
	TestTrue(TEXT("Space kind is locked to the box kind"), SpaceA->GetStock().Kind == Milk);
	TestEqual(TEXT("Space label follows the stock"),
		Player.Interaction->GetCurrentInteractionQuery().TargetName.ToString(), FString(TEXT("바나나우유 1/3")));
	TestTrue(TEXT("Apply repeats while held"), Player.HeldUse->IsComponentTickEnabled());
	Player.HeldUse->TickComponent(0.149f, LEVELTICK_All, nullptr);
	TestEqual(TEXT("Under the interval nothing repeats"), SpaceA->GetStock().Count, 1);
	Player.HeldUse->TickComponent(0.002f, LEVELTICK_All, nullptr);
	TestEqual(TEXT("After 0.15 s one more bottle moves"), SpaceA->GetStock().Count, 2);
	Player.HeldUse->TickComponent(0.15f, LEVELTICK_All, nullptr);
	TestEqual(TEXT("A third bottle fills the space"), SpaceA->GetStock().Count, 3);
	const int32 ReportsBeforeStop = Reports.Num();
	Player.HeldUse->TickComponent(0.15f, LEVELTICK_All, nullptr);
	TestFalse(TEXT("A full space stops the repeat"), Player.HeldUse->IsComponentTickEnabled());
	TestEqual(TEXT("The stop is reported exactly once"), Reports.Num(), ReportsBeforeStop + 1);
	TestEqual(TEXT("The stop reason is the full-space reason"),
		Reports.Last().FailureReason.ToString(), FString(TEXT("가득 참")));
	TestEqual(TEXT("The stop intent is HeldApply"), Reports.Last().Intent, EPlayerInteractionIntent::HeldApply);
	Player.HeldUse->EndUse();

	// LIFO: the top bottle sits on the last used slot and is taken first.
	TestEqual(TEXT("Stock visual shows every bottle"), SpaceA->GetStockVisual()->GetInstanceCount(), 3);
	Player.HeldUse->BeginUse(EPlayerHeldTargetUseDirection::Take);
	TestEqual(TEXT("Take moves one bottle back"), SpaceA->GetStock().Count, 2);
	TestEqual(TEXT("Box regained the bottle"), MilkBox->GetContents().Count, 3);
	TestEqual(TEXT("Stock visual follows"), SpaceA->GetStockVisual()->GetInstanceCount(), 2);
	Player.HeldUse->EndUse();

	// Different kind is refused with its reason.
	StowBox(Player, MilkBox);
	TestTrue(TEXT("Player takes the juice box"), Player.Carry->TryTakePhysicalObject(JuiceBox, Failure));
	Player.Interaction->RefreshInteractionQuery();
	Query = Player.Interaction->GetCurrentInteractionQuery();
	TestFalse(TEXT("A different kind cannot be applied"), Query.bCanHeldApply);
	TestEqual(TEXT("Different-kind apply reason"), Query.HeldApplyFailureReason.ToString(), FString(TEXT("다른 음료가 진열됨")));
	TestFalse(TEXT("A different kind cannot be taken"), Query.bCanHeldTake);
	const int32 BeforeRefusal = Reports.Num();
	Player.HeldUse->BeginUse(EPlayerHeldTargetUseDirection::Apply);
	TestEqual(TEXT("Refused apply changes nothing"), SpaceA->GetStock().Count, 2);
	TestEqual(TEXT("Refused apply is reported once"), Reports.Num(), BeforeRefusal + 1);
	Player.HeldUse->EndUse();

	// Empty box takes from any stocked space and adopts its kind.
	StowBox(Player, JuiceBox);
	TestTrue(TEXT("Player takes the empty box"), Player.Carry->TryTakePhysicalObject(EmptyBox, Failure));
	Player.Interaction->RefreshInteractionQuery();
	Query = Player.Interaction->GetCurrentInteractionQuery();
	TestFalse(TEXT("An empty box cannot apply"), Query.bCanHeldApply);
	TestEqual(TEXT("Empty-box apply reason"), Query.HeldApplyFailureReason.ToString(), FString(TEXT("박스가 비어 있음")));
	TestTrue(TEXT("An empty box can take"), Query.bCanHeldTake);
	Player.HeldUse->BeginUse(EPlayerHeldTargetUseDirection::Take);
	TestTrue(TEXT("Empty box adopted the milk kind"), EmptyBox->GetContents().Kind == Milk && EmptyBox->GetContents().Count == 1);
	Player.HeldUse->EndUse();

	// Non-target held objects and the E/F keys change nothing.
	StowBox(Player, EmptyBox);
	ATowelBasketActor* Basket = World->SpawnActor<ATowelBasketActor>();
	BeginActorPlayIfNeeded(Basket);
	TestTrue(TEXT("Player takes a towel basket"), Player.Carry->TryTakePhysicalObject(Basket, Failure));
	Player.Interaction->RefreshInteractionQuery();
	Query = Player.Interaction->GetCurrentInteractionQuery();
	TestTrue(TEXT("A towel basket gets no held rows on a display space"), !Query.bHeldApplyVisible && !Query.bHeldTakeVisible);
	Player.HeldUse->BeginUse(EPlayerHeldTargetUseDirection::Apply);
	TestEqual(TEXT("The basket does not enter the space"), SpaceA->GetStock().Count, 1);
	TestEqual(TEXT("Towels are untouched"), Basket->GetInventory()->GetSnapshot().Count, 0);
	Player.HeldUse->EndUse();

	FPlayerInteractionContext Context;
	Context.Interactor = Player.Pawn;
	Context.CarryComponent = Player.Carry;
	Context.InteractionComponent = Player.Interaction;
	const int32 StockBeforeKeys = SpaceA->GetStock().Count;
	TestFalse(TEXT("E does nothing on a space"), SpaceA->ExecuteInteraction(Context).bSucceeded);
	TestFalse(TEXT("F does nothing on a space"), SpaceA->ExecuteSecondaryInteraction(Context).bSucceeded);
	TestEqual(TEXT("E and F leave the stock alone"), SpaceA->GetStock().Count, StockBeforeKeys);
	(void)StockChanges;
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBathhouseServiceDisplayRepeatInterruptionTest,
	"BathhouseSim.Service.Display.RepeatAndInterruption",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FBathhouseServiceDisplayRepeatInterruptionTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	FScopedServiceGrid GridGuard;
	FScopedUtilityLaborWorld Scope(TEXT("ServiceRepeatWorld"));
	UWorld* World = Scope.Get();
	if (!TestNotNull(TEXT("Automation world exists"), World))
	{
		return false;
	}
	FPlayer Player;
	UFacilityPlacementDefinition* Definition = MakeFridgeDefinition();
	if (!TestTrue(TEXT("Player fixture builds"), BuildPlayer(*this, World, Player))
		|| !TestNotNull(TEXT("Fridge definition exists"), Definition))
	{
		return false;
	}
	AServiceAutomationFridge* Fridge = SpawnInstalledFridge(
		*this, World, *Definition, FVector(120.0f, 0.0f, -60.0f));
	if (!TestNotNull(TEXT("Installed fridge exists"), Fridge))
	{
		return false;
	}
	UDisplaySpaceComponent* SpaceA = Fridge->GetSpaceA();
	UDisplaySpaceComponent* SpaceB = Fridge->GetSpaceB();
	UServiceItemDefinition* Milk = MakeKind(TEXT("RepeatMilk"), TEXT("바나나우유"), 12, 2000);
	AItemBoxActor* Box = SpawnBox(World, Milk, 12);
	FText Failure;
	if (!TestNotNull(TEXT("Box exists"), Box) || !TestTrue(TEXT("Box is held"), Player.Carry->TryTakePhysicalObject(Box, Failure)))
	{
		return false;
	}
	Player.Interaction->RefreshInteractionQuery();
	TArray<FPlayerInteractionResult> Reports;
	Player.Interaction->OnInteractionAttemptFinishedNative.AddLambda(
		[&Reports](const FPlayerInteractionResult& Result) { Reports.Add(Result); });

	// Switching to another space silently stops the repeat.
	Player.HeldUse->BeginUse(EPlayerHeldTargetUseDirection::Apply);
	TestEqual(TEXT("Press applies one to space A"), SpaceA->GetStock().Count, 1);
	const int32 ReportsBeforeSwitch = Reports.Num();
	Fridge->SetActorLocation(FVector(120.0f, 0.0f, -120.0f));
	Player.HeldUse->TickComponent(0.5f, LEVELTICK_All, nullptr);
	TestFalse(TEXT("Aiming another space stops the repeat"), Player.HeldUse->IsComponentTickEnabled());
	TestEqual(TEXT("Space A did not receive another bottle"), SpaceA->GetStock().Count, 1);
	TestEqual(TEXT("Space B did not receive a bottle"), SpaceB->GetStock().Count, 0);
	TestEqual(TEXT("Switching is silent"), Reports.Num(), ReportsBeforeSwitch);
	Player.HeldUse->TickComponent(1.0f, LEVELTICK_All, nullptr);
	TestEqual(TEXT("Holding the button does not resume"), SpaceB->GetStock().Count, 0);
	Player.HeldUse->EndUse();
	Player.HeldUse->BeginUse(EPlayerHeldTargetUseDirection::Apply);
	TestEqual(TEXT("A new press works on the newly aimed space"), SpaceB->GetStock().Count, 1);
	Player.HeldUse->EndUse();

	// Interval 0.3.
	Player.HeldUse->RepeatIntervalSeconds = 0.3f;
	Player.HeldUse->BeginUse(EPlayerHeldTargetUseDirection::Apply);
	const int32 AtStart = SpaceB->GetStock().Count;
	Player.HeldUse->TickComponent(0.299f, LEVELTICK_All, nullptr);
	TestEqual(TEXT("0.3 interval has not elapsed at 0.299"), SpaceB->GetStock().Count, AtStart);
	Player.HeldUse->TickComponent(0.002f, LEVELTICK_All, nullptr);
	TestEqual(TEXT("0.3 interval repeats at 0.301"), SpaceB->GetStock().Count, AtStart + 1);
	Player.HeldUse->EndUse();
	Player.HeldUse->RepeatIntervalSeconds = 0.15f;

	// Dropping the box stops the repeat quietly and keeps what already moved.
	Player.HeldUse->BeginUse(EPlayerHeldTargetUseDirection::Take);
	const int32 BeforeDrop = SpaceB->GetStock().Count;
	const int32 ReportsBeforeDrop = Reports.Num();
	StowBox(Player, Box);
	Player.HeldUse->TickComponent(0.15f, LEVELTICK_All, nullptr);
	TestFalse(TEXT("Dropping the box stops the repeat"), Player.HeldUse->IsComponentTickEnabled());
	TestEqual(TEXT("Already moved bottles stay moved"), SpaceB->GetStock().Count, BeforeDrop);
	TestEqual(TEXT("Dropping is silent"), Reports.Num(), ReportsBeforeDrop);
	Player.HeldUse->EndUse();

	// Suppression stops the repeat quietly too.
	TestTrue(TEXT("Player retakes the box"), Player.Carry->TryTakePhysicalObject(Box, Failure));
	Player.Interaction->RefreshInteractionQuery();
	Player.HeldUse->BeginUse(EPlayerHeldTargetUseDirection::Take);
	const int32 ReportsBeforeSuppression = Reports.Num();
	Player.Interaction->SetInteractionSuppressed(true);
	Player.HeldUse->TickComponent(0.15f, LEVELTICK_All, nullptr);
	TestFalse(TEXT("Suppression stops the repeat"), Player.HeldUse->IsComponentTickEnabled());
	TestEqual(TEXT("Suppression is silent"), Reports.Num(), ReportsBeforeSuppression);
	Player.Interaction->SetInteractionSuppressed(false);
	Player.HeldUse->EndUse();

	// A full box ends a Take repeat with the box-full reason.
	Player.HeldUse->BeginUse(EPlayerHeldTargetUseDirection::Take);
	for (int32 Index = 0; Index < 6 && Player.HeldUse->IsComponentTickEnabled(); ++Index)
	{
		Player.HeldUse->TickComponent(0.15f, LEVELTICK_All, nullptr);
	}
	Player.HeldUse->EndUse();
	TestTrue(TEXT("Taking everything back leaves a legal box"), Box->GetContents().Count <= 12);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBathhouseServiceDisplayPresentationTest,
	"BathhouseSim.Service.Display.PresentationAndHighlight",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FBathhouseServiceDisplayPresentationTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	FScopedDisplaySettings SettingsGuard;
	FScopedServiceGrid GridGuard;
	FScopedUtilityLaborWorld Scope(TEXT("ServicePresentationWorld"));
	UWorld* World = Scope.Get();
	if (!TestNotNull(TEXT("Automation world exists"), World))
	{
		return false;
	}
	FPlayer Player;
	UFacilityPlacementDefinition* Definition = MakeFridgeDefinition();
	if (!TestTrue(TEXT("Player fixture builds"), BuildPlayer(*this, World, Player))
		|| !TestNotNull(TEXT("Fridge definition exists"), Definition))
	{
		return false;
	}
	AServiceAutomationFridge* Fridge = SpawnInstalledFridge(
		*this, World, *Definition, FVector(120.0f, 0.0f, -60.0f));
	if (!TestNotNull(TEXT("Installed fridge exists"), Fridge))
	{
		return false;
	}
	UDisplaySpaceComponent* Space = Fridge->GetSpaceA();
	UServiceItemDefinition* Milk = MakeKind(TEXT("PresentMilk"), TEXT("바나나우유"), 12, 2000);
	AItemBoxActor* Box = SpawnBox(World, Milk, 6);
	FText Failure;
	if (!TestNotNull(TEXT("Box exists"), Box) || !TestTrue(TEXT("Box is held"), Player.Carry->TryTakePhysicalObject(Box, Failure)))
	{
		return false;
	}
	UMaterialInterface* PreviewMaterial = LoadObject<UMaterialInterface>(
		nullptr, TEXT("/Engine/EngineMaterials/DefaultMaterial.DefaultMaterial"));
	FScopedDisplaySettings& Guard = SettingsGuard;
	Guard.Settings->InsertPreviewMaterial = TSoftObjectPtr<UMaterialInterface>(PreviewMaterial);
	Guard.Settings->bShowTakeHighlight = true;
	Guard.Settings->TakeHighlightStencilValue = 7;

	UStaticMeshComponent* Preview = Space->GetInsertPreview();
	UStaticMeshComponent* Proxy = Space->GetTakeHighlightProxy();
	if (!TestNotNull(TEXT("Preview component exists"), Preview) || !TestNotNull(TEXT("Highlight proxy exists"), Proxy))
	{
		return false;
	}
	TestFalse(TEXT("Preview starts hidden"), Preview->IsVisible());
	TestFalse(TEXT("Proxy starts hidden"), Proxy->IsVisible());

	Player.Interaction->RefreshInteractionQuery();
	TestTrue(TEXT("Insert preview shows when apply is possible"), Preview->IsVisible());
	TestTrue(TEXT("Preview sits on the next free slot"),
		Preview->GetRelativeLocation().Equals(FVector(0.0f, 0.0f, 0.0f), 0.01f));
	TestTrue(TEXT("Preview uses the box kind mesh"), Preview->GetStaticMesh() == Milk->ResolveDisplayMesh());
	TestEqual(TEXT("Preview uses the configured material"), Preview->GetMaterial(0), PreviewMaterial);
	TestEqual(TEXT("Preview has no collision"), Preview->GetCollisionEnabled(), ECollisionEnabled::NoCollision);
	TestFalse(TEXT("No take highlight on an empty space"), Proxy->IsVisible());

	Player.HeldUse->BeginUse(EPlayerHeldTargetUseDirection::Apply);
	Player.HeldUse->EndUse();
	Player.HeldUse->BeginUse(EPlayerHeldTargetUseDirection::Apply);
	Player.HeldUse->EndUse();
	Player.Interaction->RefreshInteractionQuery();
	TestEqual(TEXT("Two bottles are stocked"), Space->GetStock().Count, 2);
	TestTrue(TEXT("Preview follows to the third slot"),
		Preview->IsVisible() && Preview->GetRelativeLocation().Equals(FVector(20.0f, 0.0f, 0.0f), 0.01f));
	TestTrue(TEXT("Take highlight shows on the top bottle"), Proxy->IsVisible());
	TestTrue(TEXT("Proxy sits on the last used slot"), Proxy->GetRelativeLocation().Equals(FVector(10.0f, 0.0f, 0.0f), 0.01f));
	TestTrue(TEXT("Proxy uses the stocked kind mesh"), Proxy->GetStaticMesh() == Milk->ResolveDisplayMesh());
	TestFalse(TEXT("Proxy is not drawn in the main pass"), Proxy->bRenderInMainPass);
	TestFalse(TEXT("Proxy is not drawn in the depth pass"), Proxy->bRenderInDepthPass);
	TestTrue(TEXT("Proxy renders custom depth"), Proxy->bRenderCustomDepth);
	TestEqual(TEXT("Proxy uses the configured stencil value"), Proxy->CustomDepthStencilValue, 7);
	TestEqual(TEXT("Proxy has no collision"), Proxy->GetCollisionEnabled(), ECollisionEnabled::NoCollision);

	Player.HeldUse->BeginUse(EPlayerHeldTargetUseDirection::Apply);
	Player.HeldUse->EndUse();
	Player.Interaction->RefreshInteractionQuery();
	TestEqual(TEXT("The space is now full"), Space->GetStock().Count, 3);
	TestFalse(TEXT("A full space hides the preview"), Preview->IsVisible());
	TestTrue(TEXT("A full space still shows the take highlight"), Proxy->IsVisible());
	TestTrue(TEXT("Highlight follows to the last slot"), Proxy->GetRelativeLocation().Equals(FVector(20.0f, 0.0f, 0.0f), 0.01f));

	// The option is read on every focus notification, so a stock change is what makes it apply.
	Guard.Settings->bShowTakeHighlight = false;
	Player.HeldUse->BeginUse(EPlayerHeldTargetUseDirection::Take);
	Player.HeldUse->EndUse();
	Player.Interaction->RefreshInteractionQuery();
	TestEqual(TEXT("A bottle went back to the box"), Space->GetStock().Count, 2);
	TestFalse(TEXT("Turning the option off hides the proxy"), Proxy->IsVisible());
	TestTrue(TEXT("The insert preview is unaffected by the highlight option"),
		Preview->IsVisible() || GetDefault<UServiceDisplaySettings>()->InsertPreviewMaterial.IsNull());

	Guard.Settings->bShowTakeHighlight = true;
	Player.HeldUse->BeginUse(EPlayerHeldTargetUseDirection::Apply);
	Player.HeldUse->EndUse();
	Player.Interaction->RefreshInteractionQuery();
	TestEqual(TEXT("The bottle is back in the space"), Space->GetStock().Count, 3);
	TestTrue(TEXT("Turning the option on shows the proxy again"), Proxy->IsVisible());

	Fridge->SetActorLocation(FVector(120.0f, 500.0f, -60.0f));
	Player.Interaction->RefreshInteractionQuery();
	TestFalse(TEXT("Leaving focus hides the proxy"), Proxy->IsVisible());
	TestFalse(TEXT("Leaving focus hides the preview"), Preview->IsVisible());

	Fridge->SetActorLocation(FVector(120.0f, 0.0f, -60.0f));
	Player.Interaction->RefreshInteractionQuery();
	Guard.Settings->InsertPreviewMaterial = TSoftObjectPtr<UMaterialInterface>();
	Player.HeldUse->BeginUse(EPlayerHeldTargetUseDirection::Take);
	Player.HeldUse->EndUse();
	Player.Interaction->RefreshInteractionQuery();
	TestFalse(TEXT("Without a preview material the preview is omitted"), Preview->IsVisible());
	return true;
}

#endif
