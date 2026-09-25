# 코드 리뷰 요청 — 보일러 노동 가동 수직 구현

## 단계와 범위

- C++ 구현 뒤, Unreal Editor 변경 전 코드 리뷰다.
- 입력 정본: .md/PROMPT_ARCHITECTURE.md, .md/PROMPT_IMPLEMENTATION.md, .md/Architecture/UtilityLaborSystem.md.
- 대상은 LAB-001~025, LAB-037~039다. LAB-026~036의 쿨러·수동 순환기는 범위 밖이다.
- 이 결과물은 Editor 작업 승인이 아니다. 리뷰 승인 뒤에만 .md/PROMPT_UNREAL.md를 따른다.

## 구현 요약

- UUtilityOperationComponent가 가동 잔량, 게임시간 감소, 0 경계와 payload import 상태를 소유한다.
- ABathWaterBoilerFacilityActor가 기존 utility 설비의 native child composition을 만들고 Heating 전용 authoring을 검증한다.
- AUtilityShovelActor, AUtilityFuelSupplyActor, private FUtilityFuelTransaction이 기존 carry/equipment/focus 경로로 한 회분 Coal 이동·반환과 재진입 방어를 처리한다.
- capacity snapshot을 Installed/Active로 나눴다. TotalPoints는 Installed로 유지하고 ActivePoints, InstalledDeficitPoints를 추가했다. 예약 요청/회수 gate는 Installed, 물 효과는 Active를 쓴다.
- 회수 payload에 bHasOperationState, RemainingOperationPoints를 추가해 stage/cancel/rollback/reinstall clock lifecycle을 연결했다.
- 관리 화면은 예약·가동·설치와 두 부족 원인을 표시한다. Interaction context에 generic component pointer를 추가하고 EPhysicalCarryKind 끝에 Shovel을 append했다.
- 0_ARCHITECTURE와 관련 Bath Water/Interaction/Physical Carry/Placement/UI Architecture 문서를 Source 구조에 맞췄다. Content와 Unreal 정본은 구현 단계에서 수정하지 않았다.

## 시나리오와 자동화

| 자동화 이름 | 의도한 범위 |
|---|---|
| BathhouseSim.Utility.Labor.OperationAndCapacitySplit | LAB-001~002, 012~013, 017 Installed/Active·소진, LAB-023의 0/min-positive/25/50/100 계기 quaternion과 construction/BeginPlay/configure/rebind/EndPlay 수명 |
| BathhouseSim.Utility.Labor.FuelInteractionAtomicity | LAB-003, 005~009, 011 fuel 이동; exhaustion Tick 전·후 투입, operation/fuel delegate 재진입과 완전 commit 상태, authoring fail-closed, actual Hold 입력 거부, G drop와 fixed-slot E 왕복 |
| BathhouseSim.Utility.Labor.RecoveryPayloadAndRollback | LAB-018, 020, 021, 037, 038 회수/배치; stage rollback, 잔존 staged actor/clock 검사, 구·무효 payload, 0 잔량 성공 회수, public `PlaceItemAsFacility()` 성공·실패와 재설치 감소 재개 |
| 기존 Bath Water Operations 테스트 + Utility recovery integration | snapshot의 Active/Installed 부족값·summary, Installed 300 / Active 100에서 가열 정지, 두 번째 보일러 가동 후 가열 재개 |

automation world에서 장시간 tick을 돌릴 때는 프레임 카운터도 전진시켜 operation component tick과 delegate publication을 실제로 실행한다. 회수·재배치 fixture는 test 전용 mesh boiler class를 사용해 실제 staged placement 순서를 검증한다.

## 변경 파일군

- 신규 Source: Public/Utility, Private/Utility, operation/fuel/recovery별 Private test translation unit, 공통 test support와 dynamic fuel delegate probe.
- 수정 Source: Facility Operations/types/capacity/utility actor/payload, Interaction context/carry enum/equipment-use trace context, UI capacity summary, 기존 Bath Water Operations tests.
- Architecture 정본: .md/0_ARCHITECTURE.md, BathWaterOperations, BathWaterManagementUI, Interaction, PhysicalCarry, Placement, UI, 신규 UtilityLabor 문서.
- 이번 단계 Content/Config/Level 변경은 없다.

## 책임·호환성·lifecycle 검토

- reflected 타입/컴포넌트 이름, UPROPERTY와 delegate 수명, staged FinishSpawning 전 payload import 순서를 확인한다.
- 기존 reflected symbol은 rename/delete하지 않았다. Shovel enum만 append했다. BP_Boiler reparent은 별도 Editor 작업이며 새 Core Redirect는 필요하지 않은 설계다.
- fresh visibility trace가 SupplyMesh/FuelIntake에 일치하는지, computer·placement 입력 소유 중 LMB를 차단하는지, 두 상태 silent commit 뒤에만 delegate가 broadcast되는지 본다.
- Hold 동안 clock/provider, 성공 회수 publication 1회, 실패 rollback의 stage 시점 잔량, invalid payload 시 원래 아이템 보존을 추적한다.
- utility base Actor의 신규 책임이 optional labor lifecycle hook 범위로 제한됐는지 확인한다.
- 변경 뒤 크기: UtilityShovelActor.cpp 516줄, UtilityFuelTransaction.cpp 296줄, UtilityOperationComponent.cpp 301줄, UtilityGaugeComponent.cpp 163줄, utility base cpp 543줄. 자동화는 operation/capacity, fuel interaction, recovery transaction 세 translation unit으로 나누고 공통 test fixture와 fuel delegate probe를 분리했다. 삽의 authoring validation 책임이 carry/equipment 경계에 적합한지와 transaction helper의 동기 원자성 경계를 검토한다.
- PROMPT_UNREAL의 exact BP/Definition 연결, 미확정 mesh 후보, WBP 글자 폭·PIE 조건이 C++ 계약과 일치하는지 검토한다.

## 검증 상태

- UE 5.8 `Build.bat BathhouseSimEditor Win64 Development` 최종 build가 성공했다. 새 automation translation unit을 포함해 compile했고 `UnrealEditor-BathhouseSim.lib`, `UnrealEditor-BathhouseSim.dll` link까지 완료했다.
- `BathhouseSim.Utility.Labor`: 3/3 통과. `BathhouseSim.BathWater`: 10/10 통과. `BathhouseSim.Interaction`(Physical Carry 포함): 11/11 통과. `BathhouseSim.Computer`: 1/1 통과.
- `BathhouseSim.Placement`: 5개 중 `StartupLockerReconciliation`, `TraceChannelIsolation` 통과. 다음 3개는 실패했다: `ActorReplacementFailureAtomicity`의 recovery fixture가 item을 반환하지 않음, `ActorReplacementTransaction`의 native Cube fallback/비차단 recovery fixture가 유효하지 않음, `SettingsZoneLeaseAndCompatibility`에서 기본 grid 20cm가 예상 10cm와 다르고 공통 item validation도 실패함.
- 전체 `Automation RunTests BathhouseSim`: 50개 실행, 47개 통과, 위 Placement 세 테스트만 실패. 두 actor replacement 테스트는 generic `ABathhouseFacilityActor` conversion 경로이고 `.md/PROMPT_REVIEW.md` 외 구현 변경에서 그 테스트·conversion 코드·facility setting을 수정하지 않았다. grid/validation 실패 역시 변경하지 않은 `FacilityPlacementAutomationTests.cpp`와 설정 정본을 사용하며 이번 utility code path와 별개다. 이 원인 분리는 수정 파일과 실패 assertion 경로를 대조한 판단이며, 이전 baseline 실행으로 회귀 여부를 확인한 것은 아니다. 전체 로그에서 assertion/ensure/fatal marker는 발견되지 않았다.
- `git diff --check` 통과. 신규 Source/Architecture 파일 trailing whitespace 검색 결과 없음. LF→CRLF 경고만 있다. `Content/`, `Config/`, `.md/Unreal/` 및 `USER_UNREAL.md`에는 수정 사항이 없다.
- Blueprint Compile/Save/reload와 PIE는 미실행. PIE에서 actual LMB hold duration/input routing, visible mesh silhouette와 pivot, BP_Boiler/WBP authoring, computer/placement 입력 우선순위, 1024×576 layout, world gauge와 computer 동시 갱신을 확인해야 한다.

## 리뷰 결과 요청

시나리오 추적, C++/Blueprint 계약, 클래스 성장, lifecycle·rollback, 남은 3개 Placement 회귀 실패와 미검증 Editor/PIE 상태를 판정해 주세요. 문제가 있으면 .md/PROMPT_IMPLEMENTATION_R.md에 우선순위·정확한 경로·재현·수정 방향을 작성하고, 승인하더라도 이 보일러 수직 구현만 승인해 주세요.
