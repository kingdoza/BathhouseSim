# Unreal MCP 단계 보고 — 서비스 1단위 수직 (2026-09-30)

## 판정

**부분 완료; 통합 승인 보류.** asset 생성·설정·저장·새 프로세스 재로드, Config 반영, `HeldSummaryText`(Python API), Data Validation 10/10 VALID까지 끝났다. DefaultMap 수거함 배치·저장(사용자 직접)과 화면·입력 검증이 `USER_UNREAL.md`에 남아 있다.

## 연결과 세션

- Editor가 없어 작업용 백그라운드 UE 5.8 Editor(PID 10552)를 실행해 `127.0.0.1:8000`에 MCP `initialize`(protocol 2025-06-18) → `tools/list`(메타 도구 3개) → `list_toolsets` → 읽기 조회로 연결했다. 현재 대화에 Unreal 도구가 노출되지 않아 문서 3절대로 직접 MCP 호출을 썼다.
- 저장 후 PID 10552를 정상 창 종료(성공)하고 새 Editor(PID 24820)로 재로드 검증했다. 새 Editor는 검증 후 정리한다.
- Config 세 항목(ItemBoxClass, InsertPreviewMaterial, r.CustomDepth=3)은 사용자 승인 후 ini에 직접 추가했다. Computer Use, Python reflection, binary 편집은 쓰지 않았다.

## 변경 asset (모두 개별 Save, 재로드 값 일치, dirty=false)

신규: `DA_ServiceItem_BananaMilk`, `BP_ItemBox`, `BP_DrinkFridge`, `BP_DrinkCollectionBox`, `DA_FacilityPlacement_DrinkFridge`, `M_PP_TakeHighlightOutline`, `MI_DisplayInsertPreview`.
수정: `DA_ShopCatalog`(상품 2개 추가), `BP_FirstPersonCharacter`(`FirstPersonCamera` weighted blendable 1개만 추가; 다른 후처리 필드 diff 없음). 경로와 값은 `.md/Unreal/ServiceSystem.md`.

## 검증

- Compile: 세 BP `warnings_as_errors` 성공(저장 전, 재로드 후 각각). 저장 시 AssetCheck는 모든 저장 asset에서 오류 0, 경고는 Definition의 RecoveryItemMesh 미설정 하나.
- `BP_DrinkFridge` SCS readback: 공간 4개 `SpaceIndex` 0~3, `AcceptedCategory=Display.Fridge`, 공간별 `SlotTransforms` 6, slot 1.
- PIE(기본 5초)를 한 번 시작·종료했다. 그 시간 동안 관련 Error/Warning 로그는 없었다. 입력 시나리오는 실행하지 않았다.
- Data Validation(`EditorValidatorSubsystem`, 새 프로세스): DA·BP 3·Definition·Material 2·카탈로그·캐릭터 BP·WBP 10개 전부 VALID. 오류 0, 경고는 Definition의 RecoveryItemMesh 미설정 하나.
- 미검증: 외곽선·프리뷰 화면, 박스 뷰포트 미리보기, PIE 절차 1~9, 자동화 재실행(`Automation RunTests BathhouseSim`).

## 실패·차단

- `SceneTools.save_actor`(수거함 external actor): `Asset does not exist: /Game/__ExternalActors__/Maps/DefaultMap/1/P1/UWXIDD9LM1ZURKQSERVEKZ`. 과거 기록과 동일해 재시도하지 않았다. 임시 actor는 제거했고 DefaultMap은 clean이다.
- `ShopSettings`/`ServiceDisplaySettings` CDO는 메모리에서만 바뀌고 ini에 쓰이지 않았다. Config 변경 0.
- `BL_BeforeTonemapping`은 엔진이 `BL_SceneColorAfterDOF`(에디터 표기 Before Tonemapping)로 되돌린다.

## dirty·미완료

- 종료 전 dirty package 없음. 예상 밖 변경 없음(`git status`의 Content 변경은 위 asset뿐, Config·umap 변경 없음).
- `USER_UNREAL.md` 상단 "서비스 1단위 수직" 6개 항목.

## Python API 작업 — `WBP_InteractionPrompt.HeldSummaryText`

- 사용자 승인: 이 작업에 한해 `AGENT_UNREAL_PYTHON.md` 절차 사용을 승인받았다(대화에서 "위젯작업 진행해").
- 백업: `Saved/MigrationBackup/20260930_held_summary_text/`(SHA-256 `41827D52…F337`, 수정 전 디스크 해시와 일치 확인 후 진행). 스크립트: `Saved/Claude/HeldSummary/held_01~06_*.py`(01·02 조회, 03 저작, 04 GUID 재저장, 05 검증, 06 Data Validation).
- 결과: `PromptRoot` Overlay에 TextBlock `HeldSummaryText` 추가 → Compile `BS_UP_TO_DATE` → 개별 Save. 첫 프로세스의 GUID ensure는 문서대로 새 프로세스에서 재Compile·강제 저장해 해소, `Success - 0 error(s)`. 재로드 검증 `VERIFY_RESULT PASS 0`(기존 필수 BindWidget 15개 전부 도달·타입 일치, 신규 위젯 parent=PromptRoot).
- 저장한 package는 `WBP_InteractionPrompt` 하나. 종료 시 dirty package 없음.
- 발견: 저장된 트리에는 `HeldTake*`, `PrimaryKeyText`, `LmbKeyText`, `RmbKeyText`가 없다(범위 밖, `USER_UNREAL.md` 3번). 표시 위치(위쪽 여백 600)와 실제 화면은 미검증.

## 외곽선 머티리얼 수정 (Python API)

- 증상: 사용자 PIE에서 화면 전체가 노랗게 덮임(`OutlineColor`가 전 화면에 적용). 원인 추정: 후처리 Translucent + Opacity 구조에서 Opacity가 반영되지 않음. 재현은 하지 못했다(사용자 Editor가 열려 있어 MCP 접속 불가, 헤드리스는 렌더링 불가). 스텐실 읽기(`.r`)는 엔진 셰이더 소스(`MaterialTemplate.ush`/`SceneTexturesCommon.ush`)로 확인해 문제가 아님.
- 조치: 사용자 승인("원인수정까지 진행해")과 Editor 종료 후 `M_PP_TakeHighlightOutline`을 Opaque + `Lerp(SceneColor, OutlineColor, Mask)`로 재구성해 개별 Save. 백업 `Saved/MigrationBackup/20260930_outline_fix/`(SHA-256 `1BFA3CCD…6226`), 스크립트 `Saved/Claude/Outline/outline_01_rebuild.py`·`outline_02_verify.py`.
- 검증: 새 프로세스에서 domain/blend/location, Emissive=Lerp, Opacity 미연결, 파라미터 4개, Data Validation VALID, 셰이더 오류 로그 없음. **화면 결과는 미확인** — 사용자 PIE 필요. 여전히 전체가 덮이면 마스크가 1로 나오는 것이므로 Custom 노드 출력을 디버그 색으로 내보내 원인을 좁힌다.
