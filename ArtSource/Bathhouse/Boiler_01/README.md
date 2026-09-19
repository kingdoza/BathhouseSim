# Boiler 01 — 낡은 붉은색 산업용 보일러 (Vintage Industrial Boiler)

제공해주신 8방향 턴어라운드 레퍼런스 시트를 바탕으로 제작된 고품질 PBR 스냅샷 3D 에셋입니다. 빈티지 적갈색 치핑 도장, 주철 연통 및 버너 도어, 황동 밸브 배관, 적색/청색 핸들 휠, 전면 아날로그 압력 게이지 및 하단 스위치 노브를 정밀하게 재현했습니다.

## 파일 구성

- `Boiler_01.blend`: Blender 5.2.0 원본 씬 (PBR 머티리얼, 스튜디오 조명, 카메라 리그, 텍스처 패킹).
- `Boiler_01.glb`: 모든 지오메트리와 PBR 머티리얼이 조립된 표준 GLTF 모델.
- `SM_Boiler_01.fbx`: 언리얼 엔진용 Static Mesh (몸체, 하드웨어 분리 슬롯, 8개의 `UCX_` 단순 볼록 충돌체 포함).
- `textures/`:
  - `T_Boiler_RedPaint_BaseColor.png`: 도장 벗겨짐과 스크래치가 포함된 빈티지 레드 베이스컬러.
  - `T_Boiler_Gauge_Dial.png`: 0~4 bar 압력 눈금 및 인디케이터 텍스처.
- `Boiler_01_Hero.png`: 전면 3/4 스튜디오 뷰 렌더 이미지 (1600x1200).
- `Boiler_01_Turnaround.png`: 8방향(Front, Back, Left, Right, Top, Bottom, Front 3/4, Back 3/4) 턴어라운드 콜라주.
- `asset_manifest.json`: 버텍스, 폴리곤 수, 치수 통계.
- `materials.json`: 언리얼 엔진 머티리얼 인스턴스 복원용 파라미터.
- `validation_report.json`: FBX 재수입 및 충돌체 검증 보고서 (31개 검사항목 100% 통과).

## 크기 및 피벗 (Dimensions & Scale)

- **치수**: 폭 0.80m × 깊이 0.50m × 높이 1.51m (연통 및 다리 포함).
- **피벗(Origin)**: 바닥 중심 `(0, 0, 0)` (언리얼 엔진 액터 배치 최적화).
- **단위**: 미터(m) 단위 제작, FBX 단위 스케일 1.0 (언리얼 임포트 시 100cm = 1m 자동 매핑).

## 언리얼 엔진(Unreal Engine) 임포트 가이드

1. `SM_Boiler_01.fbx`를 프로젝트의 `Content/ArtSource/Props/` 또는 원하는 폴더로 드래그 앤 드롭합니다.
2. **FBX Import Options**:
   - `Generate Missing Collision`: **체크 해제** (포함된 8개의 정밀 `UCX_` 볼록 충돌체가 자동 사용됩니다).
   - `Transform > Uniform Scale`: **1.0**.
   - `Normal Import Method`: **Import Normals**.
3. **머티리얼 설정**:
   - `materials.json`의 파라미터를 참고하여 마스터 머티리얼의 인스턴스를 생성하고 슬롯에 할당합니다.
   - `UV0`: 텍스처 매핑용 채널.
   - `UV1_Lightmap`: 언리얼 엔진 라이트맵 베이킹용 비중첩 UV 채널.

## 검증 결과 (Validation)

- 재수입 오차: < 0.2mm
- 충돌체: 8개 볼록 다포체(Convex Manifold) 정상 밀폐 검증 완료.
- 스케일: 모든 구성 요소 스케일 `(1.0, 1.0, 1.0)` 완벽 적용.
