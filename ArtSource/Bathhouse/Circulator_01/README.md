# Circulator_01 — 목욕물 순환 펌프 (`BP_Circulator`용)

정면(UE −Y = Blender +Y, Back 뷰 Ctrl+Numpad 1) 기준 왼쪽(UE +X)은 무도장 강판 제어 캐비닛(작업 구역 테마 `work`)(유량계, 표시등 2개, 명판, 레버 사분원 가이드), 오른쪽(UE −X)은 냉각 핀 모터·커플링·주철 볼류트·구리 흡입/토출관·게이트 밸브가 철제 스키드 위에 놓인 구성입니다.

| FBX | BP component | mesh 원점(BP SceneRoot 로컬 cm) | 크기·비고 |
|---|---|---|---|
| `SM_Circulator_01.fbx` | VisualMesh | (0,0,0) 바닥 중심 | 120×81×120 (footprint 120×80×120), UCX 6개 |
| `SM_Circulator_01_Lever.fbx` | LeverMesh | LeverPivot (30,−44,30) | 팔이 원점에서 +Z로 38cm(빨간 손잡이). 로컬 Y −60°로 정면 기준 오른쪽으로 내려가며, 캐비닛의 사분원 가이드가 0°·30°·60° 위치를 표시. UCX 1개 |
| `SM_Circulator_01_Needle.fbx` | GaugeNeedleMesh | GaugeNeedlePivot (34.49,−44,109.92) | 정지 시 +X. 다이얼 반지름 7.9(높이 120 이내로 맞춤) |

## Unreal 교체 시 BP 조정

- VisualMesh: scale (1.2,0.8,1.2) → 1.
- LeverMesh: 상대 위치 (0,0,25) → 0, scale → 1.
- GaugeNeedleMesh: 상대 위치 (7,0,0) → 0, scale → 1.
- pivot 값은 현재 BP 값을 그대로 썼습니다.

삼각형: 본체 13,976 / 레버 692 / 바늘 306. 텍스처는 2048입니다.
