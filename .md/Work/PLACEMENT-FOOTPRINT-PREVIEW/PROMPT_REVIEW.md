# PROMPT_REVIEW — PLACEMENT-FOOTPRINT-PREVIEW 배치 미리보기의 footprint 공간 표시

- 작업 ID: `PLACEMENT-FOOTPRINT-PREVIEW`
- 단계: 구현
- 상태: 완료

## 기능 계약과 현재 단계

- 계약: 같은 폴더 `PROMPT_ARCHITECTURE.md`(FPV-001~016), 설계 `PROMPT_IMPLEMENTATION.md`. 한 작업 공통 구현(Q9 A). 리뷰 diff 기준 커밋 `0c5f8a0`.
- 지도 버그 병합분(`81a1c4c`)과 `BP_ClothesLocker.uasset` 사용자 변경은 이번 범위가 아니다.
- 구현 시작 전 확인: `git diff 42eee21 -- Source/BathhouseSim/*/Placement Config/DefaultGame.ini`가 비어 있었다(설계 기준 Source와 같음). `QNA_IMPLEMENTATION.md`는 필요 없었다.

## 시나리오 ID별 코드·테스트 연결

| 시나리오 | 구현 경로 | 자동화 |
|---|---|---|
| FPV-001 크기·중심·방향 일치 | `FacilityPlacementFootprintPreview::ComputeSurface`, `AFacilityPlacementPreviewActor::BuildFootprintSurface` | `Placement.FootprintPreview.SurfaceMath`(네 모서리 = `RelativeFootprint * Candidate` 바닥 + 높이), `SurfaceActor`(component 모서리) |
| FPV-002 반투명 비침·가림 | footprint 재질 translucent 검증, `TranslucencySortPriority` 설정(footprint < 메시) | `SurfaceActor`(우선순위 순서 계약). 화면 판정은 PIE |
| FPV-003 이동·회전·snap | footprint는 preview Actor 자식 component | `SurfaceActor`(`SetActorTransform` 뒤 모서리) |
| FPV-004 유효·무효 색 | `SetPlacementValidity`가 같은 호출에서 DMI `PreviewColor` 설정, 색 원본은 메시 재질 `Param` | `SurfaceActor`(valid/invalid 색 = 각 재질 `Param`) |
| FPV-005 벽·설비 발밑 가림 | 표시 depth test는 재질(Editor) 값, collision·shadow 없음 | `SurfaceActor`(NoCollision, 그림자 없음). 화면 판정은 PIE |
| FPV-006~008 이동·숨김·파괴 | 같은 Actor component라 D4 숨김·파괴가 공유됨 | `SurfaceActor`(`SetActorHiddenInGame` 뒤 `ShouldRender` 거짓), 기존 `PreviewHiddenWithoutAim` 통과 |
| FPV-009~012 설비 종류·공간 | 공통 경로, 설비별 코드 없음 | content 테스트 `PlacementContent.FootprintPreview`(작성·컴파일만, Editor 단계 실행) |
| FPV-013 판정 불변 | 판정 경로(`ValidateCurrentPlacement`·`ValidateWorldPlacement`) 무변경 | 기존 `Placement.*` 전부 통과 |
| FPV-014 snap 정렬 | `ValidateFootprintGridAxisAlignment`(Data Validation 전용) | `FootprintPreview.AxisAlignment` |
| FPV-015 표시 준비 실패 | `TryCreateFootprintSurface`: 실패 시 정리, Warning 한 번, 초기화 성공 유지 | `FootprintPreview.PreparationFailureFallback` |
| FPV-016 쿨러·순환기·보일러 | `ComputeScaledFootprintFullSize`(root scale 반영), 비정수·축 정렬 검사. 쿨러 extent·SceneRoot 회전은 Editor 단계 | `FootprintPreview.CellsIncludeRootScale`, `AxisAlignment` |

## 변경 파일과 구현 요약

- `Public/Placement/FacilityPlacementSettings.h`: 표시 설정 5개(`FootprintPreviewMesh`, `FootprintPreviewMaterial`, `FootprintPreviewFloorOffsetCm`, 두 우선순위)와 getter·loader. `Config/DefaultGame.ini`에 `FootprintPreviewMaterial`을 뺀 4개 키(plane mesh, 높이 = grid 표시 높이 + 0.5, footprint = grid 우선순위 + 1, 메시 = footprint + 1).
- `Public/Placement/FacilityPlacementComponent.h`, `Private/Placement/FacilityPlacementGeometry.cpp`: static `ComputeScaledFootprintFullSize`, `DeriveFootprintCells`가 `GetFootprintRelativeToRoot()` 합성 scale × root relative scale 사용(component-to-world 제거), `ValidateFootprintGridAxisAlignment`.
- `Private/Placement/FacilityPlacementDefinition.cpp`: `IsDataValid`에 축 정렬 오류 추가. `ValidateRuntime`·`DeriveFootprintCells`에는 넣지 않음.
- 신규 `Private/Placement/FacilityPlacementFootprintPreview.h/.cpp`: surface 순수 계산, plane bounds 검사(Zone grid `QueryGridGeometry`와 같은 규칙, grid 코드는 무변경), 재질 계약 검사, 색 읽기, 계약 parameter 이름 4개.
- `Public/Placement/FacilityPlacementPreviewActor.h`, `Private/Placement/FacilityPlacementPreviewActor.cpp`: `FootprintSurface`·`FootprintMaterial` transient UPROPERTY, 색 두 개, 메시 sort priority 적용, `SetPlacementValidity`에서 색 갱신, 테스트용 const getter 둘.
- 테스트: `Private/Tests/FacilityPlacementFootprintPreviewAutomationTests.cpp`, `...TestProbe.h/.cpp`(신규). 기대값은 fixture·Settings에서 계산하며 리터럴을 복제하지 않는다. 전역 Settings와 CDO extent는 scope guard로 복원한다.

## 클래스 크기·책임 변화

| 파일 | 전 → 후(줄) | 판단 |
|---|---|---|
| `FacilityPlacementPreviewActor.cpp/.h` | 176 / 48 → 285 / 64 | 400줄 경고선 아래. 표시 component·DMI 조립과 validity 색만. 계산·검사는 helper |
| `FacilityPlacementGeometry.cpp` | 191 → 241 | 기존 CDO 파생 책임. 경고선 아래 |
| `FacilityPlacementSettings.h` | 66 → 110 | 설정 5개와 getter |
| `FacilityPlacementComponent.h` | 124 → 127 | 선언 둘 추가, UPROPERTY 변경 없음 |
| `FacilityPlacementFootprintPreview.cpp/.h`(신규) | 0 → 106 / 41 | 순수 계산 helper, 독립 상태 없음 |

`UPlayerFacilityPlacementComponent`(430줄), `FacilityActorConversionTransaction`, Zone grid, 설비 Actor class는 변경하지 않았다. 새 Tick·timer·delegate·transaction 없음.

## Blueprint·API·Core Redirect 영향

- reflected 변경: Settings Config UPROPERTY 5개 추가, preview Actor `Transient` UPROPERTY 2개 추가. rename·삭제 없음 → Core Redirect 불필요. Content 변경 없음.
- Public API 추가: `UFacilityPlacementComponent::ComputeScaledFootprintFullSize`(static), `ValidateFootprintGridAxisAlignment`. 기존 시그니처 불변.
- 동작 영향(설계 7절 확인): `BP_Cooler`는 Editor 단계 전까지 CDO footprint가 비정수라 배치 초기화가 실패한다. 아래 자동화에서 `Shop.*` 3개가 이 이유로 실패한다(활성 Definition 하나가 `설비 배치 영역이 정의의 전역 그리드 셀 크기와 일치하지 않습니다`). 순환기·보일러는 runtime 동일, Data Validation만 오류다. Editor 단계(`PROMPT_UNREAL.md` 4·5항)가 해소한다.

## 빌드와 정적 검증

- 빌드 전 Unreal Editor 프로세스 없음 확인. `BathhouseSimEditor Win64 Development` 성공(`Result: Succeeded`), 로그 `Saved/Automation/Logs/fpv_r1_build.log`(재작업 1회차 빌드). 최초 구현 빌드 로그는 남아 있지 않다.
- 빌드 시점 Source 식별값: HEAD `0c5f8a083e314158e93a7b8cc119ca4313d743e5`, `git diff HEAD -- Source Config` SHA-256 `e23e902290f6746f9b8ca4b0d9915bb75d740980e56839354a8b9454719052b6`. 신규 untracked 파일 5개는 diff에 없어 별도로 정렬 sha256sum의 SHA-256 `7f620b90e290d946cd5fdf394381b0f5e9b3df65ae26fc35dd44ee970c5fa7ad`다(`git ls-files --others --exclude-standard Source Config`).
- `git diff --check -- Source Config`: 공백 오류 없음(CRLF 변환 안내만).
- 조정값 code 상수 검색: 새 숫자 상수는 허용 오차(plane 중심·두께, 축 정렬 0.01도), `2 *`, plane Z scale 1, identity 회전, Settings 예비값뿐이다.

## 자동화 결과

- `BathhouseSim.Placement.`(12개): 전부 통과. 새 5개(`FootprintPreview.SurfaceMath`, `CellsIncludeRootScale`, `AxisAlignment`, `PreparationFailureFallback`, `SurfaceActor`)와 기존 7개(`PreviewHiddenWithoutAim`, `ActorReplacementTransaction` 등). 로그 `Saved/Automation/Logs/fpv_r1_placement_editor.log`, 리포트 `Saved/Automation/Reports/20261002/fpv_r1_placement/index.json`(재작업 1회차 재실행, 12/12 통과). 최초 실행 근거는 `Saved/Automation/Reports/20261002/fpv_placement/index.json`.
- T6(`SurfaceActor`) 수행했다. transient `UMaterial`에 vector·scalar parameter expression을 넣고 `UpdateCachedExpressionData()` 뒤 parameter 조회가 안정적으로 동작했다. 조회 불가 때는 조용히 통과하지 않고 오류로 실패하게 두었다.
- 전체 `BathhouseSim` 184개 중 실패 4개, 로그 `Saved/Automation/Logs/fpv_full_editor.log`:
  - `PlacementContent.FootprintPreview`: `FootprintPreviewMaterial`이 아직 비어 있어 예상된 실패. Editor 단계 항목 3 이후 통과해야 한다.
  - `Shop.UnboxViewFront.RoomPhysics`, `Shop.SevenDefinitionSpawn`, `Shop.UnboxingPhysics`: 모두 위의 쿨러 비정수 cell 오류(Definition 5). 이 작업 이전에는 root scale이 CDO 파생에 빠져 통과하던 것이다. `BP_Cooler` 수정 뒤 재실행해 통과를 확인해야 한다. 이 회귀 판정은 Editor 단계가 한다.
- 기준선(코드 리뷰 1회차 측정): 작업 전 전체 177개 실패 0(`Saved/Automation/Reports/20261002/u3_editor3`). 184개와 비교해 추가 7개(이번 작업 6개, 지도 버그 1개), Success→Fail은 위 Shop 3개뿐이고 다른 회귀는 없다. 실제 근거는 `fpv_full/index.json`과 `fpv_full_editor.log`다.

## 리뷰 중점·미검증

- `DeriveFootprintCells`가 root scale을 새로 반영하면서 CDO 경로 호출자(Definition `ValidateRuntime`, preview 초기화, `ValidateWorldPlacement`의 `ValidateFootprintContractForDefinition`) 결과가 쿨러에서 바뀐다. 다른 15종은 변하지 않는다는 설계 가정은 content 테스트(Editor 단계)로 확인된다.
- 표시 준비 실패는 `UE_LOG(LogTemp, Warning)` 한 번이다. 기존 코드에 전용 로그 category가 없어 `LogTemp`를 썼다.
- footprint 표시가 판정 경로를 호출하지 않는지, 같은 static 함수를 cell 파생과 표시 크기가 함께 쓰는지(`ComputeSurface` 내부) 확인해 달라.
- 미검증: 화면 가림·비침·깜빡임·sort 순서(사용자 PIE, FBK-003). `PROMPT_IMPLEMENTATION.md` 7절의 "Level 쿨러 instance가 이미 `IsOperational`에서 실패할 수 있다"는 추정은 확인하지 못했다. PIE FPV-016에서 본다.
- 미검증: content 테스트(T7) 실행. 재질 MI가 생기기 전이라 실행 불가.
- Architecture 정본은 아키텍처 단계가 이미 갱신했고 구현이 구조를 바꾸지 않아 추가 변경하지 않았다.

## 재작업 1회차 (코드 리뷰 F1, F2, F4)

- F1: `PROMPT_UNREAL.md` 7절에 Shop 3개 재실행, 전체 `BathhouseSim` 실패 0 확인, 실패 시 보고·중단을 추가했다.
- F2: test probe `AFacilityPlacementFootprintHalfRootScaleProbe`(root uniform scale 0.5, `SceneRoot` Yaw −90°)를 추가하고 `CellsIncludeRootScale`의 두 번째 블록을 교체했다. extent는 `GridSizeCm`로 계산하고, unscaled가 정수 칸이며 합성 scale 적용 뒤 비정수임을 assert한다. 옛 component-to-world(CDO scale 1) 방식이면 정수로 통과했을 fixture다. 주석도 맞췄다. production Source·Config는 변경 없다.
- F4: 위 로그·리포트 경로를 실제 파일로 정정했다.
- 재검증: 빌드 성공(`Saved/Automation/Logs/fpv_r1_build.log`), `BathhouseSim.Placement.` 12/12 통과. 식별값: HEAD `9d552c8761a80b44f8311ae22fbb48df88fe50a4`, `git diff HEAD -- Source Config` SHA-256 `00c6aef22543a93a30ddd0699040690d9c074c0f5a2fd4e1484f650a8863db3e`(테스트 3개 파일만 변경), 신규 untracked Source 없음. 이전 식별값 `e23e9022…`·`7f620b90…`은 `fd7f87d`에 해당한다.
