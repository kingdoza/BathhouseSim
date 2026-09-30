#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#if WITH_EDITOR
#include "UObject/ObjectSaveContext.h"
#endif
#include "OpeningPresentationComponent.generated.h"

class FOpeningPivotRotation;
class USceneComponent;

UCLASS(ClassGroup = (Bathhouse), meta = (BlueprintSpawnableComponent))

class BATHHOUSESIM_API UOpeningPresentationComponent : public UActorComponent
{
	GENERATED_BODY()

public:

	UOpeningPresentationComponent();
	virtual ~UOpeningPresentationComponent();
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType,
							   FActorComponentTickFunction* ThisTickFunction) override;

	void Configure(USceneComponent* InPivot);
	void SetSourceInsertable(const UObject* Source, bool bInsertable);
	void RemoveSource(const UObject* Source);
	void ApplyClosedImmediately();
	bool HasValidAuthoring(FText& OutFailureReason) const;

	float GetOpenAlpha() const
	{
		return OpenAlpha;
	}

	float GetTargetAlpha() const
	{
		return TargetOpenAlpha;
	}

	UFUNCTION(CallInEditor, Category = "Interaction|Opening")
	void PreviewOpenPose();

	UFUNCTION(CallInEditor, Category = "Interaction|Opening")
	void RestoreClosedPose();

#if WITH_EDITOR
	virtual void PreSave(FObjectPreSaveContext SaveContext) override;
#endif

protected:

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Interaction|Opening")
	FVector LocalRotationAxis = FVector::UpVector;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Interaction|Opening")
	float OpenAngleDegrees = 90.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Interaction|Opening", meta = (ClampMin = "0.0"))
	float OpenSeconds = 0.2f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Interaction|Opening", meta = (ClampMin = "0.0"))
	float CloseSeconds = 0.2f;

private:

	bool IsEditorPreviewWorld() const;
	void RecalculateTargetAlpha();
	void ApplyCurrentPose();

	UPROPERTY(Transient)
	TObjectPtr<USceneComponent> Pivot = nullptr;

	FOpeningPivotRotation* PivotRotation = nullptr;
	TMap<TWeakObjectPtr<UObject>, bool> SourceInsertability;
	float OpenAlpha = 0.0f;
	float TargetOpenAlpha = 0.0f;
	bool bEditorPreviewPose = false;
};
