# 모델링 작업 방식 — 제작부터 Unreal 적용까지

- 작성일: 2026-10-02 (`MODEL-M1` 경험으로 정리)
- 범위: 스태틱 메시 교체 작업이다. 코드·사용자 동작은 바꾸지 않는다.
  - 대상과 사용처: [MODELING_STATIC_MESH_LIST.md](MODELING_STATIC_MESH_LIST.md)
  - 외형 규칙: [MODELING_STYLE_GUIDE.md](MODELING_STYLE_GUIDE.md)
  - Blender 파이프라인: `ArtSource/Bathhouse/_Pipeline/README.md`
- 공통 규칙은 [AGENT_WORKFLOW.md](AGENT_WORKFLOW.md)를 따른다. 이 문서는 모델링 작업에 맞춘 단계와 관문만 정한다.
  - 작업 폴더·결과물 첫머리·커밋 규칙
  - Editor 역할: [AGENT_UNREAL_EDITOR.md](AGENT_UNREAL_EDITOR.md)

## 1. 단계

| # | 단계 | 담당 | 결과물(`.md/Work/MODEL-<n>/`) | 다음으로 넘어가는 조건 |
|---|---|---|---|---|
| 1 | 대상 확정·사전 조사 | 마스터 + Editor 워커(읽기 전용) | `CONTEXT.md`, `REPORT_UNREAL_DISCOVERY.md` | BP pivot·축·footprint·진열/서는 위치 확보 |
| 2 | 모델링 | 마스터(Blender 파이프라인) | `ArtSource/Bathhouse/<에셋>/` 일체, 라인업 렌더 | `run_all.sh` 검증 전부 통과 |
| 3 | **사용자 모델 승인** | 사용자 | `CONTEXT.md` 승인 기록 | 에셋별 "승인" |
| 4 | Unreal import·에셋 설정 | Editor 워커 | `PROMPT_UNREAL.md`(마스터), `REPORT_UNREAL_EDITOR.md` | 저장·재로드·검증 통과 |
| 5 | BP 연결 | Editor 워커(4단계와 같은 세션) | 같은 보고서, 관련 `Unreal/*System.md` | Compile·Validation·재로드 통과 |
| 6 | 사용자 PIE 검증 | 사용자 | `PIE_CHECKLIST.md`(마스터) | 전 항목 통과 |
| 7 | 커밋·병합·정리 | 마스터 | — | 아래 7절 |

- 기능 명세·아키텍처·구현·코드 리뷰 단계는 생략한다. 근거(코드·동작 변경 없음)를 `CONTEXT.md`에 적는다.
- 작업 도중 BP만으로 해결이 안 되면(새 component, C++ 값 변경) 그 항목만 멈추고 기능 명세나 아키텍처로 보낸다. 예: 회전축 추가, 진열 칸 위치를 코드에서 바꿔야 할 때.

## 2. 사용자 모델 승인(3단계)

- 마스터가 에셋별로 보여 줄 것
  - 턴어라운드(`<에셋>_Turnaround.png`)와 기존 에셋과의 라인업
  - 구역 테마·티어
  - 크기와 footprint 대비
  - 움직이는 부품과 pivot
  - BP 조정 목록(각 README "Unreal 교체 시 BP 조정")
- 사용자 답은 `승인` / `수정: 메모` / `보류`다.
  - 수정이 나오면 2단계로 돌아가 다시 굽고 다시 보여 준다.
  - 승인된 에셋만 4단계로 간다.
- 승인은 **그 에셋의 import와 아래 4·5절 범위의 Content 수정 허가**를 겸한다. 승인 일자·대상 에셋·허가 범위를 `CONTEXT.md`에 적는다. 범위 밖 asset을 고쳐야 하면 멈추고 다시 묻는다.
- 승인 시점에 ArtSource 결과물을 커밋한다. 커밋 위치는 사용자가 정하고, 기본은 `main`이다.

## 3. 입력 프롬프트(`PROMPT_UNREAL.md`, 마스터 작성)

- 첫머리 단계 값은 Editor 작업이다. 출처는 사용자 모델 승인(일자)이다.
- 에셋별로 다음을 적는다.
  - FBX·텍스처 원본 경로
  - import 대상 Content 경로
  - 재질 인스턴스 이름
  - BP·component별 목표값: 각 README의 BP 조정 절을 그대로 옮긴다
  - 저장 allowlist
- PIE 관찰 항목의 초안도 함께 적는다(6절).

## 4. Unreal import·에셋 설정(4단계)

**Content 경로**(Bath_01 선례를 따름)

```
/Game/Bathhouse/Meshes/Facility/<에셋>/StaticMeshes/  SM_<에셋>, SM_<에셋>_<부품>
/Game/Bathhouse/Meshes/Facility/<에셋>/Textures/      T_<에셋>_BaseColor / _ORM / _Normal
/Game/Bathhouse/Meshes/Facility/<에셋>/Materials/     MI_<에셋>
/Game/Bathhouse/Materials/Props/M_BakedProp           공용 마스터 재질(최초 1회 생성)
```

**FBX import 설정**
- Force Front X Axis 끔
- Uniform Scale 1, Combine Meshes 끔
- Generate Missing Collision 끔(UCX 사용)
- Normal Import Method: Import Normals and Tangents
- Generate Lightmap UVs 끔(UV1이 이미 있음), Lightmap Coordinate Index 1
- Import Materials·Textures 끔. FBX에는 BaseColor만 실리므로 텍스처는 따로 넣는다.
- Nanite 끔, LOD는 LOD0만. 바꿀 근거가 생기면 그때 정한다.

**텍스처 import 설정**

| 텍스처 | sRGB | Compression | 기타 |
|---|---|---|---|
| BaseColor | 켬 | Default | — |
| ORM | 끔 | Masks | R=AO, G=Roughness, B=Metallic |
| Normal | 끔 | Normalmap | DirectX 방식이라 Flip Green 끔 |

- 최대 크기는 원본대로 둔다(기본 2048). 4096 텍스처(샤워기)는 사용자 판단으로 Max Texture Size를 정한다.

**공용 마스터 재질 `M_BakedProp`**
- 파라미터: `BaseColorTex`, `ORMTex`, `NormalTex`
- ORM은 R→AO, G→Roughness, B→Metallic으로 연결한다.
- `Tint`(기본 흰색, 곱하기)와 `RoughnessScale`(기본 1)을 둔다. 기본값에서는 Blender 결과와 같아야 한다.
- 에셋마다 `MI_<에셋>` 1개를 만들어 텍스처 3장을 넣는다. 그 에셋의 모든 메시(본체·문·레버·바늘)가 이 MI 하나를 쓴다.

**StaticMesh 설정**
- 재질 슬롯 1개에 `MI_<에셋>`을 넣는다.
- Collision Complexity는 Project Default로 둔다.
- UCX 개수가 README·manifest와 같은지 확인한다.
- Lightmap Resolution은 크기에 맞게 정한다(본체 64~128, 부품 16~32).

## 5. BP 연결(5단계)

- 각 README "Unreal 교체 시 BP 조정"을 그대로 적용한다. 공통 원칙은 다음과 같다.
  - 임시 mesh가 쓰던 component·actor scale은 1로 되돌린다.
  - 움직이는 부품 mesh의 pivot 기준 상대 위치는 0으로 한다. mesh 원점이 곧 BP pivot이기 때문이다.
  - 본체에 합친 임시 표시 component(예: 보일러 FuelIntake·GaugeFacePlate의 Cube)는 mesh를 비우거나 숨긴다. 기능 component 자체는 지우지 않는다.
  - BP pivot이 모델과 맞지 않는 이상값이면 README에 적힌 목표값으로 옮긴다. 예: 보일러 `GaugeNeedlePivot`을 (0,−34,90)으로.
- collision profile·Nav 설정·footprint·slot·진열 component는 바꾸지 않는다. 본체는 기존과 같은 profile(BlockAllDynamic, Nav on)을 유지하고, 부품은 기존 profile을 따른다.
- Level instance가 scale·mesh를 덮어쓰고 있으면 instance override도 같은 원칙으로 되돌린다. 예: 쿨러 actor scale 0.5. 대상 actor는 프롬프트 allowlist에 넣는다.
- 4·8칸처럼 같은 모듈을 반복하는 BP는 모든 칸 component에 같은 mesh를 넣고 칸 간격은 그대로 둔다.
- Compile, Data Validation, allowlist 개별 Save, 디스크 재로드 재검사는 [AGENT_UNREAL_EDITOR.md](AGENT_UNREAL_EDITOR.md) Editor 작업 순서대로 한다.
- 관련 `Unreal/*System.md`의 mesh 참조를 현재 상태로 갱신한다. 수치는 복제하지 않고 원본 asset 위치만 적는다.

**Editor 쪽 자동 검사**(보고서에 결과 기록)
- mesh bounds가 README·manifest 크기와 같은지(허용 오차 0.5cm)
- 본체가 footprint 안에 있는지, 예외는 README 근거와 같은지
- 정면 축: 부품 component 위치(문·계기)가 BP 정면 면 쪽에 있는지
- 재질 슬롯·MI·텍스처 압축·sRGB 설정
- UCX 개수, collision profile이 변경 전과 같은지

## 6. 사용자 PIE 검증(6단계)

마스터가 `PIE_CHECKLIST.md`(단계 값: 사용자 PIE 검증)를 만든다. 동선 순서로, 설비마다 해당하는 항목만 넣는다.

| 확인 | 기대 결과 |
|---|---|
| 배치·회수 | 미리보기 외형이 새 mesh이고, 놓을 때 footprint 판정이 이전과 같음 |
| 충돌·이동 | 플레이어·손님이 몸체에 막히고 끼지 않음, 손님 길(Nav)이 이전처럼 생김 |
| 움직이는 부품 | 문이 정면 바깥으로 열리며 프레임을 뚫지 않음, 레버가 가이드 범위로 움직임, 바늘이 다이얼 눈금 호를 따라감 |
| 진열·서는 위치 | 진열품이 상판·선반 위에 놓임(떠 있거나 묻히지 않음), 손님이 설비 앞에 맞게 섬 |
| 외형 | 조명 아래 색·노화가 승인 렌더와 크게 다르지 않음, 텍스처 늘어짐·검은 면 없음 |

실패하면 [AGENT_ORCHESTRATOR.md](AGENT_ORCHESTRATOR.md) 사용자 PIE 실패 절차를 따른다.
- 외형·pivot 결함: 2단계(모델 재작업)
- 설정 결함: Editor 재작업(`PROMPT_UNREAL_R.md`)
- 코드 계약 결함: 소유 단계

## 7. 커밋·브랜치

- ArtSource(모델 원본)는 승인 시점에 커밋한다(2절).
- Content 변경(4·5단계)은 `work/MODEL-<n>` 브랜치에서 Editor 작업 단계 커밋으로 남긴다. 사용자 PIE 통과 뒤 `--no-ff`로 병합한다(사전 허용이 없으면 먼저 묻는다).
- 다른 작업이 Editor·Content를 쓰는 중이면(예: 진행 중인 `work/<다른 작업>`) import를 시작하지 않는다. 그 작업을 단계 경계에서 보류하고, Editor를 닫고, 브랜치를 정리한 뒤 시작한다. Editor가 열린 채 브랜치를 바꾸지 않는다.
- 모든 에셋의 PIE 통과와 병합을 확인한 뒤 작업 폴더를 제거한다.

## 8. 재작업 규칙

| 상황 | 돌아갈 곳 | 다시 할 범위 |
|---|---|---|
| 색·노화·티어만 바꿈 | 2단계 `palettes.json` 수정 후 `run_all.sh` | 텍스처 재import(같은 이름으로 덮어쓰기). mesh·BP는 그대로 |
| 형상 수정, pivot 불변 | 2단계 | FBX 재import(Reimport), BP는 그대로 |
| pivot·부품 분리 변경 | 1단계 조사 확인 후 2단계 | FBX 재import와 BP 조정 다시 |
| BP 계약과 안 맞음(새 축·component 필요) | 기능 명세 또는 아키텍처 | 그 항목만 멈춤 |
