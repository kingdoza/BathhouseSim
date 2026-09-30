#if WITH_DEV_AUTOMATION_TESTS
#include "Tests/ServiceFacilityAutomationTestSupport.h"
#include "Facility/BathhouseExpansionDefinition.h"
#include "Facility/BathhouseFacilitySubsystem.h"
#include "Facility/LockerCapacitySubsystem.h"
using namespace ServiceFacilityTest;
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FBathhouseServiceGenericLockerHoldTest,
								 "BathhouseSim.Service.Facility.LockerRecoveryHoldRegression",
								 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FBathhouseServiceGenericLockerHoldTest::RunTest(const FString& Parameters)
{
	FScopedUtilityLaborWorld Scope(TEXT("GenericLockerHoldWorld"));
	FScopedServiceGrid Grid;
	auto* World = Scope.Get();
	FPlayer Player;
	if (!BuildPlayer(*this, World, Player))
	{
		return false;
	}
	auto* Expansion = NewObject<UBathhouseExpansionDefinition>();
	Expansion->Tiers = {{4, 4}};
	auto* Authority = World->SpawnActorDeferred<AServiceAutomationExpansion>(AServiceAutomationExpansion::StaticClass(),
																			 FTransform(FVector(8000, 0, 0)));
	Authority->ConfigureForTest(Expansion);
	Authority->FinishSpawning(FTransform(FVector(8000, 0, 0)));
	BeginActorPlayIfNeeded(Authority);
	auto* Definition = MakeFridgeDefinition();
	Definition->PlacedFacilityClass = AFacilityPlacementLockerAutomationActor::StaticClass();
	Definition->LockerSlotCount = 1;
	auto* Zone = World->SpawnActor<AFacilityPlacementZoneAutomationActor>(
		AFacilityPlacementZoneAutomationActor::StaticClass(), FTransform(FVector(12000, 0, 0)));
	Zone->GetZoneBounds()->SetBoxExtent(FVector(500));
	for (auto Tag : Definition->FacilityTags)
	{
		Zone->AddAllowedTag(Tag);
	}
	FText Failure;
	auto* Item =
		APlaceableFacilityItemActor::SpawnFreshItem(*World, *Definition, FTransform(FVector(10000, 0, 300)), Failure);
	if (!Item || !Item->ActivateFreeWorld(Item->GetActorTransform(), Failure) ||
		!Player.Carry->TryTakePhysicalObject(Item, Failure))
	{
		return false;
	}
	auto* Locker = Cast<ABathhouseFacilityActor>(FFacilityActorConversionTransaction::PlaceItemAsFacility(
		*Item, FTransform(FVector(12000, 0, 100)), *Zone, *Player.Carry, Failure));
	if (!TestNotNull(TEXT("Locker installs through actual transaction"), Locker))
	{
		AddError(Failure.ToString());
		return false;
	}
	BeginActorPlayIfNeeded(Locker);
	auto* Facilities = World->GetSubsystem<UBathhouseFacilitySubsystem>();
	auto* User = World->SpawnActor<AActor>();
	BeginActorPlayIfNeeded(User);
	ABathhouseFacilityActor* Selected = nullptr;
	UBathhouseFacilitySlotComponent* Slot = nullptr;
	TestTrue(TEXT("Locker is initially available"), Locker->IsAvailableForReservation());
	TestTrue(TEXT("Locker recovery hold starts"), Locker->TryBeginFacilityRecoveryHold(Failure));
	TestFalse(TEXT("Q hold blocks locker reservation"),
			  Facilities->TryReserveRandomSlot(EBathhouseFacilityType::ClothesLocker, User, Selected, Slot));
	Locker->CancelFacilityRecoveryHold();
	TestTrue(TEXT("Cancel restores locker reservation"),
			 Facilities->TryReserveRandomSlot(EBathhouseFacilityType::ClothesLocker, User, Selected, Slot));
	if (Slot)
	{
		Slot->Release(User);
	}
	auto* Recovered = FFacilityActorConversionTransaction::RecoverFacilityToItem(*Locker, Failure);
	TestNotNull(TEXT("Locker still recovers with original result"), Recovered);
	TestEqual(TEXT("Recovery removes capacity"),
			  World->GetSubsystem<ULockerCapacitySubsystem>()->GetInstalledLockerCapacity(), 0);
	return true;
}
#endif
