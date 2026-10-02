#pragma once

#include "CoreMinimal.h"
#include "Layout/Geometry.h"

/** 지도 canvas 안에서 Zone을 가운데 맞춤(letterbox)으로 담는 배치. 지도 투영과 그리드가 같이 쓴다. */
struct FBathWaterMapLetterbox
{
	float PixelsPerCm = 0.0f;
	FVector2D Origin = FVector2D::ZeroVector;
	FVector2D ContentSize = FVector2D::ZeroVector;
};

/** Zone 반 extent(cm, scale 적용 뒤)와 canvas 크기가 유한·양수일 때만 true. */
bool ComputeBathWaterMapLetterbox(
	const FVector2D& CanvasSize, float ZoneHalfXCm, float ZoneHalfYCm, FBathWaterMapLetterbox& Out);

/** 레이아웃 1 단위가 render target 몇 px인지(X, Y). 무효면 (1,1). */
FVector2D ResolveRenderPixelsPerLayoutUnit(const FGeometry& PaintSpaceGeometry);

/** render px 두께를 레이아웃 두께로 바꾼다. 최소 1 render px를 보장한다. */
float ToLayoutLineThickness(float RenderThicknessPx, float RenderPixelsPerLayoutUnit);

struct FBathWaterMapGridInput
{
	FVector2D CanvasSize = FVector2D::ZeroVector;
	float ZoneHalfXCm = 0.0f;
	float ZoneHalfYCm = 0.0f;
	float MajorSpacingCm = 0.0f;
	FVector2D RenderPixelsPerLayoutUnit = FVector2D(1.0, 1.0);
	float GridThicknessPx = 1.0f;
	float BoundaryThicknessPx = 1.0f;
};

struct FBathWaterMapGridLine
{
	FVector2D Position = FVector2D::ZeroVector;
	FVector2D Size = FVector2D::ZeroVector;
	bool bBoundary = false;
};

/** 칸 선(경계와 겹치는 선 제외)과 경계선 네 개를 만든다. 입력이 무효면 false. */
bool BuildBathWaterMapGridLines(const FBathWaterMapGridInput& Input, TArray<FBathWaterMapGridLine>& OutLines);
