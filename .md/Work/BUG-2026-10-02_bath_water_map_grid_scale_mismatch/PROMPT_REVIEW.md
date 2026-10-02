# PROMPT_REVIEW — 욕탕 관리 지도 그리드 선 누락·욕탕 타일 크기

- 작업 ID: `BUG-2026-10-02_bath_water_map_grid_scale_mismatch`
- 단계: 구현
- 상태: 완료

## 1. 기능 계약과 단계

- 계약: [PROMPT_IMPLEMENTATION.md](PROMPT_IMPLEMENTATION.md)(정본) 3~5·8~9절. 버그 리포트 `.md/BugReports/2026-10-02_bath_water_map_grid_scale_mismatch.md`. 단순 버그 수정.
- 구현 범위: 그리드 선(MAPG-001·003)과 조정값 원본 이전. 타일 크기(MAPG-002)는 Editor 계약이라 C++ 변경 없음(`PROMPT_UNREAL.md`).

## 2. 시나리오별 코드·테스트 연결

| ID | 구현 경로 | 테스트 |
|---|---|---|
| MAPG-001 | `BuildBathWaterMapGridLines`(render px 두께 → 레이아웃 두께 역산, 선 중심 정렬, 경계 겹침 선 생략) | `BathhouseSim.BathWater.Operations.MapGridLineLayout`: 배율 <1(비정수·비균일)/1/>1, 입력 두께 <1 네 경우의 render 두께, 시작 소수부 0.01 간격 가시성, 옛 규칙(레이아웃 1px) 소실 재현, 선 개수·중심 좌표·content rect, 경계선 4개 |
| MAPG-003 | `RebuildGrid` key에 `LastGridRenderScale` 추가(배율·Zone·canvas 변화 시 재생성), `ClearGrid` 초기화 | 위 테스트의 배율 변화 경우. 넓힘 뒤 실제 표시는 사용자 PIE |
| MAPG-002 보조 | 타일 상태색·opacity를 코드 상수에서 `UBathWaterBathTileWidget` 프로퍼티로 이전 | 같은 테스트의 타일 부분: 색·opacity를 같은 객체 프로퍼티와 비교, 우선순위 부족 > 선택 > 기본 |

기존 `MapProjection`·`NativeWidgetPresentation` 테스트는 수정 없이 통과(letterbox helper 공유 후).

## 3. 변경 파일과 요약

- `Public/UI/BathWaterMapWidget.h`: `GridLineThicknessPx`·`BoundaryLineThicknessPx`(ClampMin=1)·`GridLineColor`·`BoundaryLineColor` EditDefaultsOnly 추가, `LastGridRenderScale`, 테스트 friend.
- `Private/UI/BathWaterMapWidget.cpp`: `RebuildGrid`는 key 비교(+배율) → helper 호출 → `UBorder` 생성·색·slot 적용만. `ProjectFootprint`는 `ComputeBathWaterMapLetterbox` 공유(시그니처·결과 불변).
- 신규 `Private/UI/BathWaterMapGridLayout.h/.cpp`: 순수 helper 4개(letterbox, 배율 읽기, 두께 변환, 선 목록).
- `Public/UI/BathWaterBathTileWidget.h`, `Private/UI/BathWaterBathTileWidget.cpp`: `DeficitTileColor`·`SelectedTileColor`·`NormalTileColor`·`SelectedRenderOpacity`·`UnselectedRenderOpacity` 추가, 분기·우선순위 불변.
- 신규 `Private/Tests/BathWaterMapGridAutomationTests.cpp`.
- C++ 초기값은 이전 코드 상수와 같다(화면 변화 없음). 원본은 WBP Class Defaults. 문서·테스트는 수치를 복제하지 않는다(테스트는 CDO 값에서 읽음).

## 4. 클래스 크기·책임

- `BathWaterMapWidget.cpp` 350줄 → 343줄(선 계산이 helper로 빠졌고 배율 key가 더해짐). 헤더 프로퍼티 4개·멤버 1개 증가. 선 배치 책임은 helper로 분리. 타일 위젯은 프로퍼티 5개 증가, 로직 증가 없음.

## 5. Blueprint/API/Redirect 영향

- UPROPERTY 추가만(rename·삭제 없음). Core Redirect 불필요. 새 프로퍼티는 `EditDefaultsOnly`, `BindWidget` 계약 불변. Build.cs 변경 없음(SlateCore 기존 의존, inline 접근).
- `WBP_BathWaterManagementScreen` 중첩 `BathMap`에 새 프로퍼티 override는 없어 Content 저장 불필요.

## 6. 빌드와 검증

- UE 5.8 `BathhouseSimEditor Win64 Development` 빌드 성공(첫 시도는 C4458 `Slot` 이름 가림으로 실패 → `LineSlot`으로 수정 후 성공). 사용자 Editor 미실행 확인.
- Source 식별값: HEAD `5f39fd8`, `git diff HEAD -- Source Config` SHA-256 `4280D37CD78898070E96ED001D11108DF8943FB77F22629338D481359C9573B7`(신규 untracked 파일은 diff에 포함되지 않음, 파일 목록은 3절).
- 빌드 로그: `C:\Users\kdowo\AppData\Local\Temp\claude\C--UnrealProjects-BathhouseSim\0b7fbddd-9732-49b4-842d-c5b5adbfb049\scratchpad\build.log`. 실행 로그: `Saved/Logs/BathhouseSim.log`.
- 자동화: `Automation RunTests BathhouseSim.BathWater.Operations` 전체 Success(MapGridLineLayout 포함, 8개 모두 Success 확인: CapacityDemandAndRequests, FacilityTransactionAtomicity, FlowConditionAndBathers, MapGridLineLayout, MapProjection, NativeWidgetPresentation, PayloadAndUIContracts, RequestAtomicityAndRevision).
- `git diff --check -- Source` 공백 오류 없음(CRLF 경고만). `Content/` 변경은 사용자 소유 `BP_ClothesLocker.uasset`뿐.

## 7. 리뷰 중점과 미검증

- 확인점: `RebuildGrid`의 key 비교(+`ClearGrid` 초기화), 코드 리터럴 잔존 여부(예외는 PROMPT_IMPLEMENTATION 4.3 목록), `ProjectFootprint`와 그리드의 letterbox 공유, Slate draw API 미도입.
- `FSlateRenderTransform::TransformVector(FVector2D)` 사용: 5.8에서 컴파일됨. 단위 벡터 길이로 배율을 읽는다(회전 포함 transform이어도 길이 유지).
- 미검증(사용자 PIE·Editor): 실제 컴퓨터 화면 배율에서 선 누락 해소, WBP 컴파일에서 새 프로퍼티 노출, 타일 크기(Editor 작업 뒤), 넓힘 뒤 지도.
- 전역 영향: polling마다 `GetPaintSpaceGeometry` 1회와 vector 비교 추가. 전역 설정 변경 없음.
