# 코드 리뷰 입력 — Placement 설비 회수 아이템 scale 읽기 수정

## 단계와 범위

- 현재 단계: C++ 구현과 자동화 검증 완료. Editor authoring은 없으며 PIE의 사용자 확인 전이다.
- 입력: 2026-09-28 구현 프롬프트의 A1 회수 scale 수정과 A2 LOCTEXT 키 분리.
- Placement 정본과 구현 프롬프트의 scale 계약이 일치한다. 기능·Architecture 변경은 필요하지 않았다.
- 구현 QNA 미해결 사항은 없다.
- Source 외 Content, Config, Level은 수정하지 않았다. PROMPT_UNREAL.md는 생성하거나 수정하지 않았다.

## 요구사항 추적

| 항목 | 구현 및 검증 |
|---|---|
| A1 회수 collision query scale | ValidateRecoveryCandidate가 GetDefinitionItemScale의 Definition ItemRoot relative scale을 읽는다. helper 실패 시 후보 검증을 실패시킨다. |
| A1 회수 spawn scale | SpawnActorDeferred와 FinishSpawning 모두 OverrideRootScale을 명시한다. 배치 spawn 경로는 변경하지 않았다. |
| 회수 blocker 회귀 | unit-scale query가 막히고 authored scale query가 통과하는 blocker를 자동화에서 확인한 뒤 실제 회수를 완료한다. |
| Blueprint식 CDO | 직접 relative-scale setter를 쓰는 fixture CDO의 actor scale과 root relative scale을 로그로 남기고 둘이 다른 조건에서 검증한다. |
| 실제 공통 BP | BP_PlaceableFacilityItem CDO helper scale과 실제 회수 actor의 world scale이 ItemRoot relative scale과 일치한다. |
| 회수 후 carry | physics, CCD, Pawn Ignore를 확인하고 E pickup 후 G drop에서도 world scale이 유지되는지 확인한다. |
| 기존 scale assertion | 기존 “Recovery uses the derived facility item CDO scale” 검증을 fixture CDO root relative scale 비교로 바꿨다. |
| A2 LOCTEXT key | GetDefinitionItemScale의 key만 InvalidRecoveryItemRootScale로 바꿨다. BuildDefinitionCollisionQuery의 InvalidRecoveryItemScale은 유지했다. |

## Drop 위치와 Source 검색

- GetRecoveryDropTransform은 PlacementFootprint world bounds의 하단과 RecoveryDropZOffsetCm으로 위치를 계산한다. recovery item extent는 참조하지 않으므로 변경하지 않았다.
- Source 전체에서 테스트를 제외하고 authored CDO scale 읽기를 검색했다. 해당 CDO GetActorScale3D 읽기는 구현에서 제거됐다.
- 남은 production GetActorScale3D 사용은 CheckoutKeyPlacementUtils.cpp:81의 runtime Key transform 처리다. CDO authored scale 사용이 아니어서 변경하지 않았다.

## Fixture 기록

자동화 로그의 사전 기록:

- Fixture CDO GetActorScale3D: X=1.000, Y=1.000, Z=1.000
- Fixture ItemRoot relative scale: X=0.400, Y=0.600, Z=0.800
- 두 값은 다르며 equal=false로 Blueprint CDO의 갱신되지 않은 component-to-world 상황을 재현했다.
- scale 기대값은 fixture 또는 BP_PlaceableFacilityItem의 ItemRoot relative scale에서 읽는다.

## 변경 파일과 책임

- Source/BathhouseSim/Private/Placement/FacilityActorConversionTransaction.cpp: 회수 후보 scale 읽기와 회수 deferred spawn scale method 수정.
- Source/BathhouseSim/Private/Placement/PlaceableFacilityItemCollision.cpp: recovery root scale 실패 LOCTEXT key 분리.
- Source/BathhouseSim/Private/Tests/FacilityPlacementAutomationTestProbe.h/.cpp: CDO stale scale fixture와 grid-valid recovery facility fixture 추가.
- Source/BathhouseSim/Private/Tests/FacilityPlacementAutomationTests.cpp: 기존 회수 scale assertion만 수정.
- Source/BathhouseSim/Private/Tests/FacilityRecoveryScaleAutomationTests.cpp: 신규 전용 회수 scale 자동화.
- .md/PROMPT_REVIEW.md: 현재 구현 작업의 코드 리뷰 입력과 검증 결과.
- 테스트 전용 fixture 외에 production UPROPERTY, UFUNCTION, component, class API, serialization 변경은 없다. Core Redirect는 필요하지 않다.
- FacilityActorConversionTransaction.cpp는 458→468줄, 테스트 probe header 149→163줄, probe cpp 176→188줄이다. 기존 대형 Placement 테스트에는 기존 assertion만 4줄 추가했고 새 테스트는 별도 322줄 파일에 두었다.

## 빌드와 자동화

- UE 5.8 BathhouseSimEditor Win64 Development Build.bat: 성공.
- Placement 최종 재실행: 6개 완료, 경고 없는 성공 4개, 경고 있는 성공 2개, 실패 0개.
- Shop: 11/11 성공, 실패 0개.
- Interaction: 12/12 성공, 실패 0개.
- 전체 BathhouseSim: 70개 완료, 경고 없는 성공 62개, 경고 있는 성공 8개, 실패 0개, 미실행 0개.
- 전체 report: Saved/Automation/Reports/2026-09-28/BathhouseSim_RecoveryScale/index.json.
- 집중 Placement, Shop, Interaction report는 각각 Placement_RecoveryScale_Retry1, Shop_RecoveryScale, InteractionCarry_RecoveryScale 폴더의 index.json에 있다.
- 첫 Placement 실행에서 테스트 facility footprint가 프로젝트 grid와 맞지 않아 Definition validation이 실패했다. 전용 fixture가 현재 grid 설정에 맞는 footprint를 쓰도록 수정하고 재실행한 결과는 위와 같이 통과했다.
- 전체 report의 경고 있는 성공 8개에는 테스트 진단 로그와 Shop BlueprintLoad의 외부 HTTP 연결 불가 로그가 포함된다. 실패 테스트는 없다.
- tracked diff에서 git diff --check를 실행했다.

## Architecture 및 Editor

- Architecture 정본은 변경하지 않았다. 이미 ItemRoot relative scale을 공통 scale 정본으로 정의하며 이는 내부 버그 수정 범위다.
- Content/Editor Compile·Save·PIE는 수행하지 않았다.
- 사용자 PIE 확인 항목: 좁은 곳에서 설비 회수가 가능하고, 회수 아이템 크기가 변하지 않는지 확인한다.
