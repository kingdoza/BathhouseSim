#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "TimerManager.h"
#include "PlayerScrubFocusComponent.generated.h"
class AScrubTableActor;
class UFirstPersonMovementComponent;
class UPlayerInteractionComponent;
class UPlayerCarryComponent;
class APlayerController;
UENUM(BlueprintType)
enum class EPlayerScrubFocusPhase : uint8
{
	Inactive,
	FocusingIn,
	Active,
	FocusingOut
};
UCLASS(ClassGroup = (Service))

class BATHHOUSESIM_API UPlayerScrubFocusComponent : public UActorComponent
{
	GENERATED_BODY()
public:

	UPlayerScrubFocusComponent();
	virtual void EndPlay(EEndPlayReason::Type Reason) override;
	virtual void TickComponent(float Delta, ELevelTick Tick, FActorComponentTickFunction* Function) override;
	void Configure(UFirstPersonMovementComponent* InMovement, UPlayerInteractionComponent* InInteraction,
				   UPlayerCarryComponent* InCarry);
	bool BeginScrubFocus(AScrubTableActor* Table);
	void RequestEndScrubFocus();
	void AddRubInput(FVector2D LookAxis);
	void SetRubbing(bool Rubbing);
	UFUNCTION(BlueprintPure, Category = "Service")

	EPlayerScrubFocusPhase GetPhase() const
	{
		return Phase;
	}

	bool IsCapturingInput() const
	{
		return Phase != EPlayerScrubFocusPhase::Inactive;
	}

	AScrubTableActor* GetActiveTable() const
	{
		return ActiveTable.Get();
	}

	FVector2D GetCursorLocal() const
	{
		return CursorLocal;
	}

private:

	void CompleteFocusIn();
	void CompleteFocusOut();
	void ForceCleanup(bool RestoreView);
	void HandleSessionEnded();
	void UpdateCursor();
	bool HasValidContext() const;
	APlayerController* ResolveController() const;
	UPROPERTY(Transient)
	TObjectPtr<UFirstPersonMovementComponent> Movement;
	UPROPERTY(Transient)
	TObjectPtr<UPlayerInteractionComponent> Interaction;
	UPROPERTY(Transient)
	TObjectPtr<UPlayerCarryComponent> Carry;
	UPROPERTY(Transient)
	TWeakObjectPtr<AScrubTableActor> ActiveTable;
	UPROPERTY(Transient)
	TWeakObjectPtr<APlayerController> SessionController;
	UPROPERTY(Transient)
	TWeakObjectPtr<AActor> PreviousViewTarget;
	EPlayerScrubFocusPhase Phase = EPlayerScrubFocusPhase::Inactive;
	EMovementMode SavedMovementMode = MOVE_None;
	uint8 SavedCustomMovementMode = 0;
	bool bSavedCursorVisible = false;
	bool bSnapshotValid = false;
	bool bRubbing = false;
	FVector2D CursorLocal = FVector2D::ZeroVector;
	FTimerHandle BlendTimer;
	FDelegateHandle SessionEndedHandle;
};
