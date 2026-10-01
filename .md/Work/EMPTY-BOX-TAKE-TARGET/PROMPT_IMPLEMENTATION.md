# 구현 프롬프트 — 빈 박스 빼기 대상 선택 통일

- 작업 ID: `EMPTY-BOX-TAKE-TARGET`
- 단계: 아키텍처
- 상태: 완료

## 1. 입력과 단계

- 기능 계약: `.md/Work/EMPTY-BOX-TAKE-TARGET/PROMPT_ARCHITECTURE.md`(상태 완료, 2026-10-01 사용자 승인), 시나리오 EBT-001~016. 사용자 선택은 `QNA_FEATURE_SPEC.md`(Q1 A, Q2 A, Q3 A, P1~P9, S1~S3 A). Editor 사실은 `REPORT_UNREAL_DISCOVERY.md`(완료).
- 단계: **전체 경로 직접 구현**(수직 구현 없음, 명세 8절 P9). 화장대·샤워기를 한 번에 적용한다. 둘은 같은 router 코드 하나를 공유한다.
- 작업 위치: git worktree `<Worktree>`(브랜치 `work/EMPTY-BOX-TAKE-TARGET`, 아키텍처 단계 시작 커밋 `fc5b59f`). 모든 수정·빌드·테스트는 이 worktree 안에서 한다.
- 설계 정본: `.md/Architecture/ServiceFacilityDisplaySystem.md` "빈 박스 빼기 대상 선택" 절(이번 단계에서 갱신). 이 프롬프트와 다르면 멈추고 아키텍처로 복귀한다.
- 기존 승인 시나리오 변경: VANI-012 폐기, DISP-023 거리 기준 변경(명세 9절). 이를 고정한 기존 자동화 단언은 아래 6.2대로 바꾼다.

## 2. 목적·수용 기준·비목표

목적: 빈 품목 박스로 화장대·샤워기에서 RMB 빼기를 할 때, 설비 조준 영역(router Box) 안 어디를 조준해도 "꺼낼 수 있는 새것이 있는 묶음 중 화면 중앙 조준점에 화면상 가장 가까운 보이는 물품을 가진 묶음"이 대상이 되게 한다.

수용 기준:

1. 후보는 꺼낼 수 있는 새것(`TakeableCount > 0`)이 있는 묶음뿐이다(EBT-001, 004, 010).
2. 묶음 거리는 그 묶음의 보이는 물품(사용 중인 것 포함) 중 조준 방향과 이루는 각이 가장 작은 것이다. 빈 자리는 쓰지 않는다. 깊이는 따지지 않는다(EBT-002, 003).
3. 동거리면 SpaceIndex가 작은 쪽이다.
4. 후보가 없으면 강조 없음, RMB 불가. 이유는 사용 중인 것만 남은 묶음이 하나라도 있으면 `사용 중인 것은 꺼낼 수 없음`, 아니면 `꺼낼 물건 없음`. RMB를 누르면 그 이유가 한 번 보이고 아무것도 옮기지 않는다(EBT-005, 006).
5. HUD RMB 행(가능 여부·이유), 강조 위치, RMB로 실제로 빠지는 물품이 항상 같은 묶음의 마지막 채운 자리다(LIFO, Q2 A).
6. 손님 소모로 후보가 바뀌면 조준을 움직이지 않아도 다음 query에서 대상·강조가 바뀐다(EBT-012).
7. 채운 박스(박스 종류 묶음), 첫 1개 이후 연속 빼기(DISP-024), 냉장고, 집게 상태, 강조 옵션 끔은 결과가 같다(EBT-007~009, 011, 013, 015, 016).

비목표: 냉장고 `SelfAim` 규칙, 넣기 대상 선택, 묶음 안 빼기 순서, 수건·삽 대상, HUD 문구, 새 조정값·설정, 손님 가져가기 순서, Content·Config 변경.

## 3. 책임 변화

| 항목 | 판단 |
|---|---|
| 기존 책임 | `UDisplayFacilityTargetComponent::SelectSpace`가 query·execute 공용으로 묶음을 고른다. 빈 박스는 조준선 최근접(모든 자리 중심, 빈 묶음 포함) |
| 신규 책임 | 빈 박스 후보 필터·화면상 각 거리·대체 묶음 규칙 |
| 상태 owner | 없음(새 runtime 상태 없음). stock은 기존대로 `UDisplaySpaceComponent::Stock` |
| 실행 owner | 기존 router query/execute, 기존 `UPlayerHeldTargetUseComponent` 반복. 변경 없음 |
| 규칙 owner | 신규 private 순수 helper `FDisplayFacilityTakeSelection`(Private/Service). `FDisplayStockRules`·`FServiceItemTransfer`와 같은 형태 |
| 기하 owner | `UDisplaySpaceComponent`가 자기 진열 외형 위치를 낸다(`StockVisual` transform 식의 owner) |
| authoring owner | 변경 없음. 묶음 자리·품목 진열 보정·router Box는 기존 BP/DataAsset 값 |
| 의존 방향 | 변경 없음(Service 내부). Interaction·UI·Character는 수정하지 않는다 |
| 대안 거부 | (a) 조준선 수직 거리 유지: 깊이에 따라 순서가 바뀌어 P2 위반. (b) 화면 투영(`ProjectWorldToScreen`): PlayerController·viewport 의존이 생기고 결과 순서는 각과 같다. (c) router가 P7 이유 문구를 직접 생성: `EvaluateTake` 문구·우선순위를 복제하게 되어 대체 묶음 방식으로 거부. (d) 대상 잠금·완충 상태 보관: P6가 금지, query/execute 동일 함수로 일치가 이미 보장됨 |

Class growth: `DisplaySpaceComponent.cpp`(456줄)에는 자기 외형 위치 accessor 하나만 추가하고 선택 규칙은 넣지 않는다. router cpp(205줄)에는 helper 입력 조립만 추가한다.

## 4. 변경 파일과 API

### 4.1 `Private/Service/DisplayFacilityTakeSelection.h` / `.cpp` (신규)

```cpp
// Private/Service/DisplayFacilityTakeSelection.h
#pragma once
#include "CoreMinimal.h"

struct FDisplayFacilityTakeCandidate
{
	int32 SpaceIndex = INDEX_NONE;
	int32 Count = 0;            // 보이는 것 포함 전체 수(사용 중 포함)
	int32 TakeableCount = 0;    // FDisplayStockRules::GetTakeableCount
	TArray<FVector> ItemLocations; // 보이는 물품 world 위치(사용 중 포함, 빈 자리 제외)
};

class FDisplayFacilityTakeSelection
{
public:
	/** 화면 중앙 방향과 Point 사이 각(rad, 0~PI). 방향 또는 Point-Origin이 0이면 0. */
	static double GetAimAngleRadians(const FVector& TraceStart, const FVector& TraceEnd, const FVector& Point);
	/** 빈 박스 빼기 대상 SpaceIndex. Spaces가 비면 INDEX_NONE. 입력 순서를 가정하지 않는다. */
	static int32 SelectSpaceIndex(TConstArrayView<FDisplayFacilityTakeCandidate> Spaces,
								  const FVector& TraceStart, const FVector& TraceEnd);
};
```

규칙(정본: ServiceFacilityDisplaySystem.md):

1. `D = (TraceEnd − TraceStart).GetSafeNormal()`, `V = Point − TraceStart`. `D`가 0이거나 `V.IsNearlyZero()`면 0. 아니면 `FMath::Atan2(FVector::CrossProduct(D, V).Size(), FVector::DotProduct(D, V))`.
2. 후보 = `TakeableCount > 0 && !ItemLocations.IsEmpty()`. 후보 거리 = `ItemLocations` 각의 최솟값.
3. 최소 거리 후보를 고른다. `Distance < Best` 또는 `Distance == Best && SpaceIndex < BestIndex`면 교체(정확 비교, 기존 동거리 규칙과 같다).
4. 후보가 없으면: `Count > 0`인 항목 중 가장 작은 SpaceIndex, 없으면 전체 중 가장 작은 SpaceIndex.
5. Export macro는 붙이지 않는다(같은 모듈 테스트만 사용, `FDisplayStockRules`와 같음). UObject·world 의존 없음.

### 4.2 `UDisplaySpaceComponent` (`Public/Service/DisplaySpaceComponent.h`, `Private/Service/DisplaySpaceComponent.cpp`)

- 추가: `void GetVisibleItemWorldLocations(TArray<FVector>& OutLocations) const;`(public, C++ 전용, UFUNCTION 아님)
  - `OutLocations`를 비우고, `Stock.Kind`가 있고 `Stock.Count > 0`일 때 index 0..`FMath::Min(Stock.Count, SlotTransforms.Num())`−1마다 한 개를 넣는다.
  - 위치 = `(GetSlotWorldRelativeTransform(i, *Stock.Kind) * GetComponentTransform()).TransformPosition(BoundsOrigin)`. `BoundsOrigin`은 `Stock.Kind->ResolveDisplayMesh()`의 `GetBounds().Origin`, mesh가 없으면 `FVector::ZeroVector`.
  - `RefreshStockVisual`이 그리는 instance와 같은 개수·같은 transform 식이어야 한다(`StockVisual`은 이 component에 relative identity로 붙는다). 식을 복제하지 말고 `GetSlotWorldRelativeTransform`을 그대로 쓴다.
- `GetSlotsWorldCenter()`는 선택에 더 이상 쓰지 않는다. 테스트 fixture 조준용으로 남기고 동작을 바꾸지 않는다.
- 그 밖의 query·execute·focus·stock·transfer 코드는 수정하지 않는다.

### 4.3 `UDisplayFacilityTargetComponent` (`Public/Service/DisplayFacilityTargetComponent.h`, `Private/Service/DisplayFacilityTargetComponent.cpp`)

- 삭제: public static `SelectClosestSpace(...)`(reflected 아님, 사용처는 테스트뿐). 테스트는 4.1 helper로 옮긴다.
- `SelectSpace` 빈 박스 분기 교체(채운 박스 분기와 박스 없음 분기는 그대로):
  1. `Spaces` 중 `FacilityRouted` 묶음마다 `FDisplayFacilityTakeCandidate{SpaceIndex, GetStock().Count, FDisplayStockRules::GetTakeableCount(GetStock()), GetVisibleItemWorldLocations}`를 만든다(`#include "Service/DisplayStockRules.h"`).
  2. `FDisplayFacilityTakeSelection::SelectSpaceIndex(Candidates, Context.HitResult.TraceStart, Context.HitResult.TraceEnd)`로 index를 얻고 같은 SpaceIndex의 space를 반환한다.
- `QueryInteraction`·`ExecuteHeldTargetUse`·focus 알림 구조는 그대로다. 둘 다 같은 `SelectSpace`를 같은 context로 부르므로 HUD·실행이 일치하고, `HeldUseTargetKey`(= 선택 SpaceIndex)로 focus observer가 같은 묶음에만 강조를 보인다. 대체 묶음일 때도 key는 그 묶음 index다(`bCanHeldTake=false`라 강조·프리뷰 없음).
- Apply 방향(빈 박스)은 선택 묶음의 `EvaluateApply`가 기존대로 `박스가 비어 있음`을 낸다.

### 4.4 HUD·실행·강조 대상 일치 (구현 확인 사항)

| 출력 | 경로 | 같은 대상인 이유 |
|---|---|---|
| HUD RMB 행 | router `QueryInteraction` → 선택 space `BuildHeldUseQuery` → `EvaluateTake` | 선택 함수 1개 |
| 강조 | `UPlayerInteractionComponent` commit → router `NotifyInteractionFocusChanged` → key 일치 space `ShowTakeHighlight`(자리 `Count−1`) | query의 key |
| RMB 실행 | `UPlayerHeldTargetUseComponent::BeginUse` → 같은 fresh context로 query → `ExecuteHeldTargetUse` → 같은 `SelectSpace` → `TryTakeOne`(마지막 자리) | 같은 context·같은 함수, LIFO |
| 반복 | 첫 1개 뒤 박스 종류가 정해져 채운 박스 분기가 같은 SpaceIndex를 고른다. key 불변 | DISP-024 기존 |
| 소모 전환 | stock 요약 줄이 바뀌어 query `Equals`가 달라짐 → observer 재알림 | 기존 commit 규칙 |

## 5. lifecycle·rollback·전역 영향

- 새 상태·Tick·timer·delegate 없음. 선택은 query/execute 때마다 현재 stock과 조준으로 다시 계산한다(완충 없음, P6).
- 빼기 1개는 기존 `TryTakeOne` 단일 transaction이다. 실패는 아무것도 바꾸지 않는다. 선택 변화는 표시와 다음 시도만 바꾼다.
- Project/World/Input/Collision/Navigation 설정 변경 없음. trace channel·router Box·collision 변경 없음.
- 성능: query마다 routed 묶음(최대 4)의 보이는 물품(묶음당 최대 6)을 계산한다. 할당은 지역 `TArray` 수준으로 무시할 만하다.

## 6. 자동화

### 6.1 신규 `Private/Tests/ServiceFacilityEmptyBoxTakeAutomationTests.cpp`

flag는 기존과 같이 `EditorContext | EngineFilter`. 이름 접두사 `BathhouseSim.Service.Facility.EmptyBoxTake.`.

1. `...EmptyBoxTake.SelectionRules`(world 없음, 4.1 helper만)
   - 각: 축 위 점 0, 45° 점 ≈ π/4, 뒤쪽 점 > π/2, `(100,10,0)`과 `(1000,100,0)`의 각이 같음(1e-9).
   - P2 깊이 무관: 원점에서 +X, A `{(100,10,0)}`, B `{(300,20,0)}` → B(수직 거리는 A가 작아도 각은 B가 작음). 기존 수직 거리 규칙이면 A가 되는 배치임을 주석으로 남긴다.
   - P8: A `TakeableCount 0, Count 1`, 물품이 축 위 → 제외되고 먼 B가 선택.
   - `TakeableCount > 0`인데 `ItemLocations` 빔 → 후보 아님.
   - P4: y +10/−10 대칭 두 후보, 입력 순서 {3, 1} → 1.
   - P7 대체: 후보 없음에서 {0: Count 0, 2: Count 1·Takeable 0} → 2. 모두 Count 0, 입력 {3, 1, 2} → 1. 빈 배열 → `INDEX_NONE`.
2. `...EmptyBoxTake.VanityTraceQueryCueExecute`(`ServiceFacilityTest::FFixture` 화장대, `FScopedDisplaySettings`로 `bShowTakeHighlight=true`, 실제 camera trace·`RefreshInteractionQuery`·`HeldUse->BeginUse`·`EndUse` 사용)
   - 공통 단언 helper: 현재 query(`Player.Interaction->GetCurrentInteractionQuery()`)의 `HeldUseTargetKey`, `bHeldTakeVisible`, `bCanHeldTake`, `HeldTakeFailureReason`; key 묶음의 `GetTakeHighlightProxy()->IsVisible()`이고 다른 묶음은 모두 숨김; proxy world 위치가 그 묶음 `GetVisibleItemWorldLocations().Last()`와 같음(1e-3, Cube bounds origin 0이라 proxy component 위치와 같다); RMB 뒤 박스 Kind가 key 묶음 FixedKind이고 그 묶음 Count가 1 줄어 다른 묶음은 그대로.
   - EBT-001: 빗 3만, `AimAt(0)`(빈 드라이기) → key 3, 가능, 강조 빗 index 2, RMB → 박스 빗 1, 빗 2.
   - EBT-002: 드라이기 2·빗 3, `AimAt(0)` → 0, `AimAt(1)` → 0, `AimAt(2)` → 3, `AimAt(3)` → 3. 빈 묶음 위에서도 강조가 항상 하나 보인다.
   - EBT-003: 스킨로션 1(첫 자리)·면봉 4. 스킨로션 물품 위치를 조준(fixture에 `AimAtPoint(FVector)` 추가: 카메라를 점 −180X, 회전 0) → key 1. 이어서 빈 자리 중심 규칙과 결과가 갈리는 점을 조준: 스킨로션 물품과 면봉 물품의 y 중간보다 면봉 쪽이지만 스킨로션 자리 중심이 더 가까운 점(fixture 상대 y 약 +2, 계산은 `GetVisibleItemWorldLocations`·`GetSlotsWorldCenter`로 하고 조건을 테스트 안에서 먼저 단언) → key 2.
   - EBT-004: 스킨로션 `ImportStock(Kinds[1], 1, Failure, 9)`(사용 중만), 빗 2. 스킨로션 물품 조준 → key 3, RMB → 빗이 빠지고 스킨로션 Count 1·InUse 9 그대로.
   - EBT-005: 모두 빔 → 불가, 이유 `꺼낼 물건 없음`, Apply 이유 `박스가 비어 있음`, 강조 없음. `BeginUse(Take)` → 실패 보고 1회(`OnInteractionAttemptFinishedNative`로 수집, 이유 일치), 모든 stock·박스 불변.
   - EBT-006: 스킨로션 사용 중만 → 이유 `사용 중인 것은 꺼낼 수 없음`, 강조 없음, RMB 무변화.
   - EBT-012: 스킨로션 새것 1(사용 중 없음)·빗 2, 스킨로션 물품 조준 → key 1. 조준을 고정한 채 `Spaces[1]->ConsumeOneUse(bDepleted)`(사용 중 9/10) → `RefreshInteractionQuery` → key 3, 빗 강조, 스킨로션 강조 숨김, RMB → 빗.
   - EBT-013: `bShowTakeHighlight=false`, EBT-001 상태 → key 3·가능, 모든 강조 숨김, RMB → 빗.
3. `...EmptyBoxTake.ShowerAndFilledBoxUnchanged`(`FScopedDisplaySettings`로 `bShowTakeHighlight=true`, 넣기 프리뷰 확인을 위해 `InsertPreviewMaterial`을 `/Engine/EngineMaterials/DefaultMaterial`로 지정. `CleaningLitterTongsCueAutomationTests.cpp`와 같은 방식)
   - EBT-010: 샤워기 fixture(`Install(*this, true)`), 샴푸 0·바디워시 2, `AimAt(0)` → key 1, 바디워시 강조, RMB → 박스 바디워시 1.
   - EBT-011·P5: 화장대, 스킨로션 박스 2/6, 스킨로션 1·빗 3, `AimAt(3)` → key 1, 스킨로션 묶음에만 넣기 프리뷰·빼기 강조, 빗 묶음 cue 숨김.
   - EBT-008: 박스 드라이기 1/2, 드라이기 1·빗 3, `AimAt(3)` + RMB → 드라이기 2/2, 빗 3 그대로.

### 6.2 기존 테스트 수정(VANI-012·SelectClosestSpace 폐기에 따른 것만)

- `ServiceFacilityDisplayAutomationTests.cpp` `GroupsAndConsumption`: "Empty the hand and use an empty box..." 블록에서 드라이기 2·스킨로션 사용 중만 상태의 단언을 EBT-004 결과로 바꾼다. `Context(1)`·`Context(2)` query 모두 key 0·`bCanHeldTake` true·이유 빈 문자열. **execute는 호출하지 않는다**(뒤의 "Reusable dryer remains 2" 단언 유지). 주석의 "without fallback"은 지운다.
- 같은 파일 `RouterRepeatAndKeyGuard`: 맨 앞 `SelectClosestSpace` 단언 두 개를 지운다(6.1-1로 이동). 나머지(DISP-024, 이탈·재개, key guard)는 수정 없이 통과해야 한다.
- `ServiceFacilityAutomationTestSupport.h`: `AimAtPoint`·필요 시 `ContextAt(FVector)` helper만 추가한다. 기존 `Context`·`AimAt`은 바꾸지 않는다.
- 다른 기존 테스트의 기대값을 바꿔야 통과한다면 멈추고 아키텍처로 보고한다.

### 6.3 필터

```text
BathhouseSim.Service+BathhouseSim.Interaction+BathhouseSim.Cleaning.Litter.Tongs+BathhouseSim.Towel.Display
```

`BathhouseSim.Service`는 신규 EmptyBoxTake, 기존 Facility(화장대·샤워기·DISP-024), 냉장고(EBT-015 회귀), Display, BlueprintLoad를 포함한다. `Cleaning.Litter.Tongs`는 EBT-016, `Interaction`은 held-use·focus observer 회귀다.

## 7. 빌드·실행 명령

경로는 작업 브랜치 worktree 기준 변수로 쓴다. 메인 트리 경로를 쓰지 않는다. 다른 작업의 C++ 빌드·Editor와 동시에 실행하지 않는다(마스터가 순서를 정한다). 권한·sandbox 규칙은 `.md/AGENT_WORKFLOW.md` UE 5.8 Build/Headless 정책을 따른다.

```powershell
$Worktree = 'C:\UnrealProjects\BathhouseSim\.claude\worktrees\emptybox'   # 작업 브랜치 worktree 루트
$Project  = Join-Path $Worktree 'BathhouseSim.uproject'
$Report   = Join-Path $Worktree 'Saved\Automation\Reports\2026-10-01\EMPTY-BOX-TAKE-TARGET'

& 'C:\Program Files\Epic Games\UE_5.8\Engine\Build\BatchFiles\Build.bat' `
  BathhouseSimEditor Win64 Development `
  -Project="$Project" `
  -WaitMutex `
  -NoHotReloadFromIDE

& 'C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor-Cmd.exe' `
  "$Project" /Engine/Maps/Templates/Template_Default `
  -unattended -nullrhi -NoSplash -NoSound -DDC-ForceMemoryCache `
  -ExecCmds="Automation RunTests BathhouseSim.Service+BathhouseSim.Interaction+BathhouseSim.Cleaning.Litter.Tongs+BathhouseSim.Towel.Display; Quit" `
  -TestExit="Automation Test Queue Empty" `
  -ReportExportPath="$Report" -log
```

- 결과 보고에는 빌드 성공 여부, 실행한 테스트 수·실패 수, 신규 세 테스트 이름별 결과를 적는다. 에셋 로드 전 환경 Fatal은 검증 결과가 아니다.

## 8. Blueprint·API·Core Redirect·Content

- reflected 변경 없음: UPROPERTY·UFUNCTION·UCLASS·USTRUCT·UENUM 추가·삭제·rename 없음. 삭제하는 `SelectClosestSpace`와 추가하는 `GetVisibleItemWorldLocations`는 C++ 전용이다. Core Redirect 불필요.
- Blueprint 노출·serialized property·component 이름 변경 없음. copy-first load gate 불필요(필터의 `BathhouseSim.Service.BlueprintLoad`로 기존 BP load만 회귀 확인).
- **Content 변경 없음.** `BP_Vanity`·`BP_Shower`의 router Box, 묶음 자리, `DA_ServiceItem_*` 진열 보정은 사전 조사에서 P1을 만족한다(초과 0cm). 구현 단계의 `PROMPT_UNREAL.md`는 Content 변경 없음을 선언하고 `git status`로 Content·Config가 그대로임을 적는다. 수정이 필요해 보이면 asset을 건드리지 않고 멈춰 보고한다(사전 허용 S2 A).

## 9. 금지 범위와 금지 우회

- Content·Config·`.uasset`·`.umap` 수정, Editor 실행·PIE 금지.
- router Box extent·묶음 자리·진열 보정 값을 바꿔 선택 결과를 맞추지 않는다.
- `UPlayerInteractionComponent`, `UPlayerHeldTargetUseComponent`, `UPlayerEquipmentUseComponent`, Character, Widget, `FPlayerInteractionQuery`를 수정하지 않는다.
- `FServiceItemTransfer`(Evaluate/Try), `FDisplayStockRules`, LIFO·소모 규칙, HUD 문구·요약 문구를 바꾸지 않는다.
- 냉장고 `SelfAim` 경로와 `ADrinkFridgeActor`를 바꾸지 않는다.
- 채운 박스 분기(박스 종류 묶음)를 바꾸지 않는다.
- 선택 결과를 component 상태로 저장하거나 완충·지연을 두지 않는다. query와 execute에 서로 다른 선택 함수를 두지 않는다.
- 묶음 안에서 조준에 가까운 물품을 빼거나 강조하지 않는다(Q2 A).
- 기준점으로 `HitResult.ImpactPoint`나 조준선 수직 거리를 쓰지 않는다(깊이 의존, P2 위반).
- router가 `꺼낼 물건 없음`·`사용 중인 것은 꺼낼 수 없음` 문구를 직접 만들지 않는다(대체 묶음의 `EvaluateTake`를 쓴다).
- 기존 테스트 기대값은 6.2 범위만 바꾼다.

## 10. 코드 리뷰 기준

- 4.1 규칙의 각 식, 후보 조건, 정확 동거리, 대체 순서가 정본과 같다.
- `GetVisibleItemWorldLocations`의 개수·transform이 `RefreshStockVisual`과 같다(사용 중 포함, 빈 자리 제외, mesh bounds 중심).
- query·execute·focus가 같은 선택 함수와 key를 쓴다. 새 상태·Tick이 없다.
- 채운 박스·냉장고·집게 경로에 diff가 없다.
- 6.1·6.2 자동화가 시나리오를 실제 trace와 focus observer로 확인하고, VANI-012 단언 제거가 6.2 범위에 머문다.
- Content·Config diff 없음.

## 11. 사용자 PIE 관찰 항목

DefaultMap에는 화장대가 없으므로 상점에서 화장대를 사 설치한 뒤 확인한다. 빈 박스는 품목 박스를 개봉·진열해 비우거나 빈 상태 박스를 쓴다. 명세 10절 Given/When/Then 그대로다.

| ID | 관찰 |
|---|---|
| EBT-001 | 빗만 있는 화장대, 빈 드라이기 자리 조준 → 빗 맨 끝 강조·RMB `빼기` → RMB로 빗 1개 |
| EBT-002 | 드라이기 2·빗 3, 조준을 드라이기 쪽→빗 쪽으로 천천히 이동 → 가까운 쪽으로 강조 이동, 빈 묶음 위에서도 강조 유지 |
| EBT-003 | 스킨로션 1병·면봉 4, 스킨로션 병 조준 → 그 병 강조·RMB로 스킨로션 |
| EBT-004 | 스킨로션 사용 중만·빗 2, 사용 중 병 조준 → 빗 강조·RMB로 빗 |
| EBT-005·006 | 모두 빔 → `꺼낼 물건 없음`, 사용 중만 → `사용 중인 것은 꺼낼 수 없음`, 강조 없음, RMB 무변화 |
| EBT-007·008 | 연속 빼기 중 조준 이동해도 첫 품목만, 채운 박스는 박스 종류 묶음 |
| EBT-009 | 연속 빼기 중 화장대 밖 조준 → 멈춤, 재조준 자동 재개 없음 |
| EBT-010 | 샤워기 샴푸 0·바디워시 2, 빈 샴푸 자리 조준 → 바디워시 강조·RMB로 바디워시 |
| EBT-011 | 스킨로션 박스로 빗 조준 → 넣기 프리뷰·빼기 강조가 스킨로션 묶음 |
| EBT-012 | 스킨로션 새것 1병 강조 중 `bathhouse.Debug.Facility.BeginUse` → 강조가 빗으로 이동, RMB로 빗 |
| EBT-013 | 꺼내기 강조 옵션 끔 → 강조 없이 같은 대상이 빠짐 |
| EBT-014 | 보이는 진열 물품 어디를 조준해도 그 설비 HUD와 RMB 대상 |
| EBT-015 | 냉장고 공간 조준·몸체 조준 결과가 지금과 같음 |
| EBT-016 | 집게를 들고 화장대 조준 → 빼기 강조 없음 |

## 12. 복귀 재설계 여부

해당 없음(첫 설계).
