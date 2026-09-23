#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Computer/BathhouseComputerActor.h"
#include "Blueprint/UserWidget.h"
#include "Components/ProgressBar.h"
#include "Components/BoxComponent.h"
#include "Components/CanvasPanel.h"
#include "Components/Slider.h"
#include "Components/TextBlock.h"
#include "Customer/CustomerSessionComponent.h"
#include "Engine/Engine.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Facility/BathWaterConditionComponent.h"
#include "Facility/BathWaterOperationsSubsystem.h"
#include "Facility/BathWaterSettings.h"
#include "Facility/BathWaterStateComponent.h"
#include "Facility/BathWaterUtilityCapacityComponent.h"
#include "Facility/BathWaterUtilityPlacementInstanceData.h"
#include "Facility/BathWaterUtilityFacilityActor.h"
#include "Facility/BathhouseBathFacilityActor.h"
#include "GameFramework/Actor.h"
#include "GameplayTagContainer.h"
#include "Placement/FacilityActorConversionTransaction.h"
#include "Placement/FacilityPlacementComponent.h"
#include "Placement/FacilityPlacementDefinition.h"
#include "Placement/FacilityPlacementZoneActor.h"
#include "Placement/PlaceableFacilityItemActor.h"
#include "UI/BathWaterBathTileWidget.h"
#include "UI/BathWaterCapacitySummaryWidget.h"
#include "UI/BathWaterDetailWidget.h"
#include "UI/BathWaterManagementScreenWidget.h"
#include "UI/BathWaterMapWidget.h"
#include "UObject/UnrealType.h"
#include "UObject/UObjectIterator.h"

#include <limits>

namespace
{
class FScopedBathWaterOperationsWorld
{
public:
	explicit FScopedBathWaterOperationsWorld(const TCHAR* BaseName)
	{
		if (!GEngine) return;
		const FName WorldName = MakeUniqueObjectName(nullptr, UWorld::StaticClass(), BaseName);
		FWorldContext& Context = GEngine->CreateNewWorldContext(EWorldType::Game);
		World = UWorld::CreateWorld(EWorldType::Game, false, WorldName, GetTransientPackage());
		if (!World)
		{
			GEngine->DestroyWorldContext(World);
			return;
		}
		World->AddToRoot();
		Context.SetCurrentWorld(World);
		World->InitializeActorsForPlay(FURL());
	}
	~FScopedBathWaterOperationsWorld()
	{
		if (World && GEngine)
		{
			World->DestroyWorld(false);
			GEngine->DestroyWorldContext(World);
			World->RemoveFromRoot();
		}
	}
	UWorld* Get() const { return World; }
private:
	UWorld* World = nullptr;
};

UBathWaterUtilityCapacityComponent* AddProvider(
	UWorld& World,
	const TCHAR* Name,
	const EBathWaterCapacityKind Kind,
	const float Points)
{
	AActor* Owner = World.SpawnActor<AActor>();
	UBathWaterUtilityCapacityComponent* Provider = NewObject<UBathWaterUtilityCapacityComponent>(Owner, Name);
	Provider->RestoreCapacity(Kind, Points);
	Provider->RegisterComponent();
	return Provider;
}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBathWaterOperationsCapacityTest,
	"BathhouseSim.BathWater.Operations.CapacityDemandAndRequests",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FBathWaterOperationsCapacityTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	FScopedBathWaterOperationsWorld Scope(TEXT("BathWaterOperationsCapacityWorld"));
	UWorld* World = Scope.Get();
	if (!TestNotNull(TEXT("Automation world exists"), World)) return false;
	UBathWaterOperationsSubsystem* Operations = World->GetSubsystem<UBathWaterOperationsSubsystem>();
	if (!TestNotNull(TEXT("Operations subsystem exists"), Operations)) return false;

	UBathWaterSettings* Settings = NewObject<UBathWaterSettings>();
	TestEqual(TEXT("Ambient defaults to 20 C"), Settings->GetAmbientTemperatureC(), 20.0f);
	TestEqual(TEXT("Target minimum defaults to 10 C"), Settings->GetMinTargetTemperatureC(), 10.0f);
	TestEqual(TEXT("Target maximum defaults to 50 C"), Settings->GetMaxTargetTemperatureC(), 50.0f);
	TestEqual(TEXT("Ambient-anchored quantization rounds symmetrically"), Settings->ClampAndQuantizeTargetTemperature(21.6f), 22.0f);

	UBathWaterUtilityCapacityComponent* Circulation = AddProvider(
		*World, TEXT("CirculationProvider"), EBathWaterCapacityKind::Circulation, 100.0f);
	UBathWaterUtilityCapacityComponent* Heating = AddProvider(
		*World, TEXT("HeatingProvider"), EBathWaterCapacityKind::Heating, 100.0f);
	UBathWaterUtilityCapacityComponent* Cooling = AddProvider(
		*World, TEXT("CoolingProvider"), EBathWaterCapacityKind::Cooling, 50.0f);
	TestTrue(TEXT("Circulation provider registers"), Operations->RegisterProvider(Circulation));
	TestTrue(TEXT("Duplicate provider registration is idempotent"), Operations->RegisterProvider(Circulation));
	TestTrue(TEXT("Heating provider registers"), Operations->RegisterProvider(Heating));
	TestTrue(TEXT("Cooling provider registers"), Operations->RegisterProvider(Cooling));
	TestEqual(TEXT("Duplicate provider is counted once"),
		Operations->GetCapacitySnapshot(EBathWaterCapacityKind::Circulation).TotalPoints, 100.0f);

	ABathhouseBathFacilityActor* BathA = World->SpawnActor<ABathhouseBathFacilityActor>();
	ABathhouseBathFacilityActor* BathB = World->SpawnActor<ABathhouseBathFacilityActor>();
	TestTrue(TEXT("Bath A registers"), Operations->RegisterBath(BathA->GetBathWaterCondition()));
	TestTrue(TEXT("Bath B registers"), Operations->RegisterBath(BathB->GetBathWaterCondition()));
	const FBathWaterSettingRequestResult CirculationResult = Operations->RequestCirculationPercent(BathA, 60.0f);
	TestTrue(TEXT("Sixty percent circulation commits"), CirculationResult.bSucceeded && !CirculationResult.bWasLimited);
	TestEqual(TEXT("Sixty percent reserves sixty points"),
		Operations->GetCapacitySnapshot(EBathWaterCapacityKind::Circulation).UsedPoints, 60.0f);
	const FBathWaterSettingRequestResult HeatResult = Operations->RequestTargetTemperature(BathA, 38.0f);
	TestEqual(TEXT("38 C reserves ninety heating points"),
		Operations->GetCapacitySnapshot(EBathWaterCapacityKind::Heating).UsedPoints, 90.0f);
	TestTrue(TEXT("38 C request commits"), HeatResult.bSucceeded && !HeatResult.bWasLimited);

	const FBathWaterSettingRequestResult Limited = Operations->RequestTargetTemperature(BathB, 40.0f);
	TestTrue(TEXT("Second bath target is capacity-limited"), Limited.bSucceeded && Limited.bWasLimited);
	TestEqual(TEXT("Remaining ten heating points clamp second bath to 22 C"), Limited.CommittedValue, 22.0f);
	TestEqual(TEXT("Heating pool is fully reserved"),
		Operations->GetCapacitySnapshot(EBathWaterCapacityKind::Heating).UsedPoints, 100.0f);
	const FBathWaterSettingRequestResult Decrease = Operations->RequestTargetTemperature(BathA, 20.0f);
	TestTrue(TEXT("Demand decrease is allowed"), Decrease.bSucceeded && !Decrease.bWasLimited);
	TestEqual(TEXT("Demand decrease releases capacity without changing other bath"),
		Operations->GetCapacitySnapshot(EBathWaterCapacityKind::Heating).UsedPoints, 10.0f);
	const FBathWaterSettingRequestResult CrossAmbient = Operations->RequestTargetTemperature(BathB, 10.0f);
	TestTrue(TEXT("Crossing ambient releases heating before reserving cooling"),
		CrossAmbient.bSucceeded && !CrossAmbient.bWasLimited
		&& Operations->GetCapacitySnapshot(EBathWaterCapacityKind::Heating).UsedPoints == 0.0f
		&& Operations->GetCapacitySnapshot(EBathWaterCapacityKind::Cooling).UsedPoints == 50.0f);
	const float PreviousTarget = BathB->GetBathWaterCondition()->GetTargetTemperatureC();
	const FBathWaterSettingRequestResult InvalidTarget = Operations->RequestTargetTemperature(
		BathB, std::numeric_limits<float>::quiet_NaN());
	TestFalse(TEXT("NaN target request fails without mutation"), InvalidTarget.bSucceeded);
	TestEqual(TEXT("NaN target rollback preserves the prior target"),
		BathB->GetBathWaterCondition()->GetTargetTemperatureC(), PreviousTarget);
	Operations->RequestTargetTemperature(BathB, 22.0f);

	float Deficit = 0.0f;
	TestFalse(TEXT("In-use provider cannot be normally removed"), Operations->CanRemoveProvider(Heating, Deficit));
	TestEqual(TEXT("Removal reports exact ten point deficit"), Deficit, 10.0f);
	TestTrue(TEXT("Unexpected provider loss removes capacity"), Operations->UnregisterProvider(Heating, true, true));
	TestEqual(TEXT("Unexpected loss preserves demand and exposes deficit"),
		Operations->GetCapacitySnapshot(EBathWaterCapacityKind::Heating).DeficitPoints, 10.0f);
	TestTrue(TEXT("Provider restoration resumes satisfied capacity"), Operations->RegisterProvider(Heating));
	TestTrue(TEXT("Restored heating pool is satisfied"), Operations->IsCapacitySatisfied(EBathWaterCapacityKind::Heating));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBathWaterOperationsFlowAndConditionTest,
	"BathhouseSim.BathWater.Operations.FlowConditionAndBathers",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FBathWaterOperationsFlowAndConditionTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	FScopedBathWaterOperationsWorld Scope(TEXT("BathWaterOperationsConditionWorld"));
	UWorld* World = Scope.Get();
	if (!TestNotNull(TEXT("Automation world exists"), World)) return false;

	AActor* Owner = World->SpawnActor<AActor>();
	UBathWaterStateComponent* Water = NewObject<UBathWaterStateComponent>(Owner, TEXT("FlowWater"));
	Water->FillRatePercentPerSecond = 10.0f;
	Water->DrainRatePercentPerSecond = 10.0f;
	Water->RegisterComponent();
	Water->SetNormalizedAmount(0.5f);
	int32 FlowSamples = 0;
	FBathWaterFlowStep LastFlow;
	Water->OnFlowStepNative.AddLambda([&](const FBathWaterFlowStep& Step)
	{
		++FlowSamples;
		LastFlow = Step;
	});
	FText FailureReason;
	Water->RequestSetControlOpen(EBathWaterControlType::FillValve, true,
		EBathWaterControlChangeReason::PlayerInteraction, FailureReason);
	Water->RequestSetControlOpen(EBathWaterControlType::DrainLever, true,
		EBathWaterControlChangeReason::PlayerInteraction, FailureReason);
	Water->TickComponent(1.0f, LEVELTICK_All, nullptr);
	TestEqual(TEXT("Net-zero exchange emits one flow sample"), FlowSamples, 1);
	TestTrue(TEXT("Net-zero exchange reports both actual directions"),
		FMath::IsNearlyEqual(LastFlow.IncomingAmount, 0.1f)
		&& FMath::IsNearlyEqual(LastFlow.OutgoingAmount, 0.1f));
	TestEqual(TEXT("Net-zero exchange preserves amount"), Water->GetNormalizedAmount(), 0.5f);
	TestTrue(TEXT("Net-zero exchange remains tick-enabled"), Water->IsComponentTickEnabled());

	UBathWaterOperationsSubsystem* Operations = World->GetSubsystem<UBathWaterOperationsSubsystem>();
	Operations->RegisterProvider(AddProvider(*World, TEXT("ConditionCirculation"),
		EBathWaterCapacityKind::Circulation, 100.0f));
	Operations->RegisterProvider(AddProvider(*World, TEXT("ConditionHeating"),
		EBathWaterCapacityKind::Heating, 100.0f));
	UBathWaterUtilityCapacityComponent* Cooling = AddProvider(*World, TEXT("ConditionCooling"),
		EBathWaterCapacityKind::Cooling, 100.0f);
	Operations->RegisterProvider(Cooling);
	ABathhouseBathFacilityActor* Bath = World->SpawnActor<ABathhouseBathFacilityActor>();
	UBathWaterConditionComponent* Condition = Bath->GetBathWaterCondition();
	Operations->RegisterBath(Condition);
	TestEqual(TEXT("Default thermal threshold is derived as ten percent"), Condition->GetThermalThresholdPercent(), 10.0f);
	Operations->RequestCirculationPercent(Bath, 100.0f);
	Operations->RequestTargetTemperature(Bath, 40.0f);
	Bath->GetBathWaterState()->SetNormalizedAmount(0.5f);
	const float BeforeTemperature = Condition->GetActualTemperatureC();
	Condition->TickComponent(10.0f, LEVELTICK_All, nullptr);
	TestTrue(TEXT("Satisfied circulation and heating move actual temperature toward target"),
		Condition->GetActualTemperatureC() > BeforeTemperature
		&& Condition->GetActualTemperatureC() <= 40.0f);

	Condition->ActualTemperatureC = 40.0f;
	Condition->ContaminationPercent = 50.0f;
	Condition->HandleFlowStep(FBathWaterFlowStep{ 0.5f, 0.1f, 0.1f, 0.5f, 1.0f });
	TestTrue(TEXT("Net-zero clean ambient exchange mixes temperature by actual volumes"),
		FMath::IsNearlyEqual(Condition->GetActualTemperatureC(), 36.0f, 0.001f));
	TestTrue(TEXT("Net-zero clean exchange dilutes contamination"),
		FMath::IsNearlyEqual(Condition->GetContaminationPercent(), 40.0f, 0.001f));
	Condition->HandleFlowStep(FBathWaterFlowStep{ 0.5f, 0.0f, 0.1f, 0.4f, 1.0f });
	TestTrue(TEXT("Drain-only preserves temperature and concentration"),
		FMath::IsNearlyEqual(Condition->GetActualTemperatureC(), 36.0f, 0.001f)
		&& FMath::IsNearlyEqual(Condition->GetContaminationPercent(), 40.0f, 0.001f));

	Condition->ActualTemperatureC = 20.0f;
	Condition->SetCirculationPercentFromSubsystem(10.0f);
	Condition->SetTargetTemperatureFromSubsystem(40.0f);
	Condition->IntegrateTemperature(10.0f);
	TestEqual(TEXT("At exact ambient, derived ten-percent threshold resists departure"),
		Condition->GetActualTemperatureC(), 20.0f);
	Condition->SetCirculationPercentFromSubsystem(20.0f);
	Condition->IntegrateTemperature(10.0f);
	TestTrue(TEXT("Above threshold departs ambient at the net active rate"),
		FMath::IsNearlyEqual(Condition->GetActualTemperatureC(), 20.5f, 0.001f));
	Condition->ActualTemperatureC = 40.0f;
	Condition->SetCirculationPercentFromSubsystem(10.0f);
	Condition->IntegrateTemperature(10.0f);
	TestEqual(TEXT("Equal active and natural forces maintain exact target"),
		Condition->GetActualTemperatureC(), 40.0f);
	Condition->SetCirculationPercentFromSubsystem(0.0f);
	Condition->IntegrateTemperature(20.0f);
	TestTrue(TEXT("Without active control water returns one degree toward ambient in twenty seconds"),
		FMath::IsNearlyEqual(Condition->GetActualTemperatureC(), 39.0f, 0.001f));
	Condition->ActualTemperatureC = 30.0f;
	Condition->SetTargetTemperatureFromSubsystem(15.0f);
	Condition->SetCirculationPercentFromSubsystem(50.0f);
	Condition->IntegrateTemperature(10.0f);
	TestTrue(TEXT("Cooling and natural return add while both point downward"),
		FMath::IsNearlyEqual(Condition->GetActualTemperatureC(), 27.0f, 0.001f));

	UCustomerSessionComponent* Session = NewObject<UCustomerSessionComponent>(Owner, TEXT("BatherSession"));
	Session->RegisterComponent();
	TestTrue(TEXT("First actual-bather registration succeeds"), Condition->RegisterActiveBather(Session));
	TestTrue(TEXT("Duplicate actual-bather registration is idempotent"), Condition->RegisterActiveBather(Session));
	TestEqual(TEXT("Duplicate registration counts once"), Condition->GetActiveBatherCount(), 1);
	Condition->ContaminationPercent = 50.0f;
	Condition->SetCirculationPercentFromSubsystem(50.0f);
	Condition->TickComponent(10.0f, LEVELTICK_All, nullptr);
	TestTrue(TEXT("One bather and fifty-percent cleaning apply their net rate"),
		FMath::IsNearlyEqual(Condition->GetContaminationPercent(), 46.0f, 0.001f));
	UBathWaterUtilityCapacityComponent* CirculationProvider = nullptr;
	for (TObjectIterator<UBathWaterUtilityCapacityComponent> It; It; ++It)
	{
		if (It->GetWorld() == World && It->GetCapacityKind() == EBathWaterCapacityKind::Circulation)
		{
			CirculationProvider = *It;
			break;
		}
	}
	TestNotNull(TEXT("Circulation test provider can be resolved"), CirculationProvider);
	if (CirculationProvider)
	{
		Operations->UnregisterProvider(CirculationProvider, true, true);
		Condition->TickComponent(10.0f, LEVELTICK_All, nullptr);
		TestTrue(TEXT("Circulation deficit stops cleaning but bather contamination continues"),
			FMath::IsNearlyEqual(Condition->GetContaminationPercent(), 47.0f, 0.001f));
		Operations->RegisterProvider(CirculationProvider);
	}
	TestTrue(TEXT("Actual-bather unregister succeeds"), Condition->UnregisterActiveBather(Session));
	TestEqual(TEXT("Repeated unregister leaves zero bathers"), Condition->GetActiveBatherCount(), 0);
	const float FrozenTemperature = Condition->GetActualTemperatureC();
	const float FrozenContamination = Condition->GetContaminationPercent();
	TestTrue(TEXT("Condition recovery freeze captures state"), Condition->BeginRecoveryFreeze(FailureReason));
	Condition->TickComponent(10.0f, LEVELTICK_All, nullptr);
	Condition->PrepareRecoveryCommit();
	Condition->CancelRecoveryFreeze();
	TestTrue(TEXT("Condition rollback restores exact temperature and contamination"),
		Condition->GetActualTemperatureC() == FrozenTemperature
		&& Condition->GetContaminationPercent() == FrozenContamination);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBathWaterOperationsContractsTest,
	"BathhouseSim.BathWater.Operations.PayloadAndUIContracts",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FBathWaterOperationsContractsTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	UBathWaterUtilityPlacementInstanceData* Payload = NewObject<UBathWaterUtilityPlacementInstanceData>();
	Payload->CapacityKind = EBathWaterCapacityKind::Cooling;
	Payload->CapacityPoints = 175.0f;
	TestEqual(TEXT("Typed payload retains utility kind"), Payload->CapacityKind, EBathWaterCapacityKind::Cooling);
	TestEqual(TEXT("Typed payload retains instance capacity"), Payload->CapacityPoints, 175.0f);
	UBathWaterUtilityCapacityComponent* InvalidCapacity = NewObject<UBathWaterUtilityCapacityComponent>();
	InvalidCapacity->RestoreCapacity(static_cast<EBathWaterCapacityKind>(255), -1.0f);
	FText CapacityFailure;
	TestFalse(TEXT("Invalid utility kind and capacity fail closed"),
		InvalidCapacity->HasValidAuthoring(CapacityFailure));

	TestNotNull(TEXT("Computer exposes managed placement zone property"),
		FindFProperty<FObjectPropertyBase>(ABathhouseComputerActor::StaticClass(), TEXT("ManagedBathPlacementZone")));
	TestNotNull(TEXT("Management root requires CapacitySummary BindWidget"),
		FindFProperty<FObjectPropertyBase>(UBathWaterManagementScreenWidget::StaticClass(), TEXT("CapacitySummary")));
	TestNotNull(TEXT("Management root requires BathMap BindWidget"),
		FindFProperty<FObjectPropertyBase>(UBathWaterManagementScreenWidget::StaticClass(), TEXT("BathMap")));
	TestNotNull(TEXT("Management root requires BathDetail BindWidget"),
		FindFProperty<FObjectPropertyBase>(UBathWaterManagementScreenWidget::StaticClass(), TEXT("BathDetail")));
	TestNotNull(TEXT("Map requires EmptyStateText BindWidget"),
		FindFProperty<FObjectPropertyBase>(UBathWaterMapWidget::StaticClass(), TEXT("EmptyStateText")));
	TestNotNull(TEXT("Capacity summary requires circulation ProgressBar"),
		FindFProperty<FObjectPropertyBase>(UBathWaterCapacitySummaryWidget::StaticClass(), TEXT("CirculationCapacityBar")));
	TestNotNull(TEXT("Bath detail requires bath name"),
		FindFProperty<FObjectPropertyBase>(UBathWaterDetailWidget::StaticClass(), TEXT("BathNameText")));
	TestNotNull(TEXT("Bath detail requires independent capacity status"),
		FindFProperty<FObjectPropertyBase>(UBathWaterDetailWidget::StaticClass(), TEXT("CapacityStatusText")));
	TestTrue(TEXT("All management widgets are native user widgets"),
		UBathWaterCapacitySummaryWidget::StaticClass()->IsChildOf(UUserWidget::StaticClass())
		&& UBathWaterMapWidget::StaticClass()->IsChildOf(UUserWidget::StaticClass())
		&& UBathWaterBathTileWidget::StaticClass()->IsChildOf(UUserWidget::StaticClass())
		&& UBathWaterDetailWidget::StaticClass()->IsChildOf(UUserWidget::StaticClass()));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBathWaterOperationsRequestAtomicityTest,
	"BathhouseSim.BathWater.Operations.RequestAtomicityAndRevision",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FBathWaterOperationsRequestAtomicityTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	FScopedBathWaterOperationsWorld Scope(TEXT("BathWaterRequestAtomicityWorld"));
	UWorld* World = Scope.Get();
	if (!TestNotNull(TEXT("Automation world exists"), World)) return false;
	UBathWaterOperationsSubsystem* Operations = World->GetSubsystem<UBathWaterOperationsSubsystem>();
	ABathhouseBathFacilityActor* Bath = World->SpawnActor<ABathhouseBathFacilityActor>();
	UBathWaterUtilityCapacityComponent* Heating100 = AddProvider(
		*World, TEXT("Heating100"), EBathWaterCapacityKind::Heating, 100.0f);
	UBathWaterUtilityCapacityComponent* Heating50 = AddProvider(
		*World, TEXT("Heating50"), EBathWaterCapacityKind::Heating, 50.0f);
	TestTrue(TEXT("Bath registers"), Operations->RegisterBath(Bath->GetBathWaterCondition()));
	const uint64 TopologyAfterBath = Operations->GetTopologyRevision();
	TestTrue(TEXT("Initial heating provider registers"), Operations->RegisterProvider(Heating100));
	TestEqual(TEXT("Provider registration does not alter bath topology"),
		Operations->GetTopologyRevision(), TopologyAfterBath);
	TestTrue(TEXT("Initial 38 degree target commits"),
		Operations->RequestTargetTemperature(Bath, 38.0f).bSucceeded);
	TestTrue(TEXT("Unexpected capacity loss is represented"), Operations->UnregisterProvider(Heating100, true, true));
	TestTrue(TEXT("Smaller provider registers"), Operations->RegisterProvider(Heating50));

	int32 Publications = 0;
	const FDelegateHandle Handle = Operations->OnOperationsChanged.AddLambda([&Publications]() { ++Publications; });
	const uint64 RevisionBefore = Operations->GetDataRevision();
	const FBathWaterSettingRequestResult Limited = Operations->RequestTargetTemperature(Bath, 40.0f);
	TestTrue(TEXT("An outward increase under deficit is limited"), Limited.bSucceeded && Limited.bWasLimited);
	TestEqual(TEXT("A limited increase never lowers the existing target"), Limited.CommittedValue, 38.0f);
	TestEqual(TEXT("Shortage reports candidate aggregate minus capacity"), Limited.RequiredAdditionalPoints, 50.0f);
	TestEqual(TEXT("No-op limited commit does not change data revision"), Operations->GetDataRevision(), RevisionBefore);
	TestEqual(TEXT("No-op limited commit emits no publication"), Publications, 0);

	const FBathWaterSettingRequestResult Same = Operations->RequestTargetTemperature(Bath, 38.0f);
	TestTrue(TEXT("Same-value request succeeds"), Same.bSucceeded);
	TestEqual(TEXT("Same-value request remains revision-free"), Operations->GetDataRevision(), RevisionBefore);
	TestEqual(TEXT("Same-value request remains publication-free"), Publications, 0);
	const FBathWaterSettingRequestResult Decrease = Operations->RequestTargetTemperature(Bath, 30.0f);
	TestTrue(TEXT("Demand decrease remains legal under an existing deficit"),
		Decrease.bSucceeded && !Decrease.bWasLimited && FMath::IsNearlyEqual(Decrease.CommittedValue, 30.0f));
	TestEqual(TEXT("A real decrease publishes exactly once"), Publications, 1);
	Operations->OnOperationsChanged.Remove(Handle);

	UBathWaterUtilityCapacityComponent* Cooling100 = AddProvider(
		*World, TEXT("Cooling100"), EBathWaterCapacityKind::Cooling, 100.0f);
	UBathWaterUtilityCapacityComponent* Cooling20 = AddProvider(
		*World, TEXT("Cooling20"), EBathWaterCapacityKind::Cooling, 20.0f);
	TestTrue(TEXT("Cooling provider registers without changing bath topology"),
		Operations->RegisterProvider(Cooling100)
		&& Operations->GetTopologyRevision() == TopologyAfterBath);
	TestTrue(TEXT("Ambient crossing into cooling commits while capacity is available"),
		Operations->RequestTargetTemperature(Bath, 12.0f).bSucceeded);
	TestTrue(TEXT("Unexpected cooling loss is represented"), Operations->UnregisterProvider(Cooling100, true, true));
	TestTrue(TEXT("Smaller cooling provider registers"), Operations->RegisterProvider(Cooling20));
	int32 CoolingPublications = 0;
	const FDelegateHandle CoolingHandle = Operations->OnOperationsChanged.AddLambda(
		[&CoolingPublications]() { ++CoolingPublications; });
	const uint64 CoolingRevisionBefore = Operations->GetDataRevision();
	const FBathWaterSettingRequestResult CoolingLimited = Operations->RequestTargetTemperature(Bath, 8.0f);
	TestTrue(TEXT("Cooling outward increase is limited without raising the approved target"),
		CoolingLimited.bSucceeded && CoolingLimited.bWasLimited
		&& FMath::IsNearlyEqual(CoolingLimited.CommittedValue, 12.0f));
	TestEqual(TEXT("Cooling shortage uses candidate aggregate minus capacity"),
		CoolingLimited.RequiredAdditionalPoints, 30.0f);
	TestEqual(TEXT("Cooling no-op limit keeps revision unchanged"),
		Operations->GetDataRevision(), CoolingRevisionBefore);
	TestEqual(TEXT("Cooling no-op limit emits no publication"), CoolingPublications, 0);
	const FBathWaterSettingRequestResult CoolingDecrease = Operations->RequestTargetTemperature(Bath, 18.0f);
	TestTrue(TEXT("Cooling demand decrease remains legal under deficit"),
		CoolingDecrease.bSucceeded && !CoolingDecrease.bWasLimited
		&& FMath::IsNearlyEqual(CoolingDecrease.CommittedValue, 18.0f));
	TestEqual(TEXT("Cooling decrease publishes once"), CoolingPublications, 1);
	Operations->OnOperationsChanged.Remove(CoolingHandle);

	UBathWaterUtilityCapacityComponent* Ephemeral = AddProvider(
		*World, TEXT("EphemeralCirculation"), EBathWaterCapacityKind::Circulation, 33.0f);
	TestTrue(TEXT("Ephemeral provider registers"), Operations->RegisterProvider(Ephemeral));
	const uint64 PruneRevisionBefore = Operations->GetDataRevision();
	const uint64 PruneTopologyBefore = Operations->GetTopologyRevision();
	int32 PrunePublications = 0;
	const FDelegateHandle PruneHandle = Operations->OnOperationsChanged.AddLambda(
		[&PrunePublications]() { ++PrunePublications; });
	Ephemeral->DestroyComponent();
	const FBathWaterCapacitySnapshot PrunedSnapshot =
		Operations->GetCapacitySnapshot(EBathWaterCapacityKind::Circulation);
	TestTrue(TEXT("Invalid weak provider prune removes capacity and advances data revision"),
		FMath::IsNearlyZero(PrunedSnapshot.TotalPoints)
		&& Operations->GetDataRevision() == PruneRevisionBefore + 1);
	TestEqual(TEXT("Provider prune does not alter bath topology"),
		Operations->GetTopologyRevision(), PruneTopologyBefore);
	TestEqual(TEXT("Provider prune publishes exactly once"), PrunePublications, 1);
	Operations->OnOperationsChanged.Remove(PruneHandle);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBathWaterMapProjectionTest,
	"BathhouseSim.BathWater.Operations.MapProjection",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FBathWaterMapProjectionTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	const FTransform Zone(FRotator(0.0f, 37.0f, 0.0f), FVector(100.0f, 200.0f, 0.0f), FVector(2.0f, 0.5f, 1.0f));
	const FVector Extent(100.0f, 50.0f, 10.0f);
	const FVector2D Canvas(400.0f, 400.0f);
	FBathWaterMapProjection Projection;
	bool bInside = false;
	const FTransform Aligned(Zone.GetRotation(), Zone.GetLocation(), FVector(1.5f, 2.0f, 1.0f));
	TestTrue(TEXT("Aligned scaled footprint projects"), UBathWaterMapWidget::ProjectFootprint(
		Zone, Extent, Canvas, Aligned, FVector(20.0f, 10.0f, 1.0f), Projection, bInside));
	TestTrue(TEXT("Aligned footprint is inside and centered"), bInside
		&& Projection.Position.Equals(FVector2D(200.0f, 200.0f), 0.01f));
	TestTrue(TEXT("Letterbox projection preserves exact scaled footprint aspect"),
		Projection.Size.Equals(FVector2D(40.0f, 60.0f), 0.01f));
	TestTrue(TEXT("World +X maps upward and +Y maps right"), FMath::IsNearlyZero(Projection.AngleDegrees, 0.01f));

	const FQuat QuarterTurn = FQuat(FVector::UpVector, HALF_PI) * Zone.GetRotation();
	const FTransform Rotated(QuarterTurn, Zone.GetLocation(), FVector::OneVector);
	TestTrue(TEXT("Ninety-degree footprint projects"), UBathWaterMapWidget::ProjectFootprint(
		Zone, Extent, Canvas, Rotated, FVector(20.0f, 10.0f, 1.0f), Projection, bInside));
	TestTrue(TEXT("Ninety-degree projection rotates the tile"), bInside
		&& FMath::IsNearlyEqual(FMath::Abs(Projection.AngleDegrees), 90.0f, 0.01f));
	const FQuat ArbitraryTurn = FQuat(FVector::UpVector, FMath::DegreesToRadians(31.0f)) * Zone.GetRotation();
	const FTransform Arbitrary(ArbitraryTurn, Zone.GetLocation(), FVector(0.75f, 1.25f, 1.0f));
	TestTrue(TEXT("Arbitrary-yaw non-unit footprint projects with all corners inside"),
		UBathWaterMapWidget::ProjectFootprint(Zone, Extent, Canvas, Arbitrary,
			FVector(20.0f, 10.0f, 1.0f), Projection, bInside) && bInside
		&& Projection.Corners.Num() == 4);

	const FVector OutsideLocation = Zone.GetLocation() + Zone.GetUnitAxis(EAxis::X) * 195.0f;
	const FTransform Outside(Zone.GetRotation(), OutsideLocation, FVector::OneVector);
	TestTrue(TEXT("Outside footprint still produces diagnostic projection"), UBathWaterMapWidget::ProjectFootprint(
		Zone, Extent, Canvas, Outside, FVector(10.0f), Projection, bInside));
	TestFalse(TEXT("All-corner containment rejects a crossing footprint"), bInside);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBathWaterOperationsUIWidgetTest,
	"BathhouseSim.BathWater.Operations.NativeWidgetPresentation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FBathWaterOperationsUIWidgetTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	FScopedBathWaterOperationsWorld Scope(TEXT("BathWaterNativeWidgetWorld"));
	UWorld* World = Scope.Get();
	if (!TestNotNull(TEXT("Widget automation world exists"), World)) return false;
	UBathWaterCapacitySummaryWidget* Summary = NewObject<UBathWaterCapacitySummaryWidget>();
	Summary->CirculationCapacityText = NewObject<UTextBlock>(Summary);
	Summary->HeatingCapacityText = NewObject<UTextBlock>(Summary);
	Summary->CoolingCapacityText = NewObject<UTextBlock>(Summary);
	Summary->CirculationCapacityBar = NewObject<UProgressBar>(Summary);
	Summary->HeatingCapacityBar = NewObject<UProgressBar>(Summary);
	Summary->CoolingCapacityBar = NewObject<UProgressBar>(Summary);
	Summary->CirculationCapacityStatusText = NewObject<UTextBlock>(Summary);
	Summary->HeatingCapacityStatusText = NewObject<UTextBlock>(Summary);
	Summary->CoolingCapacityStatusText = NewObject<UTextBlock>(Summary);
	FBathWaterOperationsSnapshot OperationsSnapshot;
	OperationsSnapshot.Circulation.Kind = EBathWaterCapacityKind::Circulation;
	OperationsSnapshot.Circulation.TotalPoints = 100.0f;
	OperationsSnapshot.Circulation.UsedPoints = 100.0f;
	OperationsSnapshot.Heating.Kind = EBathWaterCapacityKind::Heating;
	OperationsSnapshot.Heating.TotalPoints = 50.0f;
	OperationsSnapshot.Heating.UsedPoints = 70.0f;
	OperationsSnapshot.Heating.DeficitPoints = 20.0f;
	OperationsSnapshot.Cooling.Kind = EBathWaterCapacityKind::Cooling;
	OperationsSnapshot.Cooling.TotalPoints = 100.0f;
	OperationsSnapshot.Cooling.UsedPoints = 20.0f;
	Summary->ApplyCapacitySnapshot(OperationsSnapshot);
	TestEqual(TEXT("Capacity widgets write each kind once"), Summary->PresentationWriteCount, 3);
	TestEqual(TEXT("Full capacity exposes a full status"),
		Summary->CirculationCapacityStatusText->GetText().ToString(), FString(TEXT("가득 참")));
	TestTrue(TEXT("Deficit capacity exposes shortage and clamps progress"),
		Summary->HeatingCapacityStatusText->GetText().ToString() == TEXT("부족")
		&& FMath::IsNearlyEqual(Summary->HeatingCapacityBar->GetPercent(), 1.0f));
	Summary->ApplyCapacitySnapshot(OperationsSnapshot);
	TestEqual(TEXT("An identical snapshot performs no child writes"), Summary->PresentationWriteCount, 3);

	UBathWaterDetailWidget* Detail = NewObject<UBathWaterDetailWidget>();
	Detail->BathNameText = NewObject<UTextBlock>(Detail);
	Detail->ActualTemperatureText = NewObject<UTextBlock>(Detail);
	Detail->CapacityStatusText = NewObject<UTextBlock>(Detail);
	Detail->FeedbackText = NewObject<UTextBlock>(Detail);
	Detail->CirculationSlider = NewObject<USlider>(Detail);
	Detail->TargetTemperatureSlider = NewObject<USlider>(Detail);
	Detail->SetOperationsContext(World->GetSubsystem<UBathWaterOperationsSubsystem>());
	FBathWaterBathSnapshot BathSnapshot;
	BathSnapshot.BathActor = World->SpawnActor<ABathhouseBathFacilityActor>();
	BathSnapshot.ActualTemperatureC = 37.5f;
	BathSnapshot.bCirculationCapacityDeficit = true;
	BathSnapshot.bCoolingCapacityDeficit = true;
	BathSnapshot.Revision = 7;
	Detail->ApplyBathSnapshot(&BathSnapshot);
	TestTrue(TEXT("Detail uses the degree-Celsius unit"),
		Detail->ActualTemperatureText->GetText().ToString().Contains(TEXT("°C")));
	TestTrue(TEXT("Independent bath deficits combine without collapsing kinds"),
		Detail->CapacityStatusText->GetText().ToString().Contains(TEXT("순환/냉각")));
	const int32 DetailWrites = Detail->PresentationWriteCount;
	Detail->ApplyBathSnapshot(&BathSnapshot);
	TestEqual(TEXT("Identical detail snapshot performs no presentation writes"),
		Detail->PresentationWriteCount, DetailWrites);
	FBathWaterSettingRequestResult LimitedFeedback;
	LimitedFeedback.bSucceeded = true;
	LimitedFeedback.bWasLimited = true;
	LimitedFeedback.LimitedKind = EBathWaterCapacityKind::Heating;
	LimitedFeedback.RequiredAdditionalPoints = 12.5f;
	Detail->ApplyRequestFeedback(LimitedFeedback);
	TestTrue(TEXT("Limited feedback includes capacity kind and exact shortage"),
		Detail->FeedbackText->GetText().ToString().Contains(TEXT("가열"))
		&& Detail->FeedbackText->GetText().ToString().Contains(TEXT("12.5")));
	FBathWaterBathSnapshot OtherBathSnapshot = BathSnapshot;
	OtherBathSnapshot.BathActor = World->SpawnActor<ABathhouseBathFacilityActor>();
	Detail->ApplyBathSnapshot(&OtherBathSnapshot);
	TestTrue(TEXT("Selection change clears stale limited feedback"),
		Detail->FeedbackText->GetText().IsEmpty());
	Detail->ApplyBathSnapshot(nullptr);
	TestTrue(TEXT("Empty selection clears stale values"), Detail->ActualTemperatureText->GetText().IsEmpty());

	AFacilityPlacementZoneActor* Zone = World->SpawnActor<AFacilityPlacementZoneActor>();
	Zone->GetZoneBounds()->SetBoxExtent(FVector(200.0f, 100.0f, 20.0f));
	UBathWaterMapWidget* Map = CreateWidget<UBathWaterMapWidget>(World, UBathWaterMapWidget::StaticClass());
	Map->BathTileCanvas = NewObject<UCanvasPanel>(Map);
	Map->EmptyStateText = NewObject<UTextBlock>(Map);
	Map->BathTileWidgetClass = UBathWaterBathTileWidget::StaticClass();
	Map->SetPlacementZone(Zone);
	FBathWaterOperationsSnapshot MapSnapshot;
	MapSnapshot.TopologyRevision = 1;
	FBathWaterBathSnapshot MapBath;
	MapBath.BathActor = BathSnapshot.BathActor;
	MapBath.FootprintTransform = FTransform(FVector::ZeroVector);
	MapBath.FootprintHalfExtent = FVector(20.0f, 10.0f, 1.0f);
	MapSnapshot.Baths.Add(MapBath);
	Map->ApplySnapshot(MapSnapshot, nullptr);
	UBathWaterBathTileWidget* FirstTile = Map->Tiles.FindRef(MapBath.BathActor);
	const int32 RebuildsAfterTopology = Map->RebuildCount;
	MapSnapshot.DataRevision = 2;
	MapSnapshot.Circulation.TotalPoints = 100.0f;
	Map->ApplySnapshot(MapSnapshot, nullptr);
	TestTrue(TEXT("Provider-only snapshot preserves map tile identity and rebuild count"),
		FirstTile && Map->Tiles.FindRef(MapBath.BathActor) == FirstTile
		&& Map->RebuildCount == RebuildsAfterTopology);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBathWaterOperationsFacilityTransactionTest,
	"BathhouseSim.BathWater.Operations.FacilityTransactionAtomicity",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FBathWaterOperationsFacilityTransactionTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	FScopedBathWaterOperationsWorld Scope(TEXT("BathWaterFacilityTransactionWorld"));
	UWorld* World = Scope.Get();
	if (!TestNotNull(TEXT("Automation world exists"), World)) return false;
	World->BeginPlay();
	UBathWaterOperationsSubsystem* Operations = World->GetSubsystem<UBathWaterOperationsSubsystem>();
	if (!TestNotNull(TEXT("Operations subsystem exists"), Operations)) return false;

	UFacilityPlacementDefinition* Definition = NewObject<UFacilityPlacementDefinition>();
	Definition->StableId = TEXT("BathWaterUtilityAutomation");
	Definition->FacilityTags.AddTag(FGameplayTag::RequestGameplayTag(TEXT("Facility.Placeable")));
	Definition->PlacedFacilityClass = ABathWaterUtilityFacilityActor::StaticClass();
	Definition->RecoveryItemClass = APlaceableFacilityItemActor::StaticClass();
	Definition->RecoveryItemMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
	FText FailureReason;
	TestTrue(FString::Printf(TEXT("Utility definition validates independently: %s"), *FailureReason.ToString()),
		Definition->ValidateRuntime(FailureReason));

	auto SpawnUtility = [&](const float Points, const FVector& Location)
	{
		ABathWaterUtilityFacilityActor* Utility = World->SpawnActorDeferred<ABathWaterUtilityFacilityActor>(
			ABathWaterUtilityFacilityActor::StaticClass(), FTransform(Location), nullptr, nullptr,
			ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
		if (!Utility) return static_cast<ABathWaterUtilityFacilityActor*>(nullptr);
		Utility->GetFacilityPlacementComponent()->Definition = Definition;
		Utility->GetCapacityComponent()->RestoreCapacity(EBathWaterCapacityKind::Circulation, Points);
		Utility->FinishSpawning(FTransform(Location));
		if (!Utility->HasActorBegunPlay()) Utility->DispatchBeginPlay();
		return Utility;
	};

	ABathWaterUtilityFacilityActor* Utility = SpawnUtility(100.0f, FVector(5000.0f, 0.0f, 100.0f));
	if (!TestNotNull(TEXT("Placed utility actor exists"), Utility)) return false;
	TestTrue(TEXT("Placed utility contributes native capacity"), Utility->bProviderRegistered
		&& FMath::IsNearlyEqual(Operations->GetCapacitySnapshot(EBathWaterCapacityKind::Circulation).TotalPoints, 100.0f));
	ABathhouseBathFacilityActor* Bath = World->SpawnActor<ABathhouseBathFacilityActor>(
		ABathhouseBathFacilityActor::StaticClass(), FVector(0.0f, 0.0f, 100.0f), FRotator::ZeroRotator);
	if (!TestNotNull(TEXT("Demand bath exists"), Bath)) return false;
	Operations->RegisterBath(Bath->GetBathWaterCondition());
	Operations->RequestCirculationPercent(Bath, 50.0f);
	TestFalse(TEXT("Demand-blocked recovery preserves the provider"), Utility->QueryFacilityRecovery().bSucceeded);
	TestTrue(TEXT("Blocked recovery leaves capacity and actor intact"), IsValid(Utility)
		&& FMath::IsNearlyEqual(Operations->GetCapacitySnapshot(EBathWaterCapacityKind::Circulation).TotalPoints, 100.0f));

	Operations->RequestCirculationPercent(Bath, 0.0f);
	int32 Publications = 0;
	const FDelegateHandle PublicationHandle = Operations->OnOperationsChanged.AddLambda([&Publications]() { ++Publications; });
	TestTrue(FString::Printf(TEXT("Utility recovery hold starts: %s"), *FailureReason.ToString()),
		Utility->TryBeginFacilityRecoveryHold(FailureReason));
	TWeakObjectPtr<ABathWaterUtilityFacilityActor> UtilityWeak(Utility);
	APlaceableFacilityItemActor* Item = FFacilityActorConversionTransaction::RecoverFacilityToItem(*Utility, FailureReason);
	TestTrue(FString::Printf(TEXT("Utility recovery transaction succeeds: %s"), *FailureReason.ToString()),
		Item != nullptr && !UtilityWeak.IsValid());
	TestEqual(TEXT("Committed recovery publishes exactly once"), Publications, 1);
	TestEqual(TEXT("Committed recovery removes capacity exactly once"),
		Operations->GetCapacitySnapshot(EBathWaterCapacityKind::Circulation).TotalPoints, 0.0f);
	const UBathWaterUtilityPlacementInstanceData* RecoveredData = Item
		? Cast<UBathWaterUtilityPlacementInstanceData>(Item->GetPlacementPayload().InstanceData) : nullptr;
	TestTrue(TEXT("Recovered item preserves typed utility kind and points"), RecoveredData
		&& RecoveredData->CapacityKind == EBathWaterCapacityKind::Circulation
		&& FMath::IsNearlyEqual(RecoveredData->CapacityPoints, 100.0f));
	Operations->OnOperationsChanged.Remove(PublicationHandle);

	ABathWaterUtilityFacilityActor* Staged = World->SpawnActorDeferred<ABathWaterUtilityFacilityActor>(
		ABathWaterUtilityFacilityActor::StaticClass(), FTransform(FVector(6000.0f, 0.0f, 100.0f)), nullptr, nullptr,
		ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
	Staged->GetCapacityComponent()->RestoreCapacity(EBathWaterCapacityKind::Circulation, 7.0f);
	TestTrue(TEXT("Malformed payload fixture enters staged placement"),
		Staged->GetFacilityPlacementComponent()->PrepareForStagedPlacement(*Definition, FailureReason));
	Staged->FinishSpawning(FTransform(FVector(6000.0f, 0.0f, 100.0f)));
	TestTrue(TEXT("Malformed payload fixture captures its authored collision snapshot"),
		Staged->GetFacilityPlacementComponent()->FinalizeStagedPlacementCollisionSnapshot(FailureReason));
	if (Item)
	{
		FFacilityPlacementPayload MismatchedPayload;
		MismatchedPayload.Definition = Definition;
		UBathWaterUtilityPlacementInstanceData* MismatchedData =
			NewObject<UBathWaterUtilityPlacementInstanceData>(Item);
		MismatchedData->CapacityKind = EBathWaterCapacityKind::Heating;
		MismatchedData->CapacityPoints = 100.0f;
		MismatchedPayload.InstanceData = MismatchedData;
		TestFalse(TEXT("Mismatched typed utility kind fails before staged mutation"),
			Staged->ImportPlacementPayload(*Item, MismatchedPayload, FailureReason));
		TestTrue(TEXT("Rejected import preserves staged capacity"),
			Staged->GetCapacityComponent()->GetCapacityKind() == EBathWaterCapacityKind::Circulation
			&& FMath::IsNearlyEqual(Staged->GetCapacityComponent()->GetCapacityPoints(), 7.0f));
		MismatchedData->CapacityKind = EBathWaterCapacityKind::Circulation;
		MismatchedData->CapacityPoints = std::numeric_limits<float>::quiet_NaN();
		TestFalse(TEXT("NaN utility payload fails before staged mutation"),
			Staged->ImportPlacementPayload(*Item, MismatchedPayload, FailureReason));
	}
	Staged->Destroy();
	if (Item) Item->Destroy();

	ABathWaterUtilityFacilityActor* RollbackUtility = SpawnUtility(80.0f, FVector(7000.0f, 0.0f, 100.0f));
	TestNotNull(TEXT("Rollback utility exists"), RollbackUtility);
	FFacilityActorConversionTransaction::SetTestFault(
		FFacilityActorConversionTransaction::ETestFault::RecoveryActivation);
	FailureReason = FText::GetEmpty();
	TestTrue(TEXT("Rollback utility enters recovery hold"),
		RollbackUtility && RollbackUtility->TryBeginFacilityRecoveryHold(FailureReason));
	APlaceableFacilityItemActor* FailedItem = RollbackUtility
		? FFacilityActorConversionTransaction::RecoverFacilityToItem(*RollbackUtility, FailureReason) : nullptr;
	TestNull(TEXT("Injected activation failure aborts recovery"), FailedItem);
	TestTrue(TEXT("Recovery rollback restores source, registration and exact capacity"),
		IsValid(RollbackUtility) && RollbackUtility->bProviderRegistered
		&& RollbackUtility->GetFacilityPlacementComponent()->IsPlacedDomainActive()
		&& FMath::IsNearlyEqual(Operations->GetCapacitySnapshot(EBathWaterCapacityKind::Circulation).TotalPoints, 80.0f));
	FFacilityActorConversionTransaction::ClearTestFault();

	Operations->RequestCirculationPercent(Bath, 50.0f);
	RollbackUtility->Destroy();
	const FBathWaterCapacitySnapshot Deficit = Operations->GetCapacitySnapshot(EBathWaterCapacityKind::Circulation);
	TestTrue(TEXT("Unexpected provider destruction preserves demand and exposes exact deficit"),
		FMath::IsNearlyEqual(Deficit.UsedPoints, 50.0f) && FMath::IsNearlyEqual(Deficit.DeficitPoints, 50.0f));
	bool bReentrantSnapshotObserved = false;
	const FDelegateHandle ReentrantHandle = Operations->OnOperationsChanged.AddLambda(
		[Operations, &bReentrantSnapshotObserved]()
		{
			bReentrantSnapshotObserved = Operations->GetSnapshot().Circulation.IsSatisfied();
		});
	ABathWaterUtilityFacilityActor* Replacement = SpawnUtility(80.0f, FVector(8000.0f, 0.0f, 100.0f));
	TestTrue(TEXT("Replacement registration resumes service and tolerates reentrant snapshot reads"),
		Replacement && bReentrantSnapshotObserved
		&& Operations->GetCapacitySnapshot(EBathWaterCapacityKind::Circulation).IsSatisfied());
	Operations->OnOperationsChanged.Remove(ReentrantHandle);

	UFacilityPlacementDefinition* BathDefinition = NewObject<UFacilityPlacementDefinition>();
	UBoxComponent* BathCDOFootprint = GetMutableDefault<ABathhouseBathFacilityActor>()
		->GetFacilityPlacementComponent()->GetPlacementFootprint();
	const FVector SavedBathCDOExtent = BathCDOFootprint->GetUnscaledBoxExtent();
	BathCDOFootprint->SetBoxExtent(FVector(10.0f, 10.0f, SavedBathCDOExtent.Z));
	BathDefinition->StableId = TEXT("BathWaterBathAutomation");
	BathDefinition->FacilityTags.AddTag(FGameplayTag::RequestGameplayTag(TEXT("Facility.Placeable")));
	BathDefinition->PlacedFacilityClass = ABathhouseBathFacilityActor::StaticClass();
	BathDefinition->RecoveryItemClass = APlaceableFacilityItemActor::StaticClass();
	BathDefinition->RecoveryItemMesh = Definition->RecoveryItemMesh;
	FailureReason = FText::GetEmpty();
	TestTrue(FString::Printf(TEXT("Bath definition validates independently: %s"), *FailureReason.ToString()),
		BathDefinition->ValidateRuntime(FailureReason));
	ABathhouseBathFacilityActor* TransactionBath = World->SpawnActorDeferred<ABathhouseBathFacilityActor>(
		ABathhouseBathFacilityActor::StaticClass(), FTransform(FVector(10000.0f, 0.0f, 100.0f)), nullptr, nullptr,
		ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
	TransactionBath->GetFacilityPlacementComponent()->Definition = BathDefinition;
	TransactionBath->GetFacilityPlacementComponent()->GetPlacementFootprint()->SetBoxExtent(
		FVector(10.0f, 10.0f, SavedBathCDOExtent.Z));
	TransactionBath->FinishSpawning(FTransform(FVector(10000.0f, 0.0f, 100.0f)));
	if (!TransactionBath->HasActorBegunPlay()) TransactionBath->DispatchBeginPlay();
	TransactionBath->GetBathWaterState()->SetNormalizedAmount(0.0f);
	TransactionBath->GetBathWaterCondition()->ActualTemperatureC = 33.0f;
	TransactionBath->GetBathWaterCondition()->ContaminationPercent = 27.0f;
	TestTrue(FString::Printf(TEXT("Actual bath begins a recovery freeze: %s"), *FailureReason.ToString()),
		TransactionBath->TryBeginFacilityRecoveryHold(FailureReason));
	FFacilityPlacementPublication BathPublication;
	TestTrue(FString::Printf(TEXT("Actual bath stages domain unregistration: %s"), *FailureReason.ToString()),
		TransactionBath->StagePlacedDomainUnregistration(BathPublication, FailureReason));
	FBathWaterBathSnapshot RemovedBathSnapshot;
	TestFalse(TEXT("Staged bath is absent from the operations registry"),
		Operations->GetBathSnapshot(TransactionBath, RemovedBathSnapshot));
	TestTrue(FString::Printf(TEXT("Bath transaction rollback succeeds: %s"), *FailureReason.ToString()),
		TransactionBath->RollbackPlacedDomainUnregistration(FailureReason));
	FBathWaterBathSnapshot RestoredBathSnapshot;
	TestTrue(TEXT("Bath rollback restores registration and exact condition snapshot"),
		Operations->GetBathSnapshot(TransactionBath, RestoredBathSnapshot)
		&& FMath::IsNearlyZero(TransactionBath->GetBathWaterState()->GetNormalizedAmount())
		&& FMath::IsNearlyEqual(RestoredBathSnapshot.ActualTemperatureC, 33.0f)
		&& FMath::IsNearlyEqual(RestoredBathSnapshot.ContaminationPercent, 27.0f));
	BathCDOFootprint->SetBoxExtent(SavedBathCDOExtent);
	return true;
}

#endif
