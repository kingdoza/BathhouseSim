# 구현 재작업 — 세신 포커스 중 손에 든 때수건 숨김

- 작업 ID: `SERVICE-U4`
- 단계: 코드 리뷰
- 상태: 완료
- 출처: 사용자 PIE (BugReports/2026-10-01_scrub_focus_towel_visual_and_cursor_motion.md)

## 범위

- 시나리오: SCRB-005~007/011/016(세신 포커스), 버그 현상 1. `PIE_CHECKLIST.md` 10행.
- 기준 커밋: `fcc982d` (Source는 `f15742a`와 같다).
- 현상 2(커서 이동 방향)는 이 재작업 범위가 아니다. 아래 "현상 2 판정"을 보라. 축 매핑 코드를 바꾸지 않는다.
- 사용자 결정(버그 리포트, 선택지 A): 세신 포커스 중에는 손에 든 때수건을 숨기고 커서 때수건 하나만 보인다. 포커스를 벗어나면 손에 든 때수건이 다시 보인다. 벗어나는 경로는 E/ESC, 완료 자동 이탈, 만료 자동 이탈, 비정상 종료를 모두 포함한다. 포커스 동안에도 때수건은 계속 손에 들린 상태다(carry 상태 불변).

## 확정 원인 (현상 1)

- `UPlayerScrubFocusComponent::BeginScrubFocus`(`Private/Service/PlayerScrubFocusComponent.cpp:74-79`)는 시점 전환과 `ScrubCursor` 표시만 한다. `Carry->GetHeldObject()`(손에 든 `AScrubTowelActor`)의 표시를 바꾸는 코드는 Source 전체에 없다.
- `RequestEndScrubFocus`(157행)와 `ForceCleanup`(223행)도 커서만 숨긴다.
- 그래서 `HeldAnchor`에 붙은 손의 때수건이 포커스 내내 렌더된다. 커서 메시와 함께 때수건 두 개가 보인다.
- 유입 단계:
  - 채택 전 명세(`f15742a:.md/PROMPT_ARCHITECTURE.md`)는 "때수건이 손님 몸 위에서 마우스를 따라 움직인다"와 "나와도 때수건은 계속 손에 있다"만 적었다. 포커스 중 손에 든 때수건을 어떻게 보일지는 명시하지 않았다.
  - 아키텍처(`Architecture/ServiceAmenitySystem.md` Scrub Table·Scrub Focus Session 절)는 별도 `ScrubCursor` 메시를 도입했다. 그러면서도 손에 든 때수건의 표시를 정하지 않았다.
  - 구현은 설계대로 했다. 따라서 이 결함은 구현 누락이 아니라 설계 누락이다.
  - 사용자가 동작을 이미 정했고 남은 설계 선택이 작으므로 구현 재작업으로 처리한다. 구조를 바꾸므로 Architecture 정본 갱신도 구현이 한다(`AGENT_IMPLEMENTATION.md` 10번).

## 요청 수정

### 1. 소유자와 불변식

- 숨김과 복원은 세션 owner인 `UPlayerScrubFocusComponent`가 소유한다. snapshot과 `ForceCleanup`이 대칭인 기존 구조를 따른다.
- 불변식: 손에 든 때수건을 숨긴 기간은 `ScrubCursor`가 보이는 기간과 정확히 같다.
  - `ScrubCursor`를 표시하는 곳(Begin)에서 숨긴다.
  - `ScrubCursor`를 숨기는 모든 곳(`RequestEndScrubFocus` 시작, `ForceCleanup`)에서 복원한다.
  - 그래서 때수건이 0개나 2개로 보이는 프레임이 없다.

### 2. 상태

- private에 다음 두 값을 추가한다.
  - `UPROPERTY(Transient) TWeakObjectPtr<AActor> HiddenHeldTowel`
  - `bool bHiddenHeldTowelWasHidden`: 숨기기 직전 `IsHidden()` 값의 snapshot
- 복원 대상은 숨긴 그 Actor다. 복원 시점의 `Carry->GetHeldObject()`를 다시 조회하지 않는다. 포커스 중에 carry 상태가 바뀌어(때수건 파괴, 손에서 이탈 등) 다음 Tick에 `HasValidContext()` 실패로 정리될 때도 원래 Actor를 복원해야 하기 때문이다.

### 3. helper

private helper 두 개를 둔다. 이름은 구현이 정한다.

- 숨김 helper:
  - `Carry->GetHeldObject()`가 유효하고 `HiddenHeldTowel`이 비어 있을 때만 동작한다.
  - 그 Actor를 저장하고 `IsHidden()`을 snapshot한 뒤 `SetActorHiddenInGame(true)`를 호출한다.
  - 같은 세션에서 두 번 호출해도 snapshot을 덮어쓰지 않는다.
- 복원 helper:
  - `HiddenHeldTowel`이 유효하면 `SetActorHiddenInGame(bHiddenHeldTowelWasHidden)`을 호출한다.
  - Actor 유효 여부와 관계없이 `HiddenHeldTowel`을 Reset한다.
  - 여러 번 호출해도 안전해야 한다.

### 4. 호출 지점

- `BeginScrubFocus`: 모든 진입 조건과 `TryBeginScrubSession`이 성공한 뒤, `Table->GetScrubCursor()->SetHiddenInGame(false)`와 같은 위치에서 숨김 helper를 부른다. `CompleteFocusIn` 호출(blend 0인 즉시 경로 포함)보다 앞서야 한다. 진입이 거부되면 때수건의 표시는 변하지 않아야 한다.
- `RequestEndScrubFocus`: 커서를 숨기는 157행 블록에서 복원 helper를 부른다. 유효성 실패로 `ForceCleanup`으로 빠지는 경로와 정상 FocusingOut 경로 모두 복원 뒤에 진행한다.
- `ForceCleanup`: Table 유무와 `HadSnapshot` 값과 무관하게 항상 복원 helper를 부른다. 이 경로는 `CompleteFocusOut`, 세신대 파괴, 사용자 상실, Tick 검증 실패, `EndPlay`(캐릭터 파괴·레벨 종료)를 모두 덮는다.

### 5. Architecture 정본

`.md/Architecture/ServiceAmenitySystem.md`의 Scrub Focus Session 절을 현재 구조로 갱신한다.

- Begin 순서 4: "커서를 `ScrubArea` 중심에 두고 `ScrubCursor`를 표시하며 손에 든 때수건 Actor를 숨긴다(carry 상태 불변)"
- 종료 절차: "커서를 숨기고 손에 든 때수건 표시를 snapshot 값으로 복원"
- `ForceCleanup`의 복원 대칭
- 표시 owner와 "커서 표시 기간 = 손 때수건 숨김 기간" 불변식 한 줄

날짜별 기록은 남기지 않는다. `PhysicalCarrySystem.md`는 carry 계약이 바뀌지 않으므로 수정하지 않는다.

## 충돌 검토 결과 (구현 참고)

- `PlayerCarryComponent`, `FPhysicalCarryPlacementTransaction`, `AScrubTowelActor`는 Actor hidden 플래그를 쓰지 않는다(Source grep 결과). `SetActorHiddenInGame` 사용처는 `ABathhouseKeyActor`(열쇠 자기 자신)와 `ABathhouseCashPaymentActor`(현금 자기 자신)뿐이어서 때수건과 겹치지 않는다.
- 포커스 중에는 G drop, 거치대 E, 수거, Q 회수가 입력 차단과 suppression으로 막혀 있다. 그래서 숨긴 채로 정상 carry 전환이 일어나는 경로가 없다.
- 그래도 비정상 경로를 막는다. 때수건 파괴, 복구, 캐릭터 파괴가 일어나면 `ForceCleanup`이 저장한 Actor를 복원한다. 거치대·월드에 숨은 때수건이 남지 않는다.
- hidden 플래그는 collision과 physics를 바꾸지 않는다. 손에 든 때수건은 이미 NoCollision이다.
- `OnHeldPresentationChanged`는 carry commit publication 전용이다. 숨김과 복원에 broadcast하지 않는다.

## 금지할 임시 우회

- carry 상태를 바꾸는 방식은 금지한다. 때수건을 drop·store·detach하거나, `ScrubCursor`/세신대에 reattach하거나, `Carry`의 `HeldObject`를 비우는 방식이 여기에 해당한다.
- 손의 때수건을 커서로 재사용해 옮기지 않는다. 기존 `ScrubCursor`가 커서다.
- `HeldTransform`이나 `HeldAnchor`를 화면 밖으로 옮겨 숨기지 않는다. 캐릭터 전체, 카메라, `HeldAnchor` component를 숨기지 않는다.
- `ScrubCursor`를 대신 숨겨 개수를 맞추지 않는다.
- Blueprint graph(`BP_ScrubTowel`, `BP_ScrubTable`, `BP_FirstPersonCharacter`), asset 값, Level 배치로 숨기지 않는다.
- timer나 Tick polling으로 숨김 상태를 매 프레임 다시 적용하지 않는다. 표시 상태는 세션 전환에서만 바꾼다.
- `OnHeldPresentationChanged`를 숨김 신호로 재사용하지 않는다.
- `AddRubInput`의 축 매핑(`FVector2D(Axis.Y, Axis.X)`)과 `UpdateCursor`를 바꾸지 않는다(현상 2 판정 참고).
- `UPlayerComputerUseComponent`, `ComputerFocusExitPlacement.*`, `IPhysicalCarryable`과 carry 공통 계약은 수정하지 않는다.

## 회귀 검증·자동화 테스트 조건

### 기존 테스트에 assertion 추가

`Private/Tests/ServiceAmenityScrubAutomationTests.cpp`에 assertion을 추가한다. 새 시나리오가 크면 같은 파일에 새 테스트를 둔다.

1. `FocusClampReentryCompletionCashAndHud`
   - 진입 전: `Towel->IsHidden()==false`
   - E 진입(Active) 뒤: `Towel->IsHidden()==true`, `ScrubCursor`가 보임(`bHiddenInGame==false`), `GetHeldObject()==Towel`, `GetHeldKind()==ScrubTowel`, `Towel->GetAttachParentActor()` 또는 attach parent가 진입 전과 같다.
   - `RequestEndScrubFocus` 뒤 Inactive: `Towel->IsHidden()==false`, 커서 숨김, 여전히 들고 있다.
   - 재진입 뒤 다시 숨김이다.
   - 완료 자동 이탈(SCRB-008) 뒤 다시 보인다.
   - 진입 거부(때수건 없음, 손님 없음) 뒤 표시가 바뀌지 않는다. 거부 케이스는 때수건을 든 상태에서 손님 없는 세신대로 확인한다.
2. `ExpiryKnockdownAndUserLoss`
   - 쓰러짐 자동 이탈, 만료 자동 이탈(SCRB-009), 사용자 파괴 이탈 각각의 뒤에 `IsHidden()==false`다.
3. `CashSpawnFailureAndTableDestroyCleanup`
   - 현금 spawn 실패로 Active가 유지되는 동안 숨김이다.
   - `Table->Destroy()` 즉시 정리 뒤 `IsHidden()==false`다.
4. `InputOwnershipEntryReleaseExitSearchAndTransitions`(blend 0.2초)
   - FocusingIn 직후 숨김이다.
   - G(`DropCarryInput`) 뒤에도 숨김과 들고 있음이 유지된다.
   - `CancelInput`으로 FocusingOut이 시작되는 같은 호출 직후 `IsHidden()==false`이고 커서가 숨겨져 있다.
   - blend 완료 뒤에도 보인다.

### 새 비정상 종료 검증

다음 둘 중 최소 하나를 검증한다.

- 포커스 Active 중 때수건 Actor를 파괴한다. 크래시가 없고, 다음 Tick에 Inactive가 되고, suppression이 해제되는지 확인한다.
- 포커스 Active 중 캐릭터를 파괴하거나 `EndPlay`를 일으킨다. 그 뒤 때수건이 유효하면 `IsHidden()==false`인지 확인한다.

### 사전 hidden snapshot (선택)

테스트가 진입 전에 때수건을 hidden으로 만들 수 있으면 다음도 검증한다. 진입 → 이탈 뒤 hidden이 그대로 유지되는지 본다. snapshot 복원이 "항상 false로 되돌림"이 아님을 확인하는 단위 검증이다.

### 유지할 기존 assertion

- `Input axes map Y->X X->Y`
- `Look cursor maps input`
- 게이지 수치, 이탈 위치, 입력 차단

### 실행

- UE 5.8 `Build.bat` 빌드를 한다.
- headless Automation을 실행한다. 필터는 `BathhouseSim.Service.Amenity`와 회귀용 `BathhouseSim.Computer`, `BathhouseSim.Interaction`(carry·fixed slot)이다. `.md/AGENT_WORKFLOW.md` 명령 형식을 따른다.
- `PROMPT_REVIEW.md`에 빌드 시점 Source 식별값과 테스트 결과를 남긴다.

### PIE 관찰 항목 (자동화로 대체 불가)

`PIE_CHECKLIST.md` 10행에 해당한다.

- 진입 순간부터 손의 때수건이 사라지고 커서 때수건 하나만 보인다.
- E/ESC 이탈, 완료 자동 이탈, 만료 자동 이탈 직후 손에 때수건이 다시 보인다.
- 이탈 뒤 G drop, 거치대 store가 정상이고 때수건이 보인다.

## Content 변경 여부

- 없음. `BP_ScrubTowel`, `BP_ScrubTable`, `BP_FirstPersonCharacter`, DefaultMap을 저장하지 않는다.
- 새 reflected property는 Transient다. Blueprint 재저장이나 Core Redirect가 필요 없다.
- `PROMPT_UNREAL.md`에는 Content 변경 없음과 PIE 관찰 항목만 둔다.

## 현상 2 판정 (참고, 재작업 없음)

### 정본 값

- `BP_ScrubTable` CDO(`Unreal/ServiceSystem.md` 61~62행): `ScrubArea` 회전 Pitch 180·Yaw 90·Roll 0, extent `(35,90,1)`, `ScrubCamera` `(0,-170,240)` pitch -45·yaw 90. 둘 다 `SceneRoot` 자식이다(`ScrubTableActor.cpp` 생성자).
- 세신대는 상점·배치로 생성되고 Level instance override가 없으므로 CDO 값이 적용된다.

### 회전 행렬

FRotator(P180, Y90, R0)의 축은 다음과 같다.

- local X = actor -Y
- local Y = actor -X
- local Z = actor -Z

### 카메라

camera(yaw 90, pitch -45)의 화면 오른쪽 = actor -X다. 화면 위쪽 성분 = actor +Y(카메라에서 먼 쪽)다.

### 입력

- `bEnableLegacyInputScales=True`(`Config/DefaultInput.ini:70`)다.
- 엔진 `PlayerController.InputPitchScale=-2.5`(BaseGame.ini, CoreRedirect로 `InputPitchScale_DEPRECATED`)다.
- 그래서 일반 시점 조작이 반전돼 있지 않다면 마우스 아래 = Look Y 양수다.

### 결과

- 마우스 오른쪽 → `CursorLocal.Y+` → actor -X → 화면 오른쪽이다.
- 마우스 아래 → `CursorLocal.X+` → actor -Y → 카메라 쪽, 즉 화면 아래쪽이다.
- 코드상 방향은 마우스와 일치한다. 버그 리포트의 예상 방향(위·왼쪽)은 회전 0 기준 계산이며 축 보정 뒤에는 해당하지 않는다.
- 추가 수정 없이 PIE 확인(`PIE_CHECKLIST.md` 11행) 대상으로 둔다.

### 비차단 관찰 (아키텍처 참고)

- 현재 코드 매핑(입력 X→local Y, 입력 Y→local X)은 위에서 내려다보는 카메라에서 local Z가 아래를 향해야만 성립한다. "local +Z = 표면 법선" 계약과 맞지 않는다.
- 그 결과 커서가 영역 평면보다 extent Z(1cm)만큼 아래에 놓인다. 방향 결함은 아니며 이번 재작업에서 다루지 않는다.
