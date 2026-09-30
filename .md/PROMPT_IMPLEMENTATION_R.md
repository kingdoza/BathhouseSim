# 재작업 프롬프트 — 서비스 3단위: 쓰레기·물 얼룩 생성 자리 판정 보완

## 재검토 결론

- 2026-10-01 코드 리뷰 결론: **아키텍처 재검토**(F1 규칙 확정) 후 **구현 재검토**(F1~F3).
- 입력
  - `.md/PROMPT_REVIEW.md`(서비스 3단위 C++ 완료)와 현재 작업 트리
  - 기능 계약 `.md/PROMPT_ARCHITECTURE.md`: TRSH-001~028, COLL-001~009
  - 단계: **수직 구현(단위 확장)**
- 확인된 사항
  - 작업명·시나리오 ID·단계가 입력 문서 사이에서 일치한다.
  - UE 5.8 `Build.bat` 성공. 로그 `build_10_pass` 이후 바뀐 Source가 없다.
  - 전체 회귀 `Full05` 112/112 통과(성공 101, 경고 포함 성공 11, 실패 0, 미실행 0). 신규 Cleaning 12개도 모두 성공이다.
  - copy-first 로드와 DefaultMap 로드가 각각 1/1 성공했다.
  - Content·Config 변경이 없다.
- Editor 단계로 넘기지 않는다. F1 때문에 대표 시나리오 "탈의실 구역에 쓰레기가 생긴다"와 TRSH-019(물 얼룩 생성)가 실제 DefaultMap에서 성립하지 않을 가능성이 높다.

## F1 — 치명: 배치 구역 박스가 바닥 전체의 생성을 막는다 (아키텍처 결정 필요)

### 근거

1. `FCleaningFloorSpawnQuery::Find`(`Private/Cleaning/CleaningSpawnRules.cpp`)의 clearance 검사
   - `OverlapMultiByObjectType(WorldStatic | WorldDynamic | PhysicsBody)`를 쓰고, 물 얼룩·쓰레기가 아닌 Actor가 하나라도 걸리면 기각한다.
   - 검사 박스 범위: 바닥 위 1cm ~ 1cm + `SpawnClearanceHeightCm`(기본 30cm).
2. UE의 오브젝트 타입 query는 **상대 컴포넌트의 충돌 응답을 보지 않는다.**
   - query가 켜져 있고 오브젝트 타입이 목록에 있으면 결과에 포함된다.
   - 따라서 모든 채널을 Ignore하는 QueryOnly 트리거 박스도 "겹침"으로 잡힌다.
3. `AFacilityPlacementZoneActor::ZoneBounds`(`Private/Placement/FacilityPlacementZoneActor.cpp` 27행)
   - QueryOnly이고, 전용 zone trace 채널만 Block하며 나머지 응답은 Ignore다.
   - 오브젝트 타입은 기본값(WorldDynamic 계열)이라 위 query에 포함된다.
4. DefaultMap 배치 구역(`.md/Unreal/WorldSystem.md`)
   - Location `(600,-100,0)`, `ZoneBounds` Extent `(1400,900,10)`
   - 바닥 Z=0 기준 −10~+10cm 높이로 2800×1800cm를 덮는다.
   - clearance 박스(바닥 위 1~31cm)와 높이 1~10cm 구간이 겹친다.
5. 결과: 배치 구역 안에서는 **모든 후보가 clearance에서 기각**된다. 그 범위의 쓰레기·물 얼룩이 생기지 않는다.
   - 기존 물 얼룩 생성 구역이 배치 구역 안에 있으면 이번 변경 전에는 생기던 물 얼룩도 멈춘다(회귀).
6. 자동화가 놓친 이유: `FloorAndVariation` fixture에 배치 구역이나 QueryOnly 트리거 박스가 없다.
7. 같은 성질의 다른 QueryOnly 박스(진열 공간, 설비 router, 수건 투입구 등)는 설비 주변이라 기각되어도 계약상 문제없다. 바닥 전체를 덮는 배치 구역이 문제다.

### 아키텍처 단계가 정할 것

리뷰는 규칙을 고르지 않는다. `CleaningLitterSystem.md` Floor Rule 5단계를 다음을 만족하도록 확정한다.

- 막아야 하는 것(계약 TRSH-004): 설비 몸체, 바닥에 놓인 물건(품목 박스·설비 아이템·배송 상자·묶은 봉투·도구·열쇠·바닥 수건), 벽 가장자리
- 막지 말아야 하는 것
  - 배치 구역처럼 영역만 표시하는 query 전용 박스
  - 물 얼룩·쓰레기(Q62 B)
  - 생성 구역·수거 구역 자신
- 판정 기준을 정한다. 후보(선택은 아키텍처 단계):
  - 충돌 응답 기반 판정: 가상의 바닥 물체 template 또는 채널을 정하고 Block 응답만 인정한다. `FacilityPlacementCollision::HasBlockingOverlap`과 같은 방식이다.
  - 오브젝트 타입 query는 유지하되 특정 응답 조건이나 class를 제외한다.
  - 그 밖의 방식
- 바닥 자체는 막지 않아야 한다. 경사가 허용 범위(기본 25°) 안이면 clearance 박스가 바닥 면과 겹치지 않도록 기준 높이를 정한다. 현재는 바닥 위 1cm부터라 약 4° 이상 경사 바닥이나 줄눈 단차에서 바닥 자체와 겹쳐 기각될 수 있다(아래 F2).

### 재검증 조건

- 바닥 판정 자동화에 다음 두 경우를 추가한다.
  - DefaultMap과 같은 구성의 `AFacilityPlacementZoneActor`(QueryOnly, 바닥을 덮는 얇은 bounds) 안에서 생성이 성공한다.
  - 같은 영역에 겹쳐 둔 물 얼룩 구역·쓰레기 구역이 서로를 막지 않는다.
- 기존 기각 경우는 그대로 기각된다: 설비 윗면, 박스, 벽, 욕탕 바닥, Pawn.
- director 통합 경로(실제 zone `FindSpawnTransform`)로 배치 구역 안 구역에서 쓰레기·물 얼룩이 생기는 것을 확인한다.
- `PROMPT_UNREAL.md` PIE 절차에 "배치 구역 안 탈의실·욕실 바닥에서 손님이 있을 때 실제로 생성됨"을 관찰 항목으로 명시한다.

## F2 — 보통: 경사·단차 바닥에서 바닥 자체가 clearance에 걸림 (F1과 함께 정리)

- clearance 박스는 hit 지점 위 1cm부터 수평 사각형(반폭 R)으로 검사한다.
- 반폭 15cm(쓰레기)·30cm 이상(물 얼룩) 박스는 경사 약 4°(쓰레기)·2°(물 얼룩)를 넘거나 1cm 이상 단차가 있으면 바닥 mesh와 겹친다.
- 경사 허용값 `MaximumSlopeDegrees`(기본 25°)와 실제 판정이 맞지 않는다.
- F1의 판정 기준을 정할 때 함께 정리하고, 경사 바닥 fixture를 자동화에 추가한다.

## F3 — 낮음: 복사 흔적 LOCTEXT 키

- 문구는 맞지만 다른 class에서 복사한 키 이름이 남아 있다.
  - `LitterTongsActor.cpp`: `TakeMop`, `MopNotHeldForDrop`
  - `TrashBagActor.cpp`: `InvalidBoxActivation`, `BoxPhysicsFailed`, `BoxNotDiscardable`, `InvalidBoxAuthoring`
- 현지화 키 혼동을 막도록 이 class에 맞는 키로 바꾼다. 동작은 바꾸지 않는다.

## 유지할 것 (리뷰에서 문제없음)

- **발생 clock**
  - Exp(1) 표본, n=0 불변, update당 구역 1회, 이월 없음, 구역 weak key 관리
  - 손님 위치는 update마다 한 번만 수집한다.
- **RMB 입력 순서**
  - Computer·배치·held-use·LMB 장비 입력 중에는 무시한다.
  - 보조 사용이 있는 장비는 실행하고, 없는 장비는 무시한다(기존).
  - 그 밖에는 held-use Take다. 해제는 보조 사용 owner면 아무것도 하지 않는다.
- **HUD 합성**: `MergeEquipmentQuery`가 held-use 필드를 지운 뒤 보조 사용을 RMB 행(`HeldTake*`, Instant)으로 채운다. 쓰레기 아닌 곳에서 LMB는 행도 결과 보고도 없다.
- **집게**
  - 줍기는 Instant 1개이고, 쓰레기 제거 → 개수 증가 순서다.
  - 묶기는 정면 자리 찾기 → 봉투 생성 → 성공한 뒤에만 개수를 0으로 만든다.
  - carry·fixed slot·복구는 물걸레와 같은 구조다.
- **봉투**
  - 생성 시 개수를 한 번만 초기화하고, 활성화가 실패하면 제거한다.
  - held·world 버리기 모두 Consumed 상태로 전이한 뒤 Destroy한다.
- **world 버리기**
  - 네 휴대물이 held 판정과 같은 private 종류 함수를 쓰고, FreeWorld에서만 가능하다.
  - 설비 아이템은 `PlacementConsumed` 상태라 EndPlay의 carrier 통지가 생략된다.
- **수거 구역**
  - AllObjects overlap 결과에서 discardable·FreeWorld 휴대물만 고른다.
  - carry primitive bounds 중심으로 안팎을 판정한다.
  - 목록을 먼저 만든 뒤 제거한다. held 물건은 collision이 꺼져 있어 제외된다.
- **배치 확정 이벤트**
  - 성공 publication 블록 안에서만 한 번 발행하고, preview·실패·취소에서는 발행하지 않는다.
  - 정리 handler는 제거 목록을 먼저 만든 뒤 제거하고, Placement는 Cleaning을 참조하지 않는다.
- **쓰레기 본체**: Visibility만 Block이라 물 얼룩 조준을 가린다(Q62 B). Pawn·물건·Navigation에는 영향이 없다.
- **삽**: 장비가 아니라 held-use 경로로 "집게가 필요합니다"가 표시된다(TRSH-020).

## 재작업 후 리뷰 입력

- 갱신된 `CleaningLitterSystem.md` Floor Rule(F1·F2)과 `PROMPT_IMPLEMENTATION.md` 재작업 절
- 재작성된 `PROMPT_REVIEW.md`: 선택된 판정 기준, 배치 구역·겹친 구역·경사 fixture 자동화, 전체 회귀 수치
- `PROMPT_UNREAL.md` PIE 관찰 항목 보강
- Content·Config 무변경을 유지한다. native 구조가 바뀌지 않으면 load gate를 다시 실행할 필요는 없다.
