# Utility Lever System

## Status And Scope

- [UtilityLaborSystem.md](UtilityLaborSystem.md)의 하위 문서다. 순환기 조작부, 레버 왕복 상태 기계, 보상, 취소·복귀, 레버 자세와 진행 표시를 소유한다.
- 2026-09-26 설계. 구현 상태(2026-09-26): 이 설계의 Source 구현과 코드 재작업이 끝났고 코드 리뷰는 사용자 지시로 승인됐다. Editor 단계에서 다섯 Blueprint(보일러·쿨러·순환기·삽·드라이아이스 공급함) 저장·Compile까지 진행했으나 DefaultMap instance의 새 Class Default 상속, 드라이아이스 공급함 외부 actor 저장, Data Validation, 새 프로세스 재로드와 직접 PIE 수용이 남아 통합 승인 전이다(`.md/PROMPT_INTEGRATION_REVIEW.md`, `.md/USER_UNREAL.md`). 이번 단계는 순환기 **수직 구현**이다. 대상: LAB-029~031, 033~036, 066~072. `순환기 후속`(LAB-032, 073~075)은 사용자 수직 승인 뒤 완료 처리한다. 다만 취소 규칙(컴퓨터·회수·대상 소실)과 회수 payload는 이번에 구현한다.
- 새 Input Action, Hold 입력, 레버 물리, 소리·이펙트, 순환기 이외 레버 설비는 만들지 않는다.

## Composition

`ABathWaterCirculatorFacilityActor : ABathWaterLaborUtilityFacilityActor`(신규). Capacity kind Circulation 고정.

| subobject | class | 부모 | 규칙 |
|---|---|---|---|
| `LeverOperatingVolume` | 신규 `UUtilityLeverOperatingVolumeComponent : UBoxComponent` | `SceneRoot` | 고정 조작부 판정. QueryOnly·Visibility Block·Navigation off·unit scale·extent > 0. `IPlayerInteractable`, `IPlayerInteractionFocusObserver` |
| `LeverPivot` | `USceneComponent` | `SceneRoot` | 위치가 회전 중심, 배치 회전이 위 기본 자세 |
| `LeverMesh` | `UStaticMeshComponent` | `LeverPivot` | 필수 mesh, NoCollision·Navigation off |
| `LeverLabor` | 신규 `UUtilityLeverLaborComponent : UActorComponent` | 비공간 | 왕복 상태·시간·보상·취소·레버 자세 |

- 생성자에서 `LeverLabor`에 Operation·`LeverPivot`을, `LeverOperatingVolume`에 `LeverLabor`를 native 주입한다. 주입 참조는 owner 수명 동안 유지한다.
- 조작부 판정과 레버 표현을 분리한다. 레버가 움직여 조준선에서 벗어나도 고정 Volume을 계속 조준하면 유지된다(Q58). 레버 mesh는 아무것도 막거나 밀지 않는다.
- `LeverLabor`가 상태·실행 owner이면서 레버 자세도 적용한다. 레버 자세는 왕복 상태의 순수 함수이고 독립 timer·입력이 없어 별도 표현 component를 두지 않는다. 문 표현과 달리 복귀 상태가 E 수락 여부를 결정하므로 상태와 분리할 수 없다.

## Stroke State Machine

| 상태 | 필드 | E | 레버 alpha(0 위, 1 아래) |
|---|---|---|---|
| `Idle` | 없음 | 시작 평가 | 0 |
| `Stroking` | Source(`UPlayerInteractionComponent` weak), SourceCarry(weak), Elapsed | 거부(움직이는 중) | `t<h ? t/h : 2 - t/h`, h = StrokeSeconds/2 |
| `Returning` | StartAlpha, Elapsed | 거부(돌아오는 중) | `StartAlpha * (1 - e/CancelReturnSeconds)` |

- 회전은 공용 pivot helper로 `Baseline * AxisAngle(normalize(LocalRotationAxis), DownAngleDegrees * alpha)`를 매번 계산한다.
- component Tick은 `Idle`이 아닐 때만 켠다. 게임시간 delta를 사용해 pause에 멈추고 time dilation을 따른다.

## Start

`EvaluateStart(Context)`는 side-effect 없는 평가이며 query와 execute가 같이 쓴다.

- 상태 `Idle`, Context의 carry가 빈손, interaction suppression 없음, `UtilityLaborInputGuard` 통과(컴퓨터 capture·배치 모드 아님).
- 설비 authoring 유효, placed domain active·non-staged, placed clock active, labor block 없음, `Operation` 현재값 < 최대.
- 실패 문구: 빈손 필요, 가득 참, 회수 중, 미설치, 레버가 움직이는 중, 레버가 돌아오는 중, 다른 사용자가 조작 중. 삽을 들고 있어도 삽 상태는 바꾸지 않는다.

execute(`LeverOperatingVolume::ExecuteInteraction`): 평가 → 같은 trace 거리·channel의 fresh single-hit가 이 Volume인지 확인 → `Stroking` 진입(Elapsed 0, Source·SourceCarry 기록, Tick on). E 입력은 `Started` 한 번만 들어오므로 누르고 있어도 다음 왕복을 시작하지 않는다.

## Continue, Complete, Cancel

매 Tick 순서:

1. 유지 조건: Source·SourceCarry 유효, Source가 suppression 아님, Source의 focus가 이 Volume(아래 focus 종료로 해제), SourceCarry 빈손, labor block 없음, placed domain active·clock active, input guard 통과. 하나라도 실패하면 취소한다.
2. Elapsed에 delta를 더하고 레버 자세를 적용한다.
3. Elapsed ≥ StrokeSeconds이면 완료: `Operation->ApplyLaborReward(StrokeRewardPoints)`를 한 번 호출하고 `Idle`, 레버를 정확히 기본 자세로 둔다. 보상 호출이 실패하면(재진입 guard 등) 취소로 처리하고 경고 로그를 남긴다.

- `LeverOperatingVolume`이 Stroke Source의 `NotifyInteractionFocusEnded`를 받으면 그 즉시 취소한다. 시선 이탈, 사거리 밖(trace가 닿지 않음), 컴퓨터·배치 진입(query clear), 대상 소실이 이 경로로 온다.
- 자연 감소는 Operation이 계속 계산한다. 왕복 중 0이 되어도 취소하지 않으며 완료 순간 보상으로 다시 가동한다(LAB-071). 50에서 시작하면 완료 시 59(LAB-033), 95면 100(LAB-034).
- 취소: 보상 없음. `CancelReturnSeconds > 0`이면 현재 alpha에서 `Returning`, 0이면 즉시 `Idle`. 진행 표시는 즉시 사라진다.
- `Returning`이 끝나면 `Idle`이 되고 새 E를 받을 수 있다(LAB-068).
- 회수 Hold 시작은 labor block으로 유지 조건 1에서 취소된다. staged 변환은 collision off로 focus 종료가 된다.
- EndPlay: 진행 중이면 보상 없이 취소하고 레버를 즉시 기본 자세로 둔다. 주입 참조와 baseline은 유지한다.
- construction·BeginPlay: `Idle`, 기본 자세. 신규·재설치 Actor는 기본 자세로 시작한다.

## Query And HUD

`LeverOperatingVolume::QueryInteraction(Context)`:

| 상태 | ActionName | bCanInteract | 진행 |
|---|---|---|---|
| `Idle` | `레버 왕복` | `EvaluateStart` 결과, 실패 이유 포함 | 숨김 |
| `Stroking`, Context가 Source | `레버 왕복 중` | false, 실패 문구 없음 | `bPrimaryProgressVisible=true`, `HoldProgress=Elapsed/StrokeSeconds` |
| `Stroking`, 다른 Source | `레버 왕복` | false, 다른 사용자 조작 중 | 숨김 |
| `Returning` | `레버 왕복` | false, 레버가 돌아오는 중 | 숨김 |

- `PrimaryActivationMode`는 항상 `Instant`다. 진행 막대는 레버 움직임과 같은 시간에 차고 완료·취소 뒤 다음 query refresh에서 사라진다. 기존 Q 회수 진행 row와 다른 막대다.
- 진행 중 query는 매 Tick 바뀌므로 `OnInteractionQueryChanged`와 focus 변화 알림이 매 Tick 발생한다. 이는 Hold 진행과 같은 비용이며 허용한다.

## Operation Reward

- 보상은 Operation의 단일 owner API `ApplyLaborReward`만 사용한다. 삽·공급함이 없으므로 연료 transaction을 쓰지 않는다.
- 같은 게임시각으로 settle한 뒤 `min(Max, Current + Points)`를 commit하고 한 번 publish한다. 0→양수 경계는 capacity edge를 한 번 알린다.

## Authoring

`UUtilityLeverLaborComponent`, EditDefaultsOnly, `BP_Circulator` class가 정본:

- `StrokeSeconds=1`(finite > 0), `StrokeRewardPoints=10`(finite > 0), `CancelReturnSeconds=0.2`(finite ≥ 0).
- `LocalRotationAxis=(0,1,0)`(finite, nonzero), `DownAngleDegrees=-60`(finite, 0 아님). 기본값은 시작값이며 실제 축·각도는 레버 mesh에 맞춰 Editor에서 정한다.
- CallInEditor `PreviewDownPose()`/`RestoreUpPose()`: Editor world 전용 transient 표시. Game/PIE world는 no-op. PreSave·construction·BeginPlay가 기본 자세를 복원한다.
- 조작부 Volume은 레버 움직임 전체를 덮되 본체 전체를 덮지 않는다(Editor 확인). 위치는 설비 local cm, 회전은 도다.

## Verification

실제 조준 경로 automation은 possess된 Pawn + Camera + Carry + Interaction과 `World->Tick` 0.05초 step을 사용한다. 보일러 문 test harness를 재사용한다.

| ID | 확인 |
|---|---|
| LAB-029 | 0 잔량, 빈손 조준, E 누르고 바로 놓음 → 0.5초 alpha 1, 1초 alpha 0·+10·순환 가동·정화 재개 |
| LAB-030 | 왕복 중 반복 E 거부·상태 불변, 누르고 있어도 완료 뒤 새 왕복 없음 |
| LAB-031 | 왕복 중 시선 이탈·사거리 밖 → 보상 없음, 진행 숨김, 0.2초 복귀 |
| LAB-033, 034, 071 | 50→59, 95→100, 0.5에서 시작해 도중 0 → 완료 10 |
| LAB-035, 036 | 유일 순환기 소진 시 정화·능동 수온 정지·설정 보존, 노동 복구 시 자동 재개 |
| LAB-066 | 100이면 시작 거부·가득 참, 감소 뒤 시작 |
| LAB-067 | 레버가 조준선 밖으로 움직여도 Volume 조준 유지 시 완료 |
| LAB-068 | 복귀 중 E 무시, 복귀 뒤 새 E로 시작 |
| LAB-069 | 빈 삽·재료 삽을 들고 E → 시작 안 함, 빈손 필요, 삽 불변 |
| LAB-070 | Pawn 이동 중 조준 유지 시 완료 |
| LAB-072 | `bPrimaryProgressVisible`·`HoldProgress` 진행과 완료 즉시 숨김 |
| LAB-032(후속 사전 구현) | suppression, 회수 Hold 시작, Actor 파괴 → 보상 없음·진행 숨김 |

추가: EndPlay 뒤 주입 참조 유지, 레버 authoring validation(mesh 누락·collision·Volume scale/extent), construction baseline 불변, preview 비저장.
