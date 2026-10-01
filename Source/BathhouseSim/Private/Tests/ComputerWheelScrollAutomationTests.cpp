#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Blueprint/UserWidget.h"
#include "Blueprint/WidgetBlueprintGeneratedClass.h"
#include "Blueprint/WidgetTree.h"
#include "Camera/CameraComponent.h"
#include "Character/FirstPersonCharacter.h"
#include "Components/Button.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/ScrollBox.h"
#include "Components/SizeBox.h"
#include "Components/Slider.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/WidgetComponent.h"
#include "Components/WidgetInteractionComponent.h"
#include "Computer/BathhouseComputerActor.h"
#include "ComputerAutomationTestSupport.h"
#include "ComputerWheelScrollTestProbe.h"
#include "EnhancedActionKeyMapping.h"
#include "Engine/Engine.h"
#include "Engine/LocalPlayer.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Framework/Application/SlateApplication.h"
#include "GameFramework/PlayerController.h"
#include "HAL/IConsoleManager.h"
#include "InputAction.h"
#include "InputActionValue.h"
#include "InputMappingContext.h"
#include "Interaction/PlayerInteractionComponent.h"
#include "Framework/Application/SlateUser.h"
#include "Input/Events.h"
#include "Layout/WidgetPath.h"
#include "Placement/FacilityPlacementSettings.h"
#include "Placement/FacilityPlacementZoneActor.h"
#include "Placement/PlaceableFacilityItemActor.h"
#include "Placement/PlayerFacilityPlacementComponent.h"
#include "UI/ComputerSampleScreenWidget.h"
#include "UObject/StrongObjectPtr.h"
#include "UObject/UnrealType.h"
#include "Widgets/SVirtualWindow.h"
#include "Computer/PlayerComputerUseComponent.h"

namespace ComputerWheelScrollTest
{
// Virtual user index private to this file. Other computer tests use different indices.
constexpr int32 VirtualUserIndex = 14;

template <typename T>
T* MakeWidget()
{
	return NewObject<T>(GetTransientPackage());
}

USizeBox* AddSized(UPanelWidget* Parent, UWidget* Content, const float Height)
{
	USizeBox* Box = MakeWidget<USizeBox>();
	Box->SetHeightOverride(Height);
	if (Content)
	{
		Box->AddChild(Content);
	}
	Parent->AddChild(Box);
	return Box;
}

void AddSizedItems(UPanelWidget* Parent, const int32 Count, const float Height)
{
	for (int32 Index = 0; Index < Count; ++Index)
	{
		AddSized(Parent, nullptr, Height);
	}
}

struct FOffsets
{
	float Product = 0.0f;
	float Cart = 0.0f;
	float Order = 0.0f;
	float Detail = 0.0f;
	float Inner = 0.0f;
};

// UMG fixture that mirrors the computer screen: Product list, Cart/Order siblings, Detail with a slider
// and a nested inner scroll, plus a button outside every scroll area.
struct FScrollFixture
{
	TStrongObjectPtr<UHorizontalBox> Root;
	TStrongObjectPtr<UBathhouseComputerWheelProbe> ProductProbe;
	TStrongObjectPtr<UBathhouseComputerWheelProbe> OutsideProbe;
	TStrongObjectPtr<UBathhouseComputerWheelProbe> SliderProbe;
	UScrollBox* Product = nullptr;
	UScrollBox* Cart = nullptr;
	UScrollBox* Order = nullptr;
	UScrollBox* Detail = nullptr;
	UScrollBox* Inner = nullptr;
	UVerticalBox* CartList = nullptr;
	USizeBox* ProductItem1 = nullptr;
	USizeBox* CartItem0 = nullptr;
	USizeBox* OrderItem0 = nullptr;
	USizeBox* InnerItem0 = nullptr;
	UButton* ProductButton = nullptr;
	UButton* OutsideButton = nullptr;
	UTextBlock* DetailLabel = nullptr;
	USlider* Slider = nullptr;

	void Build(const int32 ProductItemCount, const int32 OrderItemCount, const int32 DetailFillerCount)
	{
		ProductProbe.Reset(NewObject<UBathhouseComputerWheelProbe>());
		OutsideProbe.Reset(NewObject<UBathhouseComputerWheelProbe>());
		SliderProbe.Reset(NewObject<UBathhouseComputerWheelProbe>());
		Root.Reset(MakeWidget<UHorizontalBox>());

		Product = MakeWidget<UScrollBox>();
		UVerticalBox* ProductList = MakeWidget<UVerticalBox>();
		Product->AddChild(ProductList);
		ProductButton = MakeWidget<UButton>();
		ProductButton->OnClicked.AddDynamic(ProductProbe.Get(), &UBathhouseComputerWheelProbe::HandleClicked);
		AddSized(ProductList, ProductButton, 100.0f);
		ProductItem1 = AddSized(ProductList, nullptr, 100.0f);
		AddSizedItems(ProductList, ProductItemCount - 2, 100.0f);
		SetFill(Root->AddChild(Product), 1.0f);

		UVerticalBox* Column = MakeWidget<UVerticalBox>();
		SetFill(Root->AddChild(Column), 1.0f);
		Cart = MakeWidget<UScrollBox>();
		CartList = MakeWidget<UVerticalBox>();
		Cart->AddChild(CartList);
		CartItem0 = AddSized(CartList, nullptr, 50.0f);
		Cast<UVerticalBoxSlot>(Column->AddChild(Cart))->SetSize(FillSize(0.55f));
		Order = MakeWidget<UScrollBox>();
		UVerticalBox* OrderList = MakeWidget<UVerticalBox>();
		Order->AddChild(OrderList);
		OrderItem0 = AddSized(OrderList, nullptr, 100.0f);
		AddSizedItems(OrderList, OrderItemCount - 1, 100.0f);
		Cast<UVerticalBoxSlot>(Column->AddChild(Order))->SetSize(FillSize(0.45f));

		Detail = MakeWidget<UScrollBox>();
		UVerticalBox* DetailList = MakeWidget<UVerticalBox>();
		Detail->AddChild(DetailList);
		DetailLabel = MakeWidget<UTextBlock>();
		DetailLabel->SetText(FText::FromString(TEXT("Detail")));
		AddSized(DetailList, DetailLabel, 100.0f);
		Slider = MakeWidget<USlider>();
		Slider->SetValue(0.3f);
		Slider->OnValueChanged.AddDynamic(SliderProbe.Get(), &UBathhouseComputerWheelProbe::HandleSliderChanged);
		AddSized(DetailList, Slider, 40.0f);
		Inner = MakeWidget<UScrollBox>();
		UVerticalBox* InnerList = MakeWidget<UVerticalBox>();
		Inner->AddChild(InnerList);
		InnerItem0 = AddSized(InnerList, nullptr, 100.0f);
		AddSizedItems(InnerList, 2, 100.0f);
		AddSized(DetailList, Inner, 100.0f);
		AddSizedItems(DetailList, DetailFillerCount, 100.0f);
		SetFill(Root->AddChild(Detail), 1.0f);

		OutsideButton = MakeWidget<UButton>();
		OutsideButton->OnClicked.AddDynamic(OutsideProbe.Get(), &UBathhouseComputerWheelProbe::HandleClicked);
		USizeBox* OutsideBox = MakeWidget<USizeBox>();
		OutsideBox->SetWidthOverride(100.0f);
		OutsideBox->AddChild(OutsideButton);
		Root->AddChild(OutsideBox);
	}

	static FSlateChildSize FillSize(const float Value)
	{
		FSlateChildSize Size(ESlateSizeRule::Fill);
		Size.Value = Value;
		return Size;
	}

	static void SetFill(UPanelSlot* Slot, const float Value)
	{
		Cast<UHorizontalBoxSlot>(Slot)->SetSize(FillSize(Value));
	}

	FOffsets Snapshot() const
	{
		FOffsets Result;
		Result.Product = Product->GetScrollOffset();
		Result.Cart = Cart->GetScrollOffset();
		Result.Order = Order->GetScrollOffset();
		Result.Detail = Detail->GetScrollOffset();
		Result.Inner = Inner->GetScrollOffset();
		return Result;
	}
};

// Real Slate state for the fixture: a registered virtual window, a virtual user and the engine's own
// SScrollBox::Tick, which is the only place that decides whether a scroll box can scroll.
class FSlateHarness
{
public:
	FSlateHarness(FScrollFixture& InFixture)
		: Fixture(InFixture)
	{
		if (!FSlateApplication::IsInitialized())
		{
			return;
		}
		Window = SNew(SVirtualWindow).Size(FVector2D(1024.0, 576.0));
		Window->SetContent(Fixture.Root->TakeWidget());
		FSlateApplication::Get().RegisterVirtualWindow(Window.ToSharedRef());
		VirtualUser = FSlateApplication::Get().FindOrCreateVirtualUser(VirtualUserIndex);
		Refresh();
	}

	~FSlateHarness()
	{
		if (Window.IsValid() && FSlateApplication::IsInitialized())
		{
			FSlateApplication::Get().UnregisterVirtualWindow(Window.ToSharedRef());
		}
		VirtualUser.Reset();
	}

	bool IsReady() const { return Window.IsValid() && VirtualUser.IsValid(); }

	// Prepass and the engine Tick for every scroll box, outer boxes first.
	void Refresh() const
	{
		Window->SlatePrepass(1.0f);
		UScrollBox* const Boxes[] = { Fixture.Product, Fixture.Cart, Fixture.Order, Fixture.Detail, Fixture.Inner };
		for (UScrollBox* Box : Boxes)
		{
			const TSharedPtr<SWidget> Widget = Box->GetCachedWidget();
			FWidgetPath Path;
			if (Widget.IsValid()
				&& FSlateApplication::Get().FindPathToWidget(Widget.ToSharedRef(), Path, EVisibility::All)
				&& Path.Widgets.Num() > 0)
			{
				Widget->Tick(Path.Widgets.Last().Geometry, FSlateApplication::Get().GetCurrentTime(), 0.0f);
			}
		}
	}

	// Same event as UWidgetInteractionComponent::ScrollWheel, bubbled over the path to Target.
	bool RouteWheel(FAutomationTestBase& Test, const UWidget* Target, const float Delta) const
	{
		const TSharedPtr<SWidget> Widget = Target ? Target->GetCachedWidget() : nullptr;
		FWidgetPath Path;
		if (!Widget.IsValid()
			|| !FSlateApplication::Get().FindPathToWidget(Widget.ToSharedRef(), Path, EVisibility::All)
			|| Path.Widgets.Num() == 0)
		{
			Test.AddError(TEXT("The wheel target has no Slate path in the virtual window."));
			return false;
		}
		const FVector2f Center = Path.Widgets.Last().Geometry.GetAbsolutePositionAtCoordinates(FVector2f(0.5f, 0.5f));
		const FPointerEvent WheelEvent(
			VirtualUser->GetUserIndex(),
			0,
			Center,
			Center,
			TSet<FKey>(),
			EKeys::MouseWheelAxis,
			Delta,
			FModifierKeysState());
		// Bubble routing reads a virtual pointer position per path element, so build the path the way the engine does.
		TArray<FWidgetAndPointer> WidgetsAndPointers;
		for (int32 Index = 0; Index < Path.Widgets.Num(); ++Index)
		{
			WidgetsAndPointers.Add(FWidgetAndPointer(Path.Widgets[Index], TOptional<FVirtualPointerPosition>()));
		}
		const FWidgetPath PointerPath(MakeArrayView(WidgetsAndPointers));
		const bool bHandled = FSlateApplication::Get().RouteMouseWheelOrGestureEvent(PointerPath, WheelEvent, nullptr).IsEventHandled();
		Refresh();
		return bHandled;
	}

private:
	FScrollFixture& Fixture;
	TSharedPtr<SVirtualWindow> Window;
	TSharedPtr<FSlateVirtualUserHandle> VirtualUser;
};

bool NearlyEqual(const float A, const float B)
{
	return FMath::IsNearlyEqual(A, B, 0.01f);
}

void ExpectOffsets(FAutomationTestBase& Test, const TCHAR* What, const FOffsets& Actual, const FOffsets& Expected)
{
	Test.TestTrue(*FString::Printf(TEXT("%s: Product offset %.2f expected %.2f"), What, Actual.Product, Expected.Product),
		NearlyEqual(Actual.Product, Expected.Product));
	Test.TestTrue(*FString::Printf(TEXT("%s: Cart offset %.2f expected %.2f"), What, Actual.Cart, Expected.Cart),
		NearlyEqual(Actual.Cart, Expected.Cart));
	Test.TestTrue(*FString::Printf(TEXT("%s: Order offset %.2f expected %.2f"), What, Actual.Order, Expected.Order),
		NearlyEqual(Actual.Order, Expected.Order));
	Test.TestTrue(*FString::Printf(TEXT("%s: Detail offset %.2f expected %.2f"), What, Actual.Detail, Expected.Detail),
		NearlyEqual(Actual.Detail, Expected.Detail));
	Test.TestTrue(*FString::Printf(TEXT("%s: Inner offset %.2f expected %.2f"), What, Actual.Inner, Expected.Inner),
		NearlyEqual(Actual.Inner, Expected.Inner));
}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBathhouseComputerWheelSlateTest,
	"BathhouseSim.Computer.Input.WheelScrollsHoveredScrollBoxThroughSlate",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FBathhouseComputerWheelSlateTest::RunTest(const FString& Parameters)
{
	using namespace ComputerWheelScrollTest;
	(void)Parameters;
	if (!FSlateApplication::IsInitialized())
	{
		AddError(TEXT("FSlateApplication is not initialized; the real Slate wheel path cannot be verified."));
		return false;
	}

	FScrollFixture Fixture;
	Fixture.Build(20, 10, 15);
	FSlateHarness Harness(Fixture);
	if (!TestTrue(TEXT("The Slate harness (virtual window and virtual user) is ready"), Harness.IsReady()))
	{
		return false;
	}

	const FOffsets Zero;
	IConsoleVariable* ScrollAmountCvar = IConsoleManager::Get().FindConsoleVariable(TEXT("Slate.GlobalScrollAmount"));
	if (!TestNotNull(TEXT("Slate.GlobalScrollAmount exists"), ScrollAmountCvar))
	{
		return false;
	}
	// Engine default contract from the design (4.6): the per-notch distance is this cvar times the box multiplier.
	TestEqual(TEXT("Slate.GlobalScrollAmount keeps the engine default the design relies on"),
		ScrollAmountCvar->GetFloat(), 32.0f);
	const auto StepOf = [ScrollAmountCvar](const UScrollBox* Box)
	{
		return ScrollAmountCvar->GetFloat() * Box->GetWheelScrollMultiplier();
	};

	TestTrue(TEXT("Setup: ProductScroll can scroll"), Fixture.Product->GetScrollOffsetOfEnd() > 0.0f);
	TestTrue(TEXT("Setup: OrderScroll can scroll"), Fixture.Order->GetScrollOffsetOfEnd() > 0.0f);
	TestTrue(TEXT("Setup: DetailScroll can scroll"), Fixture.Detail->GetScrollOffsetOfEnd() > 0.0f);
	TestTrue(TEXT("Setup: InnerScroll can scroll"), Fixture.Inner->GetScrollOffsetOfEnd() > 0.0f);
	TestTrue(TEXT("Setup: CartScroll fits its content"), FMath::IsNearlyZero(Fixture.Cart->GetScrollOffsetOfEnd()));
	if (!FMath::IsNearlyZero(Fixture.Cart->GetScrollOffsetOfEnd()) || Fixture.Product->GetScrollOffsetOfEnd() <= 0.0f)
	{
		return false;
	}

	TSharedPtr<SWidget> FocusBefore = FSlateApplication::Get().GetUserFocusedWidget(0);

	// CWS-001: one notch down on the second product item.
	const float ProductStep = StepOf(Fixture.Product);
	FOffsets Expected = Zero;
	TestTrue(TEXT("CWS-001: a wheel notch over a product item is handled"),
		Harness.RouteWheel(*this, Fixture.ProductItem1, -1.0f));
	Expected.Product = ProductStep;
	ExpectOffsets(*this, TEXT("CWS-001"), Fixture.Snapshot(), Expected);

	// Proportional: several notches and a fractional trackpad amount.
	Harness.RouteWheel(*this, Fixture.ProductItem1, -3.0f);
	Expected.Product += 3.0f * ProductStep;
	ExpectOffsets(*this, TEXT("CWS-001 proportional: -3"), Fixture.Snapshot(), Expected);
	Harness.RouteWheel(*this, Fixture.ProductItem1, -0.25f);
	Expected.Product += 0.25f * ProductStep;
	ExpectOffsets(*this, TEXT("CWS-001 proportional: -0.25"), Fixture.Snapshot(), Expected);

	// CWS-002: opposite direction.
	TestTrue(TEXT("CWS-002: wheel up is handled"), Harness.RouteWheel(*this, Fixture.ProductItem1, 1.0f));
	Expected.Product -= ProductStep;
	ExpectOffsets(*this, TEXT("CWS-002"), Fixture.Snapshot(), Expected);

	// CWS-004: wheel over the button scrolls and never clicks.
	const FOffsets BeforeButton = Fixture.Snapshot();
	TestTrue(TEXT("CWS-004: wheel over the button is handled by the scroll box"),
		Harness.RouteWheel(*this, Fixture.ProductButton, -1.0f));
	Expected = BeforeButton;
	Expected.Product += ProductStep;
	ExpectOffsets(*this, TEXT("CWS-004"), Fixture.Snapshot(), Expected);
	TestEqual(TEXT("CWS-004: the wheel does not click the button"), Fixture.ProductProbe->ClickCount, 0);

	// CWS-005: no overscroll at either end, opposite direction is immediate.
	Harness.RouteWheel(*this, Fixture.ProductItem1, -1000.0f);
	const float ProductEnd = Fixture.Product->GetScrollOffsetOfEnd();
	TestTrue(TEXT("CWS-005: a large wheel amount stops exactly at the end"),
		NearlyEqual(Fixture.Product->GetScrollOffset(), ProductEnd));
	TestFalse(TEXT("CWS-005: wheel at the end is unhandled"), Harness.RouteWheel(*this, Fixture.ProductItem1, -1.0f));
	TestTrue(TEXT("CWS-005: wheel at the end does not move or overscroll"),
		NearlyEqual(Fixture.Product->GetScrollOffset(), ProductEnd));
	TestTrue(TEXT("CWS-005: the opposite notch handles immediately"), Harness.RouteWheel(*this, Fixture.ProductItem1, 1.0f));
	TestTrue(TEXT("CWS-005: the opposite notch moves one step from the end"),
		NearlyEqual(Fixture.Product->GetScrollOffset(), ProductEnd - ProductStep));
	Harness.RouteWheel(*this, Fixture.ProductItem1, 1000.0f);
	TestFalse(TEXT("CWS-005: wheel at the top is unhandled"), Harness.RouteWheel(*this, Fixture.ProductItem1, 1.0f));
	TestTrue(TEXT("CWS-005: wheel at the top keeps zero"), NearlyEqual(Fixture.Product->GetScrollOffset(), 0.0f));

	// CWS-006: a scroll box that fits its content ignores the wheel.
	FOffsets Before = Fixture.Snapshot();
	TestFalse(TEXT("CWS-006: a non-scrollable Cart leaves the wheel unhandled"),
		Harness.RouteWheel(*this, Fixture.CartItem0, -1.0f));
	ExpectOffsets(*this, TEXT("CWS-006"), Fixture.Snapshot(), Before);

	// CWS-008: sibling Order moves alone.
	Before = Fixture.Snapshot();
	TestTrue(TEXT("CWS-008: Order wheel is handled"), Harness.RouteWheel(*this, Fixture.OrderItem0, -1.0f));
	Expected = Before;
	Expected.Order += StepOf(Fixture.Order);
	ExpectOffsets(*this, TEXT("CWS-008"), Fixture.Snapshot(), Expected);

	// CWS-007: once Cart overflows it moves alone.
	AddSizedItems(Fixture.CartList, 10, 100.0f);
	Harness.Refresh();
	TestTrue(TEXT("CWS-007 setup: Cart now overflows"), Fixture.Cart->GetScrollOffsetOfEnd() > 0.0f);
	Before = Fixture.Snapshot();
	TestTrue(TEXT("CWS-007: Cart wheel is handled"), Harness.RouteWheel(*this, Fixture.CartItem0, -1.0f));
	Expected = Before;
	Expected.Cart += StepOf(Fixture.Cart);
	ExpectOffsets(*this, TEXT("CWS-007"), Fixture.Snapshot(), Expected);

	// CWS-009 / CWS-010: Detail text and slider both bubble to Detail; the slider value is never written.
	Before = Fixture.Snapshot();
	TestTrue(TEXT("CWS-009: Detail text wheel is handled"), Harness.RouteWheel(*this, Fixture.DetailLabel, -1.0f));
	Expected = Before;
	Expected.Detail += StepOf(Fixture.Detail);
	ExpectOffsets(*this, TEXT("CWS-009"), Fixture.Snapshot(), Expected);
	const float SliderBefore = Fixture.Slider->GetValue();
	Before = Fixture.Snapshot();
	TestTrue(TEXT("CWS-010: slider wheel bubbles to Detail"), Harness.RouteWheel(*this, Fixture.Slider, -1.0f));
	Expected = Before;
	Expected.Detail += StepOf(Fixture.Detail);
	ExpectOffsets(*this, TEXT("CWS-010"), Fixture.Snapshot(), Expected);
	TestEqual(TEXT("CWS-010: the wheel does not change the slider value"), Fixture.Slider->GetValue(), SliderBefore);
	TestEqual(TEXT("CWS-010: the wheel does not raise OnValueChanged"), Fixture.SliderProbe->SliderChangeCount, 0);

	// CWS-011: outside every scroll area nothing handles the wheel.
	Before = Fixture.Snapshot();
	TestFalse(TEXT("CWS-011: a button outside every scroll box leaves the wheel unhandled"),
		Harness.RouteWheel(*this, Fixture.OutsideButton, -1.0f));
	ExpectOffsets(*this, TEXT("CWS-011"), Fixture.Snapshot(), Before);
	TestEqual(TEXT("CWS-011: the wheel does not click the outside button"), Fixture.OutsideProbe->ClickCount, 0);

	// CWS-019 (P4): inner first, outer only at the inner end, never both in one notch.
	Before = Fixture.Snapshot();
	TestTrue(TEXT("CWS-019: inner scroll handles first"), Harness.RouteWheel(*this, Fixture.InnerItem0, -1.0f));
	Expected = Before;
	Expected.Inner += StepOf(Fixture.Inner);
	ExpectOffsets(*this, TEXT("CWS-019 inner first"), Fixture.Snapshot(), Expected);
	Harness.RouteWheel(*this, Fixture.InnerItem0, -1000.0f);
	Before = Fixture.Snapshot();
	TestTrue(TEXT("CWS-019: the inner end hands the wheel to the outer scroll"),
		Harness.RouteWheel(*this, Fixture.InnerItem0, -1.0f));
	Expected = Before;
	Expected.Detail += StepOf(Fixture.Detail);
	ExpectOffsets(*this, TEXT("CWS-019 outer at inner end"), Fixture.Snapshot(), Expected);

	// CWS-016 support: the wheel path never moves real user focus.
	TestTrue(TEXT("CWS-016: the real user focus is unchanged by wheel routing"),
		FSlateApplication::Get().GetUserFocusedWidget(0) == FocusBefore);
	return true;
}

namespace ComputerWheelScrollTest
{
bool ReadMouseWheelKeyMappingCount(const UInputMappingContext& Context, int32& OutMappingIndex)
{
	int32 Count = 0;
	OutMappingIndex = INDEX_NONE;
	const TArray<FEnhancedActionKeyMapping>& Mappings = Context.GetMappings();
	for (int32 Index = 0; Index < Mappings.Num(); ++Index)
	{
		const FKey Key = Mappings[Index].Key;
		if (Key == EKeys::MouseWheelAxis || Key == EKeys::MouseScrollUp || Key == EKeys::MouseScrollDown)
		{
			++Count;
			OutMappingIndex = Index;
		}
	}
	return Count == 1;
}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBathhouseComputerWheelRoutingTest,
	"BathhouseSim.Computer.Input.WheelRoutesByComputerPhase",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FBathhouseComputerWheelRoutingTest::RunTest(const FString& Parameters)
{
	using namespace ComputerAutomationTestSupport;
	(void)Parameters;
	FScopedComputerAutomationWorld TestWorld(TEXT("ComputerWheelRoutingWorld"));
	UWorld* World = TestWorld.Get();
	if (!TestNotNull(TEXT("Wheel routing automation world exists"), World)) return false;

	UStaticMesh* CubeMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
	TestNotNull(TEXT("The engine cube mesh is available"), CubeMesh);
	APlayerController* PlayerController = World->SpawnActor<APlayerController>();
	AFirstPersonCharacter* Character = World->SpawnActor<AFirstPersonCharacter>();
	BeginActorForComputerTest(PlayerController);
	BeginActorForComputerTest(Character);
	ULocalPlayer* LocalPlayer = NewObject<ULocalPlayer>(GEngine, TEXT("ComputerWheelRoutingLocalPlayer"));
	PlayerController->SetPlayer(LocalPlayer);
	PlayerController->Possess(Character);
	PlayerController->SetViewTarget(Character);

	ABathhouseComputerActor* Computer = World->SpawnActor<ABathhouseComputerActor>();
	Computer->ComputerMesh->SetStaticMesh(CubeMesh);
	Computer->ComputerMesh->SetWorldScale3D(FVector(0.5f));
	Computer->FocusBlendInSeconds = 0.2f;
	Computer->FocusBlendOutSeconds = 0.2f;
	UUserWidget* ScreenInstance = NewObject<UComputerSampleScreenWidget>(PlayerController, TEXT("ComputerWheelRoutingScreen"));
	Computer->ScreenWidget->SetWidget(ScreenInstance);
	BeginActorForComputerTest(Computer);
	Computer->SetActorLocation(
		Character->GetFirstPersonCamera()->GetComponentLocation()
		+ Character->GetFirstPersonCamera()->GetForwardVector() * 150.0f);
	Computer->FocusExitPoint->SetWorldLocationAndRotation(
		Computer->GetActorLocation() + FVector(350.0f, 300.0f, 0.0f),
		FRotator(24.0f, 137.0f, 38.0f));
	Computer->ComputerMesh->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	Computer->ComputerMesh->SetCollisionResponseToAllChannels(ECR_Ignore);
	Computer->ComputerMesh->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
	Computer->ComputerMesh->UpdateBounds();
	Computer->ComputerMesh->RecreatePhysicsState();
	World->UpdateWorldComponents(true, false);

	AActor* OtherOwner = World->SpawnActor<AActor>();
	UWidgetComponent* OtherScreen = NewObject<UWidgetComponent>(OtherOwner, TEXT("OtherScreen"));
	UWidgetComponent* Screen = Computer->ScreenWidget;
	UPlayerComputerUseComponent* ComputerUse = Character->GetPlayerComputerUse();
	UPlayerFacilityPlacementComponent* Placement = Character->PlayerFacilityPlacement;
	UPlayerInteractionComponent* Interaction = Character->GetPlayerInteraction();
	if (!TestNotNull(TEXT("The character owns computer-use and placement components"), ComputerUse)
		|| !TestNotNull(TEXT("The character owns the placement component"), Placement))
	{
		return false;
	}
	const float RotationStep = GetDefault<UFacilityPlacementSettings>()->GetRotationStepDegrees();
	const auto PlacePreview = [&]() -> bool
	{
		APlaceableFacilityItemActor* Item = World->SpawnActor<APlaceableFacilityItemActor>();
		if (!Item)
		{
			return false;
		}
		Placement->PreviewFacility = Item;
		Placement->AccumulatedYaw = 0.0f;
		return true;
	};
	const auto WheelUp = FInputActionValue(1.0f);

	// 1. Inactive: no injection, wheel falls through to Placement.
	TestFalse(TEXT("CWS-018: Inactive never injects"), ComputerUse->CanInjectPointerWheel(1.0f, Screen));
	Character->MouseWheelInput(WheelUp);
	if (!TestTrue(TEXT("Placement preview fixture exists"), PlacePreview())) return false;
	const float ExpectedYaw = AFacilityPlacementZoneActor::NormalizePlacementYaw(0.0f + RotationStep);
	Character->MouseWheelInput(WheelUp);
	TestTrue(TEXT("CWS-017: Inactive wheel rotates the placement preview by the configured step"),
		FMath::IsNearlyEqual(Placement->AccumulatedYaw, ExpectedYaw, 0.01f));

	// 2. Real entry path with a non-zero blend: FocusingIn.
	Interaction->RefreshInteractionQuery();
	TestTrue(TEXT("Setup: the empty-handed player can query the computer"), Interaction->GetCurrentInteractionQuery().bCanInteract);
	Character->InteractStartInput();
	if (!TestEqual(TEXT("A non-zero blend stays in FocusingIn"),
		ComputerUse->GetPhase(), EPlayerComputerUsePhase::FocusingIn)) return false;
	TestFalse(TEXT("CWS-013: FocusingIn does not inject (+1)"), ComputerUse->CanInjectPointerWheel(1.0f, Screen));
	TestFalse(TEXT("CWS-013: FocusingIn does not inject (-1)"), ComputerUse->CanInjectPointerWheel(-1.0f, Screen));
	TestFalse(TEXT("CWS-013: hit testing is off while FocusingIn"), Character->ComputerWidgetInteraction->bEnableHitTesting);
	if (!PlacePreview()) return false;
	Character->MouseWheelInput(WheelUp);
	TestTrue(TEXT("CWS-013: FocusingIn wheel is consumed and never rotates Placement"),
		FMath::IsNearlyZero(Placement->AccumulatedYaw));
	Character->InteractEndInput();

	// 3. Active: clickless injection gate.
	ComputerUse->CompleteFocusIn();
	if (!TestTrue(TEXT("The session reaches Active"), ComputerUse->IsActive())) return false;
	TestTrue(TEXT("CWS-003: Active injects wheel down without any click"), ComputerUse->CanInjectPointerWheel(-1.0f, Screen));
	TestTrue(TEXT("CWS-003: Active injects wheel up without any click"), ComputerUse->CanInjectPointerWheel(1.0f, Screen));
	TestFalse(TEXT("A zero wheel does not inject"), ComputerUse->CanInjectPointerWheel(0.0f, Screen));
	TestFalse(TEXT("A NaN wheel does not inject"),
		ComputerUse->CanInjectPointerWheel(NAN, Screen));
	TestFalse(TEXT("CWS-012: no hovered component does not inject"), ComputerUse->CanInjectPointerWheel(1.0f, nullptr));
	TestFalse(TEXT("CWS-012: another hovered component does not inject"), ComputerUse->CanInjectPointerWheel(1.0f, OtherScreen));

	// 4. Headless has no widget hit, so the production entry reports no injection and does not crash.
	TestFalse(TEXT("ScrollPointerWheel injects nothing without a hovered screen"), ComputerUse->ScrollPointerWheel(1.0f));
	AddInfo(TEXT("ScrollPointerWheel returned false: headless -nullrhi builds no widget hit-test grid, so the hovered "
		"widget component is null and the engine ScrollWheel call is covered by the Slate routing test."));

	// 5. Active: wheel never reaches Placement.
	if (!PlacePreview()) return false;
	Character->MouseWheelInput(WheelUp);
	Character->MouseWheelInput(FInputActionValue(-1.0f));
	TestTrue(TEXT("CWS-018: Active wheel does not rotate Placement"), FMath::IsNearlyZero(Placement->AccumulatedYaw));
	TestTrue(TEXT("CWS-018: Active wheel keeps the placement preview untouched"), Placement->PreviewFacility.IsValid());

	// 6. CWS-016: wheel leaves cursor and focus-out behavior alone.
	const bool bCursorBefore = PlayerController->bShowMouseCursor;
	TestTrue(TEXT("CWS-016: Active shows the cursor"), bCursorBefore);
	for (int32 Index = 0; Index < 5; ++Index)
	{
		Character->MouseWheelInput(Index % 2 == 0 ? WheelUp : FInputActionValue(-1.0f));
	}
	TestEqual(TEXT("CWS-016: the wheel does not change the cursor visibility"), PlayerController->bShowMouseCursor, bCursorBefore);
	TestTrue(TEXT("CWS-016: the wheel keeps the session Active"), ComputerUse->IsActive());
	Character->InteractStartInput();
	TestEqual(TEXT("CWS-016: E after wheel input starts focus-out without any click"),
		ComputerUse->GetPhase(), EPlayerComputerUsePhase::FocusingOut);
	TestFalse(TEXT("CWS-013: FocusingOut does not inject"), ComputerUse->CanInjectPointerWheel(1.0f, Screen));
	Character->InteractEndInput();
	ComputerUse->CompleteFocusOut();
	TestFalse(TEXT("Focus-out ends capture"), ComputerUse->IsCapturingInput());

	// 7. Abnormal end: Computer unavailable returns the wheel to Placement.
	Computer->FocusBlendInSeconds = 0.0f;
	Computer->FocusBlendOutSeconds = 0.0f;
	Character->SetActorLocation(FVector::ZeroVector);
	PlayerController->SetControlRotation(FRotator::ZeroRotator);
	Character->SetActorRotation(FRotator::ZeroRotator);
	Interaction->RefreshInteractionQuery();
	Character->InteractStartInput();
	if (!TestTrue(TEXT("The session re-enters Active for the abnormal end"), ComputerUse->IsActive())) return false;
	Character->InteractEndInput();
	ComputerUse->HandleComputerUnavailable(Computer);
	TestFalse(TEXT("HandleComputerUnavailable returns to Inactive"), ComputerUse->IsCapturingInput());
	TestFalse(TEXT("Inactive after unavailable does not inject"), ComputerUse->CanInjectPointerWheel(1.0f, Screen));
	if (!PlacePreview()) return false;
	Character->MouseWheelInput(WheelUp);
	TestTrue(TEXT("CWS-017: after an abnormal end the wheel rotates Placement again"),
		FMath::IsNearlyEqual(Placement->AccumulatedYaw, ExpectedYaw, 0.01f));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBathhouseComputerWheelContentContractTest,
	"BathhouseSim.Computer.Input.ScreenWheelContentContract",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FBathhouseComputerWheelContentContractTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	struct FTreeSpec
	{
		const TCHAR* ClassPath;
		TArray<FName> ScrollNames;
	};
	const FTreeSpec Specs[] = {
		{ TEXT("/Game/Bathhouse/UI/Shop/WBP_ShopScreen.WBP_ShopScreen_C"),
		  { FName(TEXT("ProductScroll")), FName(TEXT("CartScroll")), FName(TEXT("OrderScroll")) } },
		{ TEXT("/Game/Bathhouse/UI/WBP_BathWaterDetail.WBP_BathWaterDetail_C"),
		  { FName(TEXT("DetailScroll")) } },
		{ TEXT("/Game/Bathhouse/UI/WBP_ComputerScreenRoot.WBP_ComputerScreenRoot_C"), {} },
	};

	for (const FTreeSpec& Spec : Specs)
	{
		const UClass* Class = LoadClass<UUserWidget>(nullptr, Spec.ClassPath);
		const UWidgetBlueprintGeneratedClass* WidgetClass = Cast<UWidgetBlueprintGeneratedClass>(Class);
		const UWidgetTree* Tree = WidgetClass ? WidgetClass->GetWidgetTreeArchetype() : nullptr;
		if (!TestNotNull(*FString::Printf(TEXT("Widget class loads with a widget tree: %s"), Spec.ClassPath), Tree))
		{
			continue;
		}
		Tree->ForEachWidget([this](UWidget* Widget)
		{
			if (const UScrollBox* Scroll = Cast<UScrollBox>(Widget))
			{
				TestTrue(*FString::Printf(TEXT("%s: any scroll box consumes the wheel only when it can scroll"), *Scroll->GetName()),
					Scroll->GetConsumeMouseWheel() != EConsumeMouseWheel::Never);
			}
		});
		for (const FName ScrollName : Spec.ScrollNames)
		{
			const UScrollBox* Scroll = Tree->FindWidget<UScrollBox>(ScrollName);
			if (!TestNotNull(*FString::Printf(TEXT("%s exists in %s"), *ScrollName.ToString(), Spec.ClassPath), Scroll))
			{
				continue;
			}
			TestTrue(*FString::Printf(TEXT("%s is vertical"), *ScrollName.ToString()),
				Scroll->GetOrientation() == Orient_Vertical);
			TestTrue(*FString::Printf(TEXT("%s consumes the wheel when scrolling is possible"), *ScrollName.ToString()),
				Scroll->GetConsumeMouseWheel() != EConsumeMouseWheel::Never);
			TestFalse(*FString::Printf(TEXT("%s does not animate wheel scrolling"), *ScrollName.ToString()),
				Scroll->IsAnimateWheelScrolling());
			const float Multiplier = Scroll->GetWheelScrollMultiplier();
			TestTrue(*FString::Printf(TEXT("%s has a finite positive wheel multiplier"), *ScrollName.ToString()),
				FMath::IsFinite(Multiplier) && Multiplier > 0.0f);
			AddInfo(FString::Printf(TEXT("%s WheelScrollMultiplier = %.3f"), *ScrollName.ToString(), Multiplier));
			TestTrue(*FString::Printf(TEXT("%s is hit-test visible"), *ScrollName.ToString()),
				Scroll->GetVisibility() == ESlateVisibility::Visible);
		}
	}

	const UInputAction* RotateAction = LoadObject<UInputAction>(nullptr, TEXT("/Game/Input/Actions/IA_PlacementRotate.IA_PlacementRotate"));
	if (TestNotNull(TEXT("IA_PlacementRotate loads"), RotateAction))
	{
		TestTrue(TEXT("IA_PlacementRotate is Axis1D"), RotateAction->ValueType == EInputActionValueType::Axis1D);
		TestEqual(TEXT("IA_PlacementRotate has no modifiers"), RotateAction->Modifiers.Num(), 0);
		TestEqual(TEXT("IA_PlacementRotate has no triggers"), RotateAction->Triggers.Num(), 0);
	}
	const UInputMappingContext* Context = LoadObject<UInputMappingContext>(nullptr, TEXT("/Game/Input/IMC_FirstPerson.IMC_FirstPerson"));
	if (TestNotNull(TEXT("IMC_FirstPerson loads"), Context))
	{
		int32 MappingIndex = INDEX_NONE;
		if (TestTrue(TEXT("Exactly one wheel key mapping exists in IMC_FirstPerson"),
			ComputerWheelScrollTest::ReadMouseWheelKeyMappingCount(*Context, MappingIndex)))
		{
			const FEnhancedActionKeyMapping& Mapping = Context->GetMappings()[MappingIndex];
			TestTrue(TEXT("The wheel mapping uses MouseWheelAxis"), Mapping.Key == EKeys::MouseWheelAxis);
			TestTrue(TEXT("The wheel mapping targets IA_PlacementRotate"), Mapping.Action == RotateAction);
			TestEqual(TEXT("The wheel mapping has no modifiers"), Mapping.Modifiers.Num(), 0);
			TestEqual(TEXT("The wheel mapping has no triggers"), Mapping.Triggers.Num(), 0);
		}
	}
	const UClass* CharacterClass = LoadClass<AFirstPersonCharacter>(
		nullptr, TEXT("/Game/FirstPersonCharacter/BP_FirstPersonCharacter.BP_FirstPersonCharacter_C"));
	if (TestNotNull(TEXT("BP_FirstPersonCharacter loads"), CharacterClass))
	{
		const FObjectProperty* ActionProperty = FindFProperty<FObjectProperty>(
			AFirstPersonCharacter::StaticClass(), TEXT("PlacementRotateAction"));
		if (TestNotNull(TEXT("PlacementRotateAction keeps its reflected property"), ActionProperty))
		{
			TestTrue(TEXT("BP_FirstPersonCharacter CDO assigns IA_PlacementRotate to PlacementRotateAction"),
				RotateAction && ActionProperty->GetObjectPropertyValue_InContainer(CharacterClass->GetDefaultObject()) == RotateAction);
		}
	}
	return true;
}

#endif
