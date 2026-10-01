#pragma once
#include "Blueprint/UserWidget.h"
#include "ScrubFocusHudWidget.generated.h"
class UPlayerScrubFocusComponent;
class UProgressBar;
class UTextBlock;
UCLASS(Abstract, Blueprintable)

class BATHHOUSESIM_API UScrubFocusHudWidget : public UUserWidget
{
	GENERATED_BODY()
public:

	void SetFocusComponent(UPlayerScrubFocusComponent* Focus);

protected:

	virtual void NativeTick(const FGeometry& Geometry, float Delta) override;
	virtual void NativeDestruct() override;
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UProgressBar> ScrubGaugeBar;
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> ScrubWaitText;

private:

	UPROPERTY(Transient)
	TWeakObjectPtr<UPlayerScrubFocusComponent> FocusComponent;
};
