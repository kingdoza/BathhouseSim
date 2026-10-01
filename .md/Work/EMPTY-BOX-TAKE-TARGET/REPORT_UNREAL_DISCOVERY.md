# Editor 사전 조사 — EMPTY-BOX-TAKE-TARGET 빈 박스 빼기 대상 선택 통일

- 작업 ID: `EMPTY-BOX-TAKE-TARGET`
- 단계: Editor 사전 조사
- 상태: 완료

(Editor 워커의 Write가 하네스에 거부되어 완료 보고 전문을 마스터가 사용자 승인 후 저장함, 2026-10-01)

## 명세 가정과 다른 사실·사용자 선택에 영향을 줄 사실

1. **P1(보이는 진열 물품은 모두 설비 조준 영역 안)은 현재 Content에서 성립한다.** 화장대·샤워기의 모든 자리 물품 bounds가 각 설비 조준 박스 안에 완전히 들어간다(초과 0cm). 조준 영역을 조정하는 Content 작업은 필요 없다. 8절 기준으로 수직 구현 조건(영역 조정 필요)에 해당하지 않는다.
2. **샤워기 조준 높이 문제는 없다.**
   - `ShowerDisplayTarget`은 로컬 z 20~80이다(중심 50, ±30).
   - 샴푸 물품은 z 56~70, 바디워시 물품은 z 56~68이다(묶음 z 58, 자리 z +5/+4).
   - 샤워기 몸체 `FacilityVisual`은 80×110×12 판(z 44~56)이고 조준 박스 안에 있다.
3. 조준 박스보다 카메라 쪽(박스 밖)에 Visibility를 막는 component는 세 설비 모두 없다.
   - 화장대 `VanityBody`(z 0~100)와 샤워기 `FacilityVisual`은 조준 박스 안에 완전히 들어간다.
   - 냉장고 `FridgeBody`는 공간 박스 뒤쪽(x −30~10)에 있다.
4. DefaultMap에는 **화장대·냉장고 instance가 없고** 샤워기 instance는 1개다(override 없음). 화장대·냉장고 값은 BP 템플릿(SCS) 기준이다. 상점에서 사 배치하는 instance는 BP 기본값을 따를 것으로 본다.

## 범위와 방법

- 범위: `PROMPT_ARCHITECTURE.md` 7절. 읽기 전용이며 mutation·Compile·Save·PIE는 하지 않았다.
- 실행 방식: 숨김 Editor의 `-ExecCmds="py …"`로 DefaultMap 로드 후 실행했고, 전후 dirty는 0개였다.
- 근거 파일: `Saved/Claude/Discovery3/disc_01_probe.py`, `probe.json`(display·item_defs 절), `probe_run3.log`.
- BP 값 읽는 법: `SubobjectDataSubsystem`으로 SCS 템플릿을 읽고 부모 체인과 relative transform을 합성해 actor 공간을 만들었다. 부모는 모두 `SceneRoot`이고, `SceneRoot`의 부모는 `PackagePhysicalRoot`이며 둘 다 identity다.
- 샤워기 instance 값 읽는 법: world transform으로 같은 계산을 다시 했고 결과가 BP 계산과 일치했다.
- 물품 bounds 계산식: 품목 진열 mesh bounds × `DisplayOffset` × `SlotTransforms[i]` × 묶음 transform. Source `DisplaySpaceComponent.cpp` 256행 `Kind.DisplayOffset * SlotTransforms[i]`, ISM은 묶음에 부착된다.

## 1. 품목 정의 (`/Game/Bathhouse/Data/Service/DA_ServiceItem_*`)

모든 품목은 `DisplayMesh`가 비어 있어 `ItemMesh`를 쓴다. mesh bounds는 원점 0, extent 50이고 `DisplayOffset`은 위치·회전 0이다(빗만 yaw 90).

| 품목 | ItemMesh | DisplayOffset scale | 진열 크기(cm) | 정원·횟수 |
|---|---|---|---|---|
| HairDryer | Cube | (0.18,0.08,0.14) | 18×8×14 | 2·0 |
| SkinLotion | Cylinder | (0.05,0.05,0.12) | 5×5×12 | 6·10 |
| CottonSwab | Cylinder | (0.06,0.06,0.08) | 6×6×8 | 6·30 |
| Comb | Cube, yaw 90 | (0.08,0.03,0.02) | 8×3×2 (회전 뒤 x3·y8) | 12·0 |
| Shampoo | Cylinder | (0.06,0.06,0.14) | 6×6×14 | 6·30 |
| BodyWash | Cylinder | (0.06,0.06,0.12) | 6×6×12 | 6·30 |
| BananaMilk | Cylinder | (0.06,0.06,0.12) | 6×6×12 | 12·0 |

## 2. `/Game/Bathhouse/Blueprints/Service/BP_Vanity` (BP 템플릿, actor 공간)

- 조준 박스 `DisplayTarget`
  - 위치 (0,0,100), extent (45,75,100). 범위는 x ±45, y ±75, z 0~200.
  - QueryOnly, profile Custom, Visibility Block.
- 묶음은 모두 z 105이고 NoCollision이며 FixedKind가 해당 품목이다. 모든 자리의 물품이 조준 박스 안에 있다.

| 묶음 | 위치 | 자리 (묶음 로컬) | 물품 범위 x / y / z |
|---|---|---|---|
| `DryerGroup` | y −45 | 2: (0,∓9,2) | ±9 / −58~−32 / 100~114 |
| `LotionGroup` | y −15 | 4: (±5,∓6,1) | ±7.5 / −23.5~−6.5 / 100~112 |
| `SwabGroup` | y 15 | 4: (±5,∓7,−1) | ±8 / 5~25 / 100~108 |
| `CombGroup` | y 45 | 6 | ±9.5 / 32~58 / 100~102 |

- 물품은 모두 몸체 윗면(z 100) 위에 놓인다.
- Visibility 차단 component는 `VanityBody` 하나다(QueryAndPhysics, x ±30, y ±60, z 0~100, 조준 박스 안).
- `MirrorVisual`은 NoCollision이다. root `PackagePhysicalRoot`·`PlacementFootprint`는 Visibility를 막지 않는다.
- 플레이어 카메라(발바닥 위 166cm)는 몸체에 막혀 조준 박스 안으로 들어갈 수 없다. 앞에서 몸체를 맞고 선 capsule 중심은 x ≥ 60 > 45다.

## 3. `/Game/Bathhouse/Blueprints/Facility/BP_Shower`

- BP 템플릿
  - `ShowerDisplayTarget`: (0,0,50), extent (45,60,30), z 20~80, QueryOnly, profile Custom, Visibility Block.
  - `ShampooGroup`: (0,−25,58), 자리 (0,∓7,5)
  - `BodyWashGroup`: (0,25,58), 자리 (0,∓7,4)
  - 물품 범위: 샴푸 x ±3, y −35~−15, z 56~70 / 바디워시 y 15~35, z 56~68. 모두 안쪽이다.
- 차단 component
  - `FacilityVisual`(Cube): (0,0,50), scale (0.8,1.1,0.12), QueryAndPhysics, 범위 x ±40, y ±55, z 44~56으로 조준 박스 안이다.
  - 그 밖의 primitive는 Visibility를 막지 않는다.
- DefaultMap instance
  - label `Shower`, actor (1400,−100,0), yaw 0
  - package `/Game/__ExternalActors__/Maps/DefaultMap/9/XA/39H4HRXUJOX8YUYR10K04W`
  - override 없음(relative transform·box extent·slot 비교). world 조준 범위 z 20~80, 네 자리 모두 안쪽이다.
- 판 윗면(z 56)이 플레이어 step height(45)보다 높아 플레이어는 판 위에 올라설 수 없다. 카메라(약 166)는 조준 박스 위쪽 밖이며, 위에서 내려다보면 조준 박스 윗면(z 80)에 먼저 맞는다.

## 4. `/Game/Bathhouse/Blueprints/Service/BP_DrinkFridge` (BP 템플릿)

- 공간 4개(`DisplaySpace0~3`)
  - 위치 x 20, z 33/78/123/168. extent (10,28,8)이므로 범위는 x 10~30, y ±28이다.
  - 높이 범위: z 25~41 / 70~86 / 115~131 / 160~176.
  - QueryOnly, Visibility Block. 각 6자리 (±4, −16/0/16, −2).
- 바나나우유 물품: x 13~27, y ±19, 각 공간 z 하단~+12(예: 25~37). 24자리 모두 자기 공간 박스 안이다.
- 차단 component
  - `FridgeBody`: (−10,0,90), scale (0.4,0.6,1.8), QueryAndPhysics, 범위 x −30~10, y ±30, z 0~180.
  - 공간 사이 높이(41~70, 86~115, 131~160)를 조준하면 `FridgeBody` 앞면(x 10)에 맞는다. 명세 2.2의 "공간 사이는 몸체에 막힘"과 일치한다.
  - `ShelfVisual0~3`은 NoCollision이다.
- DefaultMap instance는 없다.

## 미확정

- 품목이 실제 mesh로 바뀌면 bounds를 다시 확인해야 한다(현재 엔진 기본 Cube·Cylinder).
- 레벨 다른 actor의 가림과 실제 첫 Visibility hit는 PIE 관찰 대상이다(사용자 몫, EBT-014).

## 기준선과 종료

- 시작 커밋 `2c24374`. 전후 git status는 Content 변경 없음, dirty 0개다.
- 작업용 Editor는 모두 종료됐고 저장한 것은 없다.
