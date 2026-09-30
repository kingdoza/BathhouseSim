# Unreal MCP/Python 단계 보고 — 서비스 3단위: 쓰레기·수거 (2026-10-01)

## 판정

**부분 완료; 통합 승인 보류.** allowlist Blueprint 9개·WBP·DefaultMap external actor의 authoring·Compile·Data Validation·개별 Save와 새 프로세스 재로드 대조(MCP 50개 항목 + Python 검증)까지 끝났다. `PROMPT_UNREAL.md`의 대표 PIE 관찰은 MCP로 입력·console을 실행할 수 없어 수행하지 못했고 `USER_UNREAL.md`로 넘겼다.
`PROMPT_REVIEW.md`는 코드 재리뷰 요청 상태로 승인 기록이 없었다. 사용자가 이번 대화에서 직접 진행을 지시했고 "MCP로 안 되는 작업은 Python API로 진행"을 허가해 그 지시를 근거로 실행했다.

## 연결과 세션

- Editor를 `-ModelContextProtocolStartServer`로 실행해 MCP에 연결했다(포트 약 20초). 포트를 제한 시간(90초)으로 감시했고 초과 시 로그를 읽도록 했다. Python 스크립트는 Editor 안(`-ExecCmds="py …"`, 맵 로드 후 실행·자동 종료)과 commandlet(`-run=pythonscript`)으로 실행했고 각각 제한 시간 내에 끝났다.
- 종료 요청이 저장 확인창에서 막힌 경우 작업 소유 PID만 종료했다(저장 필요 변경 없음). Computer Use·binary 편집은 쓰지 않았다.

## 변경 asset/package (개별 Save, 재로드 값 일치)

- 신규 Blueprint 5개: `BP_Litter`, `BP_LitterSpawnZone`, `BP_LitterTongs`, `BP_TrashBag`, `BP_TrashCollectionZone`(자식 `ZoneMarker` 포함).
- 수정 Blueprint 3개: `BP_CleaningDirector`(`LitterClass` 등), `BP_WaterStain`(`FloorRadiusCm` 30→37.5), `BP_StainSpawnZone`(삭제 property 정리 재저장).
- `WBP_InteractionPrompt`(Python): `RmbKeyText`·`HeldTakeActionNameText`·`HeldTakeFailureReasonText` 추가. 기존 필수 BindWidget 15개·`HeldSummaryText` 유지.
- DefaultMap external actor(Python `save_packages`): 기존 director·stain zone 2개 재저장(stain zone `SpawnFloor` world Z -100→0), 신규 쓰레기 구역·집게·거치대·수거 구역 4개, 기존 `BP_TrashBin` instance 제거(파일 삭제). 정확한 package·위치·값은 `.md/Unreal/CleaningSystem.md`.
- 저장하지 않은 것: `DefaultMap.umap`, Config, `BP_ItemBox`·`BP_ShopDeliveryBox`·`BP_PlaceableFacilityItem`·`BP_TrashBin`, allowlist 밖 package. `git status`에서 Content 변경은 위 항목뿐이다.

## 기록한 변경 전 값 (보존 확인)

director class `SpawnIntervalSeconds=15`·`MaxActiveStains=8`·`StainClass`·`StainSpacing 100`; 레벨 director instance도 `SpawnIntervalSeconds=15`(override 유지, `LitterClass`는 BP 상속). `BP_WaterStain` scale 범위 `(0.7..1.3, 0.8..1.2)`·yaw ±180·청소 2초·material 1개. stain zone 최대 4·경사 10·trace 300·tag 없음. 원본 백업: `Saved/MigrationBackup/20261001_service_unit3/`(git HEAD 기준 SHA 기록).

## 검증

- Compile: 8개 Blueprint `warnings_as_errors` 성공(저장 전·재로드 후). Data Validation(Python, 새 프로세스): BP 8개·WBP 모두 VALID, 오류 0.
- 새 프로세스 readback(50개): parent 8개, 각 BP의 component·값·충돌·Navigation, `Litter` 변형 3개, 집게 `TiedBagClass`, 봉투 scale/held pose, 수거 구역 300초·marker, director·stain 값과 삭제된 property(`DefaultPawnClearance`, `SelectionWeight`, `ZoneKind`, `PawnClearanceOverride`) 부재, DefaultMap의 쓰레기통 제거·stain zone floor world 0·쓰레기 구역 floor world 0·집게/거치대 `AssignedItem` exact·`bStartOccupied`·동일 pose·수거 구역 위치, director instance 상속, `DefaultMap` dirty 없음, `.umap` 변경 없음.
- WBP(Python): 새 프로세스 재로드 `VERIFY_RESULT PASS 0`, Data Validation VALID. 첫 저작 프로세스의 GUID ensure는 문서 절차대로 새 프로세스 재Compile·강제 저장으로 해소(`Success - 0 error(s)`).
- 바닥 조사: `Studio_floor`가 Mobility Static·`ECC_WorldStatic`, 14개 지점 trace 모두 상단 Z=0.
- PIE 10초 실행 동안 Error/Warning 로그 없음(입력 시나리오는 미실행).
- 실수와 수정: 수거 구역 `ZoneMarker`가 충돌 값만 바꿔 로드 때 `BlockAllDynamic` 프로파일로 되돌아갔고 재로드 대조에서 발견해 프로파일 이름까지 지정해 재저장했다.

## 조정·주의

- 위치는 실제 맵(탈의 구역 x≈900~1150, 손님 스폰 `(700,-600)`, 출입구 `(-500,300)`)을 읽고 정한 제안값이다. 쓰레기 구역은 탈의 구역 하나(300×300cm)만 덮는다. 욕탕 바닥(Z=0 동일)·다른 체류 영역의 별도 구역은 배치하지 않았다.
- `BP_WaterStain.FloorRadiusCm=37.5`는 현재 시각(Plane 100cm×0.75)에 맞춘 값이다. 조준용 `InteractionCollision`(반경 45)은 유지했다.

## 미완료

`USER_UNREAL.md` 상단 "서비스 3단위" 항목(대표 PIE 전체, 임시 도형·배치 위치 화면 확인).

## 코드 재작업 후보 — 집게 RMB가 대상의 빼기 강조를 켬 (사용자 PIE 관찰, 미수정)

- **재현(사용자 확인)**: 쓰레기 집게를 들고 봉투 개수 ≥ 1인 상태에서 수건 기계·선반·사용 수건통 또는 **냉장고**(SelfAim 공간)를 조준하면 해당 대상의 빼기 강조(수건은 뚜껑 열림 포함)가 보인다. 샤워기·화장대(FacilityRouted)는 재현되지 않는다. 수건은 바구니 없이도 같은 증상으로 보였다고 보고됐고 같은 원인으로 추정한다.
- **원인(소스 확인)**: `PlayerEquipmentUseComponent.cpp:75-84`가 집게의 보조 사용(`IHeldEquipmentSecondaryUsable`, 봉투 묶기)을 합쳐진 `Query.bHeldTakeVisible/bCanHeldTake`에 덮어쓴다. 이 합쳐진 값을 `TowelDisplayCueUtils::Update`(빼기 강조·`SetSourceInsertable` 뚜껑)와 `UDisplaySpaceComponent::NotifyInteractionFocusChanged`(`DisplaySpaceComponent.cpp:444`)가 들고 있는 물건의 종류를 확인하지 않고 그대로 읽어 대상의 빼기 가능으로 오해한다. 넣기 프리뷰는 `AItemBoxActor`인지 확인하지만 빼기 강조에는 같은 확인이 없다. FacilityRouted는 `UDisplayFacilityTargetComponent::BuildQuery`가 손에 든 것이 품목 박스일 때만 값을 채워 영향을 받지 않는다.
- **결과**: 화면은 "빼기 가능"인데 RMB는 집게의 봉투 묶기를 실행한다(거짓 안내). 수건 바구니·품목 박스를 들었을 때의 동작은 영향 없음.
- **소유**: C++ 구현 단계(대상 쪽 빼기 강조·뚜껑 조건을 "들고 있는 물건이 그 대상의 이동 도구일 때만"으로 한정). Editor·Blueprint로 우회하지 않는다. 기능 명세(TRSH-010~012, 집게 RMB는 빼기 대상 동작이 아님)는 변경 없음.
