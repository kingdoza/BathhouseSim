#if WITH_DEV_AUTOMATION_TESTS
#include "Tests/ServiceAutomationTestSupport.h"
#include "Service/DisplayCueComponent.h"
#include "Interaction/Presentation/OpeningPresentationComponent.h"
#include "Towel/TowelBasketActor.h"
#include "Towel/TowelInventoryComponent.h"
#include "Towel/TowelProcessingMachineActor.h"
#include "Towel/TowelTransferPortComponent.h"
#include "Towel/CleanTowelStackActor.h"
#include "Towel/UsedTowelBinActor.h"
#include "Towel/Presentation/TowelPileVisualComponent.h"
#include "Towel/Presentation/TowelStackVisualComponent.h"
#include "Towel/Presentation/TowelVisualMeshProfile.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Service/DisplayFacilityTargetComponent.h"
#include "Towel/TowelDisplayCueUtils.h"
#include "Towel/TowelTransferSubsystem.h"
#include "UObject/UnrealType.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FBathhouseTowelDisplayCueTest,
								 "BathhouseSim.Towel.Display.CuesDeterministicPileAndLid",
								 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FBathhouseTowelDisplayCueTest::RunTest(const FString& Parameters)
{
	using namespace ServiceTest;
	FScopedUtilityLaborWorld Scope(TEXT("TowelDisplayWorld"));
	UWorld* World = Scope.Get();
	FPlayer Player;
	if (!BuildPlayer(*this, World, Player))
	{
		return false;
	}
	FScopedDisplaySettings SettingsGuard;
	SettingsGuard.Settings->InsertPreviewMaterial = TSoftObjectPtr<UMaterialInterface>(
		LoadObject<UMaterialInterface>(nullptr, TEXT("/Engine/EngineMaterials/DefaultMaterial.DefaultMaterial")));
	SettingsGuard.Settings->bShowTakeHighlight = true;
	auto* Cube = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
	auto* Profile = NewObject<UTowelVisualMeshProfile>();
	for (auto State : {ETowelState::Used, ETowelState::Wet, ETowelState::Clean})
	{
		FTowelStateMeshVariants Entry;
		Entry.State = State;
		Entry.MeshVariants.Add(Cube);
		Profile->StateVariants.Add(Entry);
	}
	auto Commit = [](UTowelInventoryComponent* Inventory, ETowelState State, int32 Count)
	{
		auto Before = Inventory->GetSnapshot();
		if (Inventory->TryBeginTransaction())
		{
			Inventory->CommitInternal(State, Count);
			Inventory->EndTransaction();
			Inventory->BroadcastCommit(Before, Before.Revision + 1);
		}
	};
	auto* Machine = World->SpawnActor<ATowelProcessingMachineActor>();
	BeginActorPlayIfNeeded(Machine);
	auto* Port = Machine->FindComponentByClass<UTowelTransferPortComponent>();
	auto* Pile = Machine->GetTowelVisual();
	auto* Lid = Machine->GetLidPresentation();
	Pile->MeshProfile = Profile;
	Machine->GetFacilityPlacementComponent()->SetPlacedDomainActive(true);
	Commit(Machine->GetInventory(), ETowelState::Used, 3);
	Pile->SynchronizeImmediately();
	UStaticMesh* Mesh = nullptr;
	FTransform Third, Again;
	TestTrue(TEXT("Pile index presentation exists"), Pile->GetIndexPresentation(2, ETowelState::Used, Mesh, Third));
	Pile->GetIndexPresentation(2, ETowelState::Used, Mesh, Again);
	TestTrue(TEXT("Same seed and index is deterministic"), Third.Equals(Again));
	Commit(Machine->GetInventory(), ETowelState::Used, 2);
	Pile->SynchronizeImmediately();
	Pile->GetIndexPresentation(2, ETowelState::Used, Mesh, Again);
	TestTrue(TEXT("LIFO reinsertion position same"), Third.Equals(Again));
	Commit(Machine->GetInventory(), ETowelState::Used, 3);
	Pile->SynchronizeImmediately();
	if (!TestEqual(TEXT("Bound presentation has three real layers"), Pile->LayerRecords.Num(), 3))
	{
		return false;
	}
	auto& Top = Pile->LayerRecords.Last();
	FTransform Actual;
	Top.Bucket->GetInstanceTransform(Top.InstanceIndex, Actual, false);
	TestTrue(TEXT("Preview equals actual inserted instance"), Again.Equals(Actual));
	const int32 OldSeed = Pile->GetLayoutSeed();
	Commit(Machine->GetInventory(), ETowelState::None, 0);
	TestTrue(TEXT("Authoritative empty refreshes seed"), Pile->GetLayoutSeed() != OldSeed);
	// Preview while old layers are still animating away must use the new batch's layout.
	FTransform NewPreview;
	Pile->GetIndexPresentation(0, ETowelState::Used, Mesh, NewPreview);
	Commit(Machine->GetInventory(), ETowelState::Used, 1);
	Pile->SynchronizeImmediately();
	auto& First = Pile->LayerRecords[0];
	First.Bucket->GetInstanceTransform(First.InstanceIndex, Actual, false);
	TestTrue(TEXT("New batch preview equals actual despite stale animation"), NewPreview.Equals(Actual));
	auto* Basket = World->SpawnActor<ATowelBasketActor>();
	BeginActorPlayIfNeeded(Basket);
	FText Failure;
	if (!Player.Carry->TryTakePhysicalObject(Basket, Failure))
	{
		AddError(Failure.ToString());
		return false;
	}
	FPlayerInteractionContext Context;
	Context.Interactor = Player.Pawn;
	Context.CarryComponent = Player.Carry;
	Context.InteractionComponent = Player.Interaction;
	auto Query = Port->QueryInteraction(Context);
	TestTrue(TEXT("Waiting take is allowed"), Query.bCanHeldTake);
	Port->NotifyInteractionFocusChanged(*Player.Interaction, Query);
	TestEqual(TEXT("Take opens lid target"), Lid->GetTargetAlpha(), 1.f);
	TestTrue(TEXT("Take cue visible"), Machine->GetDisplayCue()->GetTakeHighlightProxy()->IsVisible());
	TestTrue(TEXT("Waiting take actually transfers input"),
			 Port->ExecuteHeldTargetUse(Context, EPlayerHeldTargetUseDirection::Take).bSucceeded);
	TestEqual(TEXT("Basket receives used towel"), Basket->GetInventory()->GetSnapshot().State, ETowelState::Used);
	TestEqual(TEXT("Waiting take empty reason"), Port->QueryInteraction(Context).HeldTakeFailureReason.ToString(),
			  FString(TEXT("꺼낼 수건 없음")));
	Query = Port->QueryInteraction(Context);
	TestTrue(TEXT("Apply allowed after take"), Query.bCanHeldApply);
	Port->NotifyInteractionFocusChanged(*Player.Interaction, Query);
	TestEqual(TEXT("Apply also opens"), Lid->GetTargetAlpha(), 1.f);
	auto* Preview = Machine->GetDisplayCue()->GetInsertPreview();
	TestTrue(TEXT("Pile insert preview shown"), Preview->IsVisible());
	FTransform BeforeInsert = Preview->GetRelativeTransform();
	TestTrue(TEXT("Apply commits one"),
			 Port->ExecuteHeldTargetUse(Context, EPlayerHeldTargetUseDirection::Apply).bSucceeded);
	Pile->SynchronizeImmediately();
	Pile->LayerRecords[0].Bucket->GetInstanceTransform(Pile->LayerRecords[0].InstanceIndex, Actual, false);
	TestTrue(TEXT("Focus preview equals committed instance"), BeforeInsert.Equals(Actual));
	Port->NotifyInteractionFocusEnded(*Player.Interaction);
	TestEqual(TEXT("Focus ended closes target"), Lid->GetTargetAlpha(), 0.f);
	TestFalse(TEXT("Focus ended hides both cues"),
			  Preview->IsVisible() || Machine->GetDisplayCue()->GetTakeHighlightProxy()->IsVisible());
	// Suppression through the real generic focus observer path.
	Machine->SetActorLocation(FVector(120, 0, 0));
	Port->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	Port->SetCollisionResponseToAllChannels(ECR_Ignore);
	Port->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
	TArray<UPrimitiveComponent*> Primitives;
	Machine->GetComponents(Primitives);
	for (auto* Primitive : Primitives)
	{
		if (Primitive != Port)
		{
			Primitive->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		}
	}
	Player.Interaction->RefreshInteractionQuery();
	TestEqual(TEXT("Actual port focus opens"), Lid->GetTargetAlpha(), 1.f);
	Player.Interaction->SetInteractionSuppressed(true);
	TestEqual(TEXT("Suppression closes"), Lid->GetTargetAlpha(), 0.f);
	TestFalse(TEXT("Suppression hides highlight"), Machine->GetDisplayCue()->GetTakeHighlightProxy()->IsVisible());
	Player.Interaction->SetInteractionSuppressed(false);
	TestTrue(TEXT("Processing starts"), Machine->StartProcessing(Failure));
	TestEqual(TEXT("Start closes immediately"), Lid->GetOpenAlpha(), 0.f);
	TestEqual(TEXT("Start clears open request"), Lid->GetTargetAlpha(), 0.f);
	Query = Port->QueryInteraction(Context);
	TestFalse(TEXT("Processing cannot apply or take"), Query.bCanHeldApply || Query.bCanHeldTake);
	FPlayerInteractionQuery Stale;
	Stale.bHeldApplyVisible = true;
	Stale.bCanHeldApply = true;
	Port->NotifyInteractionFocusChanged(*Player.Interaction, Stale);
	TestEqual(TEXT("Processing refuses even stale opening query"), Lid->GetTargetAlpha(), 0.f);
	Port->NotifyInteractionFocusEnded(*Player.Interaction);
	// Clean shelf and used-bin share cue rules and real transfer direction.
	auto* Shelf = World->SpawnActor<ACleanTowelStackActor>();
	BeginActorPlayIfNeeded(Shelf);
	Shelf->GetFacilityPlacementComponent()->SetPlacedDomainActive(true);
	auto* Stack = Shelf->FindComponentByClass<UTowelStackVisualComponent>();
	Stack->MeshProfile = Profile;
	Commit(Shelf->GetInventory(), ETowelState::Clean, 2);
	Stack->SynchronizeImmediately();
	Query = Shelf->QueryInteraction(Context);
	Shelf->NotifyInteractionFocusChanged(*Player.Interaction, Query);
	TestTrue(TEXT("Shelf LIFO highlight appears"), Shelf->GetDisplayCue()->GetTakeHighlightProxy()->IsVisible());
	TestTrue(TEXT("Shelf Take commits clean towel"),
			 Shelf->ExecuteHeldTargetUse(Context, EPlayerHeldTargetUseDirection::Take).bSucceeded);
	TestEqual(TEXT("Shelf Take leaves one"), Shelf->GetInventory()->GetSnapshot().Count, 1);
	TestEqual(TEXT("Basket becomes clean"), Basket->GetInventory()->GetSnapshot().State, ETowelState::Clean);
	Commit(Shelf->GetInventory(), ETowelState::None, 0);
	TestEqual(TEXT("Shelf empty take reason"), Shelf->QueryInteraction(Context).HeldTakeFailureReason.ToString(),
			  FString(TEXT("꺼낼 수건 없음")));
	auto* Bin = World->SpawnActor<AUsedTowelBinActor>();
	BeginActorPlayIfNeeded(Bin);
	Bin->GetFacilityPlacementComponent()->SetPlacedDomainActive(true);
	auto* BinVisual = Bin->FindComponentByClass<UTowelStackVisualComponent>();
	if (!TestNotNull(TEXT("Used bin keeps its existing Stack visual"), BinVisual))
	{
		return false;
	}
	BinVisual->MeshProfile = Profile;
	Commit(Basket->GetInventory(), ETowelState::None, 0);
	Commit(Bin->GetInventory(), ETowelState::Used, 2);
	BinVisual->SynchronizeImmediately();
	Query = Bin->QueryInteraction(Context);
	Bin->NotifyInteractionFocusChanged(*Player.Interaction, Query);
	TestFalse(TEXT("Used bin has no Apply or insert preview"),
			  Query.bCanHeldApply || Bin->GetDisplayCue()->GetInsertPreview()->IsVisible());
	TestTrue(TEXT("Used bin take highlight appears"), Bin->GetDisplayCue()->GetTakeHighlightProxy()->IsVisible());
	Bin->NotifyInteractionFocusEnded(*Player.Interaction);
	TestFalse(TEXT("Used bin focus ended hides cue"), Bin->GetDisplayCue()->GetTakeHighlightProxy()->IsVisible());

	// Dryer uses the same lid and cue contract with Wet input and Clean output.
	auto* Dryer = World->SpawnActor<ATowelProcessingMachineActor>();
	auto* KindProperty = FindFProperty<FEnumProperty>(ATowelProcessingMachineActor::StaticClass(), TEXT("MachineKind"));
	if (!TestNotNull(TEXT("Machine kind authoring property"), KindProperty))
	{
		return false;
	}
	*KindProperty->ContainerPtrToValuePtr<ETowelMachineKind>(Dryer) = ETowelMachineKind::Dryer;
	BeginActorPlayIfNeeded(Dryer);
	Dryer->GetFacilityPlacementComponent()->SetPlacedDomainActive(true);
	auto* DryerVisual = Dryer->GetTowelVisual();
	DryerVisual->MeshProfile = Profile;
	auto* DryerPort = Dryer->FindComponentByClass<UTowelTransferPortComponent>();
	Commit(Basket->GetInventory(), ETowelState::Wet, 1);
	Query = DryerPort->QueryInteraction(Context);
	DryerPort->NotifyInteractionFocusChanged(*Player.Interaction, Query);
	TestEqual(TEXT("Wet input opens dryer lid"), Dryer->GetLidPresentation()->GetTargetAlpha(), 1.f);
	TestTrue(TEXT("Dryer wet insert preview"), Dryer->GetDisplayCue()->GetInsertPreview()->IsVisible());
	TestTrue(TEXT("Dryer accepts Wet input"),
			 DryerPort->ExecuteHeldTargetUse(Context, EPlayerHeldTargetUseDirection::Apply).bSucceeded);
	TestTrue(TEXT("Dryer starts processing"), Dryer->StartProcessing(Failure));
	TestEqual(TEXT("Dryer start closes lid"), Dryer->GetLidPresentation()->GetTargetAlpha(), 0.f);
	TickWorldForDuration(World, 10.5, 0.25f);
	TestEqual(TEXT("Dryer completes to Clean"), Dryer->GetInventory()->GetSnapshot().State, ETowelState::Clean);
	Query = DryerPort->QueryInteraction(Context);
	DryerPort->NotifyInteractionFocusChanged(*Player.Interaction, Query);
	TestEqual(TEXT("Complete dryer Take opens"), Dryer->GetLidPresentation()->GetTargetAlpha(), 1.f);
	TestTrue(TEXT("Complete dryer highlights top"), Dryer->GetDisplayCue()->GetTakeHighlightProxy()->IsVisible());
	TestTrue(TEXT("Complete dryer Take commits"),
			 DryerPort->ExecuteHeldTargetUse(Context, EPlayerHeldTargetUseDirection::Take).bSucceeded);
	TestEqual(TEXT("Emptied Complete dryer returns Waiting"), Dryer->GetMachineState(), ETowelMachineState::Waiting);
	DryerPort->NotifyInteractionFocusEnded(*Player.Interaction);
	SettingsGuard.Settings->bShowTakeHighlight = false;
	Shelf->GetDisplayCue()->ShowTakeHighlight(Cube, FTransform::Identity);
	TestFalse(TEXT("Shared settings disable highlight"), Shelf->GetDisplayCue()->GetTakeHighlightProxy()->IsVisible());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FBathhouseTowelCueRefreshTest,
								 "BathhouseSim.Towel.Display.RefreshFollowsTransfersAndRepeats",
								 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FBathhouseTowelCueRefreshTest::RunTest(const FString& Parameters)
{
	using namespace ServiceTest;
	const auto* RevisionProperty =
		FindFProperty<FInt64Property>(FPlayerInteractionQuery::StaticStruct(), TEXT("PresentationRevision"));
	if (!TestNotNull(TEXT("Presentation revision is reflected int64"), RevisionProperty))
	{
		return false;
	}
	TestFalse(TEXT("Presentation revision is not Blueprint visible"),
			  RevisionProperty->HasAnyPropertyFlags(CPF_BlueprintVisible));
	FPlayerInteractionQuery Zero, Changed;
	Changed.PresentationRevision = 1;
	TestFalse(TEXT("Query equality includes presentation revision"), Zero.Equals(Changed));

	FScopedDisplaySettings SettingsGuard;
	SettingsGuard.Settings->InsertPreviewMaterial = TSoftObjectPtr<UMaterialInterface>(
		LoadObject<UMaterialInterface>(nullptr, TEXT("/Engine/EngineMaterials/DefaultMaterial.DefaultMaterial")));
	SettingsGuard.Settings->bShowTakeHighlight = true;
	const auto* VariantsProperty =
		FindFProperty<FArrayProperty>(UTowelVisualMeshProfile::StaticClass(), TEXT("StateVariants"));
	const auto* ProfileProperty =
		FindFProperty<FObjectPropertyBase>(UTowelQuantityVisualComponent::StaticClass(), TEXT("MeshProfile"));
	const auto* InitialStateProperty =
		FindFProperty<FEnumProperty>(UTowelInventoryComponent::StaticClass(), TEXT("InitialState"));
	const auto* InitialCountProperty =
		FindFProperty<FIntProperty>(UTowelInventoryComponent::StaticClass(), TEXT("InitialCount"));
	const auto* KindProperty =
		FindFProperty<FEnumProperty>(ATowelProcessingMachineActor::StaticClass(), TEXT("MachineKind"));
	if (!VariantsProperty || !ProfileProperty || !InitialStateProperty || !InitialCountProperty || !KindProperty)
	{
		AddError(TEXT("Missing reflected fixture authoring properties"));
		return false;
	}
	auto* Cube = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
	auto* Profile = NewObject<UTowelVisualMeshProfile>();
	auto* Variants = VariantsProperty->ContainerPtrToValuePtr<TArray<FTowelStateMeshVariants>>(Profile);
	for (ETowelState State : {ETowelState::Clean, ETowelState::Used, ETowelState::Wet})
	{
		FTowelStateMeshVariants Entry;
		Entry.State = State;
		Entry.MeshVariants.Add(Cube);
		Variants->Add(Entry);
	}
	auto InitializeInventory = [&](UTowelInventoryComponent* Inventory, ETowelState State, int32 Count)
	{
		*InitialStateProperty->ContainerPtrToValuePtr<ETowelState>(Inventory) = State;
		InitialCountProperty->SetPropertyValue_InContainer(Inventory, Count);
	};
	for (int32 TargetIndex = 0; TargetIndex < 4; ++TargetIndex)
	{
		const TCHAR* Names[] = {TEXT("Shelf"), TEXT("UsedBin"), TEXT("Washer"), TEXT("Dryer")};
		const FString Label = Names[TargetIndex];
		FScopedUtilityLaborWorld Scope(*FString::Printf(TEXT("TowelCueRefresh%sWorld"), Names[TargetIndex]));
		auto* World = Scope.Get();
		FPlayer Player;
		if (!BuildPlayer(*this, World, Player))
		{
			return false;
		}
		const ETowelState State = TargetIndex == 0	 ? ETowelState::Clean
								  : TargetIndex == 3 ? ETowelState::Wet
													 : ETowelState::Used;
		const bool bBin = TargetIndex == 1;
		const FTransform Location(FVector(200, 0, 0));
		AActor* Target = nullptr;
		if (TargetIndex == 0)
		{
			Target = World->SpawnActorDeferred<ACleanTowelStackActor>(ACleanTowelStackActor::StaticClass(), Location);
		}
		else if (bBin)
		{
			Target = World->SpawnActorDeferred<AUsedTowelBinActor>(AUsedTowelBinActor::StaticClass(), Location);
		}
		else
		{
			Target = World->SpawnActorDeferred<ATowelProcessingMachineActor>(
				ATowelProcessingMachineActor::StaticClass(), Location);
			*KindProperty->ContainerPtrToValuePtr<ETowelMachineKind>(Target) =
				TargetIndex == 3 ? ETowelMachineKind::Dryer : ETowelMachineKind::Washer;
		}
		auto* Inventory = Target ? Target->FindComponentByClass<UTowelInventoryComponent>() : nullptr;
		auto* Visual = Target ? Target->FindComponentByClass<UTowelQuantityVisualComponent>() : nullptr;
		auto* Cue = Target ? Target->FindComponentByClass<UDisplayCueComponent>() : nullptr;
		if (!Target || !Inventory || !Visual || !Cue)
		{
			AddError(Label + TEXT(" fixture is incomplete"));
			return false;
		}
		InitializeInventory(Inventory, State, bBin ? 4 : 3);
		ProfileProperty->SetObjectPropertyValue_InContainer(Visual, Profile);
		Target->FinishSpawning(Location);
		BeginActorPlayIfNeeded(Target);
		auto* Placement = Target->FindComponentByClass<UFacilityPlacementComponent>();
		Placement->SetPlacedDomainActive(true);
		UPrimitiveComponent* TraceTarget = TargetIndex < 2
											   ? Cast<UPrimitiveComponent>(Target->GetRootComponent())
											   : Target->FindComponentByClass<UTowelTransferPortComponent>();
		TArray<UPrimitiveComponent*> Primitives;
		Target->GetComponents(Primitives);
		for (auto* Primitive : Primitives)
		{
			Primitive->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		}
		TraceTarget->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
		TraceTarget->SetCollisionResponseToAllChannels(ECR_Ignore);
		TraceTarget->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);

		const FTransform BasketLocation(FVector(0, 500, 500));
		auto* Basket = World->SpawnActorDeferred<ATowelBasketActor>(ATowelBasketActor::StaticClass(), BasketLocation);
		InitializeInventory(Basket->GetInventory(), State, bBin ? 2 : 6);
		auto* BasketVisual = Basket->FindComponentByClass<UTowelQuantityVisualComponent>();
		ProfileProperty->SetObjectPropertyValue_InContainer(BasketVisual, Profile);
		Basket->FinishSpawning(BasketLocation);
		BeginActorPlayIfNeeded(Basket);
		FText Failure;
		if (!TestTrue(Label + TEXT(" basket is held"), Player.Carry->TryTakePhysicalObject(Basket, Failure)))
		{
			return false;
		}
		Player.Camera->SetWorldLocationAndRotation(FVector::ZeroVector, FRotator::ZeroRotator);
		Player.Interaction->RefreshInteractionQuery();
		FHitResult Hit;
		if (!TestTrue(Label + TEXT(" actual trace focuses target"),
					  Player.Interaction->GetCurrentFocusHit(Hit) && Hit.GetComponent() == TraceTarget))
		{
			return false;
		}
		auto CheckCues = [&]()
		{
			FHitResult CueHit;
			TestTrue(Label + TEXT(" cue check retains actual target focus"),
					 Player.Interaction->GetCurrentFocusHit(CueHit) && CueHit.GetComponent() == TraceTarget);
			const auto Query = Player.Interaction->GetCurrentInteractionQuery();
			const auto Snapshot = Inventory->GetSnapshot();
			TestEqual(Label + TEXT(" revision is target plus held basket"), Query.PresentationRevision,
					  Snapshot.Revision + Basket->GetInventory()->GetSnapshot().Revision);
			auto* Preview = Cue->GetInsertPreview();
			auto* Highlight = Cue->GetTakeHighlightProxy();
			TestEqual(Label + TEXT(" preview availability"), Preview->IsVisible(), Query.bCanHeldApply);
			TestEqual(Label + TEXT(" highlight availability"), Highlight->IsVisible(), Query.bCanHeldTake);
			UStaticMesh* Mesh = nullptr;
			FTransform Expected;
			if (Query.bCanHeldApply)
			{
				TestTrue(Label + TEXT(" next index exists"),
						 Visual->GetIndexPresentation(Snapshot.Count, State, Mesh, Expected));
				TestTrue(Label + TEXT(" preview follows new Count"), Preview->GetRelativeTransform().Equals(Expected));
			}
			if (Query.bCanHeldTake)
			{
				TestTrue(Label + TEXT(" top index exists"),
						 Visual->GetIndexPresentation(Snapshot.Count - 1, Snapshot.State, Mesh, Expected));
				TestTrue(Label + TEXT(" highlight follows new Count minus one"),
						 Highlight->GetRelativeTransform().Equals(Expected));
			}
		};
		auto CheckQueryChange = [&](FPlayerInteractionQuery Before)
		{
			const auto After = Player.Interaction->GetCurrentInteractionQuery();
			TestFalse(Label + TEXT(" count-only queries differ"), Before.Equals(After));
			TestTrue(Label + TEXT(" target HUD stays unchanged"), Before.TargetName.EqualTo(After.TargetName));
			Before.PresentationRevision = After.PresentationRevision;
			TestTrue(Label + TEXT(" all query fields except revision stay unchanged"), Before.Equals(After));
		};
		CheckCues();
		if (!bBin)
		{
			const int32 BeforeCount = Inventory->GetSnapshot().Count;
			for (int32 Step = 0; Step < 3; ++Step)
			{
				const auto Before = Player.Interaction->GetCurrentInteractionQuery();
				if (Step == 0)
				{
					Player.HeldUse->BeginUse(EPlayerHeldTargetUseDirection::Apply);
				}
				else
				{
					Player.HeldUse->TickComponent(0.16f, LEVELTICK_All, nullptr);
				}
				TestEqual(Label + TEXT(" each LMB transfers exactly one"), Inventory->GetSnapshot().Count,
						  BeforeCount + Step + 1);
				CheckQueryChange(Before);
				CheckCues();
			}
			Player.HeldUse->EndUse();
		}
		else
		{
			TestFalse(TEXT("Used bin has no Apply or preview"),
					  Player.Interaction->GetCurrentInteractionQuery().bCanHeldApply ||
						  Cue->GetInsertPreview()->IsVisible());
		}
		const int32 BeforeTakeCount = Inventory->GetSnapshot().Count;
		for (int32 Step = 0; Step < 3; ++Step)
		{
			const auto Before = Player.Interaction->GetCurrentInteractionQuery();
			if (Step == 0)
			{
				Player.HeldUse->BeginUse(EPlayerHeldTargetUseDirection::Take);
			}
			else
			{
				Player.HeldUse->TickComponent(0.16f, LEVELTICK_All, nullptr);
			}
			TestEqual(Label + TEXT(" each RMB transfers exactly one"), Inventory->GetSnapshot().Count,
					  BeforeTakeCount - Step - 1);
			CheckQueryChange(Before);
			CheckCues();
		}
		Player.HeldUse->EndUse();

		auto* Customer = World->SpawnActor<AActor>();
		auto* CustomerInventory = NewObject<UTowelInventoryComponent>(Customer);
		Customer->AddInstanceComponent(CustomerInventory);
		CustomerInventory->RegisterComponent();
		BeginActorPlayIfNeeded(Customer);
		auto TransferOne = [&](UTowelInventoryComponent* Source)
		{
			FTowelTransferRequest Request;
			Request.Source = Source;
			Request.Destination = CustomerInventory;
			Request.RequestedCount = 1;
			Request.ExpectedSourceRevision = Source->GetSnapshot().Revision;
			Request.ExpectedDestinationRevision = CustomerInventory->GetSnapshot().Revision;
			return World->GetSubsystem<UTowelTransferSubsystem>()->TryTransfer(Request).bSucceeded;
		};
		if (TargetIndex == 0)
		{
			const auto Before = Player.Interaction->GetCurrentInteractionQuery();
			TestTrue(TEXT("Customer-side shelf transfer commits"), TransferOne(Inventory));
			Player.Interaction->RefreshInteractionQuery();
			CheckQueryChange(Before);
			CheckCues();
		}
		const auto BeforeBasketChange = Player.Interaction->GetCurrentInteractionQuery();
		const int64 TargetRevision = Inventory->GetSnapshot().Revision;
		TestTrue(Label + TEXT(" held-basket-only change commits"), TransferOne(Basket->GetInventory()));
		TestEqual(Label + TEXT(" basket-only change leaves target revision"), Inventory->GetSnapshot().Revision,
				  TargetRevision);
		Player.Interaction->RefreshInteractionQuery();
		CheckQueryChange(BeforeBasketChange);
		CheckCues();

		FPlayerInteractionContext Context;
		Context.CarryComponent = Player.Carry;
		TestEqual(Label + TEXT(" missing target inventory still includes basket"),
				  TowelDisplayCueUtils::GetPresentationRevision(Context, nullptr),
				  Basket->GetInventory()->GetSnapshot().Revision);
		Context.CarryComponent = nullptr;
		TestEqual(Label + TEXT(" no basket includes target only"),
				  TowelDisplayCueUtils::GetPresentationRevision(Context, Inventory), Inventory->GetSnapshot().Revision);
		TestEqual(TEXT("No inventories has zero revision"),
				  TowelDisplayCueUtils::GetPresentationRevision(Context, nullptr), int64(0));
		Context.CarryComponent = Player.Carry;
		Placement->SetPlacedDomainActive(false);
		const IPlayerInteractable* Interactable = Cast<IPlayerInteractable>(TraceTarget);
		if (!Interactable)
		{
			Interactable = Cast<IPlayerInteractable>(Target);
		}
		TestEqual(Label + TEXT(" inactive query keeps revision"),
				  Interactable->QueryInteraction(Context).PresentationRevision,
				  Inventory->GetSnapshot().Revision + Basket->GetInventory()->GetSnapshot().Revision);
		Placement->SetPlacedDomainActive(true);
		TraceTarget->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
		auto* OrphanPort = NewObject<UTowelTransferPortComponent>();
		TestEqual(TEXT("Ownerless port keeps held basket revision"),
				  OrphanPort->QueryInteraction(Context).PresentationRevision,
				  Basket->GetInventory()->GetSnapshot().Revision);

		auto* Fridge = World->SpawnActor<ADrinkFridgeActor>(FVector(0, 1000, 0), FRotator::ZeroRotator);
		auto* Space = NewObject<UDisplaySpaceComponent>(Fridge);
		auto* Router = NewObject<UDisplayFacilityTargetComponent>(Fridge);
		TestEqual(TEXT("Fridge query revision remains zero"), Fridge->QueryInteraction(Context).PresentationRevision,
				  int64(0));
		TestEqual(TEXT("Display space query revision remains zero"),
				  Space->QueryInteraction(Context).PresentationRevision, int64(0));
		TestEqual(TEXT("Facility router query revision remains zero"),
				  Router->QueryInteraction(Context).PresentationRevision, int64(0));

		Player.Interaction->SetInteractionSuppressed(true);
		TestFalse(Label + TEXT(" suppression hides cues"),
				  Cue->GetInsertPreview()->IsVisible() || Cue->GetTakeHighlightProxy()->IsVisible());
		Player.Interaction->SetInteractionSuppressed(false);
		CheckCues();
		Player.Camera->SetWorldRotation(FRotator(0, 180, 0));
		Player.Interaction->RefreshInteractionQuery();
		TestFalse(Label + TEXT(" focus exit hides cues"),
				  Cue->GetInsertPreview()->IsVisible() || Cue->GetTakeHighlightProxy()->IsVisible());
	}
	return true;
}

#endif
