#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interaction/PlayerInteractable.h"
#include "Interaction/PlayerInteractionFocusObserver.h"
#include "PlayerInteractionFocusObserverAutomationTestProbe.generated.h"

class UBoxComponent;

UCLASS(Transient, NotBlueprintable)
class APlayerInteractionFocusObserverAutomationTarget final
	: public AActor
	, public IPlayerInteractable
	, public IPlayerInteractionFocusObserver
{
	GENERATED_BODY()

public:
	APlayerInteractionFocusObserverAutomationTarget();

	virtual FPlayerInteractionQuery QueryInteraction(const FPlayerInteractionContext& Context) const override;
	virtual FPlayerInteractionResult ExecuteInteraction(const FPlayerInteractionContext& Context) override;
	virtual void NotifyInteractionFocusChanged(
		const UPlayerInteractionComponent& Source,
		const FPlayerInteractionQuery& Query) override;
	virtual void NotifyInteractionFocusEnded(const UPlayerInteractionComponent& Source) override;

	void ConfigureEventLog(TArray<FString>* InEventLog, const FString& InLabel);
	void SetQueryRevision(int32 InRevision) { QueryRevision = InRevision; }
	void ClearInteractionDuringNextNotification() { bClearDuringNextNotification = true; }

	int32 ChangedCount = 0;
	int32 EndedCount = 0;

private:
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UBoxComponent> FocusBox;

	TArray<FString>* EventLog = nullptr;
	FString Label;
	int32 QueryRevision = 0;
	bool bClearDuringNextNotification = false;
};
