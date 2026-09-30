#if WITH_DEV_AUTOMATION_TESTS

#include "Tests/ServiceFacilityAutomationTestSupport.h"

#include "Customer/BathhouseCustomerCharacter.h"
#include "Customer/CustomerSessionComponent.h"
#include "Facility/BathhouseFacilitySubsystem.h"
#include "Kismet/GameplayStatics.h"
#include "Placement/FacilityPlacementComponent.h"

using namespace ServiceFacilityTest;

namespace
{
	ABathhouseCustomerCharacter* SpawnSessionCustomer(UWorld& World)
	{
		const FTransform Transform(FVector(14000, 0, 300));
		auto* Customer = World.SpawnActorDeferred<ABathhouseCustomerCharacter>(
			ABathhouseCustomerCharacter::StaticClass(), Transform);
		if (!Customer)
		{
			return nullptr;
		}
		Customer->AutoPossessAI = EAutoPossessAI::Disabled;
		Customer = Cast<ABathhouseCustomerCharacter>(UGameplayStatics::FinishSpawningActor(Customer, Transform));
		BeginActorPlayIfNeeded(Customer);
		return Customer;
	}
} // namespace

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FBathhouseServiceReservationCycleTest,
								 "BathhouseSim.Service.Facility.ReservationCycleConsumptionAndKnockdown",
								 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FBathhouseServiceReservationCycleTest::RunTest(const FString& Parameters)
{
	FScopedUtilityLaborWorld Scope(TEXT("ServiceReservationCycleWorld"));
	FFixture Fixture(Scope.Get());
	if (!Fixture.Install(*this, true))
	{
		return false;
	}
	const auto Spaces = Fixture.Facility->GetSpaces();
	const auto Slots = Fixture.Facility->GetFacilitySlots();
	FText Failure;
	TestTrue(TEXT("Two fresh shampoo bottles"), Spaces[0]->ImportStock(Fixture.Kinds[0], 2, Failure));
	TestTrue(TEXT("Two fresh bodywash bottles"), Spaces[1]->ImportStock(Fixture.Kinds[1], 2, Failure));
	auto* A = Fixture.World->SpawnActor<AActor>();
	auto* B = Fixture.World->SpawnActor<AActor>();
	BeginActorPlayIfNeeded(A);
	BeginActorPlayIfNeeded(B);
	auto CheckRemaining = [&](const TCHAR* Label, int32 Remaining)
	{
		TestEqual(FString::Printf(TEXT("%s shampoo"), Label), Spaces[0]->GetStock().InUseRemaining, Remaining);
		TestEqual(FString::Printf(TEXT("%s bodywash"), Label), Spaces[1]->GetStock().InUseRemaining, Remaining);
	};
	TestTrue(TEXT("Reserve starts cycle without consumption"), Slots[0]->TryReserve(A));
	CheckRemaining(TEXT("Reserved"), 0);
	TestTrue(TEXT("First occupancy consumes once"), Slots[0]->BeginUse(A));
	CheckRemaining(TEXT("First occupancy"), 29);
	TestTrue(TEXT("Suspension keeps reservation"), Slots[0]->EndUse(A));
	TestEqual(TEXT("Suspended state"), Slots[0]->GetSlotState(), EBathhouseFacilitySlotState::Reserved);
	TestTrue(TEXT("Same cycle resumes"), Slots[0]->BeginUse(A));
	CheckRemaining(TEXT("Resume does not consume"), 29);
	TestTrue(TEXT("Release ends cycle"), Slots[0]->Release(A));
	TestTrue(TEXT("Next cycle starts"), Slots[0]->TryReserve(A) && Slots[0]->BeginUse(A));
	CheckRemaining(TEXT("Released then reserved"), 28);
	Slots[0]->ForceRelease();
	TestTrue(TEXT("ForceRelease also ends cycle"), Slots[0]->TryReserve(A) && Slots[0]->BeginUse(A));
	CheckRemaining(TEXT("Force-released then reserved"), 27);
	TestTrue(TEXT("Second slot has an independent cycle"), Slots[1]->TryReserve(B) && Slots[1]->BeginUse(B));
	CheckRemaining(TEXT("Two slots share stock"), 26);
	TestTrue(TEXT("Second slot can suspend/resume"), Slots[1]->EndUse(B) && Slots[1]->BeginUse(B));
	CheckRemaining(TEXT("Second slot resumes without consumption"), 26);
	Slots[0]->ForceRelease();
	Slots[1]->ForceRelease();

	auto* Customer = SpawnSessionCustomer(*Fixture.World);
	if (!TestNotNull(TEXT("Actual customer session fixture"), Customer))
	{
		return false;
	}
	auto* Session = Customer->GetCustomerSession();
	TestTrue(TEXT("Actual session reserves shower"), Session->TryReserveFacility(EBathhouseFacilityType::Shower));
	TestTrue(TEXT("Actual session begins shower"), Session->BeginUseCurrentFacility());
	CheckRemaining(TEXT("Customer first shower"), 25);
	TestTrue(TEXT("Actual knockdown suspension"), Session->SuspendCurrentFacilityUseForKnockdown());
	TestTrue(TEXT("Actual knockdown resumption"), Session->ResumeCurrentFacilityUseAfterKnockdown());
	CheckRemaining(TEXT("Customer knockdown does not consume again"), 25);
	TestTrue(TEXT("Repeated resume is harmless"), Session->ResumeCurrentFacilityUseAfterKnockdown());
	CheckRemaining(TEXT("Repeated customer resume"), 25);
	Session->ReleaseCurrentFacility();
	TestTrue(TEXT("Next shower reservation succeeds"), Session->TryReserveFacility(EBathhouseFacilityType::Shower));
	TestTrue(TEXT("Next shower starts"), Session->BeginUseCurrentFacility());
	CheckRemaining(TEXT("Pre/post shower are separate cycles"), 24);
	Customer->Destroy();
	TestTrue(TEXT("Actual customer EndPlay releases its slot"), Slots[0]->IsAvailable() && Slots[1]->IsAvailable());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FBathhouseServiceCancelAvailabilityTest,
								 "BathhouseSim.Service.Facility.RecoveryCancelAvailabilityAndWaitingCustomer",
								 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FBathhouseServiceCancelAvailabilityTest::RunTest(const FString& Parameters)
{
	FScopedUtilityLaborWorld Scope(TEXT("ServiceCancelAvailabilityWorld"));
	FFixture Fixture(Scope.Get());
	if (!Fixture.Install(*this, true))
	{
		return false;
	}
	auto* Facilities = Fixture.World->GetSubsystem<UBathhouseFacilitySubsystem>();
	auto* Customer = SpawnSessionCustomer(*Fixture.World);
	if (!TestNotNull(TEXT("Waiting customer fixture"), Customer))
	{
		return false;
	}
	auto* Session = Customer->GetCustomerSession();
	int32 ShowerNotifications = 0;
	int32 OtherNotifications = 0;
	const FDelegateHandle Observer = Facilities->OnFacilityAvailabilityChanged.AddLambda(
		[&](EBathhouseFacilityType Type)
		{
			if (Type == EBathhouseFacilityType::Shower)
			{
				++ShowerNotifications;
			}
			else
			{
				++OtherNotifications;
			}
		});
	FText Failure;
	TestTrue(TEXT("Shower recovery hold starts"), Fixture.Facility->TryBeginFacilityRecoveryHold(Failure));
	TestEqual(TEXT("Hold start does not publish"), ShowerNotifications, 0);
	TestFalse(TEXT("Real session fails reservation during hold and enters WaitForFacility"),
			  Session->TryReserveFacility(EBathhouseFacilityType::Shower));
	TestTrue(TEXT("WaitForFacility subscribes actual customer session"),
			 Facilities->OnFacilityAvailabilityChanged.IsBoundToObject(Session));
	Fixture.Facility->CancelFacilityRecoveryHold();
	TestEqual(TEXT("Hold cancellation publishes shower type once"), ShowerNotifications, 1);
	TestEqual(TEXT("Hold cancellation does not publish other types"), OtherNotifications, 0);
	Fixture.Facility->CancelFacilityRecoveryHold();
	TestEqual(TEXT("Repeated cancellation is silent"), ShowerNotifications, 1);
	// The headless fixture has no running authored StateTree. Exercise its retry task's domain call explicitly.
	TestTrue(TEXT("Waiting customer's retry after cancellation reserves successfully"),
			 Session->TryReserveFacility(EBathhouseFacilityType::Shower));
	TestFalse(TEXT("Successful retry removes the actual wait subscription"),
			  Facilities->OnFacilityAvailabilityChanged.IsBoundToObject(Session));
	Session->ReleaseCurrentFacility();

	ShowerNotifications = 0;
	TestTrue(TEXT("Another hold starts"), Fixture.Facility->TryBeginFacilityRecoveryHold(Failure));
	Fixture.Facility->GetFacilityPlacementComponent()->SetPlacedDomainActive(false);
	Fixture.Facility->CancelFacilityRecoveryHold();
	TestEqual(TEXT("Inactive domain cancellation is silent"), ShowerNotifications, 0);
	Fixture.Facility->GetFacilityPlacementComponent()->SetPlacedDomainActive(true);
	TestTrue(TEXT("Recovery success hold starts"), Fixture.Facility->TryBeginFacilityRecoveryHold(Failure));
	TWeakObjectPtr<ABathhouseFacilityActor> Original(Fixture.Facility);
	auto* Recovered = FFacilityActorConversionTransaction::RecoverFacilityToItem(*Fixture.Facility, Failure);
	TestNotNull(TEXT("Held shower recovers through actual conversion transaction"), Recovered);
	TestFalse(TEXT("Recovery destroys original facility"), Original.IsValid());
	TestEqual(TEXT("Recovery success publishes once"), ShowerNotifications, 1);
	Facilities->OnFacilityAvailabilityChanged.Remove(Observer);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FBathhouseServiceRouterNameTest,
								 "BathhouseSim.Service.Facility.RouterAuthoredNameAndValidation",
								 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FBathhouseServiceRouterNameTest::RunTest(const FString& Parameters)
{
	FScopedUtilityLaborWorld Scope(TEXT("ServiceRouterNameWorld"));
	FFixture Fixture(Scope.Get());
	if (!Fixture.Install(*this))
	{
		return false;
	}
	auto* Router = Fixture.Facility->GetRouter();
	auto* Manager = Fixture.Facility->GetManager();
	FText Failure;
	TArray<UActorComponent*> Components;
	Fixture.Facility->GetComponents(Components);
	Router->FacilityDisplayName = FText::GetEmpty();
	TestFalse(TEXT("Empty name rejected"), Router->ValidateAuthoring(Failure));
	TestFalse(TEXT("Extension authoring rejects missing router name"),
			  Manager->ValidateExtensionAuthoring(Components, Failure));
	Router->FacilityDisplayName = FText::FromString(TEXT("   "));
	TestFalse(TEXT("Whitespace-only name rejected"), Router->ValidateAuthoring(Failure));
	Router->FacilityDisplayName = FText::FromString(TEXT("테스트 설비"));
	TestTrue(TEXT("Authored name validates"), Manager->ValidateExtensionAuthoring(Components, Failure));
	const auto Query = Router->QueryInteraction(Fixture.Context());
	TestTrue(TEXT("Generic router uses authored name rather than facility enum"),
			 Query.TargetName.ToString().StartsWith(TEXT("테스트 설비\n")));
	TestTrue(TEXT("Authored name preserves group HUD"), Query.TargetName.ToString().Contains(TEXT("스킨로션 0/4")));
	return true;
}

#endif
