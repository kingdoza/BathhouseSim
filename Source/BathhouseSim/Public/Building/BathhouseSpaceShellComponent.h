#pragma once

#include "CoreMinimal.h"
#include "Building/BathhouseSpaceTypes.h"
#include "Components/SceneComponent.h"
#include "BathhouseSpaceShellComponent.generated.h"

class UInstancedStaticMeshComponent;
class UMaterialInterface;
class UStaticMesh;
struct FBathhouseSpacePlan;

/** 공간 형상 생성에 필요한 표현 입력(authored 값과 Settings에서 모은 값). */
struct FBathhouseShellVisualInputs
{
	UStaticMesh* BoxMesh = nullptr;
	UMaterialInterface* WallMaterial = nullptr;
	UMaterialInterface* FloorMaterial = nullptr;
	UMaterialInterface* CeilingMaterial = nullptr;
	UMaterialInterface* StepMaterial = nullptr;
	UMaterialInterface* StairWallMaterial = nullptr;
	FBathhouseSpaceLighting Lighting;
	/** 편집 world에서만 조각 미리보기 선을 만든다. */
	bool bPreviewChunks = false;
	EBathhouseCleaningChunkKind ChunkKind = EBathhouseCleaningChunkKind::None;
	/** 조각 미리보기 선 상자의 높이 절반(바닥 윗면 위). */
	float ChunkPreviewHalfHeightCm = 0.0f;
};

/**
 * 공간 하나의 생성 component(part별 ISM, 조명, 편집용 조각 미리보기)를 소유한다.
 * 모두 RF_Transient이며 저장하지 않고, 재생성은 항상 전부 파괴 후 새로 만든다.
 */
UCLASS(ClassGroup = (Bathhouse), meta = (BlueprintSpawnableComponent = false))
class BATHHOUSESIM_API UBathhouseSpaceShellComponent : public USceneComponent
{
	GENERATED_BODY()

public:
	UBathhouseSpaceShellComponent();

	/** 이전 생성물을 모두 파괴하고 Plan대로 새로 만든다. Plan의 좌표는 world다. */
	void Rebuild(const FBathhouseSpacePlan& Plan, const FBathhouseShellVisualInputs& Inputs);
	void ClearGenerated();

	int32 GetGeneratedComponentCount() const;
	UInstancedStaticMeshComponent* FindPartComponent(EBathhouseShellPart Part) const;
	int32 GetPartInstanceCount(EBathhouseShellPart Part) const;
	int32 GetLightCount() const { return LightComponents.Num(); }
	int32 GetChunkPreviewCount() const { return ChunkPreviewComponents.Num(); }

protected:
	virtual void OnComponentDestroyed(bool bDestroyingHierarchy) override;

private:
	UPROPERTY(Transient)
	TArray<TObjectPtr<UInstancedStaticMeshComponent>> PartComponents;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UActorComponent>> LightComponents;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UActorComponent>> ChunkPreviewComponents;
};
