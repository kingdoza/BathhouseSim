# 에이전트 피드백 백로그

이 문서는 작업 중 발견된 결함과 애로사항을 원문에 가깝게 쌓아 두는 임시 기록이다. 여러 항목이 쌓이면 공통 패턴으로 일반화해 각 `AGENT_*.md`에 반영하고, 반영한 항목은 이 문서에서 제거한다.

- 에이전트 규칙 문서가 아니므로 작업 중 이 문서를 지시로 따르지 않는다.
- 항목마다 사실(증상·원인·근거)과 판단(책임·일반화 후보)을 구분한다.

---

## FB-001 회전된 부모 아래에서 피벗 회전 update가 위치를 바꿔 BP 뷰포트 갱신이 멈춤

- 기록일: 2026-09-29
- 상태: 코드 수정 완료, 빌드·자동화·Editor 검증 미실행
- 관련 커밋: `61f72c1` (쿨러,순환기 동작) — `FUtilityPivotRotation` 도입

### 증상

- `SceneRoot`에 회전이 조금이라도 있으면 BP 뷰포트에서 `GaugeNeedlePivot` 위치를 바꿔도 기즈모와 바늘이 움직이지 않는다. 디테일 값과 저장값은 바뀐다.
- Compile로는 갱신되지 않고, BP 탭을 닫았다 다시 열어야 반영된다.
- `BP_Boiler`, `BP_Cooler`, `BP_Circulator` 공통이다. `PackagePhysicalRoot` 스케일은 무관하다(0.5는 부동소수점에서 정확하므로 오차가 생기지 않는다).

### 원인

1. `FUtilityPivotRotation::Apply/Reset`이 `USceneComponent::SetRelativeRotation(FQuat)`을 호출한다. 게이지·연료 문·레버가 Editor 프리뷰의 `OnConstruction`에서 이를 실행한다.
2. 엔진 `SetRelativeRotation`은 상대 트랜스폼을 월드로 합성한 뒤 `MoveComponent → InternalSetWorldLocationAndRotation`에서 부모 기준으로 역변환해 `RelativeLocation`을 다시 쓴다(`SceneComponent.cpp:1723`, 역변환 3303행). FQuat 오버로드에는 동일값 조기 종료가 없다.
3. 부모에 회전이 있으면 이 왕복이 비트 단위로 정확하지 않아 프리뷰 인스턴스의 피벗 위치가 1e-14 수준으로 바뀐다.
4. 템플릿 값의 프리뷰 전파는 `CurrentValue == OldDefaultValue` 정확 비교다(`ComponentEditorUtils.h:223`). 3번 때문에 전파가 계속 건너뛰어진다.
5. BP 에디터는 속성 변경·Compile 시 프리뷰를 재스폰하지 않고 기존 인스턴스 값을 유지한다(`BlueprintEditor.cpp:10637-10638`). 탭 재오픈만 새로 스폰한다.

엔진 동작 자체는 인스턴스별 override 보호를 위한 의도된 설계다. 결함은 우리 코드가 계약과 달리 위치를 바꾼 데 있다.

### 조치

- `Source/BathhouseSim/Private/Utility/UtilityPivotRotation.cpp`: 회전 적용 뒤 `RelativeLocation`이 바뀌면 원래 값으로 복원하는 `SetPivotRelativeRotationPreservingLocation` 추가, `Apply`·`Reset`에 적용.
- `Source/BathhouseSim/Private/Tests/UtilityLaborAutomationTests.cpp`: `BathhouseSim.Utility.Labor.PivotRotationKeepsExactLocationUnderRotatedParent` 추가. 회전된 부모에서 Apply·반복 Apply·Reset 후 위치 bit-exact를 검사하고, 엔진 원 호출의 drift 여부를 Info로 남긴다.

### 단계별 판단

| 단계 | 판단 | 근거 |
|---|---|---|
| 구현 | 주 책임 | `Architecture/UtilityLaborSystem.md:114` "위치와 scale은 회전 update가 바꾸지 않는다"를 위반. 이를 검증하는 기존 테스트는 항등 부모 + 허용오차 `.Equals()`라서 결함을 잡을 수 없는 조건이었다. |
| 코드 리뷰 | 부분 책임 | `AGENT_REVIEW.md` transform(54행)·relative transform(80행) 점검 대상이었으나 테스트 조건의 약함을 지적하지 않았다. 엔진 내부 왕복은 놓치기 쉬운 종류다. |
| 아키텍처 | 보완점 | 계약 문장은 정확했으나 "부모 transform 무관 bit-exact"와 회전된 부모 검증 조건을 명시하지 않았다. |
| Unreal MCP·통합 리뷰 | 해당 없음 | 검증 시점 BP에는 `SceneRoot` 회전이 없어 재현 조건이 존재하지 않았다. |

### 조사 과정의 애로사항

- 초기 조사에서 원인을 에셋(메시 형상), 루트 스케일, 엔진 버그 순으로 잘못 짚었다. 우리 코드의 Editor 훅(`OnConstruction` 안의 transform 변경)을 먼저 전수 점검했으면 더 빨리 좁힐 수 있었다.
- "위치를 쓰는 코드"만 grep해서 "위치를 부수적으로 다시 쓰는 엔진 API"(`SetRelativeRotation`)를 놓쳤다.
- 사용자의 A/B 관찰(루트 스케일 무관, `SceneRoot` 회전만 트리거, 세 설비 공통)이 원인 확정에 결정적이었다.

### 일반화 후보

- 구현: Editor 프리뷰(`OnConstruction`, construction preview)에서 native component의 serialized transform을 변경하는 코드는 해당 필드 외 값이 bit 단위로 불변인지 확인한다. 회전만 바꿀 때 `SetRelativeRotation`의 위치 재계산을 고려한다.
- 구현·리뷰: "값을 바꾸지 않는다" 계약의 테스트는 허용오차 비교가 아니라 정확 비교로, 항등이 아닌 부모 transform 조건을 포함한다.
- 아키텍처: 불변 계약에는 정밀도(bit-exact 여부)와 검증해야 할 부모 transform 조건을 함께 적는다.
- 디버깅 공통: Editor 전용 증상은 프로젝트의 Editor 실행 경로(construction, PostEdit, OnRegister)를 먼저 전수 확인한 뒤 엔진·에셋 원인으로 넘어간다.

---

## FB-002 게이지 프리뷰 자세가 작성값으로 재캡처되어 컴파일·레벨 저장마다 바늘 회전이 누적됨

- 기록일: 2026-09-29
- 상태: 코드 수정 완료(사용자 선택: BP 템플릿 값을 baseline으로 사용), 빌드·자동화·Editor 검증 미실행, 오염된 레벨 인스턴스 3개 정리 미실행
- 관련 커밋: `61f72c1` (`FUtilityPivotRotation`, `UUtilityGaugeComponent::ApplyConstructionPreview`)

### 증상

- BP 에디터에서 Compile할 때마다 게이지 바늘이 `ZeroAngleDegrees`만큼 더 돌아간다. 탭을 다시 열면 원래대로 보인다.
- 레벨에 배치된 인스턴스에 프리뷰 자세가 저장돼 있다(`Content/__ExternalActors__/Maps/DefaultMap`).
  - BP_Boiler `7/1K/M6ZXZRZAAVEJCE6LP25CQY`: `GaugeNeedlePivot` RelativeRotation (30, 0, 0)
  - BP_Circulator `5/YZ/KRUXF2RNGDHA3WNR999GN3`: (30, 0, 0)
  - BP_Cooler `0/EB/P4KMP0NJE6NZLCAA8FG8AB`: (30, 180, -180). 누적 흔적이다.
  - BP 템플릿 자체에는 회전이 저장돼 있지 않다.

### 원인

1. 설계(`Architecture/UtilityLaborSystem.md:111`)는 "이미 적용한 표시 자세를 baseline으로 다시 저장하지 않는다"고 정한다. 구현은 이 판단 상태를 raw 포인터 `FUtilityPivotRotation*`(UPROPERTY 아님)에 둔다.
2. Compile 재인스턴싱: 새 컴포넌트 객체가 생기고, 피벗 `RelativeRotation`은 이전 프리뷰 값(표시 자세)이 복사된다. baseline 상태는 복사되지 않아, `CaptureBaseline`이 표시 자세를 작성값으로 캡처하고 다시 0 각도를 곱한다.
   - 재인스턴싱 기록기는 비영속이라 Transient UPROPERTY라면 복사된다(`Property.cpp:1063`). raw 포인터라 유실된다.
3. 레벨 저장: 게이지에는 문·레버와 달리 `PreSave` 복원이 없다. 표시 자세가 인스턴스 override로 저장되고, 다음 로드 때 그 값이 baseline으로 캡처돼 누적된다.
4. PIE: 복제기는 영속 모드라(`DuplicateDataWriter.cpp:41`) Transient가 복사되지 않는다. 에디터 인스턴스의 현재 표시 자세가 BeginPlay에서 baseline으로 캡처될 수 있다(추론, 미검증).

### 조치

- `Source/BathhouseSim/Private/Utility/UtilityPivotRotation.cpp`: `CaptureBaseline`이 pivot의 component template(archetype) 회전을 작성값으로 쓴다. template이 없는 컴포넌트(NewObject 생성)만 현재 값을 쓴다. 게이지·문·레버 공통이다.
- `Source/BathhouseSim/Private/Tests/UtilityLaborAutomationTests.cpp`: `BathhouseSim.Utility.Labor.GaugePreviewBaselineUsesComponentTemplate`를 추가했다. baseline 상태 유실(재인스턴싱 모사) 3회와 오염된 저장 자세에서도 zero pose가 유지되는지 검사한다.
- `Source/BathhouseSim/Private/Tests/UtilityFuelDoorAutomationTests.cpp`: 인스턴스 회전을 작성값으로 가정하던 `FuelDoorPresentation` fixture를 "인스턴스 회전은 무시되고 template 값이 기준"으로 바꿨다. 이 때문에 이 테스트의 비항등 baseline 조합 검증은 약해졌다. 비항등 baseline은 NewObject 기반 게이지·레버 테스트가 계속 다룬다.
- `.md/Architecture/UtilityLaborSystem.md`: baseline 원천과 위치 bit 불변 계약을 추가했다.
- 미처리: DefaultMap 레벨 인스턴스 3개의 오염된 `GaugeNeedlePivot` 회전 override. 수정 후 baseline으로는 쓰이지 않지만 저장값으로는 남아 있다. 필요하면 Editor에서 Reset to Default 뒤 저장한다.

### 단계별 판단

| 단계 | 판단 | 근거 |
|---|---|---|
| 구현 | 주 책임 | 재구성 불변 계약을 재인스턴싱·저장·PIE 복제에서 유지하지 못하는 상태 보관 방식(raw 포인터)을 선택했다. 문·레버에 있는 `PreSave` 복원을 게이지에 적용하지 않았다. |
| 아키텍처 | 보완점 | "반복 construction·BeginPlay·재구성"은 명시했으나 Compile 재인스턴싱, 레벨 저장·재로드, PIE 복제라는 객체 교체 경로를 나열하지 않았다. baseline의 원천(템플릿인지 인스턴스인지)도 정하지 않았다. |
| 코드 리뷰 | 부분 책임 | UObject 컴포넌트가 raw 포인터로 lifecycle 상태를 소유하는 구조는 GC·복제·재인스턴싱 관점에서 점검 대상이었다. |
| Unreal MCP·통합 리뷰 | 부분 책임 | 레벨 배치 인스턴스의 pivot 회전 override(저장값)가 이미 비정상이었으나 검증에서 발견되지 않았다. |

### 일반화 후보

- 아키텍처: "재구성 불변" 계약에는 객체가 교체되는 경로(Compile 재인스턴싱, 레벨 저장·로드, PIE 복제, Undo)를 명시적으로 나열한다.
- 구현: 컴포넌트의 lifecycle 판단 상태를 raw 포인터·비반영 멤버에 두지 않는다. 재인스턴싱·복제에서 유지되어야 하는지 먼저 판단한다.
- 구현: Editor 프리뷰가 native component의 serialized 값을 바꾸면 `PreSave` 복원과 레벨 인스턴스 override 오염을 함께 검토한다. 같은 helper를 쓰는 형제 컴포넌트(문·레버)와 보호 장치를 대조한다.
- 통합 리뷰: 레벨 배치 인스턴스의 native component override 값을 BP 템플릿과 대조한다.

---

## FB-003 DISP-024 계약 모순이 설계 단계를 통과해 구현 중 충돌로 발견됨

- 기록일: 2026-09-30
- 상태: 기능 명세 정정(2026-09-30) 후 설계 정본·구현 프롬프트 갱신 완료, 구현 재개 대기
- 관련 문서: `QNA_IMPLEMENTATION.md`(구현 충돌 보고와 아키텍처 답변), `PROMPT_ARCHITECTURE.md` DISP-024, `Architecture/ServiceFacilityDisplaySystem.md` Facility Target Router

### 증상

- 서비스 2단위 구현 중 구현 에이전트가 DISP-024를 만족할 수 없다고 보고하고 작업을 멈췄다.
- 최초 DISP-024: 빈 박스로 화장대 드라이기 쪽에서 RMB 연속 빼기 중 빗 쪽으로 조준을 옮기면 멈춘다. 빗은 빠지지 않으며 버튼을 떼고 다시 누르면 빗부터 뺀다.
- 설계 정본은 "빈 박스 연속 빼기 중 가장 가까운 묶음이 바뀌면 멈춘다(DISP-024)"라고 적고, 검증 표에 "key 변경 시 반복 멈춤", 구현 프롬프트에 해당 테스트를 넣었다.

### 원인

1. 첫 1개가 빠지는 순간 박스는 빈 박스가 아니라 드라이기 박스가 된다(박스 종류는 내용물이 정함, Q16).
2. 명세의 공통 규칙상 물품이 든 박스는 박스 종류 묶음으로 넣고 뺀다. 이후 선택은 조준과 무관하게 드라이기로 고정된다(Q51 B·Q61 A와 결합).
3. 따라서 "조준 이동 시 멈춤"과 "다시 누르면 빗부터"는 명세 자신의 규칙과 양립할 수 없었다.
4. 설계는 key를 "선택된 묶음의 SpaceIndex"로 정했으므로 key가 절대 바뀌지 않는다. 정본의 멈춤 문장은 선택 함수를 따라가면 성립하지 않는 주장이었다.

### 조치

- 기능 명세가 DISP-024를 정정했다: 첫 빼기 뒤 박스 종류 묶음에 고정, 설비를 벗어나지 않는 한 같은 묶음에서 계속 빼고, 묶음이 비거나 박스가 차면 멈춘다. 빗은 빠지지 않는다.
- 설계는 선택 함수·key 변경 없이 정정 계약을 만족한다. 정본의 DISP-024 문장·검증 표, 구현 프롬프트의 테스트와 재개 조건을 갱신했다. key guard는 방어 규칙으로 유지한다.
- 부수: 같은 보고에서 설계 정본의 세탁기·건조기 BP 경로 오류(`Blueprints/Facility/` → 실제 `Blueprints/Towel/`)도 지적돼 수정했다.

### 단계별 판단

| 단계 | 판단 | 근거 |
|---|---|---|
| 기능 명세 | 최초 원인 | 관찰 결과를 쓸 때 첫 조작 뒤 박스 종류가 바뀌는 상태 변화를 반영하지 않아 자체 규칙(Q16·Q51 B·Q61 A)과 모순됐다. 사용자 승인도 통과했다. |
| 아키텍처 | 주 책임(검출 실패) | 계약 모순은 설계 단계에서 QNA로 올려야 했다. 오히려 성립하지 않는 멈춤 규칙을 정본·검증 표·구현 프롬프트에 넣어 DISP-024 충족으로 표시했다. BP 경로도 실제 파일·Unreal 정본과 대조하지 않았다. |
| 구현 | 해당 없음(정상 동작) | 실행 순서를 추적해 충돌을 증명하고, 임의 변경 없이 멈춰 보고했다. |
| 코드 리뷰·Unreal·통합 리뷰 | 해당 없음 | 도달 전 발견. |

### 대응 과정의 애로사항

- 아키텍처가 충돌 보고를 받은 뒤 처음 낸 해결안은 "방향별 key(빼기 key = 조준선 최근접 묶음)"였다. 물품이 든 박스로 뺄 때도 옆 묶음 조준 시 멈추게 되어 "박스 종류 묶음으로 넣고 뺀다(설비 전체 조준)" 계약을 깨고, 멈춰도 재입력하면 같은 묶음에서 빠져 의미 없는 멈춤만 만드는 안이었다. 채택되지 않았다.
- 같은 답변에서 "예시 문구대로면 기능 명세 수정 단계를 거치지 않고 바로 반영하겠다"고 제안했다. 사용자가 보는 결과 변경을 설계 단계가 대신하겠다는 역할 밖 제안이었다.

### 일반화 후보

- 기능 명세: 연속 조작 시나리오는 첫 조작 뒤 바뀌는 상태(박스 종류·수량·대상 상태)를 반영해 이후 입력의 결과를 적는다. 새 시나리오를 기존 공통 규칙과 결합해 모순이 없는지 확인한다.
- 아키텍처: 시나리오 충족을 주장하는 설계 문장은 선택·판정 함수를 시나리오 입력 순서대로 따라가 확인한다. 특히 조작 도중 입력 조건이 바뀌는 반복 시나리오(held-use Repeat 등)를 우선 점검한다.
- 아키텍처: 계약 모순을 발견하면 설계로 덮지 말고 QNA로 기능 명세에 돌려보낸다. 사용자가 보는 결과의 변경을 설계 단계에서 대신 확정하지 않는다.
- 아키텍처: 정본에 적는 기존 asset 경로는 Unreal 정본 또는 실제 파일과 대조한다.

---

## FB-004 수건 프리뷰·강조가 넣기·빼기 뒤 이전 자리에 남음(수량 변화가 focus 알림을 다시 부르지 않음)

- 기록일: 2026-09-30
- 상태: 설계 확정(`TowelSystem.md` Service Unit 2 Display Changes `cue 재계산 경로`, `InteractionSystem.md` `PresentationRevision`), 구현 재작업 대기
- 관련 문서: `PROMPT_IMPLEMENTATION_R.md`(수건 cue 결함), `PROMPT_IMPLEMENTATION.md` 맨 앞 재작업 절
- 대상 시나리오: TOWL-001, 004, 005, 006, 010, 013, 016, 017, 018

### 증상

- 서비스 2단위 구현 사이클(코드 리뷰 승인) 완료 뒤 사용자가 발견했다.
- 선반·사용 수건통(Stack), 세탁기·건조기(Pile)에서 첫 조준의 넣기 프리뷰·꺼내기 외곽선은 맞는 자리에 뜬다. 넣거나 빼면 이전 자리에 남는다.
  - LMB 넣기: 프리뷰가 방금 놓인 수건과 겹쳐 남고, 외곽선은 이전 맨 위에 남는다.
  - RMB 빼기: 외곽선이 비워진 자리에 남고, 프리뷰는 맨 위보다 두 칸 위에 뜬다.
  - 누른 채 연속 옮기면 어긋남이 누적된다. 손님이 선반에서 가져가도 갱신되지 않는다.
- 조준을 뗐다 다시 하거나, 가득 참·비어 있음·기계 상태처럼 가능 여부가 바뀔 때만 우연히 맞는 자리로 돌아온다.
- 진열 공간·화장대·샤워기는 해당하지 않는다.

### 원인

1. 수건 cue는 `NotifyInteractionFocusChanged`에서만 다시 계산된다.
2. `UPlayerInteractionComponent`는 query가 이전과 `Equals`이면 focus 알림을 생략한다.
3. 수건 대상 query는 수량이 바뀌어도 같다. TargetName·행동명이 고정이고, 가능 여부도 정원·빈 상태 전까지 같으며, 바구니 요약도 비어 있다.
4. 1단위 진열은 "Count가 바뀌면 TargetName이 바뀌어 다음 알림에서 따라간다"는 전제(`ServiceSystem.md`)로 성립했다. 화장대·샤워기 router도 TargetName에 수량이 있어 같은 방식으로 따라간다.
5. 수건 설계(`TowelSystem.md` Service Unit 2 Display Changes)는 "알림 query로 자리를 정한다"만 적고, 이 전제를 옮기거나 수량 변화 뒤 알림이 다시 오는 경로를 정하지 않았다.
6. 위치 계산(`GetIndexPresentation`, 결정적 배치, 좌표계)은 맞았다. 다음 계산이 호출되지 않는 것만 결함이었다.
7. 자동화(`TowelDisplayCueAutomationTests.cpp`)는 옮길 때마다 `NotifyInteractionFocusChanged`를 직접 호출해 실제 경로의 `Equals` 생략 조건을 거치지 않았다.

### 조치

- 설계: `FPlayerInteractionQuery::PresentationRevision`(int64, `Equals` 포함, Blueprint 비노출)을 추가한다. 수건 대상 query가 대상 inventory revision + 든 바구니 revision으로 채운다. 수량이 바뀌면 다음 query commit에서 기존 focus 알림 경로로 cue가 다시 계산된다. HUD 문구·이동 조건·`UPlayerInteractionComponent`는 바꾸지 않는다(리뷰 선택지 C).
- 구현 재작업: 실제 경로(`RefreshInteractionQuery`, held-use `BeginUse` + Tick 반복) 기반 자동화와 손님 쪽 수량 변화 검증, PIE 연속 넣기·빼기 관찰 항목을 추가한다.

### 단계별 판단

| 단계 | 판단 | 근거 |
|---|---|---|
| 아키텍처 | 주 책임 | 표현이 query 알림에만 의존하는데, 수량 변화가 query를 바꾸지 않는 대상에 1단위 전제를 옮기지 않았다. "수량 변화 → 알림 재발생" 경로와 이를 검증할 실제 경로 조건을 정하지 않았다. |
| 구현 | 부분 책임 | 설계대로 구현했으나, 테스트가 알림을 직접 호출해 실제 생략 조건을 우회했다. 1장 옮긴 뒤 cue 위치를 실제 경로로 확인하지 않았다. |
| 코드 리뷰 | 부분 책임 | 승인 시점에 테스트가 실제 알림 경로를 거치지 않는 점과 1단위 전제(TargetName 수량)의 부재를 지적하지 않았다. |
| Unreal·통합 확인 | 보완점 | 대표 PIE 절차에 "옮긴 뒤 프리뷰·외곽선이 따라가는지"가 관찰 항목으로 없어 사용자 발견까지 남았다. |

### 일반화 후보

- 아키텍처: 알림·이벤트로 갱신되는 표현을 설계할 때 "상태가 바뀐 뒤 알림이 다시 오는 조건"을 명시한다. 알림이 값 비교(`Equals`)로 생략되는 경로라면, 표현이 의존하는 상태가 비교 대상에 들어가는지 확인한다.
- 아키텍처: 기존 시스템의 동작이 암묵 전제(예: TargetName에 수량 포함)에 기대고 있으면, 같은 도구를 재사용하는 새 대상에 그 전제가 성립하는지 확인한다.
- 구현·리뷰: 표현 갱신 테스트는 콜백을 직접 호출하지 말고 실제 입력·갱신 경로(query refresh, 반복 Tick)를 거친다. 한 번 조작 뒤 다음 상태까지 확인한다.
- Unreal·통합: 반복 조작이 있는 표현은 PIE 관찰 항목에 "누른 채 연속 조작 중 1회마다 따라가는지"를 넣는다.
