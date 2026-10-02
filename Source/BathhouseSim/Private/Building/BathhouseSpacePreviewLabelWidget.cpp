#include "Building/BathhouseSpacePreviewLabelWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/TextBlock.h"
#include "Fonts/SlateFontInfo.h"

bool UBathhouseSpacePreviewLabelWidget::Initialize()
{
	const bool bResult = Super::Initialize();
	// native class라 tree가 비어 있다. NativeOnInitialized는 편집 world(player context 없음)에서 불리지 않으므로 여기서 만든다.
	if (!WidgetTree)
	{
		WidgetTree = NewObject<UWidgetTree>(this, TEXT("WidgetTree"), RF_Transient);
	}
	if (WidgetTree && !WidgetTree->RootWidget)
	{
		WidgetTree->RootWidget = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("LabelText"));
	}
	return bResult;
}

void UBathhouseSpacePreviewLabelWidget::SetLabel(const FText& Text, const int32 FontSize)
{
	UTextBlock* Block = WidgetTree ? Cast<UTextBlock>(WidgetTree->RootWidget) : nullptr;
	if (!Block)
	{
		return;
	}
	Block->SetText(Text);
	Block->SetJustification(ETextJustify::Center);
	FSlateFontInfo Font = Block->GetFont();
	Font.Size = FontSize;
	Block->SetFont(Font);
}
