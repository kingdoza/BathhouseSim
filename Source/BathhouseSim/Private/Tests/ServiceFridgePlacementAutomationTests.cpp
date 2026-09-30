#if WITH_DEV_AUTOMATION_TESTS

#include "Tests/ServiceAutomationTestSupport.h"

#include "Placement/FacilityActorConversionTransaction.h"
#include "Service/DrinkSalesSubsystem.h"
#include "Shop/BathhouseTrashBinActor.h"

namespace
{
using namespace ServiceTest;

int32 CountConstructedFridges(UWorld* World)
{
	int32 Count = 0;
	for (TActorIterator<AServiceAutomationConstructedFridge> It(World); It; ++It)
	{
		if (IsValid(*It))
		{
			++Count;
		}
	}
	return Count;
}

FDisplaySpaceSnapshot Snapshot(const int32 Index, UServiceItemDefinition* Kind, const int32 Count)
{
	FDisplaySpaceSnapshot Result;
	Result.SpaceIndex = Index;
	Result.Kind = Kind;
	Result.Count = Count;
	return Result;
}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBathhouseServiceFridgeConstructedPlacementTest,
	"BathhouseSim.Service.Fridge.ConstructedPlacementTransaction",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FBathhouseServiceFridgeConstructedPlacementTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	FScopedServiceGrid GridGuard;
	FScopedUtilityLaborWorld Scope(TEXT("ServiceConstructedPlacementWorld"));
	UWorld* World = Scope.Get();
	UFacilityPlacementDefinition* Definition = MakeFridgeDefinition();
	FPlayer Player;
	if (!TestNotNull(TEXT("Automation world exists"), World) || !TestNotNull(TEXT("Definition exists"), Definition)
		|| !TestTrue(TEXT("Player fixture builds"), BuildPlayer(*this, World, Player)))
	{
		return false;
	}
	// Blueprint SCS components exist only after FinishSpawning, so the placed class creates them in construction.
	Definition->PlacedFacilityClass = AServiceAutomationConstructedFridge::StaticClass();
	FText Failure;
	if (!TestTrue(TEXT("Definition validates for the constructed fixture"), Definition->ValidateRuntime(Failure)))
	{
		AddError(Failure.ToString());
		return false;
	}
	UServiceItemDefinition* Milk = MakeKind(TEXT("PlaceMilk"), TEXT("바나나우유"), 12, 2000);
	UServiceItemDefinition* Juice = MakeKind(TEXT("PlaceJuice"), TEXT("주스"), 6, 1500);
	UServiceItemDefinition* Snack = MakeKind(TEXT("PlaceSnack"), TEXT("과자"), 4, 500, false);

	AFacilityPlacementZoneAutomationActor* Zone = World->SpawnActor<AFacilityPlacementZoneAutomationActor>(
		AFacilityPlacementZoneAutomationActor::StaticClass(),
		FTransform(FRotator::ZeroRotator, FVector(12000.0f, 0.0f, 0.0f)));
	if (!TestNotNull(TEXT("Placement zone exists"), Zone))
	{
		return false;
	}
	for (const FGameplayTag& Tag : Definition->FacilityTags)
	{
		Zone->AddAllowedTag(Tag);
	}
	Zone->GetZoneBounds()->SetBoxExtent(FVector(500.0f, 500.0f, 500.0f));
	const FTransform PlacementTransform(FRotator::ZeroRotator, FVector(12000.0f, 0.0f, 100.0f));

	APlaceableFacilityItemActor* Item = APlaceableFacilityItemActor::SpawnFreshItem(
		*World, *Definition, FTransform(FRotator::ZeroRotator, FVector(11000.0f, 0.0f, 300.0f)), Failure);
	if (!TestNotNull(TEXT("Fresh item exists"), Item)
		|| !TestTrue(TEXT("Fresh item activates"), Item->ActivateFreeWorld(Item->GetActorTransform(), Failure))
		|| !TestTrue(TEXT("Fresh item is held"), Player.Carry->TryTakePhysicalObject(Item, Failure)))
	{
		return false;
	}

	// New installation through the real transaction: four empty spaces.
	AActor* PlacedActor = FFacilityActorConversionTransaction::PlaceItemAsFacility(
		*Item, PlacementTransform, *Zone, *Player.Carry, Failure);
	AServiceAutomationConstructedFridge* Fridge = Cast<AServiceAutomationConstructedFridge>(PlacedActor);
	if (!TestNotNull(TEXT("A construction-created fridge installs"), Fridge))
	{
		AddError(Failure.ToString());
		return false;
	}
	TArray<UDisplaySpaceComponent*> Spaces;
	Fridge->GetSpacesForTest(Spaces);
	TestEqual(TEXT("Construction created four spaces"), Spaces.Num(), 4);
	int32 TotalStock = 0;
	for (const UDisplaySpaceComponent* Space : Spaces)
	{
		TotalStock += Space->GetStock().Count;
	}
	TestEqual(TEXT("A new installation has four empty spaces"), TotalStock, 0);

	// Recovery then reinstall restores per-space kind and count.
	Spaces.Sort([](const UDisplaySpaceComponent& A, const UDisplaySpaceComponent& B) { return A.GetSpaceIndex() < B.GetSpaceIndex(); });
	TestTrue(TEXT("Space 0 takes milk"), Spaces[0]->ImportStock(Milk, 3, Failure));
	TestTrue(TEXT("Space 2 takes juice"), Spaces[2]->ImportStock(Juice, 2, Failure));
	APlaceableFacilityItemActor* Recovered = FFacilityActorConversionTransaction::RecoverFacilityToItem(*Fridge, Failure);
	if (!TestNotNull(TEXT("Recovery produces an item"), Recovered))
	{
		AddError(Failure.ToString());
		return false;
	}
	UDrinkFridgePlacementInstanceData* Data = Cast<UDrinkFridgePlacementInstanceData>(Recovered->GetPlacementPayload().InstanceData);
	if (!TestNotNull(TEXT("Recovered payload has fridge data"), Data) || !TestEqual(TEXT("Payload has four snapshots"), Data->Spaces.Num(), 4))
	{
		return false;
	}
	TestTrue(TEXT("Recovered item is held"), Player.Carry->TryTakePhysicalObject(Recovered, Failure));

	const int32 FridgesBeforeBad = CountConstructedFridges(World);
	const TArray<FDisplaySpaceSnapshot> GoodSpaces = Data->Spaces;
	auto TryBad = [&](const TFunction<void()>& Mutate, const TCHAR* Label)
	{
		Mutate();
		AActor* Result = FFacilityActorConversionTransaction::PlaceItemAsFacility(
			*Recovered, PlacementTransform, *Zone, *Player.Carry, Failure);
		TestNull(FString::Printf(TEXT("%s: placement fails"), Label), Result);
		TestTrue(FString::Printf(TEXT("%s: the original item survives"), Label), IsValid(Recovered));
		TestTrue(FString::Printf(TEXT("%s: the payload is unchanged"), Label), Recovered->GetPlacementPayload().InstanceData == Data);
		TestEqual(FString::Printf(TEXT("%s: no new Actor remains"), Label), CountConstructedFridges(World), FridgesBeforeBad);
		TestTrue(FString::Printf(TEXT("%s: the item is still held"), Label), Player.Carry->GetHeldObject() == Recovered);
		Data->Spaces = GoodSpaces;
	};
	TryBad([&]() { Data->Spaces[1] = Snapshot(7, nullptr, 0); }, TEXT("Index mismatch"));
	TryBad([&]() { Data->Spaces[0] = Snapshot(0, Milk, 4); }, TEXT("Over capacity"));
	TryBad([&]() { Data->Spaces[3] = Snapshot(3, Snack, 1); }, TEXT("Category refused"));
	TryBad([&]() { Data->Spaces.Pop(); }, TEXT("Missing space"));

	// Injected finalize failure yields the same failure result.
	FFacilityActorConversionTransaction::SetTestFault(
		FFacilityActorConversionTransaction::ETestFault::PlacementFinalizePayload);
	AActor* FaultResult = FFacilityActorConversionTransaction::PlaceItemAsFacility(
		*Recovered, PlacementTransform, *Zone, *Player.Carry, Failure);
	FFacilityActorConversionTransaction::ClearTestFault();
	TestNull(TEXT("Finalize fault: placement fails"), FaultResult);
	TestTrue(TEXT("Finalize fault: item and payload survive"),
		IsValid(Recovered) && Recovered->GetPlacementPayload().InstanceData == Data && Player.Carry->GetHeldObject() == Recovered);
	TestEqual(TEXT("Finalize fault: no new Actor remains"), CountConstructedFridges(World), FridgesBeforeBad);

	// The good payload still installs and restores every space.
	AActor* Reinstalled = FFacilityActorConversionTransaction::PlaceItemAsFacility(
		*Recovered, PlacementTransform, *Zone, *Player.Carry, Failure);
	AServiceAutomationConstructedFridge* Second = Cast<AServiceAutomationConstructedFridge>(Reinstalled);
	if (!TestNotNull(TEXT("The unchanged payload reinstalls"), Second))
	{
		AddError(Failure.ToString());
		return false;
	}
	TArray<UDisplaySpaceComponent*> SecondSpaces;
	Second->GetSpacesForTest(SecondSpaces);
	SecondSpaces.Sort([](const UDisplaySpaceComponent& A, const UDisplaySpaceComponent& B) { return A.GetSpaceIndex() < B.GetSpaceIndex(); });
	if (TestEqual(TEXT("Reinstall has four spaces"), SecondSpaces.Num(), 4))
	{
		TestTrue(TEXT("Space 0 restored"), SecondSpaces[0]->GetStock().Kind == Milk && SecondSpaces[0]->GetStock().Count == 3);
		TestTrue(TEXT("Space 1 restored empty"), SecondSpaces[1]->GetStock().Count == 0);
		TestTrue(TEXT("Space 2 restored"), SecondSpaces[2]->GetStock().Kind == Juice && SecondSpaces[2]->GetStock().Count == 2);
		TestTrue(TEXT("Space 3 restored empty"), SecondSpaces[3]->GetStock().Count == 0);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBathhouseServiceFridgeLayoutRuleTest,
	"BathhouseSim.Service.Fridge.LayoutRuleAndDiscard",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FBathhouseServiceFridgeLayoutRuleTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	FScopedServiceGrid GridGuard;
	FScopedUtilityLaborWorld Scope(TEXT("ServiceLayoutRuleWorld"));
	UWorld* World = Scope.Get();
	UFacilityPlacementDefinition* Definition = MakeFridgeDefinition();
	FPlayer Player;
	if (!TestNotNull(TEXT("Automation world exists"), World) || !TestNotNull(TEXT("Definition exists"), Definition)
		|| !TestTrue(TEXT("Player fixture builds"), BuildPlayer(*this, World, Player, true)))
	{
		return false;
	}
	AServiceAutomationFridge* Fridge = SpawnInstalledFridge(*this, World, *Definition, FVector(3000.0f, 0.0f, 0.0f));
	if (!TestNotNull(TEXT("Installed fridge exists"), Fridge))
	{
		return false;
	}
	FText Failure;
	TArray<const UDisplaySpaceComponent*> Layout = { Fridge->GetSpaceA(), Fridge->GetSpaceB() };
	TestTrue(TEXT("Two contiguous spaces and one slot are valid"), ADrinkFridgeActor::ValidateSpaceLayout(Layout, 1, Failure));
	TestFalse(TEXT("No slot is invalid"), ADrinkFridgeActor::ValidateSpaceLayout(Layout, 0, Failure));
	TestFalse(TEXT("Two slots are invalid"), ADrinkFridgeActor::ValidateSpaceLayout(Layout, 2, Failure));
	TestFalse(TEXT("No spaces are invalid"), ADrinkFridgeActor::ValidateSpaceLayout({}, 1, Failure));
	const TArray<const UDisplaySpaceComponent*> Duplicated = { Fridge->GetSpaceA(), Fridge->GetSpaceA() };
	TestFalse(TEXT("A duplicated index is invalid"), ADrinkFridgeActor::ValidateSpaceLayout(Duplicated, 1, Failure));
	static_cast<UServiceAutomationDisplaySpace*>(Fridge->GetSpaceB())->ConfigureForTest(3, TAG_Display_Fridge, 3);
	TestFalse(TEXT("An index gap is invalid"), ADrinkFridgeActor::ValidateSpaceLayout(Layout, 1, Failure));
	static_cast<UServiceAutomationDisplaySpace*>(Fridge->GetSpaceB())->ConfigureForTest(1, FGameplayTag(), 3);
	TestFalse(TEXT("A missing category tag is invalid"), ADrinkFridgeActor::ValidateSpaceLayout(Layout, 1, Failure));
	static_cast<UServiceAutomationDisplaySpace*>(Fridge->GetSpaceB())->ConfigureForTest(1, TAG_Display_Fridge, 0);
	TestFalse(TEXT("A space without slots is invalid"), ADrinkFridgeActor::ValidateSpaceLayout(Layout, 1, Failure));
	static_cast<UServiceAutomationDisplaySpace*>(Fridge->GetSpaceB())->ConfigureForTest(1, TAG_Display_Fridge, 3);

	// DISP-021: a fridge item that still carries stock is discarded through the trash bin without any money change.
	UServiceItemDefinition* Milk = MakeKind(TEXT("DiscardMilk"), TEXT("바나나우유"), 12, 2000);
	TestTrue(TEXT("Stock is added"), Fridge->GetSpaceA()->ImportStock(Milk, 3, Failure));
	APlaceableFacilityItemActor* Item = FFacilityActorConversionTransaction::RecoverFacilityToItem(*Fridge, Failure);
	if (!TestNotNull(TEXT("Recovery produces an item"), Item))
	{
		AddError(Failure.ToString());
		return false;
	}
	TestTrue(TEXT("The item carries fridge data"), Cast<UDrinkFridgePlacementInstanceData>(Item->GetPlacementPayload().InstanceData) != nullptr);
	TestTrue(TEXT("Player takes the item"), Player.Carry->TryTakePhysicalObject(Item, Failure));
	UDrinkSalesSubsystem* Sales = World->GetSubsystem<UDrinkSalesSubsystem>();
	Sales->AddSale(1000);
	const int32 MoneyBefore = Player.PlayerState->GetWallet()->GetCurrentMoney();
	FPlayerInteractionContext Context;
	Context.Interactor = Player.Pawn;
	Context.CarryComponent = Player.Carry;
	Context.InteractionComponent = Player.Interaction;
	ABathhouseTrashBinActor* TrashBin = World->SpawnActor<ABathhouseTrashBinActor>();
	TestTrue(TEXT("The trash bin accepts the fridge item"), TrashBin && TrashBin->QueryInteraction(Context).bCanInteract);
	TestTrue(TEXT("The fridge item is discarded"), TrashBin && TrashBin->ExecuteInteraction(Context).bSucceeded);
	TestTrue(TEXT("The item is gone and the hand is empty"), !IsValid(Item) && Player.Carry->IsHandEmpty());
	TestEqual(TEXT("Discarding gives no money"), Player.PlayerState->GetWallet()->GetCurrentMoney(), MoneyBefore);
	TestEqual(TEXT("Discarding leaves the collection pool alone"), Sales->GetPendingAmount(), 1000);
	return true;
}

#endif
