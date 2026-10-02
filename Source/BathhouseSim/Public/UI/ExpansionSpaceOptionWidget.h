#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Building/BathhouseSpaceTypes.h"
#include "ExpansionSpaceOptionWidget.generated.h"

class UButton;
class UTextBlock;
struct FExpansionOptionDisplay;

DECLARE_MULTICAST_DELEGATE_OneParam(FOnExpansionOptionClicked, EBathhouseSpaceKind);

/** 확장 탭의 공간 선택지 한 칸. 표시 값은 부모 화면이 모델 결과로 전달하고 클릭은 C++ delegate로 알린다. */
UCLASS(Abstract, Blueprintable)
class BATHHOUSESIM_API UExpansionSpaceOptionWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void SetSpaceKind(EBathhouseSpaceKind InKind) { Kind = InKind; }
	EBathhouseSpaceKind GetSpaceKind() const { return Kind; }
	void ApplyModel(const FExpansionOptionDisplay& Display);
	FOnExpansionOptionClicked& OnClicked() { return ClickedDelegate; }

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> SelectButton;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> NameText;

	/** 선택지별 `확장 단계 N/M`. 기존 WBP에 필수 binding을 더하지 않도록 Optional이다(존재는 content 자동화가 단언). */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> StageText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> SizeText;

	/** 선택지별 `다음 넓힘 N원`. Optional 근거는 StageText와 같다. */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> PriceText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> EffectText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> StatusText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UWidget> SelectionHighlight;

private:
	UFUNCTION()
	void HandleSelectClicked();

	EBathhouseSpaceKind Kind = EBathhouseSpaceKind::Hall;
	FOnExpansionOptionClicked ClickedDelegate;
	bool bButtonBound = false;
};
