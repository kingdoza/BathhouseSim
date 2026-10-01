#include "UI/ScrubFocusHudWidget.h"
#include "Service/PlayerScrubFocusComponent.h"
#include "Service/ScrubTableActor.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"

void UScrubFocusHudWidget::SetFocusComponent(UPlayerScrubFocusComponent* Focus)
{
	FocusComponent = Focus;
	SetVisibility(ESlateVisibility::HitTestInvisible);
	SetRenderOpacity(0.f);
}

void UScrubFocusHudWidget::NativeTick(const FGeometry& Geometry, float Delta)
{
	Super::NativeTick(Geometry, Delta);
	auto* Focus = FocusComponent.Get();
	auto* Table = Focus ? Focus->GetActiveTable() : nullptr;
	const bool Active = Table && Focus->GetPhase() == EPlayerScrubFocusPhase::Active;
	// Keep the Slate host tickable so later entries are observed while its content is transparent.
	SetRenderOpacity(Active ? 1.f : 0.f);
	if (Active)
	{
		if (ScrubGaugeBar)
		{
			ScrubGaugeBar->SetPercent(Table->GetScrubProgress());
		}
		if (ScrubWaitText)
		{
			ScrubWaitText->SetText(FText::Format(FText::FromString(TEXT("대기 {0}초")),
												 FText::AsNumber(FMath::CeilToInt(Table->GetRemainingWaitSeconds()))));
		}
	}
}

void UScrubFocusHudWidget::NativeDestruct()
{
	FocusComponent.Reset();
	Super::NativeDestruct();
}
