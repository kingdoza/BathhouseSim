#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Blueprint/WidgetBlueprintGeneratedClass.h"
#include "Blueprint/WidgetTree.h"
#include "Building/BathhouseExpansionPurchaseSubsystem.h"
#include "Building/BathhouseExpansionTypes.h"
#include "Components/Button.h"
#include "Components/WidgetSwitcher.h"
#include "Economy/PlayerWalletComponent.h"
#include "Facility/BathhouseExpansionDefinition.h"
#include "Facility/LockerCapacitySubsystem.h"
#include "Placement/FacilityPlacementDefinition.h"
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
	static int32 PendingCount(const UExpansionScreenWidget& Widget) { return Widget.ConfirmPurchaseCount; }
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
		View.PurchaseCount = 0;
		View.MaxPurchaseCount = 3;
		View.NextPrice = 800;
		View.Balance = 5000;
		View.Shortfall = FMath::Max(0, View.NextPrice - View.Balance);
		View.InstalledLockerSlots = 2;
		View.LockerSlotLimit = 2;
		for (int32 Index = 0; Index < 3; ++Index)
		{
			View.Options[Index].bPresent = true;
			View.Options[Index].bCanExpand = true;
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
		TestFalse(TEXT("Unavailable hides the price"), Display.bPriceVisible);
		TestFalse(TEXT("Unavailable hides the purchase panel"), Display.bPurchasePanelVisible);
		TestFalse(TEXT("Unavailable hides the confirm panel"), Display.bConfirmPanelVisible);
		TestFalse(TEXT("Unavailable hides the shortfall"), Display.bShortfallVisible);
		TestFalse(TEXT("Unavailable hides the stage"), Display.bStageVisible);
		TestFalse(TEXT("Unavailable hides the locker line"), Display.bLockerVisible);
		TestTrue(TEXT("Unavailable keeps the balance"), Display.bBalanceVisible);
		TestTrue(TEXT("Unavailable clears the selection"), Display.bClearSelection);
	}
	// 최대 도달.
	{
		FBathhouseExpansionView View = MakeView();
		View.PurchaseCount = View.MaxPurchaseCount;
		View.bMaxReached = true;
		const FExpansionScreenDisplay Display = FExpansionScreenModel::Build(View, MakeState(Hall, false, true));
		TestEqual(TEXT("Maximum message"), Str(Display.Message), FString(TEXT("최대 확장 단계입니다")));
		TestFalse(TEXT("Maximum hides the options"), Display.bOptionsVisible);
		TestFalse(TEXT("Maximum hides the purchase panel"), Display.bPurchasePanelVisible);
		TestFalse(TEXT("Maximum hides the price"), Display.bPriceVisible);
		TestTrue(TEXT("Maximum keeps the stage"), Display.bStageVisible);
		TestTrue(TEXT("Maximum keeps the locker line"), Display.bLockerVisible);
		TestTrue(TEXT("Maximum keeps the completed text"), Display.bResultVisible);
		TestEqual(TEXT("Completed text"), Str(Display.Result), FString(TEXT("확장 완료")));
		TestTrue(TEXT("Maximum clears the selection"), Display.bClearSelection);
	}
	// EXP-021 선택 전.
	{
		const FBathhouseExpansionView View = MakeView();
		const FExpansionScreenDisplay Display = FExpansionScreenModel::Build(View, MakeState(None));
		TestEqual(TEXT("Stage text"), Str(Display.Stage), FString(TEXT("현재 확장 단계: 0")));
		TestEqual(TEXT("Locker text"), Str(Display.Locker), FString(TEXT("설치된 락커 칸 2/2")));
		TestTrue(TEXT("Balance text"), Str(Display.Balance).StartsWith(TEXT("잔액 ")) && Str(Display.Balance).EndsWith(TEXT("원")));
		TestTrue(TEXT("Price text"), Str(Display.Price).StartsWith(TEXT("이번 구입 가격 ")));
		TestTrue(TEXT("Purchase button text"), Str(Display.PurchaseButtonText).StartsWith(TEXT("확장 구입 (")) && Str(Display.PurchaseButtonText).EndsWith(TEXT("원)")));
		TestTrue(TEXT("Options are visible"), Display.bOptionsVisible);
		TestTrue(TEXT("The purchase panel is shown"), Display.bPurchasePanelVisible);
		TestFalse(TEXT("The purchase button is off without a selection"), Display.bPurchaseEnabled);
		TestFalse(TEXT("The confirm panel is hidden"), Display.bConfirmPanelVisible);
		TestFalse(TEXT("No shortfall text with enough money"), Display.bShortfallVisible);
		TestFalse(TEXT("No completed text before a purchase"), Display.bResultVisible);
		TestFalse(TEXT("Nothing to clear without a selection"), Display.bClearSelection);
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
		Display = FExpansionScreenModel::Build(View, MakeState(Bath));
		TestTrue(TEXT("Selecting another space moves the highlight"), Display.Options[1].bSelected && !Display.Options[0].bSelected);

		Display = FExpansionScreenModel::Build(View, MakeState(Hall, true));
		TestTrue(TEXT("Confirm pending shows the confirm panel"), Display.bConfirmPanelVisible);
		TestFalse(TEXT("Confirm pending hides the purchase panel"), Display.bPurchasePanelVisible);
		TestTrue(TEXT("Confirm is enabled when purchasable"), Display.bConfirmEnabled);

		View.Balance = View.NextPrice - 250;
		View.Shortfall = View.NextPrice - View.Balance;
		Display = FExpansionScreenModel::Build(View, MakeState(Hall));
		TestFalse(TEXT("A shortfall disables the purchase"), Display.bPurchaseEnabled);
		TestTrue(TEXT("The shortfall text is visible"), Display.bShortfallVisible);
		TestEqual(TEXT("Shortfall text"), Str(Display.Shortfall), FString::Printf(TEXT("%s 부족"), *Str(FExpansionScreenModel::FormatMoney(250))));
		Display = FExpansionScreenModel::Build(View, MakeState(None));
		TestTrue(TEXT("The shortfall shows regardless of the selection"), Display.bShortfallVisible);
		Display = FExpansionScreenModel::Build(View, MakeState(Hall, true));
		TestTrue(TEXT("A confirm pending screen stays"), Display.bConfirmPanelVisible);
		TestFalse(TEXT("Money lost during confirm disables the confirm button"), Display.bConfirmEnabled);

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
	int32 Price = 0;
	Fx.Definition->TryGetPurchasePrice(0, Price);
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
	TestEqual(TEXT("The confirmation remembers the purchase count"), ScreenAccess::PendingCount(*Widget), Fx.Purchase->GetPurchaseCount());

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
	TestEqual(TEXT("Cancelling expands nothing"), Fx.Purchase->GetPurchaseCount(), 0);
	Widget->CancelPendingConfirm();
	TestEqual(TEXT("A second cancel is a no-op"), Fx.Purchase->GetPurchaseCount(), 0);

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
	TestEqual(TEXT("Two confirms expand once"), Fx.Purchase->GetPurchaseCount(), 1);
	TestTrue(TEXT("The completed text is flagged"), ScreenAccess::IsCompleted(*Widget));
	TestFalse(TEXT("A successful purchase clears the selection"), ScreenAccess::HasSelection(*Widget));
	TestFalse(TEXT("No confirmation remains"), ScreenAccess::IsPending(*Widget));
	ScreenAccess::ClickOption(*Widget, EBathhouseSpaceKind::Bath);
	TestFalse(TEXT("Selecting again clears the completed flag"), ScreenAccess::IsCompleted(*Widget));

	// 돈 부족이면 확인 대기로 가지 않는다(EXP-022).
	int32 SecondPrice = 0;
	Fx.Definition->TryGetPurchasePrice(1, SecondPrice);
	Wallet->TrySpendMoney(Wallet->GetCurrentMoney() - (SecondPrice - 1));
	ScreenAccess::ClickPurchase(*Widget);
	TestFalse(TEXT("A shortfall blocks the confirmation"), ScreenAccess::IsPending(*Widget));
	Wallet->TryAddMoney(1);
	ScreenAccess::ClickPurchase(*Widget);
	TestTrue(TEXT("Money arriving later allows the confirmation"), ScreenAccess::IsPending(*Widget));

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
	const UClass* OptionClass = LoadClass<UExpansionSpaceOptionWidget>(nullptr, TEXT("/Game/Bathhouse/UI/WBP_ExpansionSpaceOption.WBP_ExpansionSpaceOption_C"));
	TestNotNull(TEXT("WBP_ExpansionSpaceOption loads with the native parent"), OptionClass);

	const UShopCatalog* Catalog = LoadObject<UShopCatalog>(nullptr, TEXT("/Game/Bathhouse/Data/Shop/DA_ShopCatalog.DA_ShopCatalog"));
	if (TestNotNull(TEXT("DA_ShopCatalog loads"), Catalog) && TestTrue(TEXT("The catalog has products"), Catalog->Products.Num() > 0))
	{
		const FShopProductEntry& Last = Catalog->Products.Last();
		const UFacilityPlacementDefinition* OneSlot = LoadObject<UFacilityPlacementDefinition>(
			nullptr, TEXT("/Game/Bathhouse/Data/Placement/DA_FacilityPlacement_ClothesLocker_1.DA_FacilityPlacement_ClothesLocker_1"));
		TestNotNull(TEXT("The one-slot locker definition loads"), OneSlot);
		TestTrue(TEXT("The last catalog product is the one-slot locker"), OneSlot && Last.PlacementDefinition == OneSlot);
		TestTrue(TEXT("The one-slot locker product is for sale with a positive price"), Last.bForSale && Last.Price > 0);
	}
	const UBathhouseExpansionDefinition* Expansion = LoadObject<UBathhouseExpansionDefinition>(
		nullptr, TEXT("/Game/Bathhouse/Data/Expansion/DA_BathhouseExpansion_Default.DA_BathhouseExpansion_Default"));
	if (TestNotNull(TEXT("The expansion definition loads"), Expansion))
	{
		FText Reason;
		TestTrue(*FString::Printf(TEXT("The expansion definition passes the purchase data rules (%s)"), *Reason.ToString()),
			Expansion->ValidatePurchaseData(Reason));
	}
	return true;
}

#endif
