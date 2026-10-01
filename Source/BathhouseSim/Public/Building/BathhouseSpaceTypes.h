#pragma once

#include "CoreMinimal.h"
#include "BathhouseSpaceTypes.generated.h"

class ABathhouseSpaceActor;
class UMaterialInterface;

UENUM(BlueprintType)
enum class EBathhouseSpaceKind : uint8
{
	Hall UMETA(DisplayName = "홀"),
	Bath UMETA(DisplayName = "목욕공간"),
	Work UMETA(DisplayName = "작업공간")
};

/** world 축 기준 벽 방향. */
UENUM(BlueprintType)
enum class EBathhouseSpaceSide : uint8
{
	East UMETA(DisplayName = "동(+X)"),
	West UMETA(DisplayName = "서(-X)"),
	North UMETA(DisplayName = "북(+Y)"),
	South UMETA(DisplayName = "남(-Y)")
};

UENUM(BlueprintType)
enum class EBathhouseCleaningChunkKind : uint8
{
	None UMETA(DisplayName = "없음"),
	Litter UMETA(DisplayName = "쓰레기 조각"),
	Stain UMETA(DisplayName = "물 얼룩 조각")
};

/** 생성 형상 part. 순수 enum이며 reflection 대상이 아니다. */
enum class EBathhouseShellPart : uint8
{
	Floor,
	Wall,
	Ceiling,
	StairStep,
	StairRamp,
	StairWall,
	StairKeepClear,
	Count
};

USTRUCT(BlueprintType)
struct BATHHOUSESIM_API FBathhouseSpaceSurfaces
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Bathhouse Space",
		meta = (ToolTip = "벽 재질. 상자를 늘려 쓰므로 world 기준 무늬 재질을 권장한다."))
	TObjectPtr<UMaterialInterface> WallMaterial = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Bathhouse Space",
		meta = (ToolTip = "바닥 재질."))
	TObjectPtr<UMaterialInterface> FloorMaterial = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Bathhouse Space",
		meta = (ToolTip = "천장 재질."))
	TObjectPtr<UMaterialInterface> CeilingMaterial = nullptr;
};

USTRUCT(BlueprintType)
struct BATHHOUSESIM_API FBathhouseSpaceLighting
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Bathhouse Space",
		meta = (ClampMin = "0.0", ForceUnits = "cm", ToolTip = "조명 사이 최대 간격(cm). 바닥을 이 간격 이하로 균등 분할한 칸마다 조명 하나."))
	float SpacingCm = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Bathhouse Space",
		meta = (ClampMin = "0.0", ToolTip = "조명 밝기(cd)."))
	float IntensityCandela = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Bathhouse Space",
		meta = (ClampMin = "0.0", ForceUnits = "cm", ToolTip = "조명이 닿는 거리(cm)."))
	float AttenuationRadiusCm = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Bathhouse Space",
		meta = (ToolTip = "조명 색."))
	FLinearColor Color = FLinearColor::White;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Bathhouse Space",
		meta = (ToolTip = "조명 그림자 사용 여부."))
	bool bCastShadows = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Bathhouse Space",
		meta = (ClampMin = "0.0", ForceUnits = "cm", ToolTip = "천장 아랫면에서 조명을 내린 높이(cm)."))
	float CeilingOffsetCm = 0.0f;
};

USTRUCT(BlueprintType)
struct BATHHOUSESIM_API FBathhouseSpaceOpening
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Bathhouse Space",
		meta = (ToolTip = "개구부가 있는 벽 방향(world 축 기준)."))
	EBathhouseSpaceSide Side = EBathhouseSpaceSide::East;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Bathhouse Space",
		meta = (ForceUnits = "cm", ToolTip = "개구부 중심의 벽 길이 방향 위치(cm). 이 공간 Actor 위치 기준. 동·서 벽은 Y, 남·북 벽은 X 방향."))
	float CenterOffsetCm = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Bathhouse Space",
		meta = (ClampMin = "0.0", ForceUnits = "cm", ToolTip = "개구부 폭(cm)."))
	float WidthCm = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Bathhouse Space",
		meta = (ClampMin = "0.0", ForceUnits = "cm", ToolTip = "개구부 높이(cm). 바닥 윗면 기준."))
	float HeightCm = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Bathhouse Space",
		meta = (ToolTip = "비우면 바깥 출입구. 지정하면 그 공간과 이어지는 통로(맞닿은 공간의 벽도 같은 구간이 뚫린다). 통로는 한쪽 공간에만 적는다."))
	TObjectPtr<ABathhouseSpaceActor> ConnectedSpace = nullptr;
};

USTRUCT(BlueprintType)
struct BATHHOUSESIM_API FBathhouseStairSpec
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Bathhouse Space",
		meta = (ToolTip = "계단이 내려가는 아래층 공간."))
	TObjectPtr<ABathhouseSpaceActor> LowerSpace = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Bathhouse Space",
		meta = (ForceUnits = "cm", ToolTip = "계단 맨 위 가장자리 중심 XY(cm). 이 공간 Actor 위치 기준."))
	FVector2D TopEdgeCenterOffsetCm = FVector2D::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Bathhouse Space",
		meta = (ToolTip = "내려가는 방향(world 축 기준)."))
	EBathhouseSpaceSide DownSide = EBathhouseSpaceSide::East;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Bathhouse Space",
		meta = (ClampMin = "0.0", ForceUnits = "cm", ToolTip = "계단 폭(cm)."))
	float WidthCm = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Bathhouse Space",
		meta = (ClampMin = "0.0", ForceUnits = "cm", ToolTip = "내려가는 방향 길이(cm). 길수록 완만하다."))
	float RunCm = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Bathhouse Space",
		meta = (ClampMin = "0", ToolTip = "보이는 계단 판 수."))
	int32 StepCount = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Bathhouse Space",
		meta = (ClampMin = "0.0", ForceUnits = "cm", ToolTip = "계단 양옆 난간 벽 높이(cm). 위층 바닥 윗면 기준."))
	float GuardHeightCm = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Bathhouse Space",
		meta = (ToolTip = "계단 판 재질."))
	TObjectPtr<UMaterialInterface> StepMaterial = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Bathhouse Space",
		meta = (ToolTip = "계단 벽 재질."))
	TObjectPtr<UMaterialInterface> StairWallMaterial = nullptr;
};
