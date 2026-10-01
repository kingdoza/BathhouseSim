#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "BathWaterOperationsAutomationTestSupport.h"
#include "Components/Slider.h"
#include "Components/TextBlock.h"
#include "Facility/BathWaterConditionComponent.h"
#include "Facility/BathWaterOperationsSubsystem.h"
#include "Facility/BathWaterSettings.h"
#include "Facility/BathhouseBathFacilityActor.h"
#include "Framework/Application/SlateApplication.h"
#include "UI/BathWaterDetailWidget.h"
#include "Utility/UtilityOperationComponent.h"
#include "Widgets/SVirtualWindow.h"

// Test-only access to the private/protected members of UBathWaterDetailWidget (friend in its header).
class FBathWaterSliderTestAccess
{
public:
	static UBathWaterDetailWidget* Create(UObject* Outer)
	{
		UBathWaterDetailWidget* Detail = NewObject<UBathWaterDetailWidget>(Outer);
		Detail->BathNameText = NewObject<UTextBlock>(Detail);
		Detail->WaterAmountText = NewObject<UTextBlock>(Detail);
		Detail->ActualTemperatureText = NewObject<UTextBlock>(Detail);
		Detail->TargetTemperatureText = NewObject<UTextBlock>(Detail);
		Detail->ContaminationText = NewObject<UTextBlock>(Detail);
		Detail->CirculationText = NewObject<UTextBlock>(Detail);
		Detail->CirculationDemandText = NewObject<UTextBlock>(Detail);
		Detail->HeatingDemandText = NewObject<UTextBlock>(Detail);
		Detail->CoolingDemandText = NewObject<UTextBlock>(Detail);
		Detail->ThermalStatusText = NewObject<UTextBlock>(Detail);
		Detail->ThermalThresholdText = NewObject<UTextBlock>(Detail);
		Detail->FeedbackText = NewObject<UTextBlock>(Detail);
		Detail->CapacityStatusText = NewObject<UTextBlock>(Detail);
		Detail->CirculationSlider = NewObject<USlider>(Detail);
		Detail->TargetTemperatureSlider = NewObject<USlider>(Detail);
		return Detail;
	}
	static void Construct(UBathWaterDetailWidget& Detail) { Detail.NativeConstruct(); }
	static void Destruct(UBathWaterDetailWidget& Detail) { Detail.NativeDestruct(); }
	static USlider* Circulation(UBathWaterDetailWidget& Detail) { return Detail.CirculationSlider; }
	static USlider* Target(UBathWaterDetailWidget& Detail) { return Detail.TargetTemperatureSlider; }
	static FString Feedback(UBathWaterDetailWidget& Detail) { return Detail.FeedbackText->GetText().ToString(); }
	static int32 PresentationWrites(const UBathWaterDetailWidget& Detail) { return Detail.PresentationWriteCount; }
	static void WriteWithoutGuard(UBathWaterDetailWidget& Detail, USlider* Slider, const float Value)
	{
		TGuardValue<bool> Guard(Detail.bWritingSliderValue, true);
		Slider->SetValue(Value);
	}
};

namespace BathWaterSliderInputTest
{
using namespace BathWaterOperationsTestSupport;

constexpr float ValueTolerance = 0.002f;
constexpr float PercentTolerance = 0.2f;

// One test world with two baths and a detail widget wired like the management screen: real delegate
// connections, plus a nested snapshot refresh on every operations change (the root widget does the same).
class FSliderFixture
{
public:
	explicit FSliderFixture(const TCHAR* WorldName)
		: Scope(WorldName)
	{
		World = Scope.Get();
		if (!World) return;
		Operations = World->GetSubsystem<UBathWaterOperationsSubsystem>();
		BathA = World->SpawnActor<ABathhouseBathFacilityActor>();
		BathB = World->SpawnActor<ABathhouseBathFacilityActor>();
		if (!Operations || !BathA || !BathB) return;
		Operations->RegisterBath(BathA->GetBathWaterCondition());
		Operations->RegisterBath(BathB->GetBathWaterCondition());
		Detail = FBathWaterSliderTestAccess::Create(GetTransientPackage());
		Detail->AddToRoot();
		FBathWaterSliderTestAccess::Construct(*Detail);
		Detail->SetOperationsContext(Operations);
		NestedRefresh = Operations->OnOperationsChanged.AddLambda([this]() { Refresh(); });
		Refresh();
	}

	~FSliderFixture()
	{
		if (Operations) Operations->OnOperationsChanged.Remove(NestedRefresh);
		if (Detail)
		{
			FBathWaterSliderTestAccess::Destruct(*Detail);
			Detail->RemoveFromRoot();
		}
	}

	bool IsValid() const { return World && Operations && BathA && BathB && Detail; }

	void Refresh()
	{
		FBathWaterBathSnapshot Snapshot;
		if (Operations->GetBathSnapshot(BathA, Snapshot))
		{
			Detail->ApplyBathSnapshot(&Snapshot);
		}
	}

	FBathWaterBathSnapshot SnapshotA() const
	{
		FBathWaterBathSnapshot Snapshot;
		Operations->GetBathSnapshot(BathA, Snapshot);
		return Snapshot;
	}

	FScopedBathWaterOperationsWorld Scope;
	UWorld* World = nullptr;
	UBathWaterOperationsSubsystem* Operations = nullptr;
	ABathhouseBathFacilityActor* BathA = nullptr;
	ABathhouseBathFacilityActor* BathB = nullptr;
	UBathWaterDetailWidget* Detail = nullptr;
	FDelegateHandle NestedRefresh;
};

float NormalizeTemperature(const float TemperatureC)
{
	const UBathWaterSettings* Settings = GetDefault<UBathWaterSettings>();
	const float Min = Settings->GetMinTargetTemperatureC();
	return (TemperatureC - Min) / (Settings->GetMaxTargetTemperatureC() - Min);
}

bool ContainsAll(const FString& Text, const TCHAR* A, const TCHAR* B = nullptr)
{
	return Text.Contains(A) && (!B || Text.Contains(B));
}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBathWaterSliderPointerDragTest,
	"BathhouseSim.BathWater.ManagementUI.SliderPointerDragHoldsCommittedValue",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FBathWaterSliderPointerDragTest::RunTest(const FString& Parameters)
{
	using namespace BathWaterSliderInputTest;
	(void)Parameters;
	if (!FSlateApplication::IsInitialized())
	{
		AddError(TEXT("FSlateApplication is not initialized; the pointer-drag path cannot be exercised."));
		return false;
	}
	FSliderFixture Fixture(TEXT("BathWaterSliderPointerWorld"));
	if (!TestTrue(TEXT("Slider fixture builds"), Fixture.IsValid())) return false;
	UBathWaterDetailWidget& Detail = *Fixture.Detail;
	UBathWaterConditionComponent* ConditionA = Fixture.BathA->GetBathWaterCondition();
	const float MaxDemand = ConditionA->GetMaxCirculationDemandPoints();
	if (!TestTrue(TEXT("Circulation demand scale is positive"), MaxDemand > KINDA_SMALL_NUMBER)) return false;

	// Installed circulation capacity covers only part of the circulation range, so the limit is below 100%.
	const float InstalledPoints = MaxDemand * 0.6f;
	UBathWaterUtilityCapacityComponent* Provider = AddProvider(
		*Fixture.World, TEXT("SliderDragCirculation"), EBathWaterCapacityKind::Circulation, InstalledPoints);
	TestTrue(TEXT("Circulation provider registers"), Fixture.Operations->RegisterProvider(Provider));
	const float LimitPercent = InstalledPoints / MaxDemand * 100.0f;
	const float LimitValue = LimitPercent * 0.01f;
	USlider* Slider = FBathWaterSliderTestAccess::Circulation(Detail);

	// Real Slate path: the same RoutePointer* calls with a virtual user that UWidgetInteractionComponent uses.
	const TSharedRef<SWidget> SliderWidget = Slider->TakeWidget();
	const FVector2D WindowSize(400.0f, 40.0f);
	const TSharedRef<SVirtualWindow> Window = SNew(SVirtualWindow).Size(WindowSize);
	Window->SetContent(SliderWidget);
	FSlateApplication& Slate = FSlateApplication::Get();
	Slate.RegisterVirtualWindow(Window);
	Window->SlatePrepass(1.0f);
	const TSharedRef<FSlateVirtualUserHandle> VirtualUser = Slate.FindOrCreateVirtualUser(13);
	const int32 UserIndex = VirtualUser->GetUserIndex();
	FVector2D LastPosition(0.0f, WindowSize.Y * 0.5f);
	bool bPointerIsDown = false;

	const auto MakeEvent = [&](const float Fraction, const bool bPressed)
	{
		const FVector2D Position(WindowSize.X * Fraction, WindowSize.Y * 0.5f);
		TSet<FKey> Pressed;
		if (bPressed) Pressed.Add(EKeys::LeftMouseButton);
		const FPointerEvent Event(static_cast<uint32>(UserIndex), 0u, Position, LastPosition, Pressed,
			EKeys::LeftMouseButton, 0.0f, FModifierKeysState());
		LastPosition = Position;
		return Event;
	};
	// FindPathToWidget reads each widget's cached paint geometry, which a headless test never paints.
	// Re-arranging the path from the prepassed window gives the real layout geometry.
	const auto Path = [&](FWidgetPath& OutPath)
	{
		FWidgetPath CachedPath;
		if (!Slate.FindPathToWidget(SliderWidget, CachedPath)) return false;
		OutPath = FWeakWidgetPath(CachedPath).ToWidgetPath(FWeakWidgetPath::EInterruptedPathHandling::ReturnInvalid);
		return OutPath.IsValid();
	};
	const auto Down = [&](const float Fraction)
	{
		FWidgetPath WidgetPath;
		if (!Path(WidgetPath)) return false;
		// UWidgetInteractionComponent hovers the widget every tick before it presses, so Slate has a hover path.
		Slate.RoutePointerMoveEvent(WidgetPath, MakeEvent(Fraction, false), false);
		Slate.RoutePointerDownEvent(WidgetPath, MakeEvent(Fraction, true));
		bPointerIsDown = true;
		return true;
	};
	const auto Move = [&](const float Fraction)
	{
		FWidgetPath WidgetPath;
		if (!Path(WidgetPath)) return false;
		Slate.RoutePointerMoveEvent(WidgetPath, MakeEvent(Fraction, true), false);
		return true;
	};
	const auto Up = [&](const float Fraction)
	{
		FWidgetPath WidgetPath;
		if (!Path(WidgetPath)) return false;
		Slate.RoutePointerUpEvent(WidgetPath, MakeEvent(Fraction, false));
		bPointerIsDown = false;
		return true;
	};
	const auto Cleanup = [&]()
	{
		if (bPointerIsDown) Up(1.0f);
		Slate.UnregisterVirtualWindow(Window);
	};

	{
		FWidgetPath Probe;
		if (!TestTrue(TEXT("The slider widget is found under the registered virtual window"), Path(Probe)))
		{
			Slate.UnregisterVirtualWindow(Window);
			return false;
		}
	}

	const auto DomainPercent = [&]() { return ConditionA->GetCirculationPercent(); };
	const auto SliderValue = [&]() { return Slider->GetValue(); };

	// SLD-005 setup and below-limit tracking: the handle follows the pointer and the domain matches it.
	Down(0.2f);
	TestTrue(TEXT("SLD-001: below the limit the press commits the slider position"),
		FMath::IsNearlyEqual(DomainPercent(), SliderValue() * 100.0f, PercentTolerance) && SliderValue() < LimitValue - 0.1f);
	TestTrue(TEXT("SLD-001: the slider captured the pointer"), SliderWidget->HasMouseCapture());
	Move(0.4f);
	TestTrue(TEXT("SLD-001: below the limit a move applies immediately"),
		FMath::IsNearlyEqual(DomainPercent(), SliderValue() * 100.0f, PercentTolerance) && SliderValue() < LimitValue - 0.05f);
	TestTrue(TEXT("SLD-001: no limit feedback below the limit"), FBathWaterSliderTestAccess::Feedback(Detail).IsEmpty());

	// First contact with the limit: domain changes, so the nested refresh path runs.
	Move(0.9f);
	TestTrue(TEXT("SLD-001: first overrun clamps the handle to the limit right after the event"),
		FMath::IsNearlyEqual(SliderValue(), LimitValue, ValueTolerance));
	TestTrue(TEXT("SLD-001: first overrun commits the limit"), FMath::IsNearlyEqual(DomainPercent(), LimitPercent, PercentTolerance));
	TestTrue(TEXT("SLD-001: the limit reason shows kind and shortage"),
		ContainsAll(FBathWaterSliderTestAccess::Feedback(Detail), TEXT("순환"), TEXT("부족")));

	// Already at the limit: the subsystem returns without a broadcast. This is the reported bug path.
	const uint64 RevisionAtLimit = Fixture.Operations->GetDataRevision();
	Move(1.0f);
	TestTrue(TEXT("SLD-001: continued dragging past the limit keeps the handle at the limit"),
		FMath::IsNearlyEqual(SliderValue(), LimitValue, ValueTolerance));
	TestTrue(TEXT("SLD-001: continued dragging keeps the committed value"),
		FMath::IsNearlyEqual(DomainPercent(), LimitPercent, PercentTolerance));
	TestEqual(TEXT("SLD-001: a no-op limited request does not publish"), Fixture.Operations->GetDataRevision(), RevisionAtLimit);
	TestTrue(TEXT("SLD-001: the limit reason stays visible"),
		ContainsAll(FBathWaterSliderTestAccess::Feedback(Detail), TEXT("순환"), TEXT("부족")));

	Up(1.0f);
	TestFalse(TEXT("SLD-002: releasing the pointer ends the capture"), SliderWidget->HasMouseCapture());
	TestTrue(TEXT("SLD-002: after release the handle sits at the committed value"),
		FMath::IsNearlyEqual(SliderValue(), DomainPercent() * 0.01f, ValueTolerance)
		&& FMath::IsNearlyEqual(SliderValue(), LimitValue, ValueTolerance));

	// Polling re-applies the same snapshot; it must not move the handle.
	Fixture.Refresh();
	TestTrue(TEXT("SLD-002: a polling refresh keeps the handle at the committed value"),
		FMath::IsNearlyEqual(SliderValue(), LimitValue, ValueTolerance));

	// SLD-005: dragging back below the limit is free and clears the reason. Bath B is untouched.
	const float BathBBefore = Fixture.BathB->GetBathWaterCondition()->GetCirculationPercent();
	Down(0.4f);
	Move(0.3f);
	TestTrue(TEXT("SLD-005: dragging back follows the pointer"),
		FMath::IsNearlyEqual(DomainPercent(), SliderValue() * 100.0f, PercentTolerance) && SliderValue() < LimitValue - 0.1f);
	TestTrue(TEXT("SLD-005: the limit reason clears"), FBathWaterSliderTestAccess::Feedback(Detail).IsEmpty());
	Up(0.3f);
	TestEqual(TEXT("SLD-005: another bath's circulation never changes"),
		Fixture.BathB->GetBathWaterCondition()->GetCirculationPercent(), BathBBefore);
	Cleanup();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBathWaterSliderCommittedSyncTest,
	"BathhouseSim.BathWater.ManagementUI.SliderCommittedSyncAndFeedback",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FBathWaterSliderCommittedSyncTest::RunTest(const FString& Parameters)
{
	using namespace BathWaterSliderInputTest;
	(void)Parameters;
	const UBathWaterSettings* Settings = GetDefault<UBathWaterSettings>();
	const float Ambient = Settings->GetAmbientTemperatureC();
	const float MinTemperature = Settings->GetMinTargetTemperatureC();
	const float MaxTemperature = Settings->GetMaxTargetTemperatureC();

	// SLD-001/002 circulation, SLD-003 heating and cooling, polling safety net, one world.
	{
		FSliderFixture Fixture(TEXT("BathWaterSliderSyncWorld"));
		if (!TestTrue(TEXT("Slider fixture builds"), Fixture.IsValid())) return false;
		UBathWaterDetailWidget& Detail = *Fixture.Detail;
		UBathWaterConditionComponent* Condition = Fixture.BathA->GetBathWaterCondition();
		USlider* CirculationSlider = FBathWaterSliderTestAccess::Circulation(Detail);
		USlider* TargetSlider = FBathWaterSliderTestAccess::Target(Detail);
		const float MaxDemand = Condition->GetMaxCirculationDemandPoints();
		const float HeatPerC = Condition->GetHeatingDemandPointsPerC();
		const float CoolPerC = Condition->GetCoolingDemandPointsPerC();
		if (!TestTrue(TEXT("Demand scales are positive"),
			MaxDemand > KINDA_SMALL_NUMBER && HeatPerC > KINDA_SMALL_NUMBER && CoolPerC > KINDA_SMALL_NUMBER)) return false;

		const float CirculationInstalled = MaxDemand * 0.6f;
		const float CirculationLimitPercent = CirculationInstalled / MaxDemand * 100.0f;
		// Headroom is a whole number of TargetTemperatureStepC units that fits both sides of the target range;
		// the installed capacity adds less than one step so quantization keeps the headroom as the limit.
		const float StepC = Settings->GetTargetTemperatureStepC();
		const float SideRangeC = FMath::Min(MaxTemperature - Ambient, Ambient - MinTemperature);
		const int32 HeadroomSteps = StepC > KINDA_SMALL_NUMBER ? FMath::FloorToInt(SideRangeC / StepC * 0.5f) : 0;
		if (!TestTrue(TEXT("The target range leaves at least one step of headroom on both sides"), HeadroomSteps >= 1)) return false;
		const float HeadroomC = StepC * HeadroomSteps;
		const float HeatingHeadroomC = HeadroomC;
		const float CoolingHeadroomC = HeadroomC;
		const float HeatingInstalled = HeatPerC * (HeatingHeadroomC + StepC * 0.5f);
		const float CoolingInstalled = CoolPerC * (CoolingHeadroomC + StepC * 0.5f);
		if (!TestTrue(TEXT("Heating and cooling fixtures stay inside the target range"),
			Ambient + HeatingHeadroomC < MaxTemperature && Ambient - CoolingHeadroomC > MinTemperature)) return false;
		TestTrue(TEXT("Circulation provider registers"), Fixture.Operations->RegisterProvider(AddProvider(
			*Fixture.World, TEXT("SyncCirculation"), EBathWaterCapacityKind::Circulation, CirculationInstalled)));
		TestTrue(TEXT("Heating provider registers"), Fixture.Operations->RegisterProvider(AddProvider(
			*Fixture.World, TEXT("SyncHeating"), EBathWaterCapacityKind::Heating, HeatingInstalled)));
		TestTrue(TEXT("Cooling provider registers"), Fixture.Operations->RegisterProvider(AddProvider(
			*Fixture.World, TEXT("SyncCooling"), EBathWaterCapacityKind::Cooling, CoolingInstalled)));

		// SLD-001/002: SetValue goes through the same OnValueChanged path as a drag.
		const uint64 RevisionBefore = Fixture.Operations->GetDataRevision();
		CirculationSlider->SetValue(0.9f);
		TestTrue(TEXT("SLD-001: SetValue past the limit clamps the handle immediately"),
			FMath::IsNearlyEqual(CirculationSlider->GetValue(), CirculationLimitPercent * 0.01f, ValueTolerance));
		TestTrue(TEXT("SLD-001: SetValue past the limit commits the limit"),
			FMath::IsNearlyEqual(Condition->GetCirculationPercent(), CirculationLimitPercent, PercentTolerance));
		TestTrue(TEXT("SLD-001: limit feedback shows the circulation shortage"),
			ContainsAll(FBathWaterSliderTestAccess::Feedback(Detail), TEXT("순환"), TEXT("부족")));
		TestEqual(TEXT("SLD-001: the correction does not issue another request"),
			Fixture.Operations->GetDataRevision(), RevisionBefore + 1);
		CirculationSlider->SetValue(1.0f);
		TestTrue(TEXT("SLD-002: a second overrun keeps the handle at the committed value"),
			FMath::IsNearlyEqual(CirculationSlider->GetValue(), CirculationLimitPercent * 0.01f, ValueTolerance)
			&& FMath::IsNearlyEqual(Condition->GetCirculationPercent(), CirculationLimitPercent, PercentTolerance));
		TestTrue(TEXT("SLD-002: a second overrun keeps the limit feedback"),
			ContainsAll(FBathWaterSliderTestAccess::Feedback(Detail), TEXT("순환"), TEXT("부족")));
		TestEqual(TEXT("SLD-002: a second overrun is a no-op for the ledger"),
			Fixture.Operations->GetDataRevision(), RevisionBefore + 1);

		// SLD-003 heating: slider position above the installed heating headroom.
		TargetSlider->SetValue(1.0f);
		const float HeatingLimitC = Ambient + HeatingHeadroomC;
		TestTrue(TEXT("SLD-003: heating overrun clamps the handle to the committed temperature"),
			FMath::IsNearlyEqual(TargetSlider->GetValue(), NormalizeTemperature(HeatingLimitC), ValueTolerance));
		TestTrue(TEXT("SLD-003: heating overrun commits the limit"),
			FMath::IsNearlyEqual(Condition->GetTargetTemperatureC(), HeatingLimitC, 0.01f));
		TestTrue(TEXT("SLD-003: heating feedback names the kind"),
			FBathWaterSliderTestAccess::Feedback(Detail).Contains(TEXT("가열")));
		TargetSlider->SetValue(1.0f);
		TestTrue(TEXT("SLD-003: repeating the heating overrun keeps handle, value and feedback"),
			FMath::IsNearlyEqual(TargetSlider->GetValue(), NormalizeTemperature(HeatingLimitC), ValueTolerance)
			&& FMath::IsNearlyEqual(Condition->GetTargetTemperatureC(), HeatingLimitC, 0.01f)
			&& FBathWaterSliderTestAccess::Feedback(Detail).Contains(TEXT("가열")));

		// SLD-003 cooling, starting again from ambient.
		Fixture.Operations->RequestTargetTemperature(Fixture.BathA, Ambient);
		TestTrue(TEXT("SLD-003: the handle follows an external target change"),
			FMath::IsNearlyEqual(TargetSlider->GetValue(), NormalizeTemperature(Ambient), ValueTolerance));
		TargetSlider->SetValue(0.0f);
		const float CoolingLimitC = Ambient - CoolingHeadroomC;
		TestTrue(TEXT("SLD-003: cooling overrun clamps the handle to the committed temperature"),
			FMath::IsNearlyEqual(TargetSlider->GetValue(), NormalizeTemperature(CoolingLimitC), ValueTolerance));
		TestTrue(TEXT("SLD-003: cooling overrun commits the limit in TargetTemperatureStepC units"),
			FMath::IsNearlyEqual(Condition->GetTargetTemperatureC(), CoolingLimitC, 0.01f));
		TestTrue(TEXT("SLD-003: cooling feedback names the kind"),
			FBathWaterSliderTestAccess::Feedback(Detail).Contains(TEXT("냉각")));
		TargetSlider->SetValue(0.0f);
		TestTrue(TEXT("SLD-003: repeating the cooling overrun keeps handle, value and feedback"),
			FMath::IsNearlyEqual(TargetSlider->GetValue(), NormalizeTemperature(CoolingLimitC), ValueTolerance)
			&& FMath::IsNearlyEqual(Condition->GetTargetTemperatureC(), CoolingLimitC, 0.01f)
			&& FBathWaterSliderTestAccess::Feedback(Detail).Contains(TEXT("냉각")));

		// Polling safety net: an out-of-sync handle is repaired by a same-snapshot refresh without presentation writes.
		const FBathWaterBathSnapshot Snapshot = Fixture.SnapshotA();
		Detail.ApplyBathSnapshot(&Snapshot);
		const int32 WritesBefore = FBathWaterSliderTestAccess::PresentationWrites(Detail);
		const float DriftedValue = FMath::IsNearlyEqual(TargetSlider->GetValue(), 0.77f, 0.05f) ? 0.33f : 0.77f;
		FBathWaterSliderTestAccess::WriteWithoutGuard(Detail, TargetSlider, DriftedValue);
		TestTrue(TEXT("Polling fixture drifted the handle"), FMath::IsNearlyEqual(TargetSlider->GetValue(), DriftedValue, 0.001f));
		Detail.ApplyBathSnapshot(&Snapshot);
		TestTrue(TEXT("Polling: a same-snapshot refresh restores the committed handle"),
			FMath::IsNearlyEqual(TargetSlider->GetValue(), NormalizeTemperature(CoolingLimitC), ValueTolerance));
		TestEqual(TEXT("Polling: the repair performs no presentation writes"),
			FBathWaterSliderTestAccess::PresentationWrites(Detail), WritesBefore);

		// Failure path: the bath leaves the ledger while the widget still selects it.
		CirculationSlider->SetValue(0.3f);
		const float StableValue = CirculationSlider->GetValue();
		Fixture.Operations->UnregisterBath(Condition);
		CirculationSlider->SetValue(0.55f);
		TestEqual(TEXT("A failed request reports the unavailable setting"),
			FBathWaterSliderTestAccess::Feedback(Detail), FString(TEXT("설정을 변경할 수 없습니다")));
		TestTrue(TEXT("A failed request returns the handle to the last known value"),
			FMath::IsNearlyEqual(CirculationSlider->GetValue(), StableValue, ValueTolerance));
	}

	// SLD-004: installed capacity is enough but nothing is operating, so nothing limits the request.
	{
		FSliderFixture Fixture(TEXT("BathWaterSliderDeficitWorld"));
		if (!TestTrue(TEXT("Deficit slider fixture builds"), Fixture.IsValid())) return false;
		UBathWaterDetailWidget& Detail = *Fixture.Detail;
		UBathWaterConditionComponent* Condition = Fixture.BathA->GetBathWaterCondition();
		USlider* CirculationSlider = FBathWaterSliderTestAccess::Circulation(Detail);
		const float MaxDemand = Condition->GetMaxCirculationDemandPoints();
		AActor* Owner = Fixture.World->SpawnActor<AActor>();
		// The operation exists but its clock never starts, so the provider is installed but not operating.
		UUtilityOperationComponent* IdleOperation = NewObject<UUtilityOperationComponent>(Owner, TEXT("IdleOperation"));
		IdleOperation->RegisterComponent();
		UBathWaterUtilityCapacityComponent* Idle = NewObject<UBathWaterUtilityCapacityComponent>(Owner, TEXT("IdleCirculation"));
		Idle->RestoreCapacity(EBathWaterCapacityKind::Circulation, MaxDemand);
		Idle->SetUtilityOperation(IdleOperation);
		Idle->RegisterComponent();
		TestTrue(TEXT("Installed-only provider registers"), Fixture.Operations->RegisterProvider(Idle));
		CirculationSlider->SetValue(0.8f);
		TestTrue(TEXT("SLD-004: an operating shortage does not limit the handle"),
			FMath::IsNearlyEqual(CirculationSlider->GetValue(), 0.8f, ValueTolerance));
		TestTrue(TEXT("SLD-004: the setting is preserved"),
			FMath::IsNearlyEqual(Condition->GetCirculationPercent(), 80.0f, PercentTolerance));
		TestFalse(TEXT("SLD-004: no installed-capacity limit feedback"),
			FBathWaterSliderTestAccess::Feedback(Detail).Contains(TEXT("용량 제한")));
		TestTrue(TEXT("SLD-004: the snapshot reports the circulation deficit"),
			Fixture.SnapshotA().bCirculationCapacityDeficit);
	}
	return true;
}

#endif
