# Unreal MCP/Python 단계 보고 — 서비스 4단위: 안마의자·평상·TV·세신 (2026-10-01)

## 판정

**부분 완료; 통합 승인 보류.** allowlist asset의 authoring·Compile·Data Validation·개별 Save와 새 프로세스 재로드 대조(MCP 83개 항목, Python Data Validation 17/17 VALID)까지 끝났다. 대표 PIE 절차(SVC4·REST·MASS·SCRB 전부)는 MCP·Python으로 입력·조준·마우스를 넣을 수 없어 수행하지 못했고 `USER_UNREAL.md`로 넘겼다.
`PROMPT_REVIEW.md`는 코드 리뷰 요청 상태로 승인 기록이 없었다. 사용자가 직접 진행을 지시했고 MCP·Python API 사용을 허가해 그 지시를 근거로 실행했다.

## 연결과 세션

- Editor를 `-ModelContextProtocolStartServer`로 실행해 MCP에 연결(포트 약 20초, 제한 시간 감시). 위젯 생성·레벨 저장·Data Validation은 Python API(commandlet·`-ExecCmds="py …"`)로 수행했고 모두 제한 시간 안에 끝났다.
- PIE를 연 뒤나 일부 시점에 종료 요청이 저장 확인창에서 막혀 작업 소유 PID만 종료했다(저장 필요 변경 없음 확인). Computer Use·binary 편집은 쓰지 않았다.

## 변경 asset/package

- 신규 Blueprint 6개: `BP_MassageChair`, `BP_RestBench`, `BP_Television`, `BP_ScrubTable`, `BP_ScrubTowel`, `BP_ServiceTestUser`. 신규 Definition 4개 `DA_FacilityPlacement_{MassageChair,RestBench,Television,ScrubTable}`.
- 신규 Widget: `WBP_ScrubFocusHud`(Python: 샘플 WBP 복제→`ScrubFocusHudWidget` reparent→트리 구성→새 프로세스 GUID 정리). 수정: `BP_BathhouseHUD`(`ScrubFocusHudWidgetClass`), `DA_ShopCatalog`(상품 4개, 총 20개).
- DefaultMap external actor 2개(Python `save_packages`): 때수건 `/Game/__ExternalActors__/Maps/DefaultMap/0/DK/0190JIWJICUQIWS8KMXOSL`, 전용 거치대 `…/E/3R/BVIBVCNJXTNW9C0FEGBWZL`(위치 `(300,900,43)`, `AssignedItem`=그 때수건, `bStartOccupied=true`).
- 변경 없음(상속·값 확인만): `BP_Shower`, `BP_MonkeyWrench`, `BP_FirstPersonCharacter`(`PlayerScrubFocus` 1개). 저장하지 않은 것: `DefaultMap.umap`, Config, 기존 external actor, 무관 asset. `git status` Content 변경은 위 항목뿐이다. 경로·값은 `.md/Unreal/ServiceSystem.md`, `WorldSystem.md`.

## 결정한 authoring 값

- **세신 영역(PIE 사용자 확정)**: 초기값(회전 0, extent `(90,35,1)`)에서 커서 때수건이 마우스와 90° 어긋나 움직였다. 사용자가 `ScrubArea`를 회전 Pitch 180·Yaw 90(패널 `(0,180,90)`), extent `(35,90,1)`로 바꿔 저장했고 새 프로세스 디스크 읽기로 확인했다. 이 회전은 로컬 Z가 아래를 향해 "+Z=표면 법선" 계약과 다르다(코드는 커서 높이에만 사용). 커서 이동 방향 판정은 사용자 PIE 확인.
- 세신 camera `(0,-170,240)` pitch -45 / yaw 90: FOV 90·16:9 계산에서 area 네 모서리·커서가 화면 안, 몸체에 가려지지 않음(실제 화면은 PIE 확인).
- `ScrubExitPoint (0,-120,0)`(몸체 앞면에서 80cm 앞, 발 Z=0). 인형 위치 `CashStandPoint`는 이탈점과 캡슐이 겹쳐 프롬프트 초기값 `(0,-180,0)`을 `(0,-200,0)`으로 조정. `CashOfferPoint (0,-160,110)` 유지.
- 감도: 기본 `RequiredRubDistanceCm 3000`·`RubCmPerInputUnit 1`을 변경하지 않았다. 실제 마우스 체감·완료 시간은 PIE 관찰 뒤 결정(미결정).
- 안마의자·평상·TV·세신대 body/footprint/slot 치수는 프롬프트 초기값 그대로(grid 20cm 정수배, native 검증 통과). 평상은 부모 기본 FacilityType `Bath`를 `RestBench`로 변경.
- 안마의자 `BrokenLabel`·TV `ScreenOnVisual`은 Event Graph(MCP `write_graph_dsl`)로 Visibility만 제어한다. 새 재질은 만들지 않았다(TV 화면은 기존 `MI_FacilityPreview_Valid` 재사용).

## 검증

- Compile: 설비 4개·때수건·테스트 인형·HUD `warnings_as_errors` 성공(저장 전·재로드 후). Data Validation(Python, 새 프로세스): 신규 asset과 `BP_Shower`·`BP_MonkeyWrench`·`BP_FirstPersonCharacter`·카탈로그·HUD·WBP 17개 모두 VALID(경고는 Definition 회수 mesh 미설정, 기존과 동일).
- 새 프로세스 readback 83개: parent·FacilityType·slot 수/이름(안마 1·평상 3·TV 0·세신 1), body/footprint/충돌·Navigation, 안마·TV·세신 값과 여섯 native component(중복 SCS 없음), Definition·상점 4개·기존 16개 유지, HUD class와 기존 widget class 유지, 때수건·거치대 `AssignedItem` exact·점유·pose, 레벨에 테스트 설비/인형 없음, map dirty 없음.
- PIE 10초 실행 동안 Error/Warning 로그 없음(입력 시나리오 미실행).
- SVC4-003: 디버그 명령 파일 전체가 `#if !UE_BUILD_SHIPPING`임을 소스에서 확인했다. Shipping 실행은 하지 않아 미검증이다.

## 미검증·미완료

대표 PIE 전체와 회귀 목록, 세신 감도·카메라·커서 화면 판정, 이탈점·현금 위치의 실제 충돌, 고장·TV 표현의 화면 구별, Shipping 실행. `USER_UNREAL.md` 상단 "서비스 4단위" 항목.
