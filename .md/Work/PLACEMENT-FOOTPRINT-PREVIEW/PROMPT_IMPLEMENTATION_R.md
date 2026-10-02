# PROMPT_IMPLEMENTATION_R — PLACEMENT-FOOTPRINT-PREVIEW 배치 미리보기의 footprint 공간 표시

- 작업 ID: `PLACEMENT-FOOTPRINT-PREVIEW`
- 단계: 코드 리뷰
- 상태: 완료
- 출처: 코드 리뷰 1회차

## 0. 리뷰 대상과 결론

- 대상: `git diff 0c5f8a0 fd7f87d`(구현 커밋 하나). `81a1c4c`(지도 버그 병합)와 `BP_ClothesLocker.uasset` 작업 트리 변경은 대상이 아니다.
- 결론: **구현 재검토(작은 범위)**. production 코드(Settings, Geometry, Definition, FootprintPreview helper, PreviewActor, Config)에는 수정할 결함이 없다. 고칠 것은 Editor 인계 문서의 검증 누락 하나, 설계 T2와 다른 테스트 fixture 하나, 정본·보고서 기록 둘이다.
- 빌드 식별값: `PROMPT_REVIEW.md`의 diff SHA-256(`e23e9022…`)과 신규 파일 SHA-256(`7f620b90…`)이 `fd7f87d`와 같다. `UnrealEditor-BathhouseSim.dll`(23:11:31)이 마지막 Source 변경(23:11:23) 뒤에 만들어졌다. 리뷰어는 다시 빌드하지 않았다.

## 1. 확인한 것 (재작업에서 바꾸지 않는다)

- 판정 불변(FPV-013): `ValidateCurrentPlacement`·`ValidateWorldPlacement`·`UPlayerFacilityPlacementComponent`·transaction·Zone grid 무변경. footprint 표시는 판정 경로를 호출하지 않는다.
- 같은 원천: 표시 = `PlaneLocal * SourceFootprintRelative`를 preview Actor 자식으로 두고, Actor transform은 판정과 같은 `CurrentCandidate`(`BuildPlacedActorTransform`, scale = root scale)다. snapshot은 매 refresh에 `ValidateSourceGeometry`가 CDO와 비교한다. FTransform 합성으로 계산하면 평면 네 모서리 = `RelativeFootprint * Candidate` 바닥 모서리 + 바닥 법선 × 높이다. non-uniform scale이나 footprint Yaw가 있어도 성립한다. 크기 parameter는 `ComputeScaledFootprintFullSize` 하나로 `DeriveFootprintCells`와 함께 쓴다.
- D4·확정·파괴: 같은 Actor의 component라 `SetActorHiddenInGame`·`Destroy`를 공유한다. 색은 `SetPlacementValidity`가 같은 호출에서 바꾼다. 초기 색은 무효 색이다.
- fail-open 범위: 표시 준비는 메시·재질 적용이 성공한 뒤에만 한다. 실패하면 component·DMI를 정리하고 `LogTemp` Warning을 한 번 남긴다(기존 Zone grid와 같은 category). 초기화는 성공한다. 메시 초기화 실패(FPV-015)에서는 표시를 만들지 않는다.
- 조정값 원본: 높이와 두 우선순위는 Project Settings(`DefaultGame.ini` 1·1·2 = grid `GridZOffsetCm`+0.5, grid 우선순위 0+1, +1). C++ 기본값은 예비값이다. 코드 상수는 설계 9절 예외(parameter 이름 4개, `2*`, 허용 오차, plane Z scale 1)뿐이다.
- `DeriveFootprintCells` 변경 범위: placed instance는 root relative scale = Actor world scale이라 결과가 같다. CDO 경로에서는 조사 보고 2절의 root 합성값과 같아져 쿨러만 비정수가 된다. 자동화 근거: `ActorReplacementTransaction`에서 Bath·Shower·Locker 1/4/8·Washer·Dryer preview 초기화 성공, Shop 테스트에서 Boiler·Circulator 성공. 나머지 6종(Vanity, DrinkFridge, MassageChair, RestBench, Television, ScrubTable)은 Editor 단계 content 테스트로 확인한다.
- Validation Yaw 검사: `IsDataValid`에만 있고 `ValidateRuntime`·`DeriveFootprintCells`에는 없다. 판정은 ±90°·180° 통과, 작은 Yaw 실패다.
- 전체 자동화 기준선 비교(리뷰어 수행): 작업 전 마지막 전체 실행 `Saved/Automation/Reports/20261002/u3_editor3/index.json`은 177개 중 실패 0이다. `fpv_full`(184개)과 비교한 결과는 다음과 같다.
  - 추가된 7개: 이번 작업 6개, 지도 버그 `BathWater.Operations.MapGridLineLayout` 1개.
  - Success→Fail 3개: `Shop.SevenDefinitionSpawn`, `Shop.UnboxingPhysics`, `Shop.UnboxViewFront.RoomPhysics`. 실패 지점이 모두 Definition 5(`ShopUnboxingScatterAutomationTests.cpp`·`ShopUnboxViewFrontAutomationTests.cpp` 목록의 `DA_FacilityPlacement_Cooler`)의 `FootprintGridMismatch`다.
  - 새 Warning: `ActorReplacementTransaction` 15개, `PreviewHiddenWithoutAim` 1개. 모두 `FootprintPreviewMaterial` 미지정 표시 준비 Warning이다.
  - 다른 회귀는 없다.

## 2. Finding과 수정 방향

### F1 (중간) `PROMPT_UNREAL.md`가 Shop 회귀와 전체 자동화 재실행을 인계하지 않는다

- `PROMPT_REVIEW.md`는 Shop 3개 실패를 "BP_Cooler 수정 뒤 재실행해 통과를 확인해야 한다. 이 회귀 판정은 Editor 단계가 한다"라고 적었다. 그런데 Editor 단계의 입력인 `PROMPT_UNREAL.md` 7절에는 `BathhouseSim.Placement.`와 content 테스트만 있다. 이대로면 쿨러 수정 확인과 "다른 회귀 없음" 판정이 아무 단계에서도 이뤄지지 않는다.
- 수정: `PROMPT_UNREAL.md` 7절 검증에 다음을 넣는다.
  - 4·5항(BP 저장)과 3항(MI 지정) 뒤 `BathhouseSim.Shop.` 3개(`SevenDefinitionSpawn`, `UnboxingPhysics`, `UnboxViewFront.RoomPhysics`)를 실행하고 통과를 확인한다.
  - 마지막에 전체 `BathhouseSim`을 실행해 실패 0을 확인한다. 기준선은 작업 전 실패 0(`u3_editor3`)이다.
  - 실패가 남으면 테스트 이름과 메시지를 보고하고 멈춘다(asset 우회 금지).
  - 남는 Warning 중 표시 준비 Warning은 transient 재질 fixture(`Param` 없음)가 원인이면 정상이다. 실제 Definition preview에서 나오면 보고한다.

### F2 (중간-낮음) T2 fixture가 설계 조건("unscaled 정수배, scale 적용 뒤 비정수")을 만족하지 않는다

- 위치: `FacilityPlacementFootprintPreviewAutomationTests.cpp` `CellsIncludeRootScale`의 두 번째 블록, 주석 "Integer unscaled, non-integer after scale (cooler shape)".
- 실제 값: `AFacilityPlacementFootprintScaleProbe`의 합성 scale이 정수(2, 3)다. X extent `Grid*0.75/2`의 unscaled full size는 0.75칸이라 unscaled에서도 비정수다. 그래서 옛 component-to-world 방식(CDO scale 1)으로도 실패한다. 이 블록은 쿨러 결함("Validation이 root scale을 빼서 비정수를 놓침")을 재현하지 못하고, 주석과 값이 다르다.
- 수정: 정수가 아닌 root scale을 가진 probe를 하나 추가한다. 쿨러처럼 `PackagePhysicalRoot` uniform scale 0.5이고, 가능하면 `SceneRoot` Yaw −90°도 준다. 크기는 다음처럼 잡는다.
  - extent는 `GridSizeCm`와 probe에서 읽은 scale로 계산한다(리터럴 결과 복제 금지).
  - fixture가 의미 있는지 assert한다: unscaled full XY ÷ grid가 정수이고, 합성 scale을 적용한 값은 비정수다.
  - 그 위에서 `DeriveFootprintCells` 실패, Definition `IsDataValid` Invalid, `ValidateRuntime` 실패를 확인한다.
  - 기존 첫 블록(2×3칸)은 그대로 둔다.

### F3 (낮음) Architecture 정본 상태 표시가 구현 뒤에도 "Source 미반영·(설계)"다

- `.md/Architecture/PlacementPreviewSystem.md`: 상태 절 "Footprint Display와 Draw Order: … Source 미반영", Source Scope의 "(설계)", Footprint Display 제목의 "(2026-10-02 설계)", Settings·Verification의 "(설계)" 표시가 남아 있다.
- 같은 표시가 `.md/Architecture/PlacementSystem.md` 126행 "Source 미반영"과 `.md/0_ARCHITECTURE.md` 251행에도 있다.
- 수정: 이 항목들을 "Source 반영, Editor 작업·사용자 PIE 전"으로 바꾼다(`AGENT_IMPLEMENTATION.md` 10단계). 구조·API 서술은 바꾸지 않는다.

### F4 (낮음) `PROMPT_REVIEW.md`가 존재하지 않는 로그를 가리킨다

- `Saved/Automation/Logs/fpv_build3.log`, `fpv_build1.log`, `fpv_test2_placement.log`가 없다. 실제로 남은 근거는 다음과 같다.
  - `Saved/Automation/Reports/20261002/fpv_placement/index.json`, `fpv_full/index.json`
  - `Saved/Automation/Logs/fpv_full_editor.log`
  - `Saved/Logs/BathhouseSim-backup-2026.10.02-14.11.53.log`(Placement 12/12)
- 수정: 경로를 실제 파일로 고친다. 재작업 결과도 같은 방식으로 실제 경로를 적는다. 1절의 기준선 비교 결과를 `PROMPT_REVIEW.md` 자동화 절에 반영하고 "기준선 미측정" 문구를 정정한다.

## 3. 범위와 금지

- production Source(`Public/`·`Private/Placement/`)와 `Config/`는 바꾸지 않는다. F2 때문에 바뀌는 것은 테스트 파일과 test probe(`Private/Tests/`)뿐이다.
- Content·`.uasset`·`.umap` 수정과 Editor 저장은 금지다. `BP_ClothesLocker.uasset`은 건드리지 않는다.
- `PROMPT_UNREAL.md`의 다른 항목(1~6, 8, PIE 관찰)은 유지한다.

## 4. 재검증 조건

- UE_BUILD_POLICY 명령으로 빌드에 성공하고, `BathhouseSim.Placement.` 전부 통과한다. 새 T2 블록이 옛 방식이었다면 실패했을 fixture라는 근거(unscaled 정수 assert)가 테스트 안에 있어야 한다.
- `PROMPT_REVIEW.md`에 새 빌드 식별값과 실제 로그·리포트 경로를 적는다.
- 리뷰어는 `git diff fd7f87d` 범위만 다시 본다. F1은 `PROMPT_UNREAL.md` 7절, F3는 정본 상태 문구, F4는 `PROMPT_REVIEW.md`만 확인한다.
