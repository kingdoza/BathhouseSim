#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Interaction/InteractionTypes.h"
#include "UtilityLeverLaborComponent.generated.h"

class UPlayerCarryComponent;
class UPlayerInteractionComponent;
class USceneComponent;
class UUtilityLeverOperatingVolumeComponent;
class UUtilityOperationComponent;
class FUtilityPivotRotation;

enum class EUtilityLeverLaborState : uint8
{
	Idle,
	Stroking,
	Returning
};

UCLASS(ClassGroup = (Bathhouse), meta = (BlueprintSpawnableComponent))
class BATHHOUSESIM_API UUtilityLeverLaborComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UUtilityLeverLaborComponent();
	virtual ~UUtilityLeverLaborComponent();
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	void Configure(UUtilityOperationComponent* InOperation, USceneComponent* InPivot);
	void SetOperatingVolume(UUtilityLeverOperatingVolumeComponent* InVolume);
	FPlayerInteractionQuery QueryInteraction(const FPlayerInteractionContext& Context) const;
	bool EvaluateStart(const FPlayerInteractionContext& Context, FText& OutFailureReason) const;
	bool TryStartStroke(const FPlayerInteractionContext& Context, FText& OutFailureReason);
	void NotifySourceFocusEnded(const UPlayerInteractionComponent& Source);
	void CancelForRecoveryStart();
	bool HasValidAuthoring(FText& OutFailureReason) const;
	UFUNCTION(CallInEditor, Category = "Utility|Lever")
	void PreviewDownPose();
	UFUNCTION(CallInEditor, Category = "Utility|Lever")
	void RestoreUpPose();

	EUtilityLeverLaborState GetState() const { return State; }
	float GetProgress() const;
	float GetPoseAlpha() const { return PoseAlpha; }

#if WITH_EDITOR
	virtual void PreSave(FObjectPreSaveContext SaveContext) override;
#endif

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Utility|Lever", meta = (ClampMin = "0.01"))
	float StrokeSeconds = 1.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Utility|Lever", meta = (ClampMin = "0.01"))
	float StrokeRewardPoints = 10.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Utility|Lever", meta = (ClampMin = "0.0"))
	float CancelReturnSeconds = 0.2f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Utility|Lever")
	FVector LocalRotationAxis = FVector(0.0f, 1.0f, 0.0f);

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Utility|Lever")
	float DownAngleDegrees = -60.0f;

private:
	bool IsEditorPreviewWorld() const;
	void RestoreUpPoseInternal();
	bool IsStrokeContextValid(FText& OutFailureReason) const;
	bool IsSameActiveSource(const UPlayerInteractionComponent* Source) const;
	void StartReturn();
	void ApplyPose();
	void ClearActiveSource();

	UPROPERTY(Transient)
	TObjectPtr<UUtilityOperationComponent> Operation = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<USceneComponent> Pivot = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UUtilityLeverOperatingVolumeComponent> OperatingVolume = nullptr;

	TWeakObjectPtr<UPlayerInteractionComponent> ActiveSource;
	TWeakObjectPtr<UPlayerCarryComponent> ActiveCarry;
	TWeakObjectPtr<AActor> ActiveUser;
	FUtilityPivotRotation* PivotRotation = nullptr;
	EUtilityLeverLaborState State = EUtilityLeverLaborState::Idle;
	float ElapsedSeconds = 0.0f;
	float ReturnElapsedSeconds = 0.0f;
	float ReturnStartAlpha = 0.0f;
	float PoseAlpha = 0.0f;
	UPROPERTY(Transient)
	bool bEditorPreviewPose = false;
};
