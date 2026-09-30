# 재작업 프롬프트 — 서비스 2단위: 수건 프리뷰·강조가 옮긴 뒤 따라가지 않음

## 재검토 결론

- 2026-09-30 서비스 2단위 구현 사이클 완료 뒤 사용자 보고로 확인한 결함이다.
  - 보고 내용: 수건 대상의 꺼내기 외곽선과 넣기 반투명 프리뷰 위치가 다른 진열 대상과 달리 적절하지 않다.
  - 이 결과는 코드 리뷰 승인 시점에 놓친 것이다.
- 결론: **아키텍처 재검토** 후 **구현 재검토**.
- 원인은 설계 누락이다.
  - `TowelSystem.md` Service Unit 2 Display Changes는 "알림 query로 자리를 정한다"만 적었다.
  - 수량이 바뀐 뒤 알림이 다시 오는 경로는 정하지 않았다.
- 대상 시나리오: TOWL-001, 004, 005, 006, 010, 013, 016, 017, 018.
- 기능 계약 `.md/PROMPT_ARCHITECTURE.md`는 바꾸지 않는다. 계약 문구는 다음과 같다.
  - TOWL-001: 넣으면 프리뷰가 "한 칸 위로 이동"
  - TOWL-005: "다음 수건으로 강조 이동"
  - TOWL-006: "프리뷰가 다음 자리로 이동"
- 이번 문서 작성에서 Source·Content·Config는 수정하지 않았다.

## 증상

첫 조준에서는 프리뷰와 외곽선이 맞는 자리에 뜬다. 넣거나 뺀 뒤에는 **이전 자리에 남는다.**

| 조작 | 실제 결과 | 기대 결과 |
|---|---|---|
| LMB 1장 넣기 | 수건은 프리뷰 자리에 놓임. 프리뷰는 그 수건과 겹쳐 남음. 외곽선은 이전 맨 위(한 칸 아래)에 남음 | 프리뷰는 다음 자리로 이동, 외곽선은 새 맨 위 |
| RMB 1장 빼기 | 외곽선이 방금 비워진 자리에 남음. 프리뷰는 맨 위보다 두 칸 위 | 외곽선은 새 맨 위, 프리뷰는 방금 빠진 자리 |
| LMB·RMB 누른 채 연속 | 어긋남이 누적됨 | 매 1장마다 따라감 |
| 손님이 선반에서 수건을 가져감 | 조준 중인 플레이어의 cue가 갱신되지 않음 | 새 맨 위 기준 |

cue가 제자리로 돌아오는 경우는 query가 우연히 바뀔 때뿐이다: 조준을 뗐다 다시 함, 가능 여부가 바뀜(가득 참·비어 있음·상태 변화), 기계 상태 변화.

선반(Stack), 사용 수건통(Stack), 세탁기·건조기(Pile) 모두 같다. 진열 공간·화장대·샤워기는 해당하지 않는다.

## 근거

1. cue는 focus 알림에서만 다시 계산된다.
   - `ACleanTowelStackActor::NotifyInteractionFocusChanged`
   - `AUsedTowelBinActor::NotifyInteractionFocusChanged`
   - `UTowelTransferPortComponent::NotifyInteractionFocusChanged`
   - 모두 `TowelDisplayCueUtils::Update`를 호출한다.
2. focus 알림은 query가 이전과 달라질 때만 보내진다.
   - `UPlayerInteractionComponent::CommitQuery` / `SyncFocusObservers`
   - `Private/Interaction/PlayerInteractionComponent.cpp` 544, 609행: `LastFocusObserverQuery.Equals(CurrentQuery)`면 생략
3. 수건 대상의 query는 수량이 바뀌어도 같다.
   - `TargetName` 고정: `CleanTowelStackActor.cpp` 48행 "깨끗한 수건 선반", `UsedTowelBinActor.cpp` 52행 "사용 수건통", `TowelTransferPortComponent.cpp` 35행 "수건 투입구"
   - 행동명 고정. 가능 여부도 정원·빈 상태에 닿기 전까지 같다.
   - 수건 바구니는 `GetHeldSummaryText`를 구현하지 않아 `HeldObjectSummary`도 비어 있다.
4. 1단위 진열은 이 조건을 명시적으로 이용했다.
   - `ServiceSystem.md` 131행: "이동 뒤 Count가 바뀌면 TargetName이 바뀌어 query가 바뀌므로 다음 알림에서 자리가 따라간다."
   - 화장대·샤워기 router도 TargetName에 수량이 들어 있어 같은 방식으로 따라간다.
   - 수건 설계는 이 전제를 옮겨 오지 않았다.
5. 위치 계산 자체는 맞다.
   - `GetIndexPresentation(Count / Count−1)`, Stack·Pile 결정적 배치, cue와 bucket의 부모 좌표계(수건 visual component)가 모두 일치한다.
   - "프리뷰 자리 = 실제 놓이는 자리"도 성립한다.
   - 결함은 **다음 계산이 호출되지 않는 것**뿐이다.
6. 자동화가 놓친 이유: `Private/Tests/TowelDisplayCueAutomationTests.cpp`는 옮길 때마다 `NotifyInteractionFocusChanged`를 직접 호출한다(106·116·170·214·224행). 실제 경로의 Equals 생략 조건을 거치지 않는다.

## 아키텍처 단계가 정할 것

리뷰는 구조를 고르지 않는다. 다음 불변식을 만족하는 **수량 변화 → cue 재계산 경로**를 `TowelSystem.md` Service Unit 2 Display Changes에 확정하고, 필요하면 Interaction 정본에도 반영한다.

- 조준 중에 대상 inventory의 수량·상태가 바뀌면 같은 프레임 안에 프리뷰와 외곽선이 새 Count 기준 자리로 옮겨 간다.
  - 수량이 바뀌는 원인: 플레이어 넣기·빼기, 연속 반복, 손님 가져가기, 기계 상태 전환
- cue는 표현 전용이다. 이동 조건, 기계 상태, HUD 문구를 바꾸지 않는다.
  - HUD 문구를 바꾸는 해법은 기능 명세 승인이 필요하다.
- focus 종료, suppression, 작동 시작에서 숨김·닫힘 규칙은 그대로다.
- 진열 공간, 화장대·샤워기 router, 냉장고의 기존 동작은 바뀌지 않는다.
- `UPlayerInteractionComponent`의 성장 정책(CoreSystem Class Growth Policy)을 지킨다.

선택지(참고, 결정은 아키텍처 단계):

| 안 | 내용 | 영향 |
|---|---|---|
| A | 수건 owner가 focus 중에 자기 inventory 변경 이벤트를 받아 마지막 알림 query로 cue를 다시 계산한다 | Towel 안에서 끝난다. 마지막 query·source 보관과 focus 종료 시 해제가 필요하다 |
| B | 수건 대상 TargetName에 수량을 넣는다(예: "깨끗한 수건 선반 5/30") | 기존 알림 경로를 그대로 쓴다. HUD 문구 변경이라 **기능 명세 승인 필요** |
| C | `FPlayerInteractionQuery`에 표현 갱신용 필드(예: 대상 revision)를 추가하고 `Equals`에 넣는다 | `HeldUseTargetKey`와 같은 방식이다. Interaction 계약이 바뀌고, 반복 query 비교에 영향이 있는지 검토해야 한다 |

## 구현 단계 재검증 조건

- 새 자동화는 `NotifyInteractionFocusChanged`를 직접 부르지 않는다. 다음 **실제 경로**로 검증한다.
  - `UPlayerInteractionComponent::RefreshInteractionQuery`
  - `UPlayerHeldTargetUseComponent`의 `BeginUse` + Tick 반복
- 선반·사용 수건통·대기 세탁기·대기 건조기 각각에서 확인한다.
  - LMB 1장마다 프리뷰가 다음 자리로 이동하고, 외곽선이 새 맨 위로 이동한다.
  - RMB 1장마다 외곽선이 새 맨 위로 이동하고, 프리뷰가 방금 빠진 자리로 이동한다.
  - 연속 반복 중 매 1장마다 위 규칙을 지킨다.
  - 손님 쪽 수량 변화(선반에서 가져감) 뒤에도 cue가 새 Count 기준이다.
- 기존 `BathhouseSim.Towel.Display.CuesDeterministicPileAndLid`, Towel 전체, Service 전체, Interaction held-use 회귀를 유지한다.
- `PROMPT_UNREAL.md` 또는 통합 PIE 절차에 "누른 채 연속 넣기·빼기 중 프리뷰·외곽선이 1장마다 따라가는지"를 관찰 항목으로 추가한다.

## 재작업 후 리뷰 입력

- 갱신된 `TowelSystem.md`(필요 시 `InteractionSystem.md`·`HeldTargetUseSystem.md`)와 `PROMPT_IMPLEMENTATION.md` 재작업 절
- 재작성된 `PROMPT_REVIEW.md`: 선택된 경로, 실제 경로 기반 자동화, 전체 회귀 수치
- Content·Config 무변경을 유지한다. native 구조가 바뀌면 해당 BP copy-first load gate를 다시 실행한다.
