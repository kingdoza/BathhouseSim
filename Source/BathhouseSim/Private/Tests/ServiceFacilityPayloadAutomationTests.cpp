#if WITH_DEV_AUTOMATION_TESTS
#include "Tests/ServiceFacilityAutomationTestSupport.h"
#include "Facility/FacilityPlacementExtensionUtils.h"
#include "Facility/BathhouseFacilitySubsystem.h"
#include "Shop/BathhouseTrashBinActor.h"

using namespace ServiceFacilityTest;
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FBathhouseServiceFacilityPayloadTest,
								 "BathhouseSim.Service.Facility.ConstructedPayloadRoundTrip",
								 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FBathhouseServiceFacilityPayloadTest::RunTest(const FString& Parameters)
{
	for (bool bShower : {false, true})
	{
		FScopedUtilityLaborWorld Scope(bShower ? TEXT("ShowerPayloadWorld") : TEXT("VanityPayloadWorld"));
		ServiceFacilityTest::FFixture Fixture(Scope.Get());
		if (!TestTrue(TEXT("Construction-created facility installs"), Fixture.Install(*this, bShower)))
		{
			return false;
		}
		FText Failure;
		auto Spaces = Fixture.Facility->GetSpaces();
		TestTrue(TEXT("Fresh payload applies after construction"),
				 Fixture.Facility->GetManager()->GetTotalStock() == 0);
		TestTrue(TEXT("Stock with remaining imports"), Spaces[1]->ImportStock(Fixture.Kinds[1], 2, Failure, 7));
		auto* Requestor = Fixture.World->SpawnActor<AActor>();
		BeginActorPlayIfNeeded(Requestor);
		auto* Subsystem = Fixture.World->GetSubsystem<UBathhouseFacilitySubsystem>();
		auto Type = Fixture.Facility->GetFacilityType();
		TestTrue(TEXT("Recovery hold begins"), Fixture.Facility->TryBeginFacilityRecoveryHold(Failure));
		ABathhouseFacilityActor* Reserved = nullptr;
		UBathhouseFacilitySlotComponent* Slot = nullptr;
		TestFalse(TEXT("Hold blocks subsystem reservation"),
				  Subsystem->TryReserveRandomSlot(Type, Requestor, Reserved, Slot));
		Fixture.Facility->CancelFacilityRecoveryHold();
		TestTrue(TEXT("Cancel restores reservation"), Subsystem->TryReserveRandomSlot(Type, Requestor, Reserved, Slot));
		TestFalse(TEXT("Reserved facility cannot recover"), Fixture.Facility->QueryFacilityRecovery().bSucceeded);
		if (!Slot)
		{
			return false;
		}
		Slot->Release(Requestor);
		auto* Item = FFacilityActorConversionTransaction::RecoverFacilityToItem(*Fixture.Facility, Failure);
		if (!TestNotNull(TEXT("Recovery item exists"), Item))
		{
			AddError(Failure.ToString());
			return false;
		}
		auto* Data = GetDisplayData(Item->GetPlacementPayload());
		if (!TestNotNull(TEXT("Generic payload contains display extension"), Data))
		{
			return false;
		}
		TestEqual(TEXT("Remaining exports"), Data->Spaces[1].InUseRemaining, 7);
		auto* Base = CastChecked<UBathhouseFacilityPlacementInstanceData>(Item->GetPlacementPayload().InstanceData);
		TestTrue(TEXT("Summary includes both fresh and used item count"),
				 Base->GetPlacementContentsSummary().ToString().Contains(bShower ? TEXT("바디워시 2개")
																				 : TEXT("스킨로션 2개")));
		TestTrue(TEXT("Recovered item held"), Fixture.Player.Carry->TryTakePhysicalObject(Item, Failure));
		const auto Good = Data->Spaces;
		auto TryBad = [&](const TCHAR* Label, const TFunction<void()>& Mutate)
		{
			Mutate();
			auto* Placed = FFacilityActorConversionTransaction::PlaceItemAsFacility(
				*Item, Fixture.Placement, *Fixture.Zone, *Fixture.Player.Carry, Failure);
			TestNull(FString::Printf(TEXT("%s rejected"), Label), Placed);
			TestTrue(TEXT("Rejected import retains original payload and held item"),
					 IsValid(Item) && GetDisplayData(Item->GetPlacementPayload()) == Data &&
						 Fixture.Player.Carry->GetHeldObject() == Item);
			int32 Count = 0;
			for (TActorIterator<AServiceAutomationDisplayFacility> It(Fixture.World); It; ++It)
			{
				if (IsValid(*It))
				{
					++Count;
				}
			}
			TestEqual(TEXT("Failed finalize leaves no constructed facility"), Count, 0);
			Data->Spaces = Good;
		};
		TryBad(TEXT("Remaining above per-item uses"),
			   [&]
			   {
				   Data->Spaces[1].InUseRemaining = 31;
			   });
		TryBad(TEXT("Remaining on empty stock"),
			   [&]
			   {
				   Data->Spaces[0].InUseRemaining = 1;
			   });
		TryBad(TEXT("Wrong fixed kind"),
			   [&]
			   {
				   Data->Spaces[1].Kind = Fixture.Kinds[0];
			   });
		TryBad(TEXT("Capacity overflow"),
			   [&]
			   {
				   Data->Spaces[1].Count = 99;
			   });
		TryBad(TEXT("Missing snapshot"),
			   [&]
			   {
				   Data->Spaces.Pop();
			   });
		auto* Reinstalled =
			Cast<AServiceAutomationDisplayFacility>(FFacilityActorConversionTransaction::PlaceItemAsFacility(
				*Item, Fixture.Placement, *Fixture.Zone, *Fixture.Player.Carry, Failure));
		if (!TestNotNull(TEXT("Valid payload reinstalls"), Reinstalled))
		{
			AddError(Failure.ToString());
			return false;
		}
		Fixture.Facility = Reinstalled;
		BeginActorPlayIfNeeded(Reinstalled);
		auto Second = Reinstalled->GetSpaces();
		TestEqual(TEXT("Round trip count"), Second[1]->GetStock().Count, 2);
		TestEqual(TEXT("Round trip remaining"), Second[1]->GetStock().InUseRemaining, 7);
		// Discard through the actual trash target, with no wallet side effect.
		auto* Discard = FFacilityActorConversionTransaction::RecoverFacilityToItem(*Reinstalled, Failure);
		if (!TestNotNull(TEXT("Reinstalled facility recovers again"), Discard))
		{
			return false;
		}
		Fixture.Player.Carry->TryTakePhysicalObject(Discard, Failure);
		const int32 Before = Fixture.Player.PlayerState->GetWallet()->GetCurrentMoney();
		auto* Trash = Fixture.World->SpawnActor<ABathhouseTrashBinActor>();
		FPlayerInteractionContext Context;
		Context.Interactor = Fixture.Player.Pawn;
		Context.CarryComponent = Fixture.Player.Carry;
		TestTrue(TEXT("Stock-bearing facility discards"), Trash->ExecuteInteraction(Context).bSucceeded);
		TestTrue(TEXT("Discard clears held item"), !IsValid(Discard) && Fixture.Player.Carry->IsHandEmpty());
		TestEqual(TEXT("Discard gives no money"), Fixture.Player.PlayerState->GetWallet()->GetCurrentMoney(), Before);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FBathhouseServiceGenericExtensionTest,
								 "BathhouseSim.Service.Facility.ExtensionAtomicValidation",
								 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FBathhouseServiceGenericExtensionTest::RunTest(const FString& Parameters)
{
	FScopedUtilityLaborWorld Scope(TEXT("GenericExtensionsWorld"));
	auto* Owner = Scope.Get()->SpawnActor<AActor>();
	auto AddExtension = [&](FName Key)
	{
		auto* Extension = NewObject<UServiceAutomationExtension>(Owner);
		Extension->FixtureKey = Key;
		Owner->AddInstanceComponent(Extension);
		Extension->RegisterComponent();
		return Extension;
	};
	auto* A = AddExtension(TEXT("First"));
	auto* B = AddExtension(TEXT("Second"));
	auto* Data = NewObject<UBathhouseFacilityPlacementInstanceData>();
	A->Value = 11;
	B->Value = 22;
	FText Failure;
	TestTrue(TEXT("Two independent extensions export"),
			 FacilityPlacementExtensionUtils::Export(*Owner, *Data, Failure));
	A->Value = 1;
	B->Value = 2;
	B->bReject = true;
	TestFalse(TEXT("One rejection aborts entire import"),
			  FacilityPlacementExtensionUtils::Import(*Owner, Data, Failure));
	TestTrue(TEXT("No extension applies before all validate"),
			 A->ApplyCalls == 0 && B->ApplyCalls == 0 && A->Value == 1 && B->Value == 2);
	B->bReject = false;
	auto Good = Data->Extensions;
	const auto Duplicate = Data->Extensions[0];
	Data->Extensions.Add(Duplicate);
	TestFalse(TEXT("Duplicate data key rejected"), FacilityPlacementExtensionUtils::Import(*Owner, Data, Failure));
	Data->Extensions = Good;
	Data->Extensions.Pop();
	TestFalse(TEXT("Missing data key rejected"), FacilityPlacementExtensionUtils::Import(*Owner, Data, Failure));
	Data->Extensions = Good;
	const FName OriginalKey = Data->Extensions[0]->Key;
	Data->Extensions[0]->Key = TEXT("Unknown");
	TestFalse(TEXT("Unknown data key rejected"), FacilityPlacementExtensionUtils::Import(*Owner, Data, Failure));
	Data->Extensions[0]->Key = OriginalKey;
	TestTrue(TEXT("Valid import applies all"), FacilityPlacementExtensionUtils::Import(*Owner, Data, Failure));
	TestTrue(TEXT("Both restored"), A->Value == 11 && B->Value == 22);
	TestTrue(TEXT("Fresh null applies to all"), FacilityPlacementExtensionUtils::Import(*Owner, nullptr, Failure));
	TestTrue(TEXT("Fresh values empty"), A->Value == 0 && B->Value == 0);
	B->FixtureKey = TEXT("First");
	TestFalse(TEXT("Duplicate component keys rejected"),
			  FacilityPlacementExtensionUtils::ValidateAuthoring(*Owner, Failure));
	B->FixtureKey = NAME_None;
	TestFalse(TEXT("Empty component key rejected"),
			  FacilityPlacementExtensionUtils::ValidateAuthoring(*Owner, Failure));
	return true;
}
#endif
