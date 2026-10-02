#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Blueprint/WidgetBlueprintGeneratedClass.h"
#include "Blueprint/WidgetTree.h"
#include "Building/BathhouseExpansionPurchaseSubsystem.h"
#include "Building/BathhouseExpansionTypes.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/WidgetSwitcher.h"
#include "Economy/PlayerWalletComponent.h"
#include "Facility/BathhouseExpansionDefinition.h"
#include "Facility/LockerCapacitySubsystem.h"
#include "Placement/FacilityPlacementDefinition.h"
#include "Placement/FacilityPlacementTypes.h"
#include "Shop/ShopCatalog.h"
#include "UI/ExpansionSpaceOptionWidget.h"
#include "Tests/BathhouseExpansionAutomationTestSupport.h"
#include "UI/ComputerScreenRootWidget.h"
#include "UI/ExpansionScreenModel.h"
#include "UI/ExpansionScreenWidget.h"

/** 확장 화면·root의 private 표시 상태와 입력 handler에 접근하는 통로. */
class FExpansionScreenAutomationAccess
{
public:
	static void ClickOption(UExpansionScreenWidget& Widget, const EBathhouseSpaceKind Kind) { Widget.HandleOptionClicked(Kind); }
	static void ClickPurchase(UExpansionScreenWidget& Widget) { Widget.HandlePurchaseClicked(); }
	static void ClickConfirm(UExpansionScreenWidget& Widget) { Widget.HandleConfirmClicked(); }
	static void ClickCancel(UExpansionScreenWidget& Widget) { Widget.HandleCancelClicked(); }
	static void Destruct(UExpansionScreenWidget& Widget) { Widget.NativeDestruct(); }
	static bool IsPending(const UExpansionScreenWidget& Widget) { return Widget.bConfirmPending; }
	static int32 PendingCount(const UExpansionScreenWidget& Widget) { return Widget.ConfirmAppliedCount; }
	static bool IsCompleted(const UExpansionScreenWidget& Widget) { return Widget.bShowCompleted; }
	static bool HasSelection(const UExpansionScreenWidget& Widget) { return Widget.SelectedKind.IsSet(); }
	static EBathhouseSpaceKind Selection(const UExpansionScreenWidget& Widget) { return Widget.SelectedKind.GetValue(); }
	static void InjectScreen(UComputerScreenRootWidget& Root, UExpansionScreenWidget* Screen) { Root.ExpansionScreen = Screen; }
	static void ClickExpansionTab(UComputerScreenRootWidget& Root) { Root.HandleExpansionTabClicked(); }
	static void ClickShopTab(UComputerScreenRootWidget& Root) { Root.HandleShopTabClicked(); }
	static void ClickManagementTab(UComputerScreenRootWidget& Root) { Root.HandleManagementTabClicked(); }
};

namespace
{
	// 아래 값은 모델 fixture 입력이다. 기대값은 같은 입력에서 계산한다.
	FBathhouseExpansionView MakeView()
	{
		FBathhouseExpansionView View;
		View.bAvailable = true;
		View.Balance = 5000;
		View.InstalledLockerSlots = 2;
		View.LockerSlotLimit = 2;
		for (int32 Index = 0; Index < 3; ++Index)
		{
			View.Options[Index].bPresent = true;
			View.Options[Index].bCanExpand = true;
			View.Options[Index].AppliedCount = Index;
			View.Options[Index].StepCount = 2 + Index;
			// 공간마다 다른 가격(D2). 잔액보다 작다.
			View.Options[Index].NextPrice = 800 + 300 * Index;
			View.Options[Index].CurrentSizeCm = FVector2D(2000.0, 1400.0);
			View.Options[Index].NextSizeCm = FVector2D(2000.0, 1600.0);
		}
		View.Options[0].bHasHallEffect = true;
		View.Options[0].KeysNow = 3;
		View.Options[0].KeysNext = 4;
		View.Options[0].LockerLimitNow = 2;
		View.Options[0].LockerLimitNext = 4;
		return View;
	}

	FExpansionScreenState MakeState(const TOptional<EBathhouseSpaceKind> Selected, const bool bConfirm = false, const bool bCompleted = false)
	{
		FExpansionScreenState State;
		State.Selected = Selected;
		State.bConfirmPending = bConfirm;
		State.bShowCompleted = bCompleted;
		return State;
	}

	FString Str(const FText& Text) { return Text.ToString(); }
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FExpansionScreenModelTest,
	"BathhouseSim.Expansion.UI.ScreenModel",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FExpansionScreenModelTest::RunTest(const FString& Parameters)
{
	const TOptional<EBathhouseSpaceKind> None;
	const TOptional<EBathhouseSpaceKind> Hall = EBathhouseSpaceKind::Hall;
	const TOptional<EBathhouseSpaceKind> Bath = EBathhouseSpaceKind::Bath;

	// 크기 문구 형식(정수·소수 1자리·world X 먼저).
	TestEqual(TEXT("Whole meters have no decimals"), Str(FExpansionScreenModel::FormatSize(FVector2D(2000.0, 1400.0))), FString(TEXT("20m×14m")));
	TestEqual(TEXT("One decimal digit at most"), Str(FExpansionScreenModel::FormatSize(FVector2D(2050.0, 1430.0))), FString(TEXT("20.5m×14.3m")));
	TestEqual(TEXT("X comes before Y"), Str(FExpansionScreenModel::FormatSize(FVector2D(300.0, 500.0))), FString(TEXT("3m×5m")));
	TestEqual(TEXT("Space names are the localized table"), Str(FExpansionScreenModel::SpaceName(EBathhouseSpaceKind::Bath)), FString(TEXT("목욕공간")));
	TestTrue(TEXT("Money ends with the won unit"), Str(FExpansionScreenModel::FormatMoney(1234)).EndsWith(TEXT("원")));

	// EXP-031 사용 불가.
	{
		FBathhouseExpansionView View = MakeView();
		View.bAvailable = false;
		const FExpansionScreenDisplay Display = FExpansionScreenModel::Build(View, MakeState(Hall, true));
		TestEqual(TEXT("Unavailable message"), Str(Display.Message), FString(TEXT("확장을 사용할 수 없습니다")));
		TestTrue(TEXT("Unavailable message is visible"), Display.bMessageVisible);
		TestFalse(TEXT("Unavailable hides the options"), Display.bOptionsVisible);
		TestFalse(TEXT("Unavailable hides the option prices"), Display.Options[0].bPriceVisible);
		TestFalse(TEXT("Unavailable hides the purchase panel"), Display.bPurchasePanelVisible);
		TestFalse(TEXT("Unavailable hides the confirm panel"), Display.bConfirmPanelVisible);
		TestFalse(TEXT("Unavailable hides the shortfall"), Display.bShortfallVisible);
		TestFalse(TEXT("Unavailable hides the option stages"), Display.Options[0].bStageVisible);
		TestFalse(TEXT("Unavailable hides the locker line"), Display.bLockerVisible);
		TestTrue(TEXT("Unavailable keeps the balance"), Display.bBalanceVisible);
		TestTrue(TEXT("Unavailable clears the selection"), Display.bClearSelection);
	}
	// 모두 상한(EXP-043): 일부만 상한이면 최대 문구가 없고, 모두 상한이면 있다.
	{
		FBathhouseExpansionView View = MakeView();
		View.bAllAtLimit = true;
		for (int32 Index = 0; Index < 3; ++Index)
		{
			View.Options[Index].bAtLimit = true;
			View.Options[Index].bCanExpand = false;
			View.Options[Index].AppliedCount = View.Options[Index].StepCount;
			View.Options[Index].NextPrice = 0;
		}
		const FExpansionScreenDisplay Display = FExpansionScreenModel::Build(View, MakeState(Hall, false, true));
		TestEqual(TEXT("Maximum message"), Str(Display.Message), FString(TEXT("최대 확장 단계입니다")));
		TestFalse(TEXT("Maximum hides the options"), Display.bOptionsVisible);
		TestFalse(TEXT("Maximum hides the purchase panel"), Display.bPurchasePanelVisible);
		TestTrue(TEXT("Maximum keeps the locker line"), Display.bLockerVisible);
		TestTrue(TEXT("Maximum keeps the completed text"), Display.bResultVisible);
		TestEqual(TEXT("Completed text"), Str(Display.Result), FString(TEXT("확장 완료")));
		TestTrue(TEXT("Maximum clears the selection"), Display.bClearSelection);
	}
	{
		FBathhouseExpansionView View = MakeView();
		View.Options[0].bAtLimit = true;
		View.Options[0].bCanExpand = false;
		View.Options[0].AppliedCount = View.Options[0].StepCount;
		View.Options[0].NextPrice = 0;
		const FExpansionScreenDisplay Display = FExpansionScreenModel::Build(View, MakeState(None));
		TestTrue(TEXT("Only the hall at its limit shows no maximum message"), !Display.bMessageVisible && Display.bOptionsVisible);
		TestTrue(TEXT("The limited option is disabled"), !Display.Options[0].bEnabled && Display.Options[1].bEnabled);
	}
	// EXP-021 선택 전.
	{
		const FBathhouseExpansionView View = MakeView();
		const FExpansionScreenDisplay Display = FExpansionScreenModel::Build(View, MakeState(None));
		TestEqual(TEXT("Locker text"), Str(Display.Locker), FString(TEXT("설치된 락커 칸 2/2")));
		TestTrue(TEXT("Balance text"), Str(Display.Balance).StartsWith(TEXT("잔액 ")) && Str(Display.Balance).EndsWith(TEXT("원")));
		TestEqual(TEXT("The purchase button has no price before a selection"), Str(Display.PurchaseButtonText), FString(TEXT("확장 구입")));
		TestTrue(TEXT("Options are visible"), Display.bOptionsVisible);
		TestTrue(TEXT("The purchase panel is shown"), Display.bPurchasePanelVisible);
		TestFalse(TEXT("The purchase button is off without a selection"), Display.bPurchaseEnabled);
		TestFalse(TEXT("The confirm panel is hidden"), Display.bConfirmPanelVisible);
		TestFalse(TEXT("There is no shortfall before a selection"), Display.bShortfallVisible);
		TestFalse(TEXT("No completed text before a purchase"), Display.bResultVisible);
		TestFalse(TEXT("Nothing to clear without a selection"), Display.bClearSelection);
		for (int32 Index = 0; Index < 3; ++Index)
		{
			const FBathhouseExpansionOptionView& Option = View.Options[Index];
			TestEqual(TEXT("Option stage is applied / step count"), Str(Display.Options[Index].Stage),
				FString::Printf(TEXT("확장 단계 %d/%d"), Option.AppliedCount, Option.StepCount));
			TestTrue(TEXT("Option stage is visible"), Display.Options[Index].bStageVisible);
			TestEqual(TEXT("Option price is its own next step price"), Str(Display.Options[Index].Price),
				FString::Printf(TEXT("다음 넓힘 %s"), *Str(FExpansionScreenModel::FormatMoney(Option.NextPrice))));
			TestTrue(TEXT("Option price is visible"), Display.Options[Index].bPriceVisible);
		}
		TestTrue(TEXT("Option prices differ between spaces"), Str(Display.Options[0].Price) != Str(Display.Options[1].Price));
		const FString Size = Str(Display.Options[0].Size);
		TestEqual(TEXT("Option size shows before and after"), Size,
			Str(FExpansionScreenModel::FormatSize(View.Options[0].CurrentSizeCm)) + TEXT(" → ") + Str(FExpansionScreenModel::FormatSize(View.Options[0].NextSizeCm)));
		TestTrue(TEXT("The hall shows its effect"), Display.Options[0].bEffectVisible);
		TestEqual(TEXT("Hall effect text"), Str(Display.Options[0].Effect),
			FString::Printf(TEXT("열쇠 %d개 → %d개\n락커 칸 한도 %d칸 → %d칸"),
				View.Options[0].KeysNow, View.Options[0].KeysNext, View.Options[0].LockerLimitNow, View.Options[0].LockerLimitNext));
		TestFalse(TEXT("The bath shows no hall effect"), Display.Options[1].bEffectVisible);
		TestTrue(TEXT("Options are enabled"), Display.Options[0].bEnabled && Display.Options[1].bEnabled && Display.Options[2].bEnabled);
		TestEqual(TEXT("Option names"), Str(Display.Options[2].Name), FString(TEXT("작업공간")));
	}
	// 선택 후, 부족, 확인 대기.
	{
		FBathhouseExpansionView View = MakeView();
		FExpansionScreenDisplay Display = FExpansionScreenModel::Build(View, MakeState(Hall));
		TestTrue(TEXT("A selection with enough money enables the purchase"), Display.bPurchaseEnabled);
		TestTrue(TEXT("The selected option is highlighted"), Display.Options[0].bSelected);
		TestFalse(TEXT("The other options are not highlighted"), Display.Options[1].bSelected);
		TestEqual(TEXT("The button shows the selected space price"), Str(Display.PurchaseButtonText),
			FString::Printf(TEXT("확장 구입 (%s)"), *Str(FExpansionScreenModel::FormatMoney(View.Options[0].NextPrice))));
		Display = FExpansionScreenModel::Build(View, MakeState(Bath));
		TestTrue(TEXT("Selecting another space moves the highlight"), Display.Options[1].bSelected && !Display.Options[0].bSelected);
		TestEqual(TEXT("The button price follows the selection"), Str(Display.PurchaseButtonText),
			FString::Printf(TEXT("확장 구입 (%s)"), *Str(FExpansionScreenModel::FormatMoney(View.Options[1].NextPrice))));

		Display = FExpansionScreenModel::Build(View, MakeState(Hall, true));
		TestTrue(TEXT("Confirm pending shows the confirm panel"), Display.bConfirmPanelVisible);
		TestFalse(TEXT("Confirm pending hides the purchase panel"), Display.bPurchasePanelVisible);
		TestTrue(TEXT("Confirm is enabled when purchasable"), Display.bConfirmEnabled);

		// EXP-022: 부족은 고른 공간의 가격 기준이고, 선택 전에는 보이지 않는다.
		View.Balance = View.Options[0].NextPrice - 250;
		Display = FExpansionScreenModel::Build(View, MakeState(Hall));
		TestFalse(TEXT("A shortfall disables the purchase"), Display.bPurchaseEnabled);
		TestTrue(TEXT("The shortfall text is visible"), Display.bShortfallVisible);
		TestEqual(TEXT("Shortfall text"), Str(Display.Shortfall), FString::Printf(TEXT("%s 부족"), *Str(FExpansionScreenModel::FormatMoney(250))));
		Display = FExpansionScreenModel::Build(View, MakeState(None));
		TestFalse(TEXT("No shortfall text before a selection"), Display.bShortfallVisible);
		Display = FExpansionScreenModel::Build(View, MakeState(Hall, true));
		TestTrue(TEXT("A confirm pending screen stays"), Display.bConfirmPanelVisible);
		TestFalse(TEXT("Money lost during confirm disables the confirm button"), Display.bConfirmEnabled);
		// 같은 잔액에서 더 싼 공간은 부족하지 않다(고른 공간의 가격 기준).
		View.Options[1].NextPrice = View.Options[0].NextPrice - 400;
		Display = FExpansionScreenModel::Build(View, MakeState(Bath));
		TestFalse(TEXT("A cheaper selected space has no shortfall"), Display.bShortfallVisible);
		TestTrue(TEXT("A cheaper selected space can be purchased"), Display.bPurchaseEnabled);
		View.Balance = View.Options[0].NextPrice;
		Display = FExpansionScreenModel::Build(View, MakeState(Hall));
		TestTrue(TEXT("Money arriving later enables the purchase at once"), Display.bPurchaseEnabled && !Display.bShortfallVisible);

		View = MakeView();
		Display = FExpansionScreenModel::Build(View, MakeState(None, false, true));
		TestTrue(TEXT("The completed text shows after a purchase"), Display.bResultVisible);
	}
	// 고를 수 없는 선택지.
	{
		FBathhouseExpansionView View = MakeView();
		View.Options[1].bCanExpand = false;
		FExpansionScreenDisplay Display = FExpansionScreenModel::Build(View, MakeState(None));
		TestFalse(TEXT("A capped option is disabled"), Display.Options[1].bEnabled);
		TestTrue(TEXT("A capped option shows its status"), Display.Options[1].bStatusVisible);
		TestEqual(TEXT("Capped status text"), Str(Display.Options[1].Status), FString(TEXT("이 공간은 더 넓힐 수 없습니다")));
		TestEqual(TEXT("A capped option shows only the current size"), Str(Display.Options[1].Size),
			Str(FExpansionScreenModel::FormatSize(View.Options[1].CurrentSizeCm)));
		Display = FExpansionScreenModel::Build(View, MakeState(Bath, true));
		TestTrue(TEXT("A selected option that becomes capped clears the selection"), Display.bClearSelection);
		TestFalse(TEXT("It cannot be purchased"), Display.bPurchaseEnabled || Display.bConfirmEnabled);
		View.Options[1].bPresent = false;
		Display = FExpansionScreenModel::Build(View, MakeState(None));
		TestTrue(TEXT("A missing space shows an empty size"), Display.Options[1].Size.IsEmpty());
		TestFalse(TEXT("A missing space is disabled"), Display.Options[1].bEnabled);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FExpansionScreenWidgetTest,
	"BathhouseSim.Expansion.UI.ScreenWidgetNative",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FExpansionScreenWidgetTest::RunTest(const FString& Parameters)
{
	using ScreenAccess = FExpansionScreenAutomationAccess;
	FBathhouseExpansionTestWorld Fx;
	if (!Fx.Create(*this, TEXT("ExpansionWidgetWorld")))
	{
		Fx.Destroy();
		return false;
	}
	UPlayerWalletComponent* Wallet = Fx.Wallet();
	ULockerCapacitySubsystem* Lockers = Fx.World->GetSubsystem<ULockerCapacitySubsystem>();
	// 가격은 고른 공간의 넓힘 줄에서 읽는다.
	const int32 Price = FBathhouseExpansionAutomationAccess::Steps(*Fx.Hall)[0].Price;
	auto AppliedTotal = [&Fx]()
	{
		return Fx.Hall->GetAppliedExpansionCount() + Fx.Bath->GetAppliedExpansionCount() + Fx.Work->GetAppliedExpansionCount();
	};
	const int32 MoneyBefore = Wallet->GetCurrentMoney();

	UExpansionScreenTestWidget* Widget = NewObject<UExpansionScreenTestWidget>(Fx.World);
	Widget->NotifyComputerUserChanged(Fx.Player);
	TestTrue(TEXT("The screen binds the wallet"), Wallet->OnMoneyChanged.IsBound());
	TestTrue(TEXT("The screen binds locker capacity"), Lockers->OnLockerCapacityChanged.IsBound());
	TestTrue(TEXT("The screen binds the expansion change"), Fx.Purchase->OnExpansionChanged.IsBound());

	// 구입 버튼은 선택이 있어야 확인 대기로 간다.
	ScreenAccess::ClickPurchase(*Widget);
	TestFalse(TEXT("Purchase without a selection does nothing"), ScreenAccess::IsPending(*Widget));
	ScreenAccess::ClickOption(*Widget, EBathhouseSpaceKind::Hall);
	TestTrue(TEXT("Clicking a space selects it"), ScreenAccess::HasSelection(*Widget) && ScreenAccess::Selection(*Widget) == EBathhouseSpaceKind::Hall);
	ScreenAccess::ClickPurchase(*Widget);
	TestTrue(TEXT("Purchase with a selection waits for confirmation"), ScreenAccess::IsPending(*Widget));
	TestEqual(TEXT("The confirmation remembers the selected space applied count"), ScreenAccess::PendingCount(*Widget), Fx.Hall->GetAppliedExpansionCount());

	// EXP-026 취소, 사용 종료, 탭 이탈은 아무것도 바꾸지 않는다.
	ScreenAccess::ClickCancel(*Widget);
	TestFalse(TEXT("Cancel drops the confirmation"), ScreenAccess::IsPending(*Widget));
	TestTrue(TEXT("Cancel keeps the selection"), ScreenAccess::HasSelection(*Widget));
	ScreenAccess::ClickPurchase(*Widget);
	Widget->NotifyComputerUseEnded();
	TestFalse(TEXT("Computer use ending drops the confirmation"), ScreenAccess::IsPending(*Widget));
	ScreenAccess::ClickPurchase(*Widget);
	Widget->CancelPendingConfirm();
	TestFalse(TEXT("CancelPendingConfirm drops the confirmation"), ScreenAccess::IsPending(*Widget));
	TestEqual(TEXT("Cancelling changes no money"), Wallet->GetCurrentMoney(), MoneyBefore);
	TestEqual(TEXT("Cancelling expands nothing"), AppliedTotal(), 0);
	Widget->CancelPendingConfirm();
	TestEqual(TEXT("A second cancel is a no-op"), AppliedTotal(), 0);

	// root 탭 전환이 취소를 부른다.
	UComputerScreenRootWidget* Root = NewObject<UComputerScreenRootWidget>(Fx.World);
	ScreenAccess::InjectScreen(*Root, Widget);
	ScreenAccess::ClickExpansionTab(*Root);
	ScreenAccess::ClickPurchase(*Widget);
	TestTrue(TEXT("Pending again on the expansion tab"), ScreenAccess::IsPending(*Widget));
	ScreenAccess::ClickShopTab(*Root);
	TestFalse(TEXT("Leaving the expansion tab drops the confirmation"), ScreenAccess::IsPending(*Widget));
	ScreenAccess::ClickExpansionTab(*Root);
	ScreenAccess::ClickPurchase(*Widget);
	ScreenAccess::ClickManagementTab(*Root);
	TestFalse(TEXT("Leaving for management drops the confirmation"), ScreenAccess::IsPending(*Widget));
	ScreenAccess::ClickExpansionTab(*Root);
	ScreenAccess::ClickPurchase(*Widget);
	Root->NotifyComputerUseEnded();
	TestFalse(TEXT("The root forwards the use end"), ScreenAccess::IsPending(*Widget));
	TestTrue(TEXT("Use end keeps the selection"), ScreenAccess::HasSelection(*Widget));

	// EXP-027 확인 두 번은 결제 한 번이다.
	ScreenAccess::ClickPurchase(*Widget);
	TestTrue(TEXT("Waiting before the double confirm"), ScreenAccess::IsPending(*Widget));
	ScreenAccess::ClickConfirm(*Widget);
	ScreenAccess::ClickConfirm(*Widget);
	TestEqual(TEXT("Two confirms charge once"), Wallet->GetCurrentMoney(), MoneyBefore - Price);
	TestEqual(TEXT("Two confirms expand once"), AppliedTotal(), 1);
	TestTrue(TEXT("The completed text is flagged"), ScreenAccess::IsCompleted(*Widget));
	TestFalse(TEXT("A successful purchase clears the selection"), ScreenAccess::HasSelection(*Widget));
	TestFalse(TEXT("No confirmation remains"), ScreenAccess::IsPending(*Widget));
	ScreenAccess::ClickOption(*Widget, EBathhouseSpaceKind::Bath);
	TestFalse(TEXT("Selecting again clears the completed flag"), ScreenAccess::IsCompleted(*Widget));

	// 돈 부족이면 확인 대기로 가지 않는다(EXP-022).
	const int32 SecondPrice = FBathhouseExpansionAutomationAccess::Steps(*Fx.Bath)[0].Price;
	Wallet->TrySpendMoney(Wallet->GetCurrentMoney() - (SecondPrice - 1));
	ScreenAccess::ClickPurchase(*Widget);
	TestFalse(TEXT("A shortfall blocks the confirmation"), ScreenAccess::IsPending(*Widget));
	Wallet->TryAddMoney(1);
	ScreenAccess::ClickPurchase(*Widget);
	TestTrue(TEXT("Money arriving later allows the confirmation"), ScreenAccess::IsPending(*Widget));

	// 선택지 카드는 StageText·PriceText 같은 optional widget이 없어도 ApplyModel이 안전하다.
	{
		UExpansionOptionTestWidget* Option = NewObject<UExpansionOptionTestWidget>(Fx.World);
		FExpansionOptionDisplay Display;
		Display.Name = FText::FromString(TEXT("홀"));
		Display.Stage = FText::FromString(TEXT("확장 단계 0/2"));
		Display.Price = FText::FromString(TEXT("다음 넓힘 1원"));
		Display.bStageVisible = true;
		Display.bPriceVisible = true;
		Option->ApplyModel(Display);
		TestTrue(TEXT("ApplyModel without bound text widgets does not crash"), true);
	}

	// 대칭 해제.
	ScreenAccess::Destruct(*Widget);
	TestFalse(TEXT("Destruct unbinds the wallet"), Wallet->OnMoneyChanged.IsBound());
	TestFalse(TEXT("Destruct unbinds locker capacity"), Lockers->OnLockerCapacityChanged.IsBound());
	TestFalse(TEXT("Destruct unbinds the expansion change"), Fx.Purchase->OnExpansionChanged.IsBound());
	Fx.Destroy();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FExpansionScreenContentContractTest,
	"BathhouseSim.Expansion.Content.ScreenContract",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FExpansionScreenContentContractTest::RunTest(const FString& Parameters)
{
	// Editor 작업 뒤에 실행하는 content 계약이다. Editor 작업 전에는 새 asset이 없어 실패한다.
	const UClass* RootClass = LoadClass<UUserWidget>(nullptr, TEXT("/Game/Bathhouse/UI/WBP_ComputerScreenRoot.WBP_ComputerScreenRoot_C"));
	const UWidgetBlueprintGeneratedClass* RootGenerated = Cast<UWidgetBlueprintGeneratedClass>(RootClass);
	const UWidgetTree* Tree = RootGenerated ? RootGenerated->GetWidgetTreeArchetype() : nullptr;
	if (TestNotNull(TEXT("WBP_ComputerScreenRoot loads with a widget tree"), Tree))
	{
		const UButton* TabButton = Tree->FindWidget<UButton>(TEXT("ExpansionTabButton"));
		const UExpansionScreenWidget* Screen = Tree->FindWidget<UExpansionScreenWidget>(TEXT("ExpansionScreen"));
		const UWidgetSwitcher* Switcher = Tree->FindWidget<UWidgetSwitcher>(TEXT("ScreenSwitcher"));
		TestNotNull(TEXT("ExpansionTabButton exists in the root"), TabButton);
		TestNotNull(TEXT("ExpansionScreen exists in the root"), Screen);
		if (TestNotNull(TEXT("ScreenSwitcher exists in the root"), Switcher))
		{
			TestEqual(TEXT("The screen switcher has three screens"), Switcher->GetChildrenCount(), 3);
			if (Switcher->GetChildrenCount() == 3)
			{
				TestTrue(TEXT("The third screen is the expansion screen"), Switcher->GetChildAt(2) == Screen);
			}
		}
	}
	const UClass* ScreenClass = LoadClass<UExpansionScreenWidget>(nullptr, TEXT("/Game/Bathhouse/UI/WBP_ExpansionScreen.WBP_ExpansionScreen_C"));
	TestNotNull(TEXT("WBP_ExpansionScreen loads with the native parent"), ScreenClass);
	// 화면 헤더·정보 영역의 단계·가격 글자는 선택지 카드로 옮겨졌다(D2, Editor 작업 뒤 계약).
	if (const UWidgetBlueprintGeneratedClass* ScreenGenerated = Cast<UWidgetBlueprintGeneratedClass>(ScreenClass))
	{
		if (const UWidgetTree* ScreenTree = ScreenGenerated->GetWidgetTreeArchetype())
		{
			TestNull(TEXT("WBP_ExpansionScreen no longer has StageText"), ScreenTree->FindWidget(TEXT("StageText")));
			TestNull(TEXT("WBP_ExpansionScreen no longer has PriceText"), ScreenTree->FindWidget(TEXT("PriceText")));
		}
	}
	const UClass* OptionClass = LoadClass<UExpansionSpaceOptionWidget>(nullptr, TEXT("/Game/Bathhouse/UI/WBP_ExpansionSpaceOption.WBP_ExpansionSpaceOption_C"));
	TestNotNull(TEXT("WBP_ExpansionSpaceOption loads with the native parent"), OptionClass);
	if (const UWidgetBlueprintGeneratedClass* OptionGenerated = Cast<UWidgetBlueprintGeneratedClass>(OptionClass))
	{
		if (const UWidgetTree* OptionTree = OptionGenerated->GetWidgetTreeArchetype())
		{
			TestNotNull(TEXT("WBP_ExpansionSpaceOption has StageText"), OptionTree->FindWidget<UTextBlock>(TEXT("StageText")));
			TestNotNull(TEXT("WBP_ExpansionSpaceOption has PriceText"), OptionTree->FindWidget<UTextBlock>(TEXT("PriceText")));
		}
	}

	const UShopCatalog* Catalog = LoadObject<UShopCatalog>(nullptr, TEXT("/Game/Bathhouse/Data/Shop/DA_ShopCatalog.DA_ShopCatalog"));
	if (TestNotNull(TEXT("DA_ShopCatalog loads"), Catalog) && TestTrue(TEXT("The catalog has products"), Catalog->Products.Num() > 0))
	{
		// 맨 뒤 세 상품은 1·4·8칸 락커 순서, 판매 중이고 가격 > 0. 4·8칸 정의는 칸 수·Discardable 없음이 계약이다.
		if (TestTrue(TEXT("The catalog ends with the 1, 4 and 8 slot lockers"), Catalog->Products.Num() >= 3))
		{
			const TCHAR* DefinitionPaths[3] = {
				TEXT("/Game/Bathhouse/Data/Placement/DA_FacilityPlacement_ClothesLocker_1.DA_FacilityPlacement_ClothesLocker_1"),
				TEXT("/Game/Bathhouse/Data/Placement/DA_FacilityPlacement_ClothesLocker_4.DA_FacilityPlacement_ClothesLocker_4"),
				TEXT("/Game/Bathhouse/Data/Placement/DA_FacilityPlacement_ClothesLocker_8.DA_FacilityPlacement_ClothesLocker_8") };
			for (int32 Index = 0; Index < 3; ++Index)
			{
				const FShopProductEntry& Product = Catalog->Products[Catalog->Products.Num() - 3 + Index];
				const UFacilityPlacementDefinition* Definition = LoadObject<UFacilityPlacementDefinition>(nullptr, DefinitionPaths[Index]);
				if (TestNotNull(*FString::Printf(TEXT("Locker definition %d loads"), Index), Definition))
				{
					TestTrue(*FString::Printf(TEXT("Catalog product %d uses locker definition %d"), Index, Index), Product.PlacementDefinition == Definition);
					TestTrue(*FString::Printf(TEXT("Locker definition %d has slots"), Index), Definition->LockerSlotCount > 0);
					TestFalse(*FString::Printf(TEXT("Locker definition %d is not discardable"), Index),
						Definition->FacilityTags.HasTagExact(TAG_Facility_Discardable));
				}
				TestTrue(*FString::Printf(TEXT("Locker product %d is for sale with a positive price"), Index), Product.bForSale && Product.Price > 0);
			}
			const UFacilityPlacementDefinition* FourSlot = Catalog->Products[Catalog->Products.Num() - 2].PlacementDefinition;
			const UFacilityPlacementDefinition* EightSlot = Catalog->Products[Catalog->Products.Num() - 1].PlacementDefinition;
			TestTrue(TEXT("The second last product has four slots"), FourSlot && FourSlot->LockerSlotCount == 4);
			TestTrue(TEXT("The last product has eight slots"), EightSlot && EightSlot->LockerSlotCount == 8);
		}
	}
	const UBathhouseExpansionDefinition* Expansion = LoadObject<UBathhouseExpansionDefinition>(
		nullptr, TEXT("/Game/Bathhouse/Data/Expansion/DA_BathhouseExpansion_Default.DA_BathhouseExpansion_Default"));
	if (TestNotNull(TEXT("The expansion definition loads"), Expansion))
	{
		FText Reason;
		const bool bDataValid = Expansion->ValidatePurchaseData(Reason);
		TestTrue(*FString::Printf(TEXT("The expansion definition passes the purchase data rules (%s)"), *Reason.ToString()), bDataValid);
	}
	return true;
}

#endif
