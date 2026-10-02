#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Building/BathhouseSpaceTypes.h"
#include "Computer/ComputerScreenContext.h"
#include "Misc/Optional.h"
#include "ExpansionScreenWidget.generated.h"

class ABathhousePlayerState;
class UBathhouseExpansionPurchaseSubsystem;
class UButton;
class ULockerCapacitySubsystem;
class UExpansionSpaceOptionWidget;
class UPanelWidget;
class UPlayerWalletComponent;
class UTextBlock;
struct FExpansionScreenDisplay;

/**
 * 컴퓨터 `확장` 탭. 구입 가능 여부와 값은 매번 UBathhouseExpansionPurchaseSubsystem에서 받고
 * 이 위젯은 선택·확인 대기·완료 표시 같은 표시 상태만 가진다. Tick과 timer는 쓰지 않는다.
 */
UCLASS(Abstract, Blueprintable)
class BATHHOUSESIM_API UExpansionScreenWidget
	: public UUserWidget
	, public IComputerScreenContextReceiver
{
	GENERATED_BODY()

public:
	virtual void InitializeComputerScreen(const FComputerScreenContext& Context) override;
	virtual void NotifyComputerUserChanged(APlayerState* PlayerState) override;
	virtual void NotifyComputerUseEnded() override;

	/** domain에서 view를 다시 받아 화면에 적용한다. transaction 중이면 적용하지 않는다. */
	void RefreshFromDomain();
	/** 확인 대기 중이면 대기만 취소하고 화면을 갱신한다. */
	void CancelPendingConfirm();

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> BalanceText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> LockerText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UPanelWidget> OptionsPanel;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UExpansionSpaceOptionWidget> HallOption;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UExpansionSpaceOptionWidget> BathOption;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UExpansionSpaceOptionWidget> WorkOption;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UPanelWidget> PurchasePanel;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> PurchaseButton;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> PurchaseButtonText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UPanelWidget> ConfirmPanel;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> ConfirmButton;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> CancelButton;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> ShortfallText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> ResultText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> MessageText;

private:
	friend class FExpansionScreenAutomationAccess;

	UFUNCTION()
	void HandlePurchaseClicked();

	UFUNCTION()
	void HandleConfirmClicked();

	UFUNCTION()
	void HandleCancelClicked();

	UFUNCTION()
	void HandleMoneyChanged(int32 PreviousMoney, int32 CurrentMoney);

	UFUNCTION()
	void HandleLockerCapacityChanged(int32 InstalledCapacity, int32 ActiveLeaseCount);

	void HandleExpansionChanged();
	void HandleOptionClicked(EBathhouseSpaceKind Kind);
	void RebindDomain();
	void UnbindDomain();
	void ApplyDisplay(const FExpansionScreenDisplay& Display);

	FComputerScreenContext ScreenContext;
	TWeakObjectPtr<ABathhousePlayerState> CurrentPlayerState;
	TWeakObjectPtr<UPlayerWalletComponent> Wallet;
	TWeakObjectPtr<ULockerCapacitySubsystem> Lockers;
	TWeakObjectPtr<UBathhouseExpansionPurchaseSubsystem> Purchase;
	FDelegateHandle ExpansionChangedHandle;
	FDelegateHandle OptionHandles[3];

	// 위젯 표시 상태. domain 값은 보관하지 않는다.
	TOptional<EBathhouseSpaceKind> SelectedKind;
	bool bConfirmPending = false;
	int32 ConfirmAppliedCount = 0;
	bool bShowCompleted = false;
	bool bButtonsBound = false;
};
