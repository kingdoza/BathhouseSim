# 구현 프롬프트 — 보일러 노동 가동 수직 구현

## 단계와 승인

- 2026-09-23 사용자의 “이대로 설계작업 진행해” 승인으로 확정한 설계의 다음 단계 입력이다. 이 문서 작성 시 Source/Content 구현은 미실행이다.
- `.md/AGENT_WORKFLOW.md` → `.md/AGENT_IMPLEMENTATION.md`를 읽고 C++ 구현 단계만 수행한다.
- 기능 입력: `.md/PROMPT_ARCHITECTURE.md`, `.md/QNA_FEATURE_SPEC.md` Q29~47. 기능 파일의 이전 승인 대기 문구와 이번 사용자 승인을 구분한다.
- 중심 설계: `.md/Architecture/UtilityLaborSystem.md`. 세부 API·authoring·lifecycle의 정본이며 반드시 전체 읽는다.
- 관련 정본: BathWaterOperationsSystem, BathWaterManagementUISystem, InteractionSystem, PhysicalCarrySystem, PlacementSystem, UISystem, CoreSystem.
- Editor 사실: `.md/REPORT_UNREAL_DISCOVERY.md`, `.md/Unreal/0_UNREAL.md`와 관련 BathWater/Placement/InteractionUI 문서.
- 기존 물 Operations와 관리 화면을 처음부터 재구현하지 않는다. 현재 Source에 적용하는 변경이다.

## 수직 범위와 비목표

- 보일러 + 석탄 무제한 공급함 + 한 회분 공용 삽 + exact 거치 + 실제 투입구 + 원형 계기 + 용량/UI/회수 연결.
- 이번 수용 시나리오: LAB-001~025, LAB-037~039. 검증표와 기능 원문을 함께 사용한다.
- LAB-026~036은 쿨러·수동 순환기 후속 확장이다. 현재 완료로 보고하지 않는다.
- 기존 순환기 공급은 보일러 가열 검증의 준비 조건으로 유지한다. 최종 노동 순환기의 신규 잔량 0 계약을 폐기하는 항시 공급 옵션을 만들지 않는다.
- DryIce actor, 순환기 왕복/취소/진행 UI, 설비 목록/지도 아이콘, 자동 설정 감소·비례 분배, SaveGame/replication은 범위 밖이다.
- 새 Input Action, collision channel, Navigation 설정, StateTree graph, module dependency 변경은 필요 없다.
- Content/Level 편집, 임의 asset resave와 Editor 완료 표시는 금지한다. 코드 리뷰 후 별도 단계다.

## Source 작업 경계

신규 `Public/Utility`, `Private/Utility`:

1. `UtilityFuelTypes.h`: None/Coal kind와 load snapshot/result/failure 타입.
2. `UtilityOperationComponent.*`: 잔량, 게임시간 감소, 가동 경계, 회수 lifecycle.
3. `UtilityFuelIntakeComponent.*`: 실제 static mesh 기반 interactable endpoint.
4. `UtilityGaugeComponent.*`: 주입받은 Operation을 needle pivot 회전으로 표시.
5. `BathWaterBoilerFacilityActor.*`: 기존 utility base의 native child, component 조립.
6. `UtilityShovelActor.*`: 기존 carry + equipment use, load와 시각화.
7. `UtilityFuelSupplyActor.*`: unlimited Coal 공급과 F 반환.
8. private `UtilityFuelTransaction.*`: 두 owner를 검증하고 silent commit/publish하는 동기 helper.

기존 최소 변경:

- Facility `BathWaterUtilityFacilityActor`, `BathWaterUtilityCapacityComponent`, `BathWaterUtilityPlacementInstanceData`.
- Facility `BathWaterOperationsTypes`, `BathWaterOperationsSubsystem`; condition은 active query 연결에 필요한 부분만.
- Interaction carry kind에 Shovel append, 필요하면 기존 trace를 호출하는 generic read-only refresh API.
- UI `BathWaterCapacitySummaryWidget` 및 기존 snapshot 소비자의 deficit/cache 처리.
- `Private/Tests`에 labor focused automation과 기존 Operations/Placement/Equipment/UI 회귀 추가.
- 기존 reflected symbol 삭제/rename 금지. boiler subclass 추가이므로 Core Redirect 불필요하다.

## 조립과 authoring

- `ABathWaterBoilerFacilityActor : ABathWaterUtilityFacilityActor`는 inherited component 이름/구조를 보존한다.
- 신규 native subobject: `Operation`, `FuelIntake`, `GaugeFace`, `GaugeNeedlePivot`, `GaugeNeedleMesh`, `GaugePresentation`.
- base에는 optional native `GetUtilityOperation()`와 `RequiresLaborOperation()` hook만 둔다. 보일러 필수 Operation 누락이나 Heating 아닌 kind는 fail-closed validation이다.
- 기존 Circulator/Cooler base는 이번 수직 단계의 legacy 공급만 유지한다. editable AlwaysOn flag를 만들지 않는다.
- MaxOperationPoints=100, DecayPointsPerSecond=1은 Operation EditDefaultsOnly. 신규 설치/선배치 초기 잔량은 0 고정이며 InitialPoints 옵션 없음.
- 공급함 FuelKind=Coal, ScoopPoints=25는 공급함 class의 EditDefaultsOnly 한 곳이 정본이다. 삽 적재 시 종류와 양을 복사한다.
- 삽 WorldMesh는 physical root, LoadVisual은 collision-free child. 삽 HeldTransform은 기존 일반 도구와 같이 위치/회전만 적용한다.
- 계기의 signed local axis, zero/max angle, ActiveStartRatio=1/3은 component EditAnywhere. pivot relative location으로 회전 중심을 정한다.
- intake 실제 mesh QueryOnly/Visibility Block/Nav false. 계기/적재 표현은 NoCollision/Nav false. 기존 설비 몸체와 footprint 계약 유지.
- mesh asset 자체는 Editor에서 연결한다. Source에 미확인 hard-coded asset path를 넣지 않는다.

## 시간과 lifecycle

- 잔량 anchor와 게임시각으로 `max(0, anchor-rate*elapsed)`를 계산한다. pure getter는 상태나 delegate를 변경하지 않는다.
- 설치 양수 동안만 component Tick으로 계기와 소진 경계를 알린다. pause는 감소 중지, game time dilation 적용, 컴퓨터 사용은 감소 지속.
- capacity getter도 같은 시각의 projected 잔량을 사용하여 component Tick 순서로 0 이후 공급이 남지 않게 한다.
- 0↔양수에서만 capacity publication. 양수↔양수 감소/투입은 계기 갱신이며 topology publication 없음.
- 신규 0 초기화와 import state를 구분한다. BeginPlay가 import 잔량을 지우면 안 된다.
- component delegate 해제, EndPlay, clock start/stop, register/unregister는 중복 호출에 안전해야 한다.

## 입력과 원자적 연료 작업

- LMB Instant BeginEquipmentUse에서만 한 번 실행한다. 홀드 반복, release 투입, delay와 montage 완료 gating 없음.
- 삽 E pickup/거치, G 내려놓기는 기존 carry 구현 패턴과 exact slot 계약을 따른다. Load는 동일 Actor에 보존한다.
- F는 현재 같은 종류 공급함에 한 회분 전량 반환한다. 공급함은 유한 재고가 없다.
- fresh camera single-hit는 기존 Interaction 거리/channel을 사용한다. 현재 held identity, range, occlusion, input mode/suppression과 target lifetime을 재검증한다.
- intake component 실제 hit만 투입 허용. body hit로 대체하거나 앞 blocker를 건너뛰지 않는다.
- target query는 side-effect 없이 안내/실패 이유만 반환한다. E/F/LMB/Q prompt 우선순위를 보존한다.
- transaction은 game thread 동기 실행: validation → 양쪽 guard → 같은 시각 settle/candidate → silent commit → 결과 publication.
- commit 사이에는 Blueprint/delegate/latent call을 넣지 않는다. guard 동안 재진입 mutation 거부, callback 뒤 참조 재검증.
- 삽이 비었을 때만 공급. 이미 적재했으면 추가 퍼담기 거부.
- 최대 잔량이면 투입 거부·재료 보존. 조금이라도 공간이 있으면 load 전부 소비 후 min(max,current+load), 초과 폐기.
- 실패는 연료를 이동하지 않는다. 실패/회수 거부 중 정상 자연 감소까지 rollback하지 않는다.
- Instant use End/Cancel은 완료된 거래를 되돌리지 않는다. post-commit actor 파괴도 재료 복제 복구하지 않는다.

## 설치/가동 용량과 화면

- 기존 TotalPoints는 Installed. UsedPoints는 예약. ActivePoints와 InstalledDeficitPoints를 추가한다.
- DeficitPoints는 max(Used-Active,0), InstalledDeficitPoints는 max(Used-Total,0). IsSatisfied는 가동 효과 판단이다.
- Snapshot 수동 생성 테스트와 모든 소비자를 점검한다. 기존 Total을 Active로 치환하는 일괄 수정은 금지한다.
- 설정 request/clamp와 CanRemoveProvider는 Installed. 기존 deficit 상태 감량 허용, no-op publication 생략과 다른 Bath 보존을 유지한다.
- 실제 정화/가열/냉각 effect는 Active 기준. 부족 종류를 요구하는 욕탕 전체 중지, 용량 복구 시 자동 재개, 비례 배분 없음.
- circulation 부족은 정화·능동 thermal 중지. heating/cooling 부족은 해당 thermal만 중지. 자연 복귀와 실제 입욕자 오염은 유지한다.
- provider register/unregister와 가동 경계는 capacity/data revision만 바꾸며 bath topology 변경 없음.
- 기존 9개 summary binding 재사용: `예약 Used / 가동 Active / 설치 Total`. status는 두 부족을 구분, bar는 예약/설치.
- summary cache에 새 값을 포함한다. map tile identity, computer focus/session과 기존 slider request 경로는 유지한다.

## 원형 계기

- v==0이면 p=0, 양수면 p=a+(1-a)*v/Max. Max100/a1/3에서 v25는 1/2, v50은 2/3이다.
- baseline relative quaternion * local-axis angle로 매번 계산한다. 누적 회전, world-axis 오인, 위치/scale 변경 금지.
- axis normalize/음수 지원, finite/zero-axis/ratio validation. 별도 damping/noise 지연 없음.
- face와 needle은 독립 mesh. 0으로 소진 시 바늘 0 위치와 Active0을 표시한다.

## 회수 transaction과 payload

- 기존 FFacilityActorConversionTransaction의 source-last-destroy 순서와 fault injection을 재사용한다.
- Hold begin은 Installed 기준 회수 허용과 labor block. provider·clock 유지. Hold cancel은 block만 풀며 시작 잔량 복원 금지.
- 성공 직전 export와 stage는 같은 synchronous game time이다. export 현재 잔량, stage clock stop + silent unregister.
- 실패 시 source/domain/collision/provider 복원과 stage 잔량 clock 재개. Hold 시작값 rewind나 중복 capacity publication 금지.
- 성공 package는 잔량 보존, 시간 감소/가동 기여 없음.
- payload에 bHasOperationState, RemainingOperationPoints 추가. 기존 kind/capacity/Definition 보존.
- 구 payload 새 필드 없음은 신규 empty로 호환. invalid kind/NaN/음수/Max초과는 import 거부하고 원본 item 유지.
- import는 FinishSpawning 이전, staged clock off. item 소비와 최종 placed domain commit 성공 후에만 clock 시작.
- 기존 욕탕 water/condition freeze는 변경하지 않는다. 보일러 Hold의 감소 지속과 혼동 금지.

## 검증 추적

| 시나리오 | Native 자동화 + 후속 PIE 확인 |
|---|---|
| LAB-001~002 | 신규0, Installed100/Active0, 설치 기준 목표 예약 허용 |
| LAB-003~005 | 무제한 공급25, 단발 Begin만 실행, 재적재 거부 |
| LAB-006~009 | 투입·가열 재개,90+25 overflow,full 거부,empty/range/occlusion 실패 |
| LAB-010~011 | drop/slot 왕복 적재 보존, F 반환 원자성 |
| LAB-012~014 | 25→15→0 시간 감소,0 경계 once,재투입 자동 재개 |
| LAB-015~016 | 설치200/가동100/예약150 전체 가열 정지와 복구 |
| LAB-017~021 | 회수 gate,Hold 감소·취소·투입 차단,package 보존,설치 실패 rollback |
| LAB-022~024 | summary 세 값,계기50→2/3,Computer/Placement 입력 우선권 |
| LAB-025 | 가열 부족 중 정화·자연 복귀·입욕자 오염 보존 |
| LAB-037~039 | Installed만으로 회수 허용,Hold 중0,열린 화면과 월드 동시 소진 표시 |

추가 edge automation:

- 같은 frame 중복 입력/재진입 callback/Actor EndPlay, dead weak target, stale cached hit와 다른 held Actor.
- 동일 게임시각 다중 query는 감소/publication 중복 없음, pause/time dilation, 큰 delta와 정확한 소진 경계.
- 최소 양수는 정격 기여, full 판정은 클릭 시점의 현재값. 자연 감소 뒤 빈 공간은 허용한다.
- callback은 완성된 삽/가동 양쪽 상태만 관찰한다.
- fault point별 회수/재설치 rollback, import-before-BeginPlay, 구 payload 호환과 invalid payload 거부.
- provider-only capacity revision에서 map tile 재생성 없음; no-op setting request publication 없음.
- 기존 Equipment/Carry/Placement/BathWater/Computer/UI focused suite 회귀. StateTree를 수정하지 않았음을 확인한다.

## 빌드와 단계 결과물

- 새 reflected layout은 Editor 종료 후 full build/restart로 검증한다. Live Coding만으로 검증 완료하지 않는다.
- `.md/AGENT_WORKFLOW.md`의 UE5.8 Build.bat 진입점을 사용한다. system dotnet/UBT/MSBuild 직접 실행 금지.
- 필요한 실행 권한이 없으면 차단을 보고한다. JIT 팝업·.NET 오류를 반복 retry로 숨기지 않는다.
- 수행한 build/automation 결과와 미수행 PIE를 구분한다. 실패 또는 환경 차단을 완료로 표시하지 않는다.
- 구현 완료 시 `.md/PROMPT_REVIEW.md`, `.md/PROMPT_UNREAL.md`를 작성한다. architecture target을 실제 구현과 대조해 상태를 갱신한다.

## 후속 Editor 인계 — 지금 실행하지 않음

- 기존 BP_Boiler를 native boiler child로 reparent, inherited 이름/값 보존, compile/save/reload.
- 조사에서 BP/instance FacilityPlacement.Definition=None이었다. 기존 DA_FacilityPlacement_Boiler 연결을 실제 저장·검증한다.
- Utility/BP_UtilityShovel, BP_CoalSupply와 기존 generic fixed-slot의 exact assigned instance를 authoring한다.
- 보일러/투입구, 삽/적재, face/needle의 실제 구분 가능한 mesh를 연결하고 pivot·Visibility hit·Nav 제외를 검증한다. 미확인 mesh를 승인된 최종 art로 간주하지 않는다.
- existing Circulator 공급을 보일러 테스트 전제조건으로 명시하고 실제 등록/Definition 상태를 확인한다. 신규 순환기 노동 구현으로 범위를 확대하지 않는다.
- 기존 WBP capacity summary의 긴 문구가 1024×576에서 잘리지 않는지 확인한다. BindWidget 이름/타입 유지.
- DefaultMap에서 LAB 현재 범위를 실제 E/F/G/LMB/Q와 computer/gauge/가열/소진/회수로 검증한다.
- MCP 불가 작업은 실제 근거와 exact asset path로 USER_UNREAL에 인계한다. 현재 설계 단계에서 선제 작성하지 않는다.
- 코드 리뷰 → Editor → 통합 리뷰 → 사용자 보일러 수직 승인 뒤에만 쿨러/수동 순환기 확장 설계를 시작한다.
