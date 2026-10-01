#if WITH_DEV_AUTOMATION_TESTS
#include "Tests/CleaningLitterAutomationTestSupport.h"
#include "Interaction/Presentation/OpeningPresentationComponent.h"
#include "Service/DisplayCueComponent.h"
#include "Towel/CleanTowelStackActor.h"
#include "Towel/TowelBasketActor.h"
#include "Towel/TowelInventoryComponent.h"
#include "Towel/TowelProcessingMachineActor.h"
#include "Towel/TowelTransferPortComponent.h"
#include "Towel/Presentation/TowelPileVisualComponent.h"
#include "Towel/Presentation/TowelStackVisualComponent.h"
#include "Towel/Presentation/TowelVisualMeshProfile.h"
#include "Towel/UsedTowelBinActor.h"
#include "UI/InteractionPromptWidget.h"

using namespace CleaningLitterTest;

namespace
{
/** Gives a placed actor a Visibility-blocking sphere so the real camera trace can aim at it. */
USphereComponent* AddTraceSphere(AActor* Target)
{
	USphereComponent* Sphere = NewObject<USphereComponent>(Target);
	Target->AddInstanceComponent(Sphere);
	Sphere->SetupAttachment(Target->GetRootComponent());
	Sphere->InitSphereRadius(28.0f);
	Sphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	Sphere->SetCollisionResponseToAllChannels(ECR_Ignore);
	Sphere->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
	Sphere->SetCanEverAffectNavigation(false);
	Sphere->RegisterComponent();
	return Sphere;
}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCleaningTongsNoFalseTakeCueTest, "BathhouseSim.Cleaning.Litter.TongsDoNotDriveTakeCues",
								 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCleaningTongsNoFalseTakeCueTest::RunTest(const FString&)
{
	// Lambda so the friend access of this test class applies.
	auto CommitTowels = [](UTowelInventoryComponent* Inventory, const ETowelState State, const int32 Count)
	{
		const FTowelInventorySnapshot Before = Inventory->GetSnapshot();
		if (Inventory->TryBeginTransaction())
		{
			Inventory->CommitInternal(State, Count);
			Inventory->EndTransaction();
			Inventory->BroadcastCommit(Before, Before.Revision + 1);
		}
	};
	FScopedServiceGrid GridGuard;
	FScopedDisplaySettings SettingsGuard;
	SettingsGuard.Settings->InsertPreviewMaterial = TSoftObjectPtr<UMaterialInterface>(
		LoadObject<UMaterialInterface>(nullptr, TEXT("/Engine/EngineMaterials/DefaultMaterial.DefaultMaterial")));
	SettingsGuard.Settings->bShowTakeHighlight = true;
	FScopedUtilityLaborWorld Scope(TEXT("TongsCueWorld"));
	UWorld* World = Scope.Get();
	FPlayer Player;
	if (!TestNotNull(TEXT("World"), World) || !BuildPlayer(*this, World, Player))
	{
		return false;
	}
	AddCapsule(Player);
	// Cue meshes come from a profile; without one a hidden cue would prove nothing.
	auto* Cube = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
	auto* Profile = NewObject<UTowelVisualMeshProfile>();
	for (const ETowelState State : {ETowelState::Used, ETowelState::Wet, ETowelState::Clean})
	{
		FTowelStateMeshVariants Entry;
		Entry.State = State;
		Entry.MeshVariants.Add(Cube);
		Profile->StateVariants.Add(Entry);
	}
	const FVector Aim = Player.Camera->GetComponentLocation() + Player.Camera->GetForwardVector() * 200.0f;
	const FVector Away(0.0f, 4000.0f, 0.0f);

	// Three litter pieces in the tongs.
	ALitterTongsActor* Tongs = CleaningLitterTest::Tongs(World);
	FText Failure;
	if (!TestTrue(TEXT("Tongs are held"), Player.Carry->TryTakePhysicalObject(Tongs, Failure)))
	{
		return false;
	}
	auto Refill = [&]()
	{
		for (int32 Index = Tongs->GetBagCount(); Index < 3; ++Index)
		{
			auto* Piece = Litter(World, FVector(300 + Index * 30, 0, 0));
			Tongs->BeginEquipmentUse(Context(Player, Tongs, Piece));
		}
	};
	Refill();
	TestEqual(TEXT("Tongs hold three bags"), Tongs->GetBagCount(), 3);

	TArray<FPlayerInteractionResult> Reports;
	Player.Interaction->OnInteractionAttemptFinishedNative.AddLambda(
		[&Reports](const FPlayerInteractionResult& Result) { Reports.Add(Result); });

	auto ExpectTieRow = [&](const TCHAR* Label)
	{
		const FPlayerInteractionQuery Query = Player.Interaction->GetCurrentInteractionQuery();
		TestTrue(FString::Printf(TEXT("%s: RMB shows tie"), Label),
				 Query.bEquipmentSecondaryVisible && Query.bCanEquipmentSecondary
					 && Query.EquipmentSecondaryActionName.ToString() == TEXT("봉투 묶기"));
		TestFalse(FString::Printf(TEXT("%s: no held Take row from tongs"), Label),
				  Query.bHeldTakeVisible || Query.bCanHeldTake);
	};
	auto ExpectTieKeepsStock = [&](const TCHAR* Label, const TFunction<int32()>& Stock)
	{
		const int32 Before = Stock();
		Reports.Reset();
		TestTrue(FString::Printf(TEXT("%s: tie succeeds"), Label), Player.EquipmentUse->ExecuteSecondaryEquipmentUse().bSucceeded);
		TestEqual(FString::Printf(TEXT("%s: tie does not touch target stock"), Label), Stock(), Before);
		TestEqual(FString::Printf(TEXT("%s: bags are tied"), Label), Tongs->GetBagCount(), 0);
		// A tied bag lies in the next tie's drop spot and in the held pose; clear it between targets.
		TArray<ATrashBagActor*> Tied;
		for (TActorIterator<ATrashBagActor> It(World); It; ++It)
		{
			Tied.Add(*It);
		}
		for (ATrashBagActor* Bag : Tied)
		{
			Bag->Destroy();
		}
		TestTrue(FString::Printf(TEXT("%s: result intent is EquipmentSecondaryUse"), Label),
				 Reports.Num() == 1 && Reports[0].Intent == EPlayerInteractionIntent::EquipmentSecondaryUse);
		Refill();
	};

	// Fridge display space with stock.
	UFacilityPlacementDefinition* Definition = MakeFridgeDefinition();
	UServiceItemDefinition* Milk = MakeKind(TEXT("TongsMilk"), TEXT("바나나우유"), 12, 2000);
	if (!TestNotNull(TEXT("Fridge definition"), Definition))
	{
		return false;
	}
	AServiceAutomationFridge* Fridge = SpawnInstalledFridge(*this, World, *Definition, Aim - FVector(0, 0, 60));
	if (!TestNotNull(TEXT("Fridge"), Fridge))
	{
		return false;
	}
	Fridge->GetSpaceA()->ImportStock(Milk, 3, Failure);
	Player.Interaction->RefreshInteractionQuery();
	TestEqual(TEXT("Fridge space is the aim target"), Player.Interaction->GetCurrentInteractionQuery().TargetName.ToString(),
			  FString(TEXT("바나나우유 3/3")));
	TestFalse(TEXT("Tongs: fridge take highlight hidden"), Fridge->GetSpaceA()->GetTakeHighlightProxy()->IsVisible());
	TestFalse(TEXT("Tongs: fridge insert preview hidden"), Fridge->GetSpaceA()->GetInsertPreview()->IsVisible());
	ExpectTieRow(TEXT("Fridge"));
	ExpectTieKeepsStock(TEXT("Fridge"), [&]() { return Fridge->GetSpaceA()->GetStock().Count; });
	Fridge->SetActorLocation(Away);

	// Clean towel shelf and used bin with stock.
	auto* Shelf = World->SpawnActor<ACleanTowelStackActor>();
	BeginActorPlayIfNeeded(Shelf);
	Shelf->GetFacilityPlacementComponent()->SetPlacedDomainActive(true);
	AddTraceSphere(Shelf);
	Shelf->FindComponentByClass<UTowelStackVisualComponent>()->MeshProfile = Profile;
	CommitTowels(Shelf->GetInventory(), ETowelState::Clean, 3);
	Shelf->FindComponentByClass<UTowelStackVisualComponent>()->SynchronizeImmediately();
	Shelf->SetActorLocation(Aim);
	Player.Interaction->RefreshInteractionQuery();
	TestFalse(TEXT("Tongs: shelf cue hidden"), Shelf->GetDisplayCue()->GetTakeHighlightProxy()->IsVisible());
	ExpectTieRow(TEXT("Shelf"));
	ExpectTieKeepsStock(TEXT("Shelf"), [&]() { return Shelf->GetInventory()->GetSnapshot().Count; });
	Shelf->SetActorLocation(Away);

	auto* Bin = World->SpawnActor<AUsedTowelBinActor>();
	BeginActorPlayIfNeeded(Bin);
	Bin->GetFacilityPlacementComponent()->SetPlacedDomainActive(true);
	AddTraceSphere(Bin);
	Bin->FindComponentByClass<UTowelStackVisualComponent>()->MeshProfile = Profile;
	CommitTowels(Bin->GetInventory(), ETowelState::Used, 3);
	Bin->FindComponentByClass<UTowelStackVisualComponent>()->SynchronizeImmediately();
	Bin->SetActorLocation(Aim);
	Player.Interaction->RefreshInteractionQuery();
	TestFalse(TEXT("Tongs: used bin cue hidden"), Bin->GetDisplayCue()->GetTakeHighlightProxy()->IsVisible());
	ExpectTieRow(TEXT("Used bin"));
	ExpectTieKeepsStock(TEXT("Used bin"), [&]() { return Bin->GetInventory()->GetSnapshot().Count; });
	Bin->SetActorLocation(Away);

	// Waiting washer port with towels.
	auto* Machine = World->SpawnActor<ATowelProcessingMachineActor>();
	BeginActorPlayIfNeeded(Machine);
	Machine->GetFacilityPlacementComponent()->SetPlacedDomainActive(true);
	Machine->GetTowelVisual()->MeshProfile = Profile;
	CommitTowels(Machine->GetInventory(), ETowelState::Used, 3);
	Machine->GetTowelVisual()->SynchronizeImmediately();
	auto* Port = Machine->FindComponentByClass<UTowelTransferPortComponent>();
	Port->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	Port->SetCollisionResponseToAllChannels(ECR_Ignore);
	Port->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
	TArray<UPrimitiveComponent*> Primitives;
	Machine->GetComponents(Primitives);
	for (UPrimitiveComponent* Primitive : Primitives)
	{
		if (Primitive != Port)
		{
			Primitive->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		}
	}
	Machine->SetActorLocation(Aim);
	Player.Interaction->RefreshInteractionQuery();
	TestFalse(TEXT("Tongs: washer highlight hidden"), Machine->GetDisplayCue()->GetTakeHighlightProxy()->IsVisible());
	TestEqual(TEXT("Tongs: washer lid target stays closed"), Machine->GetLidPresentation()->GetTargetAlpha(), 0.f);
	ExpectTieRow(TEXT("Washer"));
	ExpectTieKeepsStock(TEXT("Washer"), [&]() { return Machine->GetInventory()->GetSnapshot().Count; });

	// Contrast: a towel basket on the same washer still opens the lid and highlights the top towel.
	TestTrue(TEXT("Tongs are dropped"), Player.Carry->TryFreeDropHeldObject(FVector::ForwardVector).bSucceeded);
	Tongs->SetActorLocation(Away + FVector(0, 500, 0)); // keep the dropped item out of the aim ray and held pose
	auto* Basket = World->SpawnActor<ATowelBasketActor>();
	BeginActorPlayIfNeeded(Basket);
	TestTrue(TEXT("Basket is held"), Player.Carry->TryTakePhysicalObject(Basket, Failure));
	Player.Interaction->RefreshInteractionQuery();
	const FPlayerInteractionQuery BasketQuery = Player.Interaction->GetCurrentInteractionQuery();
	TestTrue(TEXT("Basket: washer shows held Take"), BasketQuery.bHeldTakeVisible && BasketQuery.bCanHeldTake);
	TestFalse(TEXT("Basket: no equipment secondary row"), BasketQuery.bEquipmentSecondaryVisible);
	TestTrue(TEXT("Basket: washer highlight shows"), Machine->GetDisplayCue()->GetTakeHighlightProxy()->IsVisible());
	TestEqual(TEXT("Basket: washer lid opens"), Machine->GetLidPresentation()->GetTargetAlpha(), 1.f);

	// Contrast: an item box on a stocked fridge space still highlights and previews.
	Machine->SetActorLocation(Away);
	Basket->Destroy(); // EndPlay releases the hand
	TestTrue(TEXT("Hand is empty again"), Player.Carry->IsHandEmpty());
	Fridge->SetActorLocation(Aim - FVector(0, 0, 60));
	Fridge->GetSpaceA()->ImportStock(Milk, 2, Failure); // below capacity so insert is possible too
	AItemBoxActor* ItemBox = SpawnBox(World, Milk, 5);
	TestTrue(TEXT("Item box is held"), Player.Carry->TryTakePhysicalObject(ItemBox, Failure));
	Player.Interaction->RefreshInteractionQuery();
	TestTrue(TEXT("Item box: fridge take highlight shows"), Fridge->GetSpaceA()->GetTakeHighlightProxy()->IsVisible());
	TestTrue(TEXT("Item box: fridge insert preview shows"), Fridge->GetSpaceA()->GetInsertPreview()->IsVisible());
	TestFalse(TEXT("Item box: no equipment secondary row"),
			  Player.Interaction->GetCurrentInteractionQuery().bEquipmentSecondaryVisible);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCleaningTongsRmbRowQueryTest, "BathhouseSim.Cleaning.Litter.TongsRmbRowQuery",
								 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCleaningTongsRmbRowQueryTest::RunTest(const FString&)
{
	// No widget Blueprint is available headless, so verify the query contract the RMB row reads.
	FPlayerInteractionQuery Base, Other;
	Other.bEquipmentSecondaryVisible = true;
	TestFalse(TEXT("Equals sees the equipment secondary row"), Base.Equals(Other));
	Other = Base;
	Other.EquipmentSecondaryFailureReason = FText::FromString(TEXT("봉투가 비어 있음"));
	TestFalse(TEXT("Equals sees the equipment secondary reason"), Base.Equals(Other));
	TestTrue(TEXT("New intent is appended after HeldTake"),
			 static_cast<uint8>(EPlayerInteractionIntent::EquipmentSecondaryUse)
				 == static_cast<uint8>(EPlayerInteractionIntent::HeldTake) + 1);

	FScopedUtilityLaborWorld Scope(TEXT("TongsRmbRowWorld"));
	UWorld* World = Scope.Get();
	FPlayer Player;
	if (!TestNotNull(TEXT("World"), World) || !BuildPlayer(*this, World, Player))
	{
		return false;
	}
	AddCapsule(Player);
	ALitterTongsActor* Tongs = CleaningLitterTest::Tongs(World);
	FText Failure;
	TestTrue(TEXT("Tongs are held"), Player.Carry->TryTakePhysicalObject(Tongs, Failure));
	Player.Interaction->ClearInteractionQuery();
	const FPlayerInteractionQuery Query =
		Player.EquipmentUse->MergeEquipmentQuery(Player.Interaction->GetCurrentInteractionQuery());
	TestTrue(TEXT("Empty tongs: row visible but not usable"),
			 Query.bEquipmentSecondaryVisible && !Query.bCanEquipmentSecondary);
	TestEqual(TEXT("Empty tongs: the RMB row text"), Query.EquipmentSecondaryActionName.ToString(), FString(TEXT("봉투 묶기")));
	TestEqual(TEXT("Empty tongs: the RMB row reason"), Query.EquipmentSecondaryFailureReason.ToString(),
			  FString(TEXT("봉투가 비어 있음")));
	TestTrue(TEXT("Held Take fields stay empty"),
			 !Query.bHeldTakeVisible && Query.HeldTakeActionName.IsEmpty() && Query.HeldTakeFailureReason.IsEmpty());

	// A failed tie reports the new intent, which the widget routes to the single RMB row.
	TArray<FPlayerInteractionResult> Reports;
	Player.Interaction->OnInteractionAttemptFinishedNative.AddLambda(
		[&Reports](const FPlayerInteractionResult& Result) { Reports.Add(Result); });
	TestFalse(TEXT("Empty tie fails"), Player.EquipmentUse->ExecuteSecondaryEquipmentUse().bSucceeded);
	TestTrue(TEXT("Failure carries EquipmentSecondaryUse and the reason"),
			 Reports.Num() == 1 && Reports[0].Intent == EPlayerInteractionIntent::EquipmentSecondaryUse
				 && Reports[0].FailureReason.ToString() == TEXT("봉투가 비어 있음"));
	return true;
}
#endif
