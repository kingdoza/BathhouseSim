# QNA — 서비스 3단위(쓰레기·수거) 기술 설계

2026-09-30 작성. 입력은 `.md/PROMPT_ARCHITECTURE.md`(서비스 3단위)다. 각 질문의 `답변:` 뒤에 선택지를 하나 적어 주세요. 하나의 질문은 하나의 기술 결정만 다룹니다.

이 파일의 이전 내용(설비 배치 Authoring 단순화)은 Git 이력에 있다.

## 확인된 현재 구조

- 물 얼룩 생성은 레벨의 `ACleaningDirectorActor` 하나가 전역 타이머(`SpawnIntervalSeconds`, C++ 기본 15초)마다 한 번 실행한다. 이때 구역 제한이 남은 `AStainSpawnZoneActor` 중 하나를 `SelectionWeight` 가중치로 고른다.
- `AStainSpawnZoneActor::ZoneKind`(`EStainSpawnZoneKind` BathFloor/DressingFloor)는 저장만 되고 어떤 코드도 읽지 않는다.
- Content에서 이 계약을 참조하는 것: `BP_CleaningDirector`, `BP_StainSpawnZone`, `BP_TrashBin`, DefaultMap external actor 4개.
- 없어진 UPROPERTY는 tagged serialization이 로드 때 건너뛰므로, 삭제해도 asset 로드가 실패하지 않는다. resave하면 남은 값이 정리된다.

## 확정 전제 (설계 결정, 질문 아님)

- 물 얼룩 1명 기준 평균 간격은 기존 `SpawnIntervalSeconds` property를 그대로 쓰고 의미만 "손님 1명당 평균 간격"으로 바꾼다. 레벨 값이 옮겨 적기 없이 보존된다. 이름을 바꾸지 않으므로 Core Redirect도 필요 없다.
- 전체·구역별 최대, 얼룩 간격, Pawn 여유와 `MaxPlacementAttemptsPerInterval`은 의미가 유지되므로 그대로 둔다.
- 구역 선택 가중치는 새 규칙(구역마다 따로 계산)에서 쓸 곳이 없다.

## Q1. 쓸 곳이 없어지는 물 얼룩 구역 property는 어떻게 폐기할까요?

대상: `AStainSpawnZoneActor::SelectionWeight`, `AStainSpawnZoneActor::ZoneKind`, `EStainSpawnZoneKind` enum

- A: 이번 단위에서 즉시 삭제한다. Editor 단계에서 `BP_StainSpawnZone`과 레벨 구역 instance를 load·compile·resave해 남은 값을 정리한다.
- B: 한 migration cycle 동안 deprecated·숨김으로 보존하고 runtime에서 무시한다. 삭제는 다음 단위에서 한다.
- C: 그대로 두고 무시한다(Editor에 계속 보임).
- 권장안: A. 이전 배치 authoring QNA(Q1~Q3)에서도 즉시 삭제를 선택하셨습니다. 로드 위험이 없고, 의미 없는 값이 Editor에 남지 않습니다.
- 답변: A

## 기능 명세 단계 확인 필요 (이 문서에서 답하지 않음)

다음은 사용자에게 보이는 결과라 설계 단계가 정하지 않는다. 기능 명세 단계에서 `.md/QNA_FEATURE_SPEC.md`로 확정한 뒤 `.md/PROMPT_ARCHITECTURE.md`에 반영해야 한다.

상태: 모두 해결. F1~F4는 NextWork QNA Q62 B, Q63 A, Q64 A, Q65 A로 답변됐고, F5는 기능 명세 정정(Q63 반영 정정, TRSH-020 수정·TRSH-028 추가)으로 반영됐다. 설계는 `.md/Architecture/CleaningLitterSystem.md`에 반영했다.

- F5(해결). Q63 반영본(본문 "집게 없이(빈손·다른 물건)", TRSH-020 "대걸레·몽키스패너")이 기존 도구 동작 유지 계약과 충돌한다.
  - 현재 LMB가 자기 행동을 가진 물건은 조준 대상과 무관하게 LMB 행에 그 행동을 표시하고, 실제로 LMB를 누르면 그 행동을 한다.
    - 대걸레: `물걸레질`
    - 몽키스패너: `휘두르기`
    - 배송 상자: `상자 열기`
    - 설비 아이템: 배치 확정
  - 물 얼룩을 몽키스패너로 조준할 때도 `물걸레가 필요합니다`가 아니라 `휘두르기`가 보인다. 따라서 Q63의 "물 얼룩과 같은 방식"과 TRSH-020이 서로 다르다.
  - 이 물건들로 쓰레기를 조준할 때 `집게가 필요합니다`를 보이면, HUD는 불가라고 하는데 LMB는 휘두르기·상자 열기를 실행하게 된다.
  - 확인 필요: 이 네 경우 LMB 행에 기존 도구 행동을 유지하고, 빈손·품목 박스·수건바구니·삽처럼 LMB가 조준 대상에 쓰이는 경우에만 `집게가 필요합니다`를 보이는지.

- F1. 쓰레기와 물 얼룩이 서로 겹쳐 생겨도 되는가. 겹치면 위에 있는 쓰레기가 조준을 가려, 쓰레기를 치우기 전에는 그 얼룩을 물걸레로 닦지 못할 수 있다.
- F2. 집게 없이(빈손·다른 물건) 쓰레기를 조준했을 때 HUD. 물 얼룩처럼 LMB 행에 이유("집게가 필요합니다")를 보이는지, 대상 이름은 "쓰레기" 하나인지 외형별 이름("빈 병" 등)인지.
- F3. 배치 확정 때 사라지는 범위. 설치 자리와 조금이라도 겹치는 쓰레기·얼룩까지인지, 중심이 설치 자리 안인 것만인지. 중심 기준이면 설비 가장자리에 반쯤 깔린 얼룩이 남는다.
- F4. 정면에 공간이 없어 봉투를 묶지 못할 때 HUD 이유 문구(TRSH-012). 다른 거부 문구는 정해져 있지만 이것만 정해지지 않았다.

### 2026-10-01 코드 리뷰 재검토 중 사용자 결정 (기능 명세 반영 완료: TRSH-029·030, QNA_FEATURE_SPEC 정정 기록)

- F6. 벽 가장자리: **A 유지.** 벽에서 종류별 바닥 반경 R(쓰레기 약 15cm, 물 얼룩 30~45cm) 안에는 쓰레기·물 얼룩이 생기지 않는다. 벽에 반쯤 묻혀 보이지 않게 하기 위함이다. 현재 명세의 금지 목록에 벽이 없으므로 추가가 필요하다.
- F7. 생성 위치는 플레이어·손님과 무관하다. 그들 바로 옆이나 발밑에도 생길 수 있다. 현재 명세의 다음 문구와 반대이므로 삭제·수정이 필요하다.
  - `PROMPT_ARCHITECTURE.md` 53행(물 얼룩 현재·목표 "겹침 방지는 유지")
  - 102행(쓰레기 금지 목록 "플레이어·손님과 겹치는 자리")
  - 125행(물 얼룩 "플레이어·손님과 겹침 방지는 그대로다")
  - NextWork 전체 명세 299행
- 설계 반영: `Architecture/CleaningLitterSystem.md` Floor Rule(Pawn 검사 삭제, Pawn component 무시, 벽 유지), `PROMPT_IMPLEMENTATION.md` 재작업 절 2-1.
