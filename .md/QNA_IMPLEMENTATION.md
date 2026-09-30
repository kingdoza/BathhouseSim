# 구현 질문 — 서비스 2단위 DISP-024와 설비 router 선택 계약 충돌

## 상태

해결됨(아래 2026-09-30 아키텍처 답변). 정정된 구현 프롬프트에 따라 기존 Source 초안을 유지하고 2단위 구현을 재개했다. 아래 질문·중단 기록과 아키텍처 답변은 보존한다.

## 승인된 사용자 결과

`.md/PROMPT_ARCHITECTURE.md` DISP-024: 빈 박스로 화장대 드라이기 쪽에서 RMB 연속 빼기 중, 버튼을 누른 채 빗 쪽으로 조준 이동하면 멈춘다. 빗은 빠지지 않으며 버튼을 떼고 다시 누르면 빗부터 뺀다.

## 현재 설계와 실제 실행 순서

`.md/Architecture/ServiceFacilityDisplaySystem.md` Facility Target Router:

- 비지 않은 박스는 `FixedKind == 박스 종류`인 묶음을 선택한다.
- 빈 박스는 조준선 최근접 묶음을 선택한다.
- query·execute는 같은 선택 함수를 사용한다.
- `HeldUseTargetKey`는 선택된 묶음의 SpaceIndex다.

실행 순서:

1. 빈 박스로 드라이기 묶음 조준 → 최근접 드라이기 묶음 선택.
2. 첫 RMB 실행 성공 → 박스에 드라이기 1개가 생긴다.
3. 이후 query부터 박스가 비지 않았으므로 드라이기 묶음이 고정 선택된다.
4. 빗 쪽으로 조준을 옮겨도 선택은 드라이기, key도 드라이기다.
5. 현재 설계의 key 비교는 변화를 감지하지 못한다. 연속 조작이 멈추지 않으며, 버튼을 떼고 다시 눌러도 드라이기 박스이므로 빗을 선택할 수 없다.

따라서 중단뿐 아니라 `떼고 다시 누르면 빗부터`라는 후속 결과도 박스의 단일 품목 규칙 및 고정 품목 선택과 양립하지 않는다. 이 충돌은 코드 수정만으로 승인 범위 안에서 해소할 수 없다.

## 필요한 확정

기능 명세 단계에서 DISP-024의 박스 종류·내용물 변화까지 포함한 관찰 가능한 결과를 확정한 뒤, 아키텍처 단계에서 연속 조작과 재입력의 묶음 선택 및 key 계약을 일치시켜야 한다. 단일 품목 박스 규칙, 비지 않은 박스 종류에 따른 선택, DISP-024 결과 중 구현자가 임의로 바꾸는 항목은 없다.

구현 단계는 수정된 기능 계약 및 일치하는 `.md/PROMPT_IMPLEMENTATION.md`와 정본을 받은 뒤 재개한다. 기존 Architecture/Unreal 정본과 Content/Config는 변경하지 않았다.

## 이미 수행한 작업

- 시작 시 Source/Content/Config clean과 UnrealEditor 종료 확인.
- 지정된 기존 asset 5개를 `Saved/MigrationBackup/20260930_service_unit2/`에 복사.
- 공용 extension·manager·cue, FixedKind·소모 상태, router, held key, 수건 이동·표현·뚜껑의 C++ 초안 작성.
- 기존 냉장고 테스트의 payload 접근을 새 extension 구성에 맞게 조정.
- Blueprint load gate의 대상 추가. 아직 실행하지 않음.
- 신규 2단위 자동화 테스트, 전체 회귀, copy-first load gate, 최종 코드 리뷰 인계는 미완료.

## 별도 경로 대조 결과

설계 Editor Authoring 표의 `/Game/Bathhouse/Blueprints/Facility/BP_Washer`·`BP_Dryer` 경로는 기존 Unreal 정본 및 실제 파일과 다르다. 실제 경로는 `/Game/Bathhouse/Blueprints/Towel/BP_Washer`·`BP_Dryer`이며, 백업과 load gate 대상에는 실제 경로를 사용했다. 설계 정본은 수정하지 않았다.

## 아키텍처 답변 (2026-09-30)

- 기능 명세가 DISP-024를 정정했다: 빈 박스로 첫 1개를 빼면 박스 종류가 정해지고, 이후 조준 이동과 무관하게 그 품목 묶음에서만 계속 빠진다. 조준이 설비를 벗어나거나 묶음이 비거나 박스가 차면 멈춘다. 떼고 다시 눌러 빗부터 빼는 결과는 삭제됐다.
- 보고한 실행 순서(첫 빼기 뒤 박스 종류 묶음 고정, key 불변)가 곧 정정 계약이다. router 선택 함수와 `HeldUseTargetKey`는 변경 없다. 별도 잠금 상태를 추가하지 않는다.
- key guard는 방어 규칙으로 유지한다. DISP-024 경로에서는 발동하지 않는다.
- 갱신: `Architecture/ServiceFacilityDisplaySystem.md` Facility Target Router·Held-Use Extension·Verification, `PROMPT_IMPLEMENTATION.md` 시작 조건(재개)·DISP-024 검증.
- 경로 지적 수용: `BP_Washer`·`BP_Dryer`는 `/Game/Bathhouse/Blueprints/Towel/`로 정본 수정 완료.
- 구현을 재개한다. 이 질의는 해결됨.
