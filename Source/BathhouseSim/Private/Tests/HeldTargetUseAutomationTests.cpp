#if WITH_DEV_AUTOMATION_TESTS

#include "Tests/UtilityLaborAutomationTestSupport.h"

#include "Camera/CameraComponent.h"
#include "Blueprint/WidgetBlueprintGeneratedClass.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Widget.h"
#include "Engine/Blueprint.h"
#include "Engine/BlueprintGeneratedClass.h"
#include "InputAction.h"
#include "Misc/CommandLine.h"
#include "Misc/PackageName.h"
#include "Misc/Parse.h"
#include "UObject/UObjectHash.h"
#include "Combat/MonkeyWrenchActor.h"
#include "Components/SphereComponent.h"
#include "Computer/PlayerComputerUseComponent.h"
#include "Engine/LocalPlayer.h"
#include "EnhancedInputComponent.h"
#include "InputAction.h"
#include "Interaction/PlayerHeldTargetUseComponent.h"
#include "Character/FirstPersonCharacter.h"
#include "Placement/PlaceableFacilityItemActor.h"
#include "Placement/PlayerFacilityPlacementComponent.h"
#include "Towel/CleanTowelStackActor.h"
#include "Towel/TowelBasketActor.h"
#include "Towel/TowelHeldTransferRules.h"
#include "Towel/TowelInventoryComponent.h"
#include "Towel/UsedTowelBinActor.h"
#include "UI/InteractionPromptWidget.h"
#include "UObject/UnrealType.h"

namespace
{
struct FHeldBasketStateCase
{
	const TCHAR* Name;
	bool bPresent;
	ETowelState State;
	int32 Count;
	int32 Capacity;
};

struct FHeldRuleTargetCase
{
	const TCHAR* Name;
	ETowelHeldTransferTargetKind Kind;
	ETowelMachineState MachineState;
	ETowelState TargetState;
	int32 TargetCount;
	int32 TargetCapacity;
	ETowelState InputState;
	ETowelState OutputState;
};

struct FExpectedHeldDirection
{
	bool bVisible = false;
	bool bCanUse = false;
	const TCHAR* Action = TEXT("");
	FString Failure;
	EPlayerInteractionActivationMode Mode = EPlayerInteractionActivationMode::Repeat;
};

FTowelInventorySnapshot MakeSnapshot(
	const ETowelState State,
	const int32 Count,
	const int32 Capacity)
{
	FTowelInventorySnapshot Snapshot;
	Snapshot.State = State;
	Snapshot.Count = Count;
	Snapshot.Capacity = Capacity;
	return Snapshot;
}

FExpectedHeldDirection EvaluateExpectedHeldDirection(
	const bool bApply,
	const TCHAR* Action,
	const FString& TargetName,
	const int32 TargetCount,
	const int32 TargetCapacity,
	const FHeldBasketStateCase& Basket,
	const bool bMachineProcessing,
	const ETowelState RequiredState,
	const bool bVisible,
	const TCHAR* FixedFailure = nullptr)
{
	FExpectedHeldDirection Expected;
	Expected.bVisible = bVisible;
	Expected.Action = Action;
	if (!bVisible)
	{
		Expected.Failure = FixedFailure ? FixedFailure : TEXT("");
		return Expected;
	}
	if (!Basket.bPresent)
	{
		Expected.Failure = TEXT("수건 바구니 필요");
		return Expected;
	}
	if (bMachineProcessing)
	{
		Expected.Failure = TEXT("작동 중");
		return Expected;
	}
	if (Basket.Count > 0 && Basket.State != RequiredState)
	{
		Expected.Failure = TEXT("다른 상태의 수건");
		return Expected;
	}
	if (bApply)
	{
		if (Basket.Count <= 0)
		{
			Expected.Failure = TEXT("바구니 비어 있음");
			return Expected;
		}
		if (TargetCount >= TargetCapacity)
		{
			Expected.Failure = FString::Printf(TEXT("%s 가득 참"), *TargetName);
			return Expected;
		}
	}
	else
	{
		if (TargetCount <= 0)
		{
			Expected.Failure = FString::Printf(TEXT("%s 비어 있음"), *TargetName);
			return Expected;
		}
		if (Basket.Count >= Basket.Capacity)
		{
			Expected.Failure = TEXT("바구니 가득 참");
			return Expected;
		}
	}
	Expected.bCanUse = true;
	return Expected;
}

bool ConfigureVisibilityTraceTarget(AActor* Target, const FVector& Location)
{
	if (!Target)
	{
		return false;
	}
	USphereComponent* TraceSphere = NewObject<USphereComponent>(Target, TEXT("HeldUseAutomationTrace"));
	if (!TraceSphere)
	{
		return false;
	}
	Target->AddInstanceComponent(TraceSphere);
	TraceSphere->InitSphereRadius(28.0f);
	TraceSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	TraceSphere->SetCollisionResponseToAllChannels(ECR_Ignore);
	TraceSphere->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
	TraceSphere->SetCanEverAffectNavigation(false);
	Target->SetRootComponent(TraceSphere);
	TraceSphere->RegisterComponent();
	Target->SetActorLocation(Location);
	return true;
}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBathhouseHeldTargetUseTowelRuleMatrixTest,
	"BathhouseSim.Interaction.HeldTargetUse.TowelRuleMatrix",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FBathhouseHeldTargetUseTowelRuleMatrixTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	const FHeldBasketStateCase BasketStates[] = {
		{ TEXT("NoBasket"), false, ETowelState::None, 0, 0 },
		{ TEXT("Empty"), true, ETowelState::None, 0, 5 },
		{ TEXT("Used"), true, ETowelState::Used, 2, 5 },
		{ TEXT("Wet"), true, ETowelState::Wet, 2, 5 },
		{ TEXT("Clean"), true, ETowelState::Clean, 2, 5 },
		{ TEXT("FullUsed"), true, ETowelState::Used, 5, 5 }
	};
	const FHeldRuleTargetCase Targets[] = {
		{ TEXT("CleanShelf"), ETowelHeldTransferTargetKind::CleanShelf, ETowelMachineState::Waiting,
			ETowelState::Clean, 3, 5, ETowelState::Clean, ETowelState::Used },
		{ TEXT("UsedBin"), ETowelHeldTransferTargetKind::UsedBin, ETowelMachineState::Waiting,
			ETowelState::Used, 3, 5, ETowelState::Wet, ETowelState::Used },
		{ TEXT("MachineWaiting"), ETowelHeldTransferTargetKind::MachinePort, ETowelMachineState::Waiting,
			ETowelState::None, 0, 5, ETowelState::Wet, ETowelState::Used },
		{ TEXT("MachineComplete"), ETowelHeldTransferTargetKind::MachinePort, ETowelMachineState::Complete,
			ETowelState::Used, 3, 5, ETowelState::Wet, ETowelState::Used },
		{ TEXT("MachineProcessing"), ETowelHeldTransferTargetKind::MachinePort, ETowelMachineState::Processing,
			ETowelState::Wet, 2, 5, ETowelState::Wet, ETowelState::Used },
		{ TEXT("WorldUsedTowel"), ETowelHeldTransferTargetKind::WorldUsedTowel, ETowelMachineState::Waiting,
			ETowelState::Used, 1, 1, ETowelState::None, ETowelState::None }
	};

	int32 DirectionCount = 0;
	for (const FHeldRuleTargetCase& Target : Targets)
	{
		const FString TargetName = Target.Kind == ETowelHeldTransferTargetKind::CleanShelf
			? TEXT("깨끗한 수건 선반")
			: Target.Kind == ETowelHeldTransferTargetKind::UsedBin
			? TEXT("사용 수건통")
			: Target.Kind == ETowelHeldTransferTargetKind::MachinePort
			? TEXT("수건 기계")
			: TEXT("사용한 수건");
		const FTowelInventorySnapshot TargetSnapshot = MakeSnapshot(
			Target.TargetState, Target.TargetCount, Target.TargetCapacity);
		for (const FHeldBasketStateCase& Basket : BasketStates)
		{
			const FTowelInventorySnapshot BasketSnapshot = MakeSnapshot(
				Basket.State, Basket.Count, Basket.Capacity);
			const FTowelHeldTransferQuery Actual = FTowelHeldTransferRules::Build(
				Target.Kind,
				FText::FromString(TargetName),
				TargetSnapshot,
				Basket.bPresent,
				BasketSnapshot,
				Target.MachineState,
				Target.InputState,
				Target.OutputState);

			FExpectedHeldDirection ExpectedApply;
			FExpectedHeldDirection ExpectedTake;
			switch (Target.Kind)
			{
			case ETowelHeldTransferTargetKind::CleanShelf:
				ExpectedApply = EvaluateExpectedHeldDirection(
					true, TEXT("넣기"), TargetName, Target.TargetCount, Target.TargetCapacity,
					Basket, false, ETowelState::Clean, true);
				ExpectedTake = EvaluateExpectedHeldDirection(
					false, TEXT("빼기"), TargetName, Target.TargetCount, Target.TargetCapacity,
					Basket, false, ETowelState::Clean, true);
				if (Target.TargetCount==0 && ExpectedTake.Failure==FString::Printf(TEXT("%s 비어 있음"),*TargetName))
					ExpectedTake.Failure=TEXT("꺼낼 수건 없음");
				break;
			case ETowelHeldTransferTargetKind::UsedBin:
				ExpectedApply.Failure = TEXT("여기에는 넣을 수 없음");
				ExpectedTake = EvaluateExpectedHeldDirection(
					false, TEXT("빼기"), TargetName, Target.TargetCount, Target.TargetCapacity,
					Basket, false, ETowelState::Used, true);
				break;
			case ETowelHeldTransferTargetKind::MachinePort:
				if (Target.MachineState == ETowelMachineState::Waiting)
				{
					ExpectedApply = EvaluateExpectedHeldDirection(
						true, TEXT("넣기"), TargetName, Target.TargetCount, Target.TargetCapacity,
						Basket, false, Target.InputState, true);
					ExpectedTake = EvaluateExpectedHeldDirection(
						false,TEXT("빼기"),TargetName,Target.TargetCount,Target.TargetCapacity,
						Basket,false,Target.InputState,true);
					if (Target.TargetCount==0 && ExpectedTake.Failure==FString::Printf(TEXT("%s 비어 있음"),*TargetName))
						ExpectedTake.Failure=TEXT("꺼낼 수건 없음");
				}
				else if (Target.MachineState == ETowelMachineState::Complete)
				{
					ExpectedApply = { true, false, TEXT("넣기"), TEXT("비운 뒤 넣을 수 있음"), EPlayerInteractionActivationMode::Repeat };
					ExpectedTake = EvaluateExpectedHeldDirection(
						false, TEXT("빼기"), TargetName, Target.TargetCount, Target.TargetCapacity,
						Basket, false, Target.OutputState, true);
				}
				else
				{
					ExpectedApply = EvaluateExpectedHeldDirection(
						true, TEXT("넣기"), TargetName, Target.TargetCount, Target.TargetCapacity,
						Basket, true, Target.InputState, true);
					ExpectedTake = EvaluateExpectedHeldDirection(
						false, TEXT("빼기"), TargetName, Target.TargetCount, Target.TargetCapacity,
						Basket, true, Target.OutputState, true);
				}
				break;
			case ETowelHeldTransferTargetKind::WorldUsedTowel:
				ExpectedApply = EvaluateExpectedHeldDirection(
					true, TEXT(""), TargetName, Target.TargetCount, Target.TargetCapacity,
					Basket, false, ETowelState::Used, false, TEXT("여기에는 넣을 수 없음"));
				ExpectedApply.Mode = EPlayerInteractionActivationMode::Instant;
				ExpectedTake = EvaluateExpectedHeldDirection(
					false, TEXT("줍기"), TargetName, Target.TargetCount, Target.TargetCapacity,
					Basket, false, ETowelState::Used, true);
				ExpectedTake.Mode = EPlayerInteractionActivationMode::Instant;
				break;
			}

			const FString Prefix = FString::Printf(TEXT("%s/%s"), Target.Name, Basket.Name);
			auto CheckDirection = [this, &Prefix, &DirectionCount](
				const TCHAR* DirectionName,
				const FTowelHeldTransferDirection& ActualDirection,
				const FExpectedHeldDirection& ExpectedDirection)
			{
				const FString Label = FString::Printf(TEXT("%s/%s"), *Prefix, DirectionName);
				TestEqual(*FString::Printf(TEXT("%s visibility"), *Label),
					ActualDirection.bVisible, ExpectedDirection.bVisible);
				TestEqual(*FString::Printf(TEXT("%s availability"), *Label),
					ActualDirection.bCanUse, ExpectedDirection.bCanUse);
				TestEqual(*FString::Printf(TEXT("%s action"), *Label),
					ActualDirection.ActionName.ToString(), FString(ExpectedDirection.Action));
				TestEqual(*FString::Printf(TEXT("%s failure"), *Label),
					ActualDirection.FailureReason.ToString(), ExpectedDirection.Failure);
				TestEqual(*FString::Printf(TEXT("%s activation"), *Label),
					ActualDirection.ActivationMode, ExpectedDirection.Mode);
				++DirectionCount;
			};
			CheckDirection(TEXT("Apply"), Actual.Apply, ExpectedApply);
			CheckDirection(TEXT("Take"), Actual.Take, ExpectedTake);
		}
	}
	TestEqual(TEXT("Six target states times six basket states times two directions"), DirectionCount, 72);

	const FTowelHeldTransferQuery FullShelfQuery = FTowelHeldTransferRules::Build(
		ETowelHeldTransferTargetKind::CleanShelf,
		FText::FromString(TEXT("깨끗한 수건 선반")),
		MakeSnapshot(ETowelState::Clean, 5, 5),
		true,
		MakeSnapshot(ETowelState::Clean, 1, 5));
	TestEqual(TEXT("Apply target-full reason names the target"),
		FullShelfQuery.Apply.FailureReason.ToString(), FString(TEXT("깨끗한 수건 선반 가득 참")));
	const FTowelHeldTransferQuery EmptyBinQuery = FTowelHeldTransferRules::Build(
		ETowelHeldTransferTargetKind::UsedBin,
		FText::FromString(TEXT("사용 수건통")),
		MakeSnapshot(ETowelState::None, 0, 5),
		true,
		MakeSnapshot(ETowelState::None, 0, 5));
	TestEqual(TEXT("Take target-empty reason names the target"),
		EmptyBinQuery.Take.FailureReason.ToString(), FString(TEXT("사용 수건통 비어 있음")));

	FPlayerInteractionQuery EmptyQuery;
	FPlayerInteractionQuery WithHeldApply = EmptyQuery;
	WithHeldApply.bHeldApplyVisible = true;
	WithHeldApply.bCanHeldApply = true;
	WithHeldApply.HeldApplyActionName = FText::FromString(TEXT("넣기"));
	WithHeldApply.HeldApplyActivationMode = EPlayerInteractionActivationMode::Repeat;
	TestFalse(TEXT("Held Apply fields participate in query equality"), EmptyQuery.Equals(WithHeldApply));
	FPlayerInteractionQuery WithHeldTake = EmptyQuery;
	WithHeldTake.bHeldTakeVisible = true;
	WithHeldTake.HeldTakeFailureReason = FText::FromString(TEXT("바구니 가득 참"));
	TestFalse(TEXT("Held Take fields participate in query equality"), EmptyQuery.Equals(WithHeldTake));
	FProperty* OptionalTakeAction = FindFProperty<FProperty>(
		UInteractionPromptWidget::StaticClass(), TEXT("HeldTakeActionNameText"));
	FProperty* OptionalTakeFailure = FindFProperty<FProperty>(
		UInteractionPromptWidget::StaticClass(), TEXT("HeldTakeFailureReasonText"));
	TestTrue(TEXT("RMB action widget is optional"), OptionalTakeAction
		&& OptionalTakeAction->HasMetaData(TEXT("BindWidgetOptional")));
	TestTrue(TEXT("RMB failure widget is optional"), OptionalTakeFailure
		&& OptionalTakeFailure->HasMetaData(TEXT("BindWidgetOptional")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBathhouseHeldTargetUseRepeatTest,
	"BathhouseSim.Interaction.HeldTargetUse.RepeatLifecycle",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FBathhouseHeldTargetUseRepeatTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	FScopedUtilityLaborWorld Scope(TEXT("HeldTargetUseRepeatWorld"));
	UWorld* World = Scope.Get();
	if (!TestNotNull(TEXT("Automation world exists"), World) || !GEngine)
	{
		return false;
	}

	APawn* User = World->SpawnActor<APawn>();
	UCameraComponent* Camera = User ? NewObject<UCameraComponent>(User, TEXT("HeldUseCamera")) : nullptr;
	UPlayerCarryComponent* Carry = User ? NewObject<UPlayerCarryComponent>(User, TEXT("HeldUseCarry")) : nullptr;
	UPlayerInteractionComponent* Interaction = User
		? NewObject<UPlayerInteractionComponent>(User, TEXT("HeldUseInteraction")) : nullptr;
	UPlayerEquipmentUseComponent* EquipmentUse = User
		? NewObject<UPlayerEquipmentUseComponent>(User, TEXT("HeldUseEquipment")) : nullptr;
	UPlayerHeldTargetUseComponent* HeldUse = User
		? NewObject<UPlayerHeldTargetUseComponent>(User, TEXT("HeldUseComponent")) : nullptr;
	if (!TestNotNull(TEXT("Repeat owner exists"), User)
		|| !TestNotNull(TEXT("Repeat camera exists"), Camera)
		|| !TestNotNull(TEXT("Repeat carry exists"), Carry)
		|| !TestNotNull(TEXT("Repeat interaction exists"), Interaction)
		|| !TestNotNull(TEXT("Repeat equipment use exists"), EquipmentUse)
		|| !TestNotNull(TEXT("Repeat component exists"), HeldUse))
	{
		return false;
	}

	User->SetRootComponent(Camera);
	User->AddInstanceComponent(Camera);
	User->AddInstanceComponent(Carry);
	User->AddInstanceComponent(Interaction);
	User->AddInstanceComponent(EquipmentUse);
	User->AddInstanceComponent(HeldUse);
	Camera->RegisterComponent();
	Carry->RegisterComponent();
	Interaction->RegisterComponent();
	EquipmentUse->RegisterComponent();
	HeldUse->RegisterComponent();
	Camera->SetWorldLocationAndRotation(FVector::ZeroVector, FRotator::ZeroRotator);
	Carry->ConfigureHeldAnchor(Camera);
	Carry->ConfigureEquipmentUse(EquipmentUse);
	Interaction->Configure(Camera, Carry);
	Interaction->ConfigureEquipmentUse(EquipmentUse);
	EquipmentUse->Configure(Camera, Carry, Interaction, nullptr);
	HeldUse->Configure(Interaction, Carry, EquipmentUse);
	BeginActorPlayIfNeeded(User);

	APlayerController* Controller = World->SpawnActor<APlayerController>();
	ULocalPlayer* LocalPlayer = NewObject<ULocalPlayer>(GEngine);
	if (!TestNotNull(TEXT("Repeat player controller exists"), Controller)
		|| !TestNotNull(TEXT("Repeat local player exists"), LocalPlayer))
	{
		return false;
	}
	BeginActorPlayIfNeeded(Controller);
	Controller->SetPlayer(LocalPlayer);
	Controller->Possess(User);
	if (!TestTrue(TEXT("Held-use owner is locally controlled"), User->IsLocallyControlled()))
	{
		return false;
	}

	ATowelBasketActor* Basket = World->SpawnActor<ATowelBasketActor>();
	AUsedTowelBinActor* Bin = World->SpawnActor<AUsedTowelBinActor>();
	if (!TestNotNull(TEXT("Used basket exists"), Basket)
		|| !TestNotNull(TEXT("Used towel bin exists"), Bin))
	{
		return false;
	}
	BeginActorPlayIfNeeded(Basket);
	BeginActorPlayIfNeeded(Bin);
	Basket->GetInventory()->Capacity = 3;
	Basket->GetInventory()->State = ETowelState::None;
	Basket->GetInventory()->Count = 0;
	Bin->GetInventory()->Capacity = 10;
	Bin->GetInventory()->State = ETowelState::Used;
	Bin->GetInventory()->Count = 5;
	FText CarryFailure;
	if (!TestTrue(TEXT("A used basket fixture is carried"), Carry->TryTakePhysicalObject(Basket, CarryFailure))
		|| !TestTrue(TEXT("Bin has a visibility target for the camera trace"),
			ConfigureVisibilityTraceTarget(Bin, Camera->GetComponentLocation() + Camera->GetForwardVector() * 120.0f)))
	{
		return false;
	}
	Bin->GetFacilityPlacementComponent()->SetPlacedDomainActive(true);
	Interaction->RefreshInteractionQuery();
	FHitResult InitialHit;
	if (!TestTrue(TEXT("Fresh trace hits the used bin"), Interaction->GetCurrentFocusHit(InitialHit)
		&& InitialHit.GetActor() == Bin))
	{
		return false;
	}

	TArray<FPlayerInteractionResult> Reports;
	Interaction->OnInteractionAttemptFinishedNative.AddLambda(
		[&Reports](const FPlayerInteractionResult& Result) { Reports.Add(Result); });
	HeldUse->BeginUse(EPlayerHeldTargetUseDirection::Take);
	TestEqual(TEXT("Press begins with one immediate unit transfer"), Basket->GetInventory()->GetSnapshot().Count, 1);
	TestEqual(TEXT("First unit leaves the used bin"), Bin->GetInventory()->GetSnapshot().Count, 4);
	TestTrue(TEXT("A Repeat target remains active after the first unit"), HeldUse->bRepeatActive);
	HeldUse->TickRepeat(0.149f);
	TestEqual(TEXT("Less than the default interval does not repeat"), Basket->GetInventory()->GetSnapshot().Count, 1);
	HeldUse->TickRepeat(0.002f);
	TestEqual(TEXT("Elapsed 0.151 seconds produces one repeat"), Basket->GetInventory()->GetSnapshot().Count, 2);
	HeldUse->TickRepeat(0.5f);
	TestEqual(TEXT("A large Tick still performs at most one action"), Basket->GetInventory()->GetSnapshot().Count, 3);
	TestEqual(TEXT("The large Tick removes one towel only"), Bin->GetInventory()->GetSnapshot().Count, 2);
	HeldUse->TickRepeat(0.15f);
	TestFalse(TEXT("A full basket stops the repeat"), HeldUse->bRepeatActive);
	TestEqual(TEXT("Full basket reports a failure once"), Reports.Num(), 4);
	TestEqual(TEXT("Repeat failure intent is held Take"),
		Reports.Last().Intent, EPlayerInteractionIntent::HeldTake);
	TestEqual(TEXT("Repeat failure surfaces the basket-full reason"),
		Reports.Last().FailureReason.ToString(), FString(TEXT("바구니 가득 참")));
	HeldUse->TickRepeat(0.5f);
	TestEqual(TEXT("A stopped press does not repeat again"), Basket->GetInventory()->GetSnapshot().Count, 3);
	TestEqual(TEXT("A stopped press does not report duplicate failure"), Reports.Num(), 4);

	Basket->GetInventory()->Capacity = 5;
	HeldUse->TickRepeat(0.5f);
	TestEqual(TEXT("A newly available query does not restart a stopped press"), Basket->GetInventory()->GetSnapshot().Count, 3);
	HeldUse->EndUse();
	HeldUse->BeginUse(EPlayerHeldTargetUseDirection::Take);
	TestEqual(TEXT("A fresh press succeeds after release"), Basket->GetInventory()->GetSnapshot().Count, 4);

	HeldUse->EndUse();
	Bin->GetInventory()->State = ETowelState::Used;
	Bin->GetInventory()->Count = 5;
	Basket->GetInventory()->State = ETowelState::None;
	Basket->GetInventory()->Count = 0;
	HeldUse->RepeatIntervalSeconds = 0.3f;
	HeldUse->BeginUse(EPlayerHeldTargetUseDirection::Take);
	const int32 AtStartOfCustomInterval = Basket->GetInventory()->GetSnapshot().Count;
	HeldUse->TickRepeat(0.299f);
	TestEqual(TEXT("Custom 0.3 interval has not elapsed at 0.299"),
		Basket->GetInventory()->GetSnapshot().Count, AtStartOfCustomInterval);
	HeldUse->TickRepeat(0.002f);
	TestEqual(TEXT("Custom 0.3 interval repeats after 0.301 seconds"),
		Basket->GetInventory()->GetSnapshot().Count, AtStartOfCustomInterval + 1);
	const int32 BeforeCatchUpTick = Basket->GetInventory()->GetSnapshot().Count;
	HeldUse->TickRepeat(0.9f);
	TestEqual(TEXT("A 0.9 second Tick performs only one action"),
		Basket->GetInventory()->GetSnapshot().Count, BeforeCatchUpTick + 1);
	const int32 BeforeNextTick = Basket->GetInventory()->GetSnapshot().Count;
	HeldUse->TickRepeat(0.0f);
	TestEqual(TEXT("A later Tick consumes only one retained interval"),
		Basket->GetInventory()->GetSnapshot().Count, BeforeNextTick + 1);

	HeldUse->EndUse();
	Bin->GetInventory()->Count = 5;
	Basket->GetInventory()->State = ETowelState::None;
	Basket->GetInventory()->Count = 0;
	HeldUse->BeginUse(EPlayerHeldTargetUseDirection::Take);
	const int32 BeforeAimLossReports = Reports.Num();
	const int32 BeforeAimLossCount = Basket->GetInventory()->GetSnapshot().Count;
	Bin->SetActorLocation(FVector(1000.0f, 0.0f, 0.0f));
	HeldUse->TickRepeat(0.5f);
	TestFalse(TEXT("Aim loss silently stops Repeat"), HeldUse->bRepeatActive);
	Bin->SetActorLocation(Camera->GetComponentLocation() + Camera->GetForwardVector() * 120.0f);
	HeldUse->TickRepeat(1.0f);
	TestEqual(TEXT("Re-aiming while held does not resume"), Basket->GetInventory()->GetSnapshot().Count, BeforeAimLossCount);
	TestEqual(TEXT("Aim loss reports no failure"), Reports.Num(), BeforeAimLossReports);
	HeldUse->EndUse();
	HeldUse->BeginUse(EPlayerHeldTargetUseDirection::Take);
	TestEqual(TEXT("Release and a new press resumes one action"),
		Basket->GetInventory()->GetSnapshot().Count, BeforeAimLossCount + 1);

	HeldUse->EndUse();
	Bin->GetInventory()->Count = 5;
	Basket->GetInventory()->State = ETowelState::None;
	Basket->GetInventory()->Count = 0;
	HeldUse->BeginUse(EPlayerHeldTargetUseDirection::Take);
	const int32 BeforeHeldChangeReports = Reports.Num();
	Carry->CommitReleasePhysicalObject(Basket);
	HeldUse->TickRepeat(0.15f);
	TestFalse(TEXT("Changing the held object stops Repeat"), HeldUse->bRepeatActive);
	TestEqual(TEXT("Held-object change is quiet"), Reports.Num(), BeforeHeldChangeReports);

	Carry->TryTakePhysicalObject(Basket, CarryFailure);
	Basket->SetActorLocation(FVector(-500.0f, 0.0f, 0.0f));
	Interaction->RefreshInteractionQuery();
	HeldUse->BeginUse(EPlayerHeldTargetUseDirection::Take);
	const int32 BeforeSuppressionReports = Reports.Num();
	Interaction->SetInteractionSuppressed(true);
	HeldUse->TickRepeat(0.15f);
	TestFalse(TEXT("Interaction suppression stops Repeat"), HeldUse->bRepeatActive);
	TestEqual(TEXT("Suppression stops Repeat without a transient failure"), Reports.Num(), BeforeSuppressionReports);
	Interaction->SetInteractionSuppressed(false);
	HeldUse->EndUse();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBathhouseHeldTargetUseOwnerRoutingTest,
	"BathhouseSim.Interaction.HeldTargetUse.InputOwners",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FBathhouseHeldTargetUseOwnerRoutingTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	FScopedUtilityLaborWorld Scope(TEXT("HeldTargetUseOwnerWorld"));
	UWorld* World = Scope.Get();
	if (!TestNotNull(TEXT("Automation world exists"), World))
	{
		return false;
	}
	AFirstPersonCharacter* Character = World->SpawnActor<AFirstPersonCharacter>();
	if (!TestNotNull(TEXT("First person owner exists"), Character))
	{
		return false;
	}
	BeginActorPlayIfNeeded(Character);
	APlayerController* Controller = World->SpawnActor<APlayerController>();
	ULocalPlayer* LocalPlayer = NewObject<ULocalPlayer>(GEngine);
	if (!TestNotNull(TEXT("Input owner player controller exists"), Controller)
		|| !TestNotNull(TEXT("Input owner local player exists"), LocalPlayer))
	{
		return false;
	}
	BeginActorPlayIfNeeded(Controller);
	Controller->SetPlayer(LocalPlayer);
	Controller->Possess(Character);
	if (!TestTrue(TEXT("Input owner character is locally controlled"), Character->IsLocallyControlled()))
	{
		return false;
	}

	UInputAction* PrimaryAction = NewObject<UInputAction>(Character);
	UInputAction* SecondaryAction = NewObject<UInputAction>(Character);
	Character->PrimaryUseAction = PrimaryAction;
	Character->SecondaryUseAction = SecondaryAction;
	UEnhancedInputComponent* Input = NewObject<UEnhancedInputComponent>(Character);
	Character->SetupPlayerInputComponent(Input);
	int32 PrimaryBindingCount = 0;
	int32 SecondaryBindingCount = 0;
	for (const TUniquePtr<FEnhancedInputActionEventBinding>& Binding : Input->GetActionEventBindings())
	{
		PrimaryBindingCount += Binding->GetAction() == PrimaryAction ? 1 : 0;
		SecondaryBindingCount += Binding->GetAction() == SecondaryAction ? 1 : 0;
	}
	TestEqual(TEXT("LMB action retains four lifecycle bindings"), PrimaryBindingCount, 4);
	TestEqual(TEXT("RMB action binds Started/Completed/Canceled"), SecondaryBindingCount, 3);

	Character->PlayerComputerUse->Phase = EPlayerComputerUsePhase::Active;
	Character->PrimaryUseStartInput();
	TestEqual(TEXT("Computer capture owns LMB first"),
		Character->PrimaryUsePressOwner, AFirstPersonCharacter::EPrimaryUsePressOwner::Computer);
	Character->PrimaryUseEndInput();
	Character->SecondaryUseStartInput();
	TestEqual(TEXT("Computer capture consumes RMB"),
		Character->SecondaryUsePressOwner, AFirstPersonCharacter::ESecondaryUsePressOwner::Ignored);
	Character->SecondaryUseEndInput();
	Character->PlayerComputerUse->Phase = EPlayerComputerUsePhase::Inactive;

	APlaceableFacilityItemActor* PreviewItem = World->SpawnActor<APlaceableFacilityItemActor>();
	if (!TestNotNull(TEXT("Placement preview fixture exists"), PreviewItem))
	{
		return false;
	}
	Character->PlayerFacilityPlacement->PreviewFacility = PreviewItem;
	Character->SecondaryUseStartInput();
	TestEqual(TEXT("Placement consumes RMB"), Character->SecondaryUsePressOwner,
		AFirstPersonCharacter::ESecondaryUsePressOwner::Ignored);
	Character->SecondaryUseEndInput();
	Character->PrimaryUseStartInput();
	TestEqual(TEXT("Placement owns LMB after higher-priority Computer capture"),
		Character->PrimaryUsePressOwner, AFirstPersonCharacter::EPrimaryUsePressOwner::Placement);
	Character->PrimaryUseEndInput();
	Character->PlayerFacilityPlacement->PreviewFacility.Reset();


	ACleanTowelStackActor* Shelf = World->SpawnActor<ACleanTowelStackActor>();
	if (!TestNotNull(TEXT("Clean shelf query fixture exists"), Shelf)
		|| !TestTrue(TEXT("Clean shelf fixture exposes visibility collision"),
			ConfigureVisibilityTraceTarget(
				Shelf,
				Character->GetFirstPersonCamera()->GetComponentLocation()
					+ Character->GetFirstPersonCamera()->GetForwardVector() * 140.0f)))
	{
		return false;
	}
	BeginActorPlayIfNeeded(Shelf);
	Character->GetPlayerInteraction()->RefreshInteractionQuery();
	const FPlayerInteractionQuery ShelfQuery = Character->GetPlayerInteraction()->GetCurrentInteractionQuery();
	TestTrue(TEXT("Empty hand receives the towel Apply hint"),
		ShelfQuery.bHeldApplyVisible
		&& ShelfQuery.HeldApplyFailureReason.ToString() == TEXT("수건 바구니 필요"));
	Character->PrimaryUseStartInput();
	TestEqual(TEXT("An Apply row routes empty-hand LMB to held target use"),
		Character->PrimaryUsePressOwner, AFirstPersonCharacter::EPrimaryUsePressOwner::HeldTargetUse);
	Character->PrimaryUseEndInput();

	Shelf->SetActorLocation(FVector(1000.0f, 0.0f, 0.0f));
	Character->GetPlayerInteraction()->ClearInteractionQuery();
	Character->PrimaryUseStartInput();
	TestEqual(TEXT("Empty-space LMB keeps the legacy equipment fallback"),
		Character->PrimaryUsePressOwner, AFirstPersonCharacter::EPrimaryUsePressOwner::Equipment);
	Character->PrimaryUseEndInput();

	ATowelBasketActor* Basket = World->SpawnActor<ATowelBasketActor>();
	if (!TestNotNull(TEXT("Non-equipment held object exists"), Basket))
	{
		return false;
	}
	BeginActorPlayIfNeeded(Basket);
	FText CarryFailure;
	TestTrue(TEXT("The hand takes the non-equipment basket"),
		Character->GetPlayerCarry()->TryTakePhysicalObject(Basket, CarryFailure));
	Character->PrimaryUseStartInput();
	TestEqual(TEXT("A held non-equipment object routes LMB to held target use"),
		Character->PrimaryUsePressOwner, AFirstPersonCharacter::EPrimaryUsePressOwner::HeldTargetUse);
	Character->SecondaryUseStartInput();
	TestEqual(TEXT("RMB is ignored while LMB held-use owns the component"),
		Character->SecondaryUsePressOwner, AFirstPersonCharacter::ESecondaryUsePressOwner::Ignored);
	Character->SecondaryUseEndInput();
	Character->PrimaryUseEndInput();

	Character->SecondaryUseStartInput();
	TestEqual(TEXT("RMB routes to held target Take with a basket"),
		Character->SecondaryUsePressOwner, AFirstPersonCharacter::ESecondaryUsePressOwner::HeldTargetUse);
	Character->PrimaryUseStartInput();
	TestEqual(TEXT("LMB is ignored while RMB held-use owns the component"),
		Character->PrimaryUsePressOwner, AFirstPersonCharacter::EPrimaryUsePressOwner::Ignored);
	Character->PrimaryUseEndInput();
	Character->SecondaryUseEndInput();

	TestTrue(TEXT("The test releases the non-equipment held object"),
		Character->GetPlayerCarry()->RecoverHeldPhysicalObject(Basket));
	AMonkeyWrenchActor* Wrench = World->SpawnActor<AMonkeyWrenchActor>();
	if (!TestNotNull(TEXT("Held equipment fixture exists"), Wrench))
	{
		return false;
	}
	UStaticMesh* CubeMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (UStaticMeshComponent* WrenchMesh = Wrench->FindComponentByClass<UStaticMeshComponent>())
	{
		WrenchMesh->SetStaticMesh(CubeMesh);
		WrenchMesh->SetWorldScale3D(FVector(0.15f));
		WrenchMesh->UpdateBounds();
	}
	BeginActorPlayIfNeeded(Wrench);
	TestTrue(TEXT("The hand takes held equipment"), Character->GetPlayerCarry()->TryTakePhysicalObject(Wrench, CarryFailure));
	Character->SecondaryUseStartInput();
	TestEqual(TEXT("RMB is ignored while held equipment owns LMB"),
		Character->SecondaryUsePressOwner, AFirstPersonCharacter::ESecondaryUsePressOwner::Ignored);
	Character->SecondaryUseEndInput();
	Character->PrimaryUseStartInput();
	TestEqual(TEXT("Held equipment keeps primary input ownership"),
		Character->PrimaryUsePressOwner, AFirstPersonCharacter::EPrimaryUsePressOwner::Equipment);
	Character->PrimaryUseEndInput();
	TestTrue(TEXT("Equipment fixture releases after the routing case"),
		Character->GetPlayerCarry()->RecoverHeldPhysicalObject(Wrench));
	return true;
}


namespace
{
struct FHeldUseBlueprintSpec
{
	const TCHAR* Name;
	const TCHAR* OriginalPackage;
	const TCHAR* CopyPackage;
	UClass* NativeParent;
	bool bCharacter;
};

UBlueprint* FindHeldUseBlueprint(UPackage* Package)
{
	if (!Package)
	{
		return nullptr;
	}
	UBlueprint* Found = nullptr;
	ForEachObjectWithOuter(Package, [&Found](UObject* Object)
	{
		if (UBlueprint* Blueprint = Cast<UBlueprint>(Object))
		{
			Found = Blueprint;
		}
	}, EGetObjectsFlags::None);
	return Found;
}

bool HasWidgetNamed(const UWidgetTree* Tree, const TCHAR* WidgetName)
{
	return Tree && Tree->FindWidget(FName(WidgetName)) != nullptr;
}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBathhouseHeldTargetUseBlueprintLoadTest,
	"BathhouseSim.Interaction.HeldTargetUse.BlueprintLoad",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FBathhouseHeldTargetUseBlueprintLoadTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	const FHeldUseBlueprintSpec Specs[] = {
		{
			TEXT("BP_FirstPersonCharacter"),
			TEXT("/Game/FirstPersonCharacter/BP_FirstPersonCharacter"),
			TEXT("/Game/Developers/MigrationCheck/BP_FirstPersonCharacter"),
			AFirstPersonCharacter::StaticClass(),
			true
		},
		{
			TEXT("WBP_InteractionPrompt"),
			TEXT("/Game/Bathhouse/UI/WBP_InteractionPrompt"),
			TEXT("/Game/Developers/MigrationCheck/WBP_InteractionPrompt"),
			UInteractionPromptWidget::StaticClass(),
			false
		}
	};
	FString RequestedPath;
	FParse::Value(FCommandLine::Get(), TEXT("BathhouseHeldUseLoadPath="), RequestedPath);
	TArray<const FHeldUseBlueprintSpec*> SelectedSpecs;
	if (RequestedPath.IsEmpty())
	{
		for (const FHeldUseBlueprintSpec& Spec : Specs)
		{
			SelectedSpecs.Add(&Spec);
		}
	}
	else
	{
		if (!FPackageName::IsValidLongPackageName(RequestedPath))
		{
			AddError(FString::Printf(TEXT("Invalid held-use Blueprint package path: %s"), *RequestedPath));
			return false;
		}
		for (const FHeldUseBlueprintSpec& Spec : Specs)
		{
			if (RequestedPath.Equals(Spec.OriginalPackage, ESearchCase::IgnoreCase)
				|| RequestedPath.Equals(Spec.CopyPackage, ESearchCase::IgnoreCase))
			{
				SelectedSpecs.Add(&Spec);
				break;
			}
		}
		if (SelectedSpecs.IsEmpty())
		{
			AddError(FString::Printf(TEXT("Unrecognized held-use load path: %s"), *RequestedPath));
			return false;
		}
	}

	const TCHAR* RequiredPromptWidgets[] = {
		TEXT("PromptRoot"),
		TEXT("TargetNameText"),
		TEXT("ActionNameText"),
		TEXT("FailureReasonText"),
		TEXT("SecondaryActionNameText"),
		TEXT("SecondaryFailureReasonText"),
		TEXT("InteractionProgressBar"),
		TEXT("EquipmentActionNameText"),
		TEXT("EquipmentFailureReasonText"),
		TEXT("EquipmentProgressBar"),
		TEXT("PlacementActionNameText"),
		TEXT("PlacementFailureReasonText"),
		TEXT("RecoveryActionNameText"),
		TEXT("RecoveryFailureReasonText"),
		TEXT("RecoveryProgressBar")
	};

	for (const FHeldUseBlueprintSpec* Spec : SelectedSpecs)
	{
		const FString PackageToLoad = RequestedPath.IsEmpty()
			? FString(Spec->OriginalPackage) : RequestedPath;
		UPackage* Package = LoadPackage(nullptr, *PackageToLoad, LOAD_None);
		UBlueprint* Blueprint = FindHeldUseBlueprint(Package);
		if (!TestNotNull(FString::Printf(TEXT("%s Blueprint package loads"), Spec->Name), Package)
			|| !TestNotNull(FString::Printf(TEXT("%s Blueprint object loads"), Spec->Name), Blueprint)
			|| !TestNotNull(FString::Printf(TEXT("%s generated class loads"), Spec->Name),
				Blueprint ? Blueprint->GeneratedClass.Get() : nullptr))
		{
			return false;
		}
		TestTrue(FString::Printf(TEXT("%s retains its native parent"), Spec->Name),
			Blueprint->GeneratedClass->GetSuperClass() == Spec->NativeParent);

		if (Spec->bCharacter)
		{
			AFirstPersonCharacter* CharacterCDO = Cast<AFirstPersonCharacter>(
				Blueprint->GeneratedClass->GetDefaultObject());
			if (!TestNotNull(TEXT("Character Blueprint CDO loads"), CharacterCDO))
			{
				return false;
			}
			UPlayerHeldTargetUseComponent* HeldUse =
				CharacterCDO->FindComponentByClass<UPlayerHeldTargetUseComponent>();
			TestNotNull(TEXT("Character CDO contains PlayerHeldTargetUse"), HeldUse);
			if (HeldUse)
			{
				TestEqual(TEXT("PlayerHeldTargetUse keeps the 0.15 second default"),
					HeldUse->RepeatIntervalSeconds, 0.15f);
				TestEqual(TEXT("Default subobject keeps its serialized name"),
					HeldUse->GetFName(), FName(TEXT("PlayerHeldTargetUse")));
			}

			const auto CheckInputReference = [this, CharacterCDO](
				const TCHAR* PropertyName,
				const TCHAR* AssetPath)
			{
				const FObjectPropertyBase* Property = FindFProperty<FObjectPropertyBase>(
					AFirstPersonCharacter::StaticClass(), PropertyName);
				UObject* Expected = LoadObject<UInputAction>(nullptr, AssetPath);
				TestNotNull(FString::Printf(TEXT("%s property remains reflected"), PropertyName), Property);
				TestNotNull(FString::Printf(TEXT("%s action asset loads"), PropertyName), Expected);
				if (Property && Expected)
				{
					TestTrue(FString::Printf(TEXT("%s preserves its existing action assignment"), PropertyName),
						Property->GetObjectPropertyValue_InContainer(CharacterCDO) == Expected);
				}
			};
			CheckInputReference(TEXT("PrimaryUseAction"), TEXT("/Game/Input/Actions/IA_PrimaryUse.IA_PrimaryUse"));
			CheckInputReference(TEXT("InteractAction"), TEXT("/Game/Input/Actions/IA_Interact.IA_Interact"));
			CheckInputReference(
				TEXT("SecondaryInteractAction"),
				TEXT("/Game/Input/Actions/IA_SecondaryInteract.IA_SecondaryInteract"));
			TestNotNull(TEXT("Character class exposes the new SecondaryUseAction property"),
				FindFProperty<FObjectPropertyBase>(AFirstPersonCharacter::StaticClass(), TEXT("SecondaryUseAction")));
		}
		else
		{
			const UWidgetBlueprintGeneratedClass* WidgetClass =
				Cast<UWidgetBlueprintGeneratedClass>(Blueprint->GeneratedClass);
			if (!TestNotNull(TEXT("Prompt generated widget class loads"), WidgetClass))
			{
				return false;
			}
			TestNotNull(TEXT("Prompt widget tree archetype loads"), WidgetClass->GetWidgetTreeArchetype());
			for (const TCHAR* WidgetName : RequiredPromptWidgets)
			{
				TestTrue(FString::Printf(TEXT("WBP_InteractionPrompt retains required widget %s"), WidgetName),
					HasWidgetNamed(WidgetClass->GetWidgetTreeArchetype(), WidgetName));
			}
		}
	}
	return true;
}
#endif