# Boiler 02 — 빈티지 적색 산업용 보일러

8방향 턴어라운드 레퍼런스 시트를 기준으로 Blender 5.2에서 새로 모델링한 게임용 스태틱 메시입니다. 칠이 벗겨진 적색 도장 몸체, 주철 연통과 화구 도어, 0~4 bar 압력 게이지, 명판, 적색 버너 노브, 우측면 황동 배관과 적색·청색 밸브 휠, 뒷면 볼트 체결 서비스 패널, 다리 4개로 구성됩니다.

기존 `Boiler_01`과는 별개 폴더이며 기존 파일은 수정하지 않았습니다.

**상태:** 익스포트 직전 단계까지 완료했습니다. FBX는 요청에 따라 아직 내보내지 않았습니다.

## 파일 구성

- `Boiler_02.blend`: 원본 씬입니다. 텍스처는 상대경로로 연결되어 있습니다.
  - `Boiler02_Export`: `SM_Boiler_02`와 `UCX_SM_Boiler_02_00~03` 충돌체 4개. **익스포트 대상은 이 컬렉션입니다.**
  - `Boiler02_Source` (뷰레이어에서 제외): 수정용 파트 22개(`Boiler02_Parts`)와 베이크 원본 메시(`Boiler02_Bake`, 절차적 소스 머티리얼 포함).
  - `Boiler02_Preview`: 스튜디오 조명과 카메라.
- `textures/`
  - `T_Boiler02_BaseColor.png`: 2048, sRGB.
  - `T_Boiler02_ORM.png`: 2048, Linear. R=AO, G=Roughness, B=Metallic.
  - `T_Boiler02_Normal.png`: 2048, Linear, **DirectX 방식(G-)**이라 언리얼에서 Flip Green이 필요 없습니다.
  - `T_Boiler02_Dial_Src.png`: 게이지 다이얼 소스로, 베이크 입력용입니다.
- `renders/`, `Boiler_02_Turnaround.png`: 8방향 확인 렌더.
- `scripts/`: 전체 재현이 가능한 빌드 파이프라인입니다.
  - `build_boiler_02.py`: 모델링과 소스 머티리얼.
  - `bake_boiler_02.py`: 파트 병합, UV0 아틀라스, UV1 라이트맵, 베이크.
  - `finalize_boiler_02.py`: UCX 충돌체와 검증.
  - `preview_boiler_02.py`: 확인 렌더.
  - `export_fbx_boiler_02.py`: **아직 실행하지 않은** FBX 익스포트 스크립트.
  - `make_dial.py`: 다이얼 텍스처 생성.
  - `check_uv_overlap.py`: UV 겹침 검사.
- `_validation/`: UV 검사용 삼각형 덤프입니다. 삭제해도 됩니다.
- `asset_manifest.json`, `materials.json`, `validation_report.json`.

## 크기와 피벗

| 항목 | 값 |
|---|---|
| 도장 몸체 | 폭 0.70 × 깊이 0.455 × 높이 1.135 m |
| 전체 (밸브·전면 하드웨어 포함) | 0.824 × 0.532 × 1.368 m |
| 다리 높이 / 연통 상단 | 0.09 m / 1.368 m |
| 피벗 | 바닥 중심 (0,0,0) |
| 방향 | 정면 = Blender −Y, 밸브 = +X 측면 |
| 트랜스폼 | 위치 0, 회전 0, 스케일 1 (적용 완료), 미터 단위, Unit Scale 1.0 |

치수는 레퍼런스 시트의 비율을 측정해 몸체 폭 0.70 m 기준으로 환산한 값입니다.

## 메시 사양

- **폴리곤:** 삼각형 28,016개, 버텍스 14,424개. n-gon은 없습니다.
- **머티리얼:** 슬롯 1개, `M_Boiler02`. 모든 파트를 하나의 2048 아틀라스로 베이크했습니다.
- **UV:**
  - `UV0` (텍스처): 겹침 0, 커버리지 64%. 게이지 다이얼에는 3배 텍셀 밀도를 줬습니다.
  - `UV1_Lightmap`: 겹침 0, 커버리지 48%.
- **스무딩:** 35° 기준 하드엣지로 지정했습니다. FBX를 `Smoothing: Face`로 내보내면 스무딩 그룹으로 전달됩니다.
- **충돌체:** UCX 볼록 헐 4개.
  - `_00`: 몸체와 다리.
  - `_01`: 연통.
  - `_02`: 밸브.
  - `_03`: 전면 게이지, 도어, 노브.

## FBX 익스포트 (준비 완료, 미실행)

`Boiler_02.blend`를 연 상태에서 `scripts/export_fbx_boiler_02.py`를 실행하면 `SM_Boiler_02.fbx`가 생성됩니다. 스크립트에 들어 있는 설정은 다음과 같습니다.

- Selected Objects: `SM_Boiler_02`와 `UCX_*` 4개
- Object Types: Mesh
- Scale 1.0, Apply Unit, `FBX_SCALE_NONE`
- Forward −Z, Up Y
- Smoothing: Face
- Tangent Space: 켬
- Leaf Bones: 끔
- Animation: 끔

## 언리얼 임포트 가이드

1. `SM_Boiler_02.fbx`를 임포트합니다. 옵션은 다음과 같이 설정합니다.
   - Generate Missing Collision: 끔 (UCX 사용)
   - Normal Import Method: Import Normals and Tangents
   - Uniform Scale: 1.0
2. 텍스처 설정:
   - `BaseColor`: sRGB 켬
   - `ORM`: sRGB 끔, Compression Masks
   - `Normal`: Normalmap (Flip Green 끔)
3. 머티리얼 연결:
   - ORM.R → Ambient Occlusion
   - ORM.G → Roughness
   - ORM.B → Metallic
4. 라이트맵 좌표 인덱스는 1입니다 (`UV1_Lightmap`).

## 다시 만들기

Blender에서 스크립트를 이 순서로 실행하면 처음부터 다시 만들 수 있습니다.

1. `build_boiler_02.py`
2. `bake_boiler_02.py`: `STAGE`를 `prep` → `bake_base` / `bake_rough` / `bake_metal` / `bake_normal` / `bake_ao` → `finalize` 순서로 지정해 실행합니다.
3. `finalize_boiler_02.py`

형태나 칠을 수정하려면 `build_boiler_02.py`의 치수·머티리얼 파라미터를 바꾼 뒤 위 순서를 다시 실행하면 됩니다.
