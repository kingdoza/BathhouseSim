#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "FacilityPlacementPreviewActor.generated.h"

class USceneComponent;
class UMaterialInterface;
class UMaterialInstanceDynamic;
class UStaticMeshComponent;

UCLASS(Blueprintable, NotPlaceable)
class BATHHOUSESIM_API AFacilityPlacementPreviewActor : public AActor
{
	GENERATED_BODY()

public:
	AFacilityPlacementPreviewActor();
	bool InitializeFromPlacedClass(TSubclassOf<AActor> PlacedClass, FText& OutFailureReason);
	bool ValidateSourceGeometry(TSubclassOf<AActor> PlacedClass, FText& OutFailureReason) const;
	void SetPlacementValidity(bool bValid, const FText& FailureReason);
	const TArray<TObjectPtr<UStaticMeshComponent>>& GetPreviewMeshes() const { return PreviewMeshes; }
	const UStaticMeshComponent* GetFootprintSurface() const { return FootprintSurface; }
	const UMaterialInstanceDynamic* GetFootprintMaterial() const { return FootprintMaterial; }

	UFUNCTION(BlueprintImplementableEvent, Category = "Facility Placement|Presentation")
	void OnPlacementValidityChanged(bool bValid, const FText& FailureReason);

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Facility Placement")
	TObjectPtr<USceneComponent> SceneRoot;

private:
	bool ApplyPreviewMaterial(UMaterialInterface* Material, FText& OutFailureReason);
	// Display only: failure leaves the mesh preview intact and only logs a warning.
	void TryCreateFootprintSurface();
	bool BuildFootprintSurface(FText& OutFailureReason);
	void DestroyFootprintSurface();

	UPROPERTY(Transient)
	TArray<TObjectPtr<UStaticMeshComponent>> PreviewMeshes;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInterface> ValidMaterial;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInterface> InvalidMaterial;

	UPROPERTY(Transient)
	TObjectPtr<UStaticMeshComponent> FootprintSurface;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> FootprintMaterial;

	UPROPERTY(Transient)
	TSubclassOf<AActor> SourcePlacedClass;

	FLinearColor ValidPreviewColor = FLinearColor::White;
	FLinearColor InvalidPreviewColor = FLinearColor::White;

	FTransform SourceFootprintRelative = FTransform::Identity;
	FVector SourceFootprintExtent = FVector::ZeroVector;
	FVector SourceRootScale = FVector::OneVector;
};
