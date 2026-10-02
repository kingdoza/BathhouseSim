#include "UI/ExpansionSpaceOptionWidget.h"

#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "UI/ExpansionScreenModel.h"

void UExpansionSpaceOptionWidget::NativeConstruct()
{
	Super::NativeConstruct();
	if (SelectButton && !bButtonBound)
	{
		SelectButton->OnClicked.AddDynamic(this, &UExpansionSpaceOptionWidget::HandleSelectClicked);
		bButtonBound = true;
	}
}

void UExpansionSpaceOptionWidget::NativeDestruct()
{
	if (SelectButton && bButtonBound)
	{
		SelectButton->OnClicked.RemoveDynamic(this, &UExpansionSpaceOptionWidget::HandleSelectClicked);
		bButtonBound = false;
	}
	Super::NativeDestruct();
}

void UExpansionSpaceOptionWidget::HandleSelectClicked()
{
	ClickedDelegate.Broadcast(Kind);
}

void UExpansionSpaceOptionWidget::ApplyModel(const FExpansionOptionDisplay& Display)
{
	if (NameText)
	{
		NameText->SetText(Display.Name);
	}
	if (SizeText)
	{
		SizeText->SetText(Display.Size);
	}
	if (EffectText)
	{
		EffectText->SetText(Display.Effect);
		EffectText->SetVisibility(Display.bEffectVisible ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	}
	if (StatusText)
	{
		StatusText->SetText(Display.Status);
		StatusText->SetVisibility(Display.bStatusVisible ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	}
	if (SelectButton)
	{
		SelectButton->SetIsEnabled(Display.bEnabled);
	}
	if (SelectionHighlight)
	{
		SelectionHighlight->SetVisibility(Display.bSelected ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	}
}
