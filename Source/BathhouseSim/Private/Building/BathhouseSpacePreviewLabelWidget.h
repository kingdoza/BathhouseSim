#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "BathhouseSpacePreviewLabelWidget.generated.h"

/**
 * 편집 world 전용 넓힘 미리보기 글자. 코드로 만든 UTextBlock 하나만 가진다(WBP·asset 없음).
 * 글꼴 object는 UTextBlock 기본(엔진 UMG 글꼴)을 그대로 쓰고 크기만 정한다.
 */
UCLASS(NotBlueprintable)
class UBathhouseSpacePreviewLabelWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual bool Initialize() override;

	/** 문구를 넣고 가운데 정렬한다. FontSize는 글꼴 크기(렌더 해상도)다. */
	void SetLabel(const FText& Text, int32 FontSize);
};
