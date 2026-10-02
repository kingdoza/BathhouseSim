#pragma once

#include "CoreMinimal.h"
#include "Building/BathhouseSpaceTypes.h"
#include "Placement/FacilityPlacementZoneActor.h"
#include "BathhouseSpaceActor.generated.h"

class UBathhouseSpaceShellComponent;
struct FBathhouseExpansionStepSnapshot;
struct FBathhouseLayoutValues;
struct FBathhouseSpaceSnapshot;

DECLARE_LOG_CATEGORY_EXTERN(LogBathhouseBuilding, Log, All);

/** 공간 넓힘 한 번을 되돌리는 데 필요한 값. */
struct FBathhouseSpaceExpansionUndo
{
	int32 PreviousCount = INDEX_NONE;
	int32 PreviousChunkCount = 0;
	bool IsSet() const { return PreviousCount != INDEX_NONE; }
};

/**
 * 공간 하나(홀·목욕공간·작업공간)의 authoring Actor.
 * 직사각형 바닥 경계를 따라 벽·바닥·천장·조명·개구부·계단을 만들고, 그 바닥의 설비 배치 구역이자 생성 조각의 생성자다.
 * 값은 Level instance가 정본이고 생성물은 저장하지 않는다.
 */
UCLASS(Blueprintable)
class BATHHOUSESIM_API ABathhouseSpaceActor : public AFacilityPlacementZoneActor
{
	GENERATED_BODY()

public:
	ABathhouseSpaceActor();

	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
#if WITH_EDITOR
	virtual EDataValidationResult IsDataValid(FDataValidationContext& Context) const override;
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

	EBathhouseSpaceKind GetSpaceKind() const { return SpaceKind; }
	/** 효과 횟수의 안쪽 바닥 world XY. */
	FBox2D GetInteriorRect() const;
	/** 넓힘 0회의 안쪽 바닥. Actor XY +- FloorSizeCm/2. */
	FBox2D GetBaseInteriorRect() const;
	/** 넓힘 Count회(0..줄 수 clamp)의 안쪽 바닥. */
	FBox2D GetInteriorRectForCount(int32 Count) const;
	/** 형상이 따르는 횟수. 편집 world는 미리보기 횟수(clamp), 그 밖은 적용된 횟수. */
	int32 GetEffectiveExpansionCount() const;
	int32 GetAppliedExpansionCount() const { return AppliedExpansionCount; }
	int32 GetExpansionStepCount() const { return ExpansionSteps.Num(); }
	float GetFloorZ() const { return static_cast<float>(GetActorLocation().Z); }
	float GetCeilingZ() const { return GetFloorZ() + CeilingHeightCm; }
	EBathhouseCleaningChunkKind GetCleaningChunkKind() const { return CleaningChunkKind; }
	UBathhouseSpaceShellComponent* GetShell() const { return Shell; }
	const TArray<TWeakObjectPtr<AActor>>& GetCleaningChunks() const { return CleaningChunks; }

	/** authored 값만 snapshot으로 옮긴다. 연결 index는 모으는 쪽이 채운다. */
	void FillSnapshot(FBathhouseSpaceSnapshot& OutSnapshot) const;

	/** ZoneBounds XY extent와 상대 위치를 현재 안쪽 직사각형에 맞춘다(Z extent와 상대 Z는 Blueprint 값 유지). */
	void ApplyZoneGeometry();

	/** 형상 계획을 다시 계산해 이 공간의 shell만 재생성한다. OnConstruction을 부르지 않는다. */
	bool RebuildShell();

	/** 다음 넓힘을 적용할 수 있는지 본다(부작용 없음). 실패 문구는 로그용이다. */
	bool CanApplyNextExpansion(FText& OutFailureReason) const;
	/** 다음 넓힘을 적용한다(횟수·구역·shell·띠 조각). 실패하면 스스로 되돌리고 false. */
	bool ApplyNextExpansion(FBathhouseSpaceExpansionUndo& OutUndo, FText& OutFailureReason);
	/** ApplyNextExpansion을 되돌린다. 비어 있는 Undo는 아무것도 하지 않는다. */
	void UndoExpansion(const FBathhouseSpaceExpansionUndo& Undo);

protected:
	friend class FBathhouseBuildingAutomationAccess;
	friend class FBathhouseExpansionAutomationAccess;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Bathhouse Space")
	TObjectPtr<USceneComponent> SpaceRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Bathhouse Space")
	TObjectPtr<UBathhouseSpaceShellComponent> Shell;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Bathhouse Space",
		meta = (ToolTip = "공간 종류. 종류마다 공간 Actor가 하나씩 있어야 한다. Location XY = 0회 바닥 직사각형 중심, Z = 바닥 윗면 높이(Rotation 0, Scale 1)."))
	EBathhouseSpaceKind SpaceKind = EBathhouseSpaceKind::Hall;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Bathhouse Space",
		meta = (ClampMin = "0.0", ForceUnits = "cm", ToolTip = "안쪽 바닥 X, Y 크기(cm). 벽 안쪽 면 사이."))
	FVector2D FloorSizeCm = FVector2D::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Bathhouse Space",
		meta = (ClampMin = "0.0", ForceUnits = "cm", ToolTip = "천장 높이(cm). 바닥 윗면에서 천장 아랫면까지."))
	float CeilingHeightCm = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Bathhouse Space",
		meta = (ToolTip = "벽·바닥·천장 재질."))
	FBathhouseSpaceSurfaces Surfaces;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Bathhouse Space",
		meta = (ToolTip = "실내 조명."))
	FBathhouseSpaceLighting Lighting;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Bathhouse Space",
		meta = (ToolTip = "출입구(연결 공간 없음)와 통로(연결 공간 지정). 통로는 한쪽 공간에만 적는다."))
	TArray<FBathhouseSpaceOpening> Openings;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Bathhouse Space",
		meta = (ToolTip = "이 공간(위층)에서 아래층으로 내려가는 계단."))
	TArray<FBathhouseStairSpec> Stairs;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Bathhouse Space",
		meta = (ToolTip = "이 공간 바닥에 생성할 청소 조각 종류. 한 공간에 한 종류. 없음이면 조각이 없다."))
	EBathhouseCleaningChunkKind CleaningChunkKind = EBathhouseCleaningChunkKind::None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Bathhouse Space|Expansion",
		meta = (ToolTip = "줄 = 이 공간의 몇 번째 넓힘. 줄 수 = 공간별 넓힘 횟수 상한."))
	TArray<FBathhouseSpaceExpansionStep> ExpansionSteps;

#if WITH_EDITORONLY_DATA
	UPROPERTY(Transient, EditInstanceOnly, Category = "Bathhouse Space|Expansion",
		meta = (ClampMin = "0", ToolTip = "편집 화면에서만 넓힌 모습을 미리 본다(0~줄 수, 저장 안 됨). PIE는 항상 0회에서 시작한다."))
	int32 EditorPreviewExpansionCount = 0;
#endif

	UPROPERTY(Transient, VisibleInstanceOnly, Category = "Bathhouse Space|Expansion",
		meta = (ToolTip = "게임 중 지금까지 넓힌 횟수(저장 안 됨)."))
	int32 AppliedExpansionCount = 0;

	UPROPERTY(Transient)
	TArray<TWeakObjectPtr<AActor>> CleaningChunks;

private:
	void CollectStepSnapshots(TArray<FBathhouseExpansionStepSnapshot>& OutSteps) const;
	FString BuildExpansionPreviewLabel(
		const TArray<FBathhouseSpaceSnapshot>& Snapshots, int32 Index, const FBathhouseLayoutValues& Values) const;
	void RegisterWithPurchaseSubsystem();
	void UnregisterFromPurchaseSubsystem();
	void MarkExpansionNavigationDirty() const;
	void SpawnCleaningChunks();
	void DestroyCleaningChunks();
	void LogValidationProblems() const;
};
