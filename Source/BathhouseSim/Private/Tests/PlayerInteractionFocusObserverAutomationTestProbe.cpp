#include "Tests/PlayerInteractionFocusObserverAutomationTestProbe.h"

#include "Components/BoxComponent.h"
#include "Interaction/PlayerInteractionComponent.h"

#define LOCTEXT_NAMESPACE "PlayerInteractionFocusObserverAutomationTarget"

APlayerInteractionFocusObserverAutomationTarget::APlayerInteractionFocusObserverAutomationTarget()
{
	PrimaryActorTick.bCanEverTick = false;
	FocusBox = CreateDefaultSubobject<UBoxComponent>(TEXT("FocusBox"));
	SetRootComponent(FocusBox);
	FocusBox->SetBoxExtent(FVector(15.0f));
	FocusBox->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	FocusBox->SetCollisionObjectType(ECC_WorldDynamic);
	FocusBox->SetCollisionResponseToAllChannels(ECR_Ignore);
	FocusBox->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
	FocusBox->SetCanEverAffectNavigation(false);
}

FPlayerInteractionQuery APlayerInteractionFocusObserverAutomationTarget::QueryInteraction(
	const FPlayerInteractionContext& Context) const
{
	(void)Context;
	FPlayerInteractionQuery Query;
	Query.bVisible = true;
	Query.bCanInteract = true;
	Query.TargetName = LOCTEXT("TargetName", "focus observer test target");
	Query.ActionName = FText::FromString(FString::Printf(TEXT("Action %d"), QueryRevision));
	return Query;
}

FPlayerInteractionResult APlayerInteractionFocusObserverAutomationTarget::ExecuteInteraction(
	const FPlayerInteractionContext& Context)
{
	(void)Context;
	return FPlayerInteractionResult::Succeeded();
}

void APlayerInteractionFocusObserverAutomationTarget::NotifyInteractionFocusChanged(
	const UPlayerInteractionComponent& Source,
	const FPlayerInteractionQuery& Query)
{
	(void)Query;
	++ChangedCount;
	if (EventLog)
	{
		EventLog->Add(Label + TEXT(".Changed"));
	}
	if (bClearDuringNextNotification)
	{
		bClearDuringNextNotification = false;
		const_cast<UPlayerInteractionComponent&>(Source).ClearInteractionQuery();
	}
}

void APlayerInteractionFocusObserverAutomationTarget::NotifyInteractionFocusEnded(
	const UPlayerInteractionComponent& Source)
{
	(void)Source;
	++EndedCount;
	if (EventLog)
	{
		EventLog->Add(Label + TEXT(".Ended"));
	}
}

void APlayerInteractionFocusObserverAutomationTarget::ConfigureEventLog(
	TArray<FString>* InEventLog,
	const FString& InLabel)
{
	EventLog = InEventLog;
	Label = InLabel;
}

#undef LOCTEXT_NAMESPACE
