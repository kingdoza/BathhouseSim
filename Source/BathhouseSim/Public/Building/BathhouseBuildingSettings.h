#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "BathhouseBuildingSettings.generated.h"

class ALitterSpawnZoneActor;
class AStainSpawnZoneActor;
class UStaticMesh;

/** 모든 공간이 함께 쓰는 건물 값. Project Settings > Game > Bathhouse Building (Config/DefaultGame.ini). */
UCLASS(Config = Game, DefaultConfig, meta = (DisplayName = "Bathhouse Building"))
class BATHHOUSESIM_API UBathhouseBuildingSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	UBathhouseBuildingSettings();

	virtual FName GetCategoryName() const override { return TEXT("Game"); }

	float GetWallThicknessCm() const { return WallThicknessCm; }
	float GetSlabThicknessCm() const { return SlabThicknessCm; }
	/** 값이 0 이하이거나 유한하지 않으면 공간 검증이 오류로 알린다(숨은 하한 없음). */
	FVector2D GetCleaningChunkMaxSizeCm() const { return CleaningChunkMaxSizeCm; }
	/** 편집 화면 전용 넓힘 미리보기 글자 크기(cm). */
	float GetEditorPreviewLabelWorldSizeCm() const { return EditorPreviewLabelWorldSizeCm; }
	float GetEditorPreviewLabelHeightCm() const { return EditorPreviewLabelHeightCm; }
	int32 GetEditorPreviewLabelFontSize() const { return EditorPreviewLabelFontSize; }
	UStaticMesh* LoadShellBoxMesh() const;
	UClass* LoadLitterChunkZoneClass() const;
	UClass* LoadStainChunkZoneClass() const;
	bool HasLitterChunkZoneClass() const { return !LitterChunkZoneClass.IsNull(); }
	bool HasStainChunkZoneClass() const { return !StainChunkZoneClass.IsNull(); }

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Shell",
		meta = (ClampMin = "1.0", UIMin = "1.0", ForceUnits = "cm",
			ToolTip = "벽 두께(cm). 공간 안쪽 면 바깥으로 이만큼 벽이 선다."))
	float WallThicknessCm = 20.0f;

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Shell",
		meta = (ClampMin = "1.0", UIMin = "1.0", ForceUnits = "cm",
			ToolTip = "바닥·천장 판 두께(cm)."))
	float SlabThicknessCm = 20.0f;

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Shell",
		meta = (ToolTip = "벽·바닥·천장을 만드는 상자 mesh. 늘려 쓰므로 원점 중심의 상자를 쓴다."))
	TSoftObjectPtr<UStaticMesh> ShellBoxMesh;

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Editor Preview",
		meta = (ClampMin = "1.0", UIMin = "1.0", ForceUnits = "cm",
			ToolTip = "편집 화면에서 넓힘 미리보기 횟수를 켰을 때 공간 위에 뜨는 글자 높이(대략, cm). 게임에는 영향이 없다."))
	float EditorPreviewLabelWorldSizeCm = 100.0f;

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Editor Preview",
		meta = (ClampMin = "1.0", UIMin = "1.0", ForceUnits = "cm",
			ToolTip = "가장 높은 천장 판 윗면에서 미리보기 글자까지의 여유(cm). 게임에는 영향이 없다."))
	float EditorPreviewLabelHeightCm = 300.0f;

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Editor Preview",
		meta = (ClampMin = "1", UIMin = "1",
			ToolTip = "미리보기 글자를 그리는 글꼴 크기(렌더 해상도). 실제 크기는 글자 높이 / 글꼴 크기로 정해진다. 게임에는 영향이 없다."))
	int32 EditorPreviewLabelFontSize = 64;

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Cleaning",
		meta = (ClampMin = "1.0", UIMin = "1.0", ForceUnits = "cm",
			ToolTip = "생성 조각 최대 크기(X, Y cm). 공간 바닥을 이 크기 이하의 같은 칸으로 나눈다."))
	FVector2D CleaningChunkMaxSizeCm = FVector2D(400.0, 400.0);

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Cleaning",
		meta = (ToolTip = "공간의 조각 종류가 쓰레기일 때 바닥에 까는 구역 Blueprint."))
	TSoftClassPtr<ALitterSpawnZoneActor> LitterChunkZoneClass;

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Cleaning",
		meta = (ToolTip = "공간의 조각 종류가 물 얼룩일 때 바닥에 까는 구역 Blueprint."))
	TSoftClassPtr<AStainSpawnZoneActor> StainChunkZoneClass;
};
