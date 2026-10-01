# PROMPT_IMPLEMENTATION_R — UNBOX-SPAWN-VIEW 코드 리뷰 1회차 재작업

- 작업 ID: `UNBOX-SPAWN-VIEW`
- 단계: 코드 리뷰
- 상태: 완료
- 출처: 코드 리뷰 1회차

## 0. 범위와 판정

- 리뷰 범위: `git diff bd95749 515c7d2`(구현 커밋 `515c7d2`). 입력: `PROMPT_ARCHITECTURE.md`(USV-001~021), `PROMPT_IMPLEMENTATION.md`(재작업 `98db7d6` 반영), `PROMPT_REVIEW.md`, `PROMPT_UNREAL.md`.
- 제품 코드(`PlayerViewFrontPlacement`, `ShopUnboxingPlacement`, `ShopUnboxingCluster`, `ShopUnboxingTuning`, `UShopSettings`, `ShopUnboxingTransaction`, `TrashBagDropPlacement`, `ALitterTongsActor`)는 승인 계약·설계와 일치한다. 단계 순서, 시야 기준점(카메라/capsule 중심/capsule 윗면), P9 몸 겹침 허용, 봉투 Pawn 검사 없음, P10 push, 기본값에서 바닥 정면·머리 위·무리 결과 보존, 원본 읽기 지점 두 곳(`FShopUnboxingTuning::FromSettings`, `BuildTieDropRequest`)을 확인했다. **제품 코드는 고치지 않는다.**
- 재작업 대상은 자동화 테스트뿐이다. 조정값 원본 원칙(`AGENT_WORKFLOW.md`, 테스트 기대값 리터럴 복제 금지)과 설계 10절의 명시 단언을 지키지 않은 곳이 있다.

## 1. Finding

### R1 (중요) 갱신한 기존 테스트에 조정값 기대값·fixture 리터럴이 남음

설계 10.2: "`BagScaleAndFrontDrop`의 바닥 정면 기대값(높이, 당김 한 단계, offset mesh 중심)은 … request 값에서 계산해 검증한다", "기존 숫자 기대값(바닥 정면 거리·바닥 여유·당김 간격 …)은 원본 읽기로 바꾼다". 설계 10 공통: "벽·천장 fixture는 설정값 기준 상대 위치로 둔다".

- `Source/BathhouseSim/Private/Tests/CleaningLitterToolAutomationTests.cpp` `FCleaningLitterBagScaleTest`
  - `Transform.GetLocation().Z == 30.0`: 봉투 half Z + `TieFloorClearanceCm` 리터럴 복제.
  - `Transform.GetLocation().X == 50.0`: `TieForwardDistanceCm − TieForwardPullStepCm` 리터럴 복제.
  - `QueryCenter.Equals(FVector(60, 0, 55))`: `TieForwardDistanceCm`, offset mesh half Z + `TieFloorClearanceCm` 리터럴 복제.
  - fixture `FarWall` X=109, `Wall` X=55: 기본 거리 60·당김 10을 전제로 한 절대 위치.
- `Source/BathhouseSim/Private/Tests/ShopAutomationTests.cpp` `FShopFreshInstallTrashAndUnboxingAutomationTest`
  - 752행 `FootLocation.Z + 20.0f`(생성 actor 충돌 밑면): `FloorStageTuning.ForwardFloorClearanceCm` 리터럴 복제.
  - 844행 `BlockedRowCenter`의 `+ 20.0f`: 바닥 여유 기준 fixture가 리터럴.
  - 825~829행 벽 여유 단언의 `8.0f`/`4.0f`: 이 테스트가 넘긴 겹침 깊이 입력(`MakeFloorStageTuning(…, 8.0f)`)과 Dc/2의 복제. 사용한 tuning 값(`OverlapDepthCm`, `* 0.5f`)에서 계산한다.

수정 방향:

- 봉투 기대값은 `TieFloorOnlyRequest(World, Player)`로 얻은 request(`FloorForwardDistanceCm`, `FloorPullStepCm`, `FloorClearanceCm`)와 `ATrashBagActor::BuildClassCollisionQuery`의 shape로 계산한다. 벽 fixture는 request 거리·간격과 봉투 half extent 기준 상대 위치로 둔다(예: 첫 후보만 막는 벽 = 발 + 전방 × (`FloorForwardDistanceCm` + half X) 바로 앞, 두 번째 후보는 통과).
- 상점 기대값·fixture는 같은 테스트의 `FloorStageTuning`(또는 각 호출에 넘긴 tuning) 멤버에서 계산한다.
- 위 두 파일의 나머지 unbox·tie 관련 단언도 같은 기준으로 다시 점검한다. 값 자체를 고정해야 하는 회귀 단언이 있으면 이유를 주석으로 남긴다.

### R2 (중요) USV-015 품목 박스 혼합과 대표 무리 거리 단언이 설계 10.1보다 약함

설계 10.1 `.OpenAndRepresentative`: "샤워기 1개·SHOP-033 구성 4개·설비와 박스 혼합: seed 목록 모두 ViewFront, 모든 꼭짓점 V 투영 ≥ `UnboxViewDistanceCm`(허용 오차)이고 최솟값 = 그 값".

- `Source/BathhouseSim/Private/Tests/ShopUnboxViewFrontAutomationTests.cpp` `FShopUnboxViewFrontOpenAutomationTest`
  - "mixed facilities"는 `RepeatIndices(5)`로 설비 정의만 싣는다. 품목 박스 shape가 하나도 없고(AddInfo도 "facility-only mix"를 기록), `bRequireViewFront=false`라 과반만 본다. 품목 박스(정육면체가 아닌 유일한 개봉 shape, yaw별 support function 차이가 실제로 나는 경우)의 시선 앞 배치가 자동화로 검증되지 않는다.
  - "four representative items"는 `bExpectExactDistance=false`라 최솟값이 당김 거리열 안인지만 본다. 천장 없는 트인 곳 pitch 0에서는 당김이 일어날 이유가 없으므로 최솟값 = `UnboxViewDistanceCm`을 단언해야 한다.
- `PROMPT_REVIEW.md` 2절은 USV-015를 "혼합 5개"로 검증했다고 적었으나 실제로는 품목 박스가 없다.

수정 방향:

- 혼합 case에 `FShopUnboxItemShape::FromItemBoxClass`로 만든 품목 박스 shape를 섞는다(클래스는 `AItemBoxActor::StaticClass()` 또는 Settings 클래스, 기존 `ServiceFacilityShopAutomationTests.cpp`의 방식 참고). 픽스처의 `Find`·`ResolveBoxes`가 정의 배열만 다루므로 shape 목록·결과 shape 해석을 품목 박스까지 받게 확장한다.
- 혼합 case와 대표 4개 case는 seed 목록 모두 ViewFront이고 V 투영 최솟값 = `UnboxViewDistanceCm`(허용 오차)으로 단언한다. 실제로 당김이 일어나는 seed가 있으면 단언을 낮추지 말고 원인(어느 검사가 막는지)을 `QNA_IMPLEMENTATION.md`에 기록하고 멈춘다.
- `PROMPT_REVIEW.md` 2절 USV-015 행을 실제 검증 내용으로 고친다.

### R3 (경미) `.RoomPhysics`가 설계 단언 일부를 빠뜨림

설계 10.1: "닫힌 방 … tick 뒤 모두 벽 안·바닥 위·천장 아래, 최저 밑면이 바닥 근처".

- `ShopUnboxViewFrontAutomationTests.cpp` `FShopUnboxViewFrontRoomPhysicsAutomationTest`는 방 반폭 6000cm·천장 4000cm라 벽 안 단언이 사실상 항상 참이고, "최저 밑면이 바닥 근처" 단언이 없다. 단계(`Stage`)도 기록하지 않는다.
- 수정 방향: 방 크기를 무리 크기·설정 거리 기준 상대값으로 줄여 벽·천장 근처에서 생성·튐이 일어나게 하고, tick 뒤 각 물품 충돌 밑면이 바닥 위 허용 오차 안(낙하 완료)임을 단언한다. 채택 단계를 AddInfo로 남긴다. 기존 `Shop.UnboxingPhysics` 방 테스트의 상수(같은 6000/4000)는 범위 밖이므로 바꾸지 않는다.

### R4 (경미) 정리

- `ShopUnboxViewFrontAutomationTests.cpp` 775행 `DistanceTolerance + Tuning.CameraClearanceCm * 0.0f`: 의미 없는 항을 지운다.
- `ShopUnboxingCluster.cpp` 204행 `PairLimit + 0.01f`: 구현자 질의 (2)에 대한 판단은 아래 2절. 동작에는 손대지 않고, 같은 파일의 `AxisEpsilon`처럼 이름 있는 수치 안정 상수로 바꾸는 것은 선택이다(난수 소비·결과 불변 조건 유지).

## 2. 구현자 질의에 대한 리뷰 판단(재작업 불필요)

1. FinalStack 카메라 규칙 없음: 기능 계약 5.3의 4단계 "이 단계만 카메라를 감쌀 수 있다"와 일치한다. `PROMPT_UNREAL.md` 2절 관찰 항목에 이미 인계돼 있다.
2. `PairLimit + 0.01f`: 조정 가능한 허용 오차(`UnboxClusterDepthToleranceCm`) 위에 더하는 float 비교 여유로, 설계 4.4 첫 행(0 판정·수치 안정)과 같은 성격이다. 예외로 인정한다. 정본·설계 수정은 요구하지 않는다.
3. 기존 회귀 테스트의 무효 간격 단계 건너뛰기: 허용한다. `BuildPullDistances`/`IsStageValid`의 "무효 값이면 그 단계만 건너뜀"은 문서화된 계약이고, 시선 단계 → 바닥 정면 전환은 판으로 막는 새 테스트(`FloorFrontUnchanged`, `PitchAndFloor`, `TieViewFront.Placement`·`TuningSource`)가 검증한다. 단 R1대로 기대값은 원본에서 계산해야 한다.
4. `LitterTongsActor.cpp` 397→437줄: 늘어난 내용은 설계가 지정한 값 읽기 지점(`BuildTieDropRequest`)과 새 property의 `IsDataValid` 검사뿐이며 독립 책임이 아니다. 예외로 인정한다. 400줄 경고선을 넘었으므로 이후 이 파일에 새 책임을 더하지 않는다(정본 반영은 아키텍처 판단).

## 3. 금지 범위

- 제품 Source(`Private/Shop/*`, `Private/Cleaning/*`, `Private/Interaction/*`, `Public/*`) 동작 변경. R4의 선택 정리 외 제품 코드 수정 없음.
- 테스트를 통과시키려고 단언·허용 오차를 완화하거나 seed를 골라 내는 것.
- Content·Config 수정, `AGENT_*.md`·정본 수정.

## 4. 재검증 조건

- UE 5.8 빌드 성공, 경고 증가 없음(`UE_BUILD_POLICY.md`).
- Automation 전부 통과: `BathhouseSim.Interaction.ViewFrontPlacement`, `BathhouseSim.Shop`, `BathhouseSim.Cleaning`, `BathhouseSim.Service.Shop`, `BathhouseSim.Service.Amenity.Shop`, `BathhouseSim.Interaction.Equipment`. 로그 경로를 `PROMPT_REVIEW.md`에 남긴다.
- `PROMPT_REVIEW.md` 갱신: R1~R4 처리 내역, USV-015 검증 내용 정정, 혼합·대표 case의 ViewFront 비율과 최솟값 단언 결과, 빌드 시점 Source 식별값(재현 가능한 방식: 커밋 해시 또는 `git diff <기준> -- Source` SHA-256과 그 명령).
- `git status --short -- Content Config` 출력 없음.
- 재검증은 같은 리뷰어가 R1~R4와 새 diff만 본다.
