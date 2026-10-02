# Boiler_03 — 레트로 법랑 목욕탕 보일러 (`BP_Boiler`용)

작업 구역 테마(`work`, 무도장 철): 건메탈 강판 몸체, 아연도 강판 상판, 주철 화구 문(사분 경첩), 크롬 베젤 압력계(0~4 bar), 황동 명판, 연통, 안전밸브, 우측(UE −X) 온·냉수 밸브, 좌측(UE +X) 루버, 뒷면 서비스 패널로 구성했습니다. 정면은 UE −Y입니다. 공용 규칙과 import 방법은 `../_Pipeline/README.md`를 보세요.

| FBX | BP component | mesh 원점(BP SceneRoot 로컬 cm) | 크기·비고 |
|---|---|---|---|
| `SM_Boiler_03.fbx` | VisualMesh | (0,0,0) 바닥 중심 | 99×68×120 (앞 하드웨어 포함, footprint 100×60×120), UCX 4개 |
| `SM_Boiler_03_Door.fbx` | FuelDoorMesh | 경첩축 (12.5,−37,42) | 판이 원점에서 −X로 25cm. 로컬 Z +90°로 정면 바깥 열림. UCX 1개 |
| `SM_Boiler_03_Needle.fbx` | GaugeNeedleMesh | 허브 (0,−34,90) | 정지 시 +X(정면에서 왼쪽)를 가리킴. 다이얼 호는 GaugePresentation −30°(10시)~−150°(2시)에 맞춤 |

## Unreal 교체 시 BP 조정(Editor 작업 몫)

- VisualMesh: mesh 교체, scale (1,0.6,1.2) → 1.
- FuelDoorMesh: 상대 위치 (−12.5,0,0) → 0, scale → 1.
- GaugeNeedleMesh: 상대 위치 (7,0,0) → 0, scale → 1.
- **GaugeNeedlePivot: 현재 (67.77,−34,129.13)으로 본체 밖에 있음 → (0,−34,90)으로 옮겨야 함** (조사 보고서 8절).
- FuelIntake·GaugeFacePlate의 임시 Cube 표시는 본체에 통합됐으므로 숨기거나 mesh를 비웁니다. FuelIntake의 투입 기능 component 자체는 유지합니다.

삼각형: 본체 10,396 / 문 1,396 / 바늘 340. 텍스처는 2048 BaseColor·ORM·Normal 1세트입니다.
