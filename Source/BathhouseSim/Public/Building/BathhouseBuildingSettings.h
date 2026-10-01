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

	float GetWallThicknessCm() const { return FMath::Max(1.0f, WallThicknessCm); }
	float GetSlabThicknessCm() const { return FMath::Max(1.0f, SlabThicknessCm); }
	FVector2D GetCleaningChunkMaxSizeCm() const
	{
		return FVector2D(FMath::Max(1.0f, CleaningChunkMaxSizeCm.X), FMath::Max(1.0f, CleaningChunkMaxSizeCm.Y));
	}
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
