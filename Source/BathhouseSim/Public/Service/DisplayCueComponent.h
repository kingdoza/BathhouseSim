#pragma once
#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include "DisplayCueComponent.generated.h"
class UStaticMesh;
class UStaticMeshComponent;
/** Shared presentation only; never mutates stock or inventory. Children are transient. */
UCLASS(ClassGroup = (Bathhouse), meta = (BlueprintSpawnableComponent))

class BATHHOUSESIM_API UDisplayCueComponent : public USceneComponent
{
	GENERATED_BODY()
public:

	UDisplayCueComponent();
	virtual void OnRegister() override;
	virtual void OnUnregister() override;
	virtual void OnComponentDestroyed(bool bDestroyingHierarchy) override;
	virtual void EndPlay(const EEndPlayReason::Type Reason) override;
	void ShowInsertPreview(UStaticMesh* Mesh, const FTransform& RelativeTransform);
	void ShowTakeHighlight(UStaticMesh* Mesh, const FTransform& RelativeTransform);
	void HideInsertPreview();
	void HideTakeHighlight();
	void HideAll();

	UStaticMeshComponent* GetInsertPreview() const
	{
		return InsertPreview;
	}

	UStaticMeshComponent* GetTakeHighlightProxy() const
	{
		return TakeHighlightProxy;
	}

private:

	void EnsureChildren();
	void DestroyChildren();
	UPROPERTY(Transient)
	TObjectPtr<UStaticMeshComponent> InsertPreview;
	UPROPERTY(Transient)
	TObjectPtr<UStaticMeshComponent> TakeHighlightProxy;
	bool bWarnedMissingPreviewMaterial = false;
};
