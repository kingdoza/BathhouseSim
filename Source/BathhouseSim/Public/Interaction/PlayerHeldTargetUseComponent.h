#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Interaction/InteractionTypes.h"
#include "PlayerHeldTargetUseComponent.generated.h"

class IPlayerInteractable;
class UPlayerCarryComponent;
class UPlayerEquipmentUseComponent;
class UPlayerInteractionComponent;
class UObject;

UCLASS(ClassGroup = (Interaction), meta = (BlueprintSpawnableComponent))
class BATHHOUSESIM_API UPlayerHeldTargetUseComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UPlayerHeldTargetUseComponent();

	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	void Configure(
		UPlayerInteractionComponent* InInteraction,
		UPlayerCarryComponent* InCarry,
		UPlayerEquipmentUseComponent* InEquipmentUse);

	void BeginUse(EPlayerHeldTargetUseDirection Direction);
	void EndUse();
	void CancelUse();
	bool IsUseActive() const { return bUseActive; }

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Interaction|Held Target Use", meta = (ClampMin = "0.05", UIMin = "0.05"))
	float RepeatIntervalSeconds = 0.15f;

private:
	friend class FBathhouseHeldTargetUseRepeatTest;
	friend class FBathhouseHeldTargetUseOwnerRoutingTest;

	bool IsLocallyControlledOwner() const;
	bool IsReadyForUse() const;
	void TickRepeat(float DeltaTime);
	void StopRepeating();
	void ClearUseState();

	UPROPERTY(Transient)
	TObjectPtr<UPlayerInteractionComponent> InteractionComponent = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UPlayerCarryComponent> CarryComponent = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UPlayerEquipmentUseComponent> EquipmentUseComponent = nullptr;

	TWeakObjectPtr<UObject> RepeatTarget;
	TWeakObjectPtr<AActor> RepeatHeldObject;
	EPlayerHeldTargetUseDirection ActiveDirection = EPlayerHeldTargetUseDirection::Apply;
	float RepeatElapsedSeconds = 0.0f;
	int32 RepeatTargetKey = INDEX_NONE;
	bool bUseActive = false;
	bool bRepeatActive = false;
};