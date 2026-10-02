# PROMPT_IMPLEMENTATION — EXP-U3 전체 확장 묶음

- 작업 ID: `EXP-U3`
- 단계: 아키텍처
- 상태: 완료

- 상위 계약: [../PROMPT_ARCHITECTURE.md](../PROMPT_ARCHITECTURE.md)(상태 완료). 이번 범위는 10절 U3 묶음이다. 사후 결정 근거는 [../QNA_FEATURE_SPEC.md](../QNA_FEATURE_SPEC.md)(D1~D4)에 있다.
- 현재 단계: 전체 확장(수직 구현 `EXP-U2` 승인 뒤). 단계 시작 커밋 `2f11710`, 브랜치 `work/EXP-U3`.
- 선행 결과: `EXP-U1` 병합 `8ca6a24`, `EXP-U2` 병합 `3c17e41`. D2 설계 출발점은 `git show 3c17e41:.md/Work/EXPANSION-PURCHASE/EXP-U2/PROMPT_IMPLEMENTATION.md` 19절이다. **이 문서가 19절을 대체한다.** 19절과 다르면 이 문서를 따른다(FBK-006). 바뀐 점은 1.3에 있다.
- 구조 정본(이번 단계에서 갱신): [../../../Architecture/ExpansionPurchaseSystem.md](../../../Architecture/ExpansionPurchaseSystem.md), [../../../Architecture/BuildingSystem.md](../../../Architecture/BuildingSystem.md) Expansion·Validation 절, [../../../Architecture/PlacementSystem.md](../../../Architecture/PlacementSystem.md) Preview Without Aim 절. 정본 내용은 이 문서에서 반복하지 않고, 구현 지시와 검증 기준을 적는다.
- 질문: 새로 정할 사용자 동작이 없어 `QNA_ARCHITECTURE.md`는 만들지 않았다.

---

## 0. 사용자 확인용 — Editor 데이터 관리 변화 (U3)

수치는 적지 않는다. 기본 제안값은 상위 계약 4.7 표에 있고, Editor 작업 단계가 13.3 표대로 그 값을 넣는다.

### 0.1 가격·방향·양은 어디서 바꾸나

**한 공간의 넓힘 값(가격·방향·양·횟수)은 모두 그 공간 Actor 한 곳에 있다.** 확장 정의 DataAsset에는 가격과 전체 상한이 더 이상 없다.

| 값 | 원본(어디에) | Editor에서 바꾸는 법 | 단위·기준 |
|---|---|---|---|
| 넓힘 가격 (D2) | 공간 Actor `Expansion Steps`의 줄마다 `Price` | 레벨에서 `Space_Hall`(또는 `Space_Bath`·`Space_Work`) 선택 → Details `Bathhouse Space › Expansion › Expansion Steps` → 줄 `[k]` 펼침 → `Price` | 원. `[0]` = 그 공간의 1번째 넓힘 가격, `[1]` = 2번째. 다른 공간을 몇 번 샀는지와 무관 |
| 넓힘 방향 (D3) | 같은 줄 `[k]` 안 `Sides` 목록의 항목마다 `Side` | 줄 `[k]` → `Sides` → `+`로 항목 추가, 항목마다 `Side` 선택(동 +X·서 −X·북 +Y·남 −Y) | 한 넓힘에 함께 물러날 벽을 항목 수만큼 적는다. 같은 벽은 한 번만 |
| 벽별 넓힘 양 (D3) | 같은 `Sides` 항목의 `Amount Cm` | 항목마다 숫자 입력 | cm. 벽마다 다르게 줄 수 있다(예: 북 200, 남 600) |
| 공간별 넓힘 횟수 상한 (D2) | `Expansion Steps`의 줄 수 | 줄을 더하거나 지운다 | 회. 줄 2개 = 2번까지. 따로 적는 상한 값과 전체 구입 상한은 없다 |
| 홀 넓힘 횟수별 열쇠 수·락커 칸 한도 | `DA_BathhouseExpansion_Default` `Tiers`(그대로) | DataAsset 열기 | 1번째 줄 = 홀 0회. 줄 수는 `Space_Hall` 줄 수 + 1 이상이어야 한다 |
| 락커 상품 가격(1·4·8칸) | `DA_ShopCatalog`의 `ClothesLocker1`·`ClothesLocker4`(신규)·`ClothesLocker8`(신규) 줄 `Price` | DataAsset 열기 → 그 줄 | 원, 상품별. 목록 맨 뒤에 1·4·8칸 순서 |
| 손님 길 범위 | Level `NavMeshBounds` | 상자 크기 조정 | 넓힌 끝 모습의 홀·목욕공간 전체를 덮어야 한다. 높이는 지상만 |
| 넓힘 미리보기 | 공간 Actor `Editor Preview Expansion Count`(편집 전용, 저장 안 됨) | 숫자 입력 | 그 횟수만큼 여러 벽이 함께 물러난 모습을 편집 화면에서 보여 준다 |

- 확장 정의 `DA_BathhouseExpansion_Default`의 `Max Purchase Count`·`Purchase Prices` 칸은 Details에서 사라진다(Editor 작업 단계가 재저장해 파일에서도 지운다).
- Details에서 `Sides` 항목은 접힌 상태에서도 벽 방향 이름이 보인다(`TitleProperty`).

### 0.2 작업 흐름 예

- **홀 2번째 넓힘 가격을 바꾸고 싶다:** `Space_Hall` → `Expansion Steps [1]` → `Price`.
- **홀 1번째 넓힘을 북쪽 2m·남쪽 6m로(EXP-048):** `Space_Hall` → `Expansion Steps [0]` → `Sides`의 북 항목 `Amount Cm` = 200, 남 항목 = 600. 열쇠·한도는 그대로 한 번만 오른다.
- **목욕공간을 동쪽으로만 넓히게:** `Space_Bath` → 각 줄 `Sides`에서 남·북 항목을 지운다.
- **홀을 3번까지 넓히게(EXP-045):** `Space_Hall` → `Expansion Steps`에 줄 `[2]` 추가(`Sides`·`Price` 채움) → `DA_BathhouseExpansion_Default` `Tiers`에 4번째 줄(홀 3회) 추가(열쇠 수는 열쇠걸이 자리 수 이하) → 미리보기 3으로 끝 모습 확인 → 손님 길 범위 오류가 나오면 `NavMeshBounds`를 넓힌다. 빠뜨리면 저장·검사할 때 오류가 뜬다.
- **넓힌 모습 미리 보기:** 세 공간 `Editor Preview Expansion Count`를 2로 → 벽·바닥·천장·조명·배치 격자·조각 미리보기 선이 여러 방향으로 넓힌 모습을 따른다. 저장 전에 0으로 돌린다.
- **4칸 락커 가격:** `DA_ShopCatalog` → `ClothesLocker4` 줄 `Price`.

### 0.3 잘못된 설정은 어떻게 알려 주나

- 공간 Actor(저장·Data Validation, 게임 시작 로그도 같음. 미리보기 횟수와 무관하게 0회와 목록 끝 모습을 검사):
  - 오류(신규·변경): 넓힘 줄에 물러날 벽이 하나도 없음, 같은 줄에 같은 벽이 두 번, 줄 가격이 0 이하, 홀 효과 표(`Tiers`)가 홀 넓힘 줄 수 + 1보다 짧음, 벽별 양이 0 이하, 0회에 다른 공간과 맞닿은 벽(통로 벽 포함)을 넓힘(항목마다).
  - 오류(기존 그대로): 목록 끝 모습끼리 겹침, 끝 모습에서 바깥 출입구 앞이 막힘, 끝 모습 손님 공간 바닥이 손님 길 범위 밖, 끝 모습 작업공간 바닥이 손님 길 범위 안.
  - 경고: 바깥 출입구가 있는 벽을 넓힘(항목마다). (U2의 "줄 수 합 < 전체 상한" 경고는 전체 상한이 없어져 사라진다.)
- 확장 정의: 효과 표 규칙만(비어 있음, 줄끼리 줄어듦, 열쇠 < 락커 한도).
- 열쇠걸이: 효과 표 **모든 줄**의 열쇠 수가 자리 수보다 많으면 오류.
- 게임 중 확장 데이터가 없거나 잘못되면 확장 탭에 `확장을 사용할 수 없습니다`(기존). 한 공간의 다음 줄만 잘못이면 그 선택지만 `이 공간은 더 넓힐 수 없습니다`.

### 0.4 새로 만들거나 바뀌는 asset

| 구분 | 대상 | 이번 단위에서 |
|---|---|---|
| 기존 DataAsset | `/Game/Bathhouse/Data/Expansion/DA_BathhouseExpansion_Default` | 재저장만(지운 필드 정리). `Tiers` 그대로 |
| 기존 Level Actor | `Space_Hall`, `Space_Bath`, `Space_Work` | `Expansion Steps` 각 2줄을 새 형식(`Sides` 목록 + `Price`)으로 다시 입력. 방향은 D3 기본값 |
| 기존 Level Actor | `NavMeshBounds` | 넓힌 끝 모습(홀 남·북, 목욕공간 남·북·동)을 덮도록 X·Y 확대. Z 그대로 |
| 기존 Widget Blueprint | `/Game/Bathhouse/UI/WBP_ExpansionScreen` | `StageText`·`PriceText` 삭제 |
| 기존 Widget Blueprint | `/Game/Bathhouse/UI/WBP_ExpansionSpaceOption` | 카드에 `StageText`·`PriceText` 추가 |
| 기존 DataAsset | `/Game/Bathhouse/Data/Shop/DA_ShopCatalog` | 맨 뒤에 `ClothesLocker4`, `ClothesLocker8` 추가 |

### 0.5 핵심 결정과 대안

| 결정 | 채택 | 버린 대안과 이유 |
|---|---|---|
| 넓힘 한 번의 여러 벽 (D3) | 넓힘 줄 `FBathhouseSpaceExpansionStep` = `Sides`(벽·양 목록) + `Price`. 옛 `Side`·`AmountCm`은 지운다 | 줄마다 동·서·남·북 양 4칸(0 = 안 움직임): 같은 벽 중복이 표현되지 않아 EXP-048의 "같은 넓힘에 같은 변 두 번 → 잘못된 데이터" 결과를 관찰할 수 없고, "양 0 이하 = 오류" 규칙과 뜻이 겹친다. 줄 하나 = 벽 하나 유지 + 묶음 번호: 가격·횟수가 넓힘 단위라 묶음 경계를 따로 맞춰야 한다 |
| 가격 위치 (D2) | 넓힘 줄의 `Price`(U2 19.1 그대로) | DataAsset 공간별 가격 표: 한 넓힘의 값이 두 곳으로 갈린다 |
| 늘어난 바닥의 생성 조각 | 넓힌 뒤 직사각형에서 넓히기 전 직사각형을 뺀 영역(모서리 포함)을 기존 직사각형 빼기로 나눠 같은 크기 규칙으로 조각 추가 | 벽마다 띠 하나: 두 벽이 함께 물러날 때 모서리가 빠지거나 겹친다 |
| 미리보기 숨김 (D4) | 이번 갱신에서 놓을 후보 위치를 계산하지 못하면 미리보기 Actor를 숨긴다(파괴하지 않음). 후보가 있으면 그 자리로 옮기고 보인다 | 미리보기 Actor 파괴·재생성: 회전·격자·세션 상태와 실패 시 미리보기 보존 계약이 깨지고 매번 mesh 복제 비용이 든다 |

---

## 1. 기능 계약과 범위

### 1.1 시나리오

- 원래 U3: EXP-040~046.
- D2·D3 신규·변경: EXP-047·048, 문구가 바뀐 EXP-021·022·023·040·041·042(재확인).
- D4: EXP-050·051·052.
- 깨지면 안 되는 기존: EXP-020, 024~032(U2), EXP-001~015(U1).
- 대표: EXP-042(다른 공간 구입 뒤 홀 2번째, 홀 2번째 가격, 열쇠 5~8), EXP-047(목욕공간 세 벽 한 번에), EXP-050(거리 밖 미리보기 숨김).

### 1.2 묶음 구성

| 묶음 | 내용 | 절 |
|---|---|---|
| (1) 원래 U3 | 목욕·작업공간 넓힘, 같은 공간 두 번째, 최대 표시, 상한 조정은 기존 경로 + D2·D3로 성립. 4·8칸 락커는 상점 데이터만 | 5~10 |
| (2) D2 | 전체 상한 삭제, 공간별·횟수별 가격, 선택지별 `확장 단계 N/M`·가격 | 5~9 |
| (3) D3 | 넓힘 한 번에 여러 벽, 벽별 양 | 5·6 |
| (4) D4 | 배치 미리보기 위치 없음 숨김 | 11 |

### 1.3 U2 19절과 달라진 점

- 19.2의 `Price` 추가는 그대로이고, 같은 struct에서 `Side`·`AmountCm`을 `Sides` 목록으로 바꾼다(D3).
- 19.3 검사 코드에 `ExpansionSidesEmpty`, `ExpansionSideDuplicate`를 더하고, `ExpansionAmountInvalid`·`ExpansionTouchingSide`·`ExpansionEntranceSide`를 벽 항목마다 본다.
- `CanApplyNextExpansion` 조건이 "다음 줄 양 유효"에서 "다음 줄이 적용 가능(벽 1개 이상, 모든 양 유효, 중복 없음)·가격 > 0"으로 바뀐다.
- 넓힘 띠 `ExpansionBand`(직사각형 하나)를 `ExpansionBandRects`(직사각형 목록)로 바꾼다.
- 19.10 content 자동화의 "세 공간 넓힘 줄 `Price` > 0"(Level 값)은 자동화에서 빼고 Editor 단계 Data Validation 확인으로 옮긴다(자동화가 DefaultMap을 로드하지 않게 함).
- 19.8 Editor 단계에 Nav 범위 확대, 방향 재입력, 4·8칸 락커가 더해진다(13.3).

## 2. 목적·수용 기준·비목표

목적: 세 공간을 각자 상한까지 공간별·횟수별 가격으로 넓히고, 넓힘 한 번에 여러 벽이 함께 물러나며, 4·8칸 락커를 팔고, 배치 미리보기는 놓을 위치가 없으면 숨긴다.

수용 기준(관찰 결과, 수치는 Editor 값에서):

- EXP-021: 탭에 `현재 확장 단계: N` 없음. 선택지마다 `확장 단계 0/2`, 전후 크기(여러 벽이 물러난 결과 하나), 그 공간 1번째 가격(공간마다 다름). 홀 효과 문구. 구입 버튼 꺼짐.
- EXP-022: 홀 선택 후 잔액 < 홀 1번째 가격 → 버튼 꺼짐·`N원 부족`(홀 가격 기준). 잔액이 오르면 즉시 켜짐. 선택 전에는 부족액이 없다.
- EXP-023: 확인 순간 홀 1번째 가격 1회 차감, 남·북 벽 동시에 물러남, 서쪽 출입구 벽·동쪽 통로 벽 그대로, 4번 열쇠, `확장 완료`, `설치된 락커 칸 2/4`, 홀 `확장 단계 1/2`·홀 2번째 가격, 다른 공간 `0/2`·각자 1번째 가격.
- EXP-040·041·047: 목욕공간 남·북·동, 작업공간 동·북이 한 번에 물러남. 모서리까지 바닥·천장이 직사각형 하나. 목욕공간은 모서리 포함 늘어난 바닥에 물 얼룩 조각, 작업공간은 조각 없음. 열쇠·한도 그대로. 단계 +1, 가격 1회.
- EXP-042: 목욕 1회·홀 1회 뒤 홀 구입 → 홀 2번째 가격, 열쇠 5~8, 한도 8칸, 홀 `2/2`.
- EXP-043: 세 공간 모두 상한일 때만 `최대 확장 단계입니다`.
- EXP-044: 홀 `2/2` 선택지만 막힘(`이 공간은 더 넓힐 수 없습니다`), 다른 공간은 1번째 가격으로 구입 가능.
- EXP-045: 홀 줄 3개 + `Tiers` 4줄 설정에서 홀 세 번 구입, 매번 홀의 그 번째 가격. 효과 표가 짧으면 `Space_Hall` 검사 오류.
- EXP-046: 4·8칸 락커 구입·배송·개봉·운반·설치, 설치 칸 수가 4·8 오름.
- EXP-048: 북 2m·남 6m가 각각 적용되고 열쇠·한도 1회. 홀 넓힘에 동쪽 또는 같은 벽 두 번 → 오류. 서쪽 → 경고.
- EXP-050·051: 거리 밖·벽·천장·계단·구역 없는 바닥·하늘 조준 → 미리보기 없음(마지막 위치에 빨간 미리보기 없음). 아이템 손에 있음, 격자 그대로, 안내 문구 그대로. 설치 입력 무시. 다시 구역 바닥 → 그 자리에 즉시 보이고 회전 유지.
- EXP-052: 공간 불허·한도·겹침은 조준한 자리에 빨간 미리보기와 이유 문구(숨지 않음).

비목표: 상위 계약 11절. 추가로 이번 단위는 Nav 범위를 공간 넓힘에 맞춰 자동으로 바꾸지 않는다(기존 결정: 넓은 고정 범위 + 검증).

## 3. 설계에 맡김 항목의 결정 (상위 계약 12절 중 U3 해당)

| 항목 | 결정 | 근거 |
|---|---|---|
| 공간별·횟수별 가격과 공간별 상한의 표현·검증 | 넓힘 줄 `Price`, 상한 = 줄 수. 가격 ≤ 0은 공간 검증 오류, 효과 표 길이는 홀 공간 검증 오류 | 한 넓힘의 값이 한 곳(U2 19.1) |
| 넓힘 하나의 여러 벽·벽별 양 표현 | 줄 안 `Sides` 목록(`Side`, `AmountCm`) | 0.5 |
| 여러 벽을 한 번에 적용 | 순수 `ExpandInterior`가 줄 하나의 모든 벽을 같은 직사각형에 적용(순서 무관). 공간 shell은 기존대로 한 번 파괴·재생성 | 한 함수 호출 안에서 끝나 중간 모습이 없다(계약 4.4) |
| 여러 벽 검증 | 벽 항목마다 기존 맞닿음·출입구 판정, 줄마다 빈 목록·중복·가격. 끝 모습 검사는 그대로(직사각형이 커지기만 하므로 모든 중간 조합을 덮음) | 기존 판정 재사용 |
| 늘어난 바닥 조각 | `ExpansionBandRects` = 넓힌 뒤 − 넓히기 전(기존 `SubtractRects`) → 각 직사각형 `SplitChunks` | 모서리 포함, 겹침 없음 |
| 기존 확장 단계 데이터와 홀 넓힘 횟수 | tier index = 홀 넓힘 횟수(U2 그대로). 여러 벽이어도 홀 줄 하나 = 한 번 | 계약 4.4·D3 |
| 미리보기 숨김·다시 보임, 위치 없음 판정 경로 | 판정: 기존 `ValidateCurrentPlacement`가 후보 transform을 계산했는지(조준선 구역 바닥 hit → 후보 계산 성공). 숨김: `AActor::SetActorHiddenInGame` | 11절 |

## 4. 대상 시스템·파일과 책임 변화

| 대상 | 기존 책임 | 변화 | 판단 |
|---|---|---|---|
| `FBathhouseSpaceExpansionStep`(Building, USTRUCT) | 줄 = 벽 하나·양 | 줄 = 벽·양 목록 + 가격 | 기존 struct 변경(이름 유지) |
| `FBathhouseSpaceExpansionSide`(Building, 신규 USTRUCT) | 없음 | 벽 하나와 양 | 신규 |
| `FBathhouseSpaceLayout`(순수) | 넓힌 직사각형·띠 하나 | 여러 벽 적용, 띠 직사각형 목록, 줄 적용 가능 판정 | `BathhouseSpaceExpansionLayout.cpp`만(101줄). `BathhouseSpaceLayout.cpp`(419줄) 변경 금지 |
| `ABathhouseSpaceActor` | 넓힘 적용·되돌림 | 다음 가격·상한 도달 조회, 적용 조건, 띠 조각 목록 | `BathhouseSpaceExpansion.cpp`(204줄)만. `BathhouseSpaceActor.cpp`는 변경 없음(`FillSnapshot`이 `CollectStepSnapshots`를 이미 부름) |
| `FBathhouseSpaceValidation` | 넓힘 검사 | 벽 항목·가격·효과 표 길이 검사 | `BathhouseSpaceExpansionValidation.cpp`·`BathhouseSpaceValidation.h`·`BathhouseSpaceWorldValidation.cpp`(효과 표 줄 수 읽기). `BathhouseSpaceValidation.cpp`(399줄) 변경 금지 |
| `UBathhouseExpansionDefinition`(Facility) | 가격·전체 상한·효과 표 | 효과 표만 | 필드·API 삭제 |
| `ABathhouseKeyRackActor`(Interaction) | 도달 가능 줄 열쇠 검증 | 모든 줄 검증 | 검증 몇 줄 |
| `UBathhouseExpansionPurchaseSubsystem` | 전체 횟수·전체 가격 | 공간별 횟수·가격 | 기존 확장 |
| `FExpansionScreenModel`, 확장 widget 2개 | 전체 단계·화면 가격 | 선택지별 단계·가격 | 기존 확장 |
| `UPlayerFacilityPlacementComponent`(Placement) | 미리보기 갱신 | 후보 없으면 숨김 | 418줄 cpp에 `RefreshPreview` 몇 줄. 같은 책임(미리보기 갱신)의 표시 조건이라 독립 책임 추가가 아니다(CoreSystem Class Growth). `PlayerFacilityPlacementValidation.cpp`(229줄)에 후보 flag |

새 의존·module 없음. Building → Facility 의존은 그대로(전체 상한 대신 효과 표 줄 수를 읽음).

## 5. 공간 넓힘 데이터와 계산 (D2 + D3, Building)

### 5.1 데이터 타입 (`Public/Building/BathhouseSpaceTypes.h`)

```cpp
/** 넓힘 한 번에 물러나는 벽 하나와 그 양. */
USTRUCT(BlueprintType)
struct BATHHOUSESIM_API FBathhouseSpaceExpansionSide
{
	GENERATED_BODY()
	// EditAnywhere, BlueprintReadOnly, Category "Bathhouse Space"
	EBathhouseSpaceSide Side = EBathhouseSpaceSide::East;   // ToolTip: "물러나는 벽 방향(world 축 기준). 0회에 다른 공간과 맞닿은 벽은 넓힐 수 없다."
	float AmountCm = 0.0f;                                   // ClampMin 0, ForceUnits cm. ToolTip: "이 벽이 바깥으로 물러나는 양(cm)."
};

/** 공간 넓힘 한 번(한 단계). 여러 벽이 함께 물러나고 가격은 이 넓힘 단위다. */
USTRUCT(BlueprintType)
struct BATHHOUSESIM_API FBathhouseSpaceExpansionStep
{
	GENERATED_BODY()
	// meta = (TitleProperty = "Side", ToolTip = "이 넓힘에 함께 물러나는 벽과 양. 항목 하나 = 벽 하나. 같은 벽은 한 번만.")
	TArray<FBathhouseSpaceExpansionSide> Sides;
	// meta = (ClampMin = "0", ToolTip = "이 넓힘의 구입 가격(원). 그 공간의 몇 번째 넓힘인지별, 전체 구입 순번과 무관.")
	int32 Price = 0;
};
```

- 옛 `Side`·`AmountCm` UPROPERTY는 **지운다**. struct 이름은 유지하므로 Core Redirect가 필요 없다. 이름 있는 tagged property라 옛 Level 데이터를 load하면 없는 property 값은 건너뛰고 새 property는 기본값(빈 목록, 가격 0)이다. 방향이 D3 기본값으로 바뀌어 어차피 다시 입력해야 하고, 사용처는 공간 Level instance 3개뿐이다. Editor 단계가 다시 입력·저장한다(13.3).
- C++ 기본값(빈 목록, 0)은 "입력 안 됨"이고 검증 오류로 알린다. 값 원본은 Level instance다.
- `ABathhouseSpaceActor::ExpansionSteps` tooltip: "줄 = 이 공간의 몇 번째 넓힘(여러 벽·가격). 줄 수 = 공간별 넓힘 횟수 상한."

### 5.2 snapshot과 순수 계산 (`Private/Building/BathhouseSpaceLayout.h`, `BathhouseSpaceExpansionLayout.cpp`)

```cpp
struct FBathhouseExpansionSideSnapshot { EBathhouseSpaceSide Side = EBathhouseSpaceSide::East; double AmountCm = 0.0; };
struct FBathhouseExpansionStepSnapshot { TArray<FBathhouseExpansionSideSnapshot> Sides; int32 Price = 0; };
```

- `static bool IsUsableSide(const FBathhouseExpansionSideSnapshot&)`: 양 > 0이고 유한.
- `static bool IsStepApplicable(const FBathhouseExpansionStepSnapshot&)`: 벽 1개 이상, 모든 벽 `IsUsableSide`, 같은 `Side` 중복 없음. 가격은 보지 않는다(형상 판정).
- `ExpandInterior(Base, Steps, Count)`: Count를 `[0, Steps.Num()]`로 clamp. index `0..Count-1` 줄마다, 그 줄의 벽 항목 중 `IsUsableSide`이고 **그 줄에서 처음 나온 `Side`인 것**을 같은 직사각형에 적용한다(벽 하나는 그 변만 바깥으로 양만큼). 쓸 수 없는 항목·같은 줄의 뒤쪽 중복은 형상에서 건너뛴다(검증 오류가 알린다). 결과는 항목 순서와 무관하다.
- `ExpansionBand`를 지우고 `static void ExpansionBandRects(const FBox2D& Base, const TArray<FBathhouseExpansionStepSnapshot>& Steps, int32 StepIndex, TArray<FBox2D>& OutRects)`를 둔다: `After = ExpandInterior(StepIndex+1)`, `Before = ExpandInterior(StepIndex)`, `SubtractRects(After, {Before}, OutRects)`. 범위 밖 index나 넓어진 면적이 없으면 빈 목록. 기존 `SubtractRects`는 guillotine으로 서·동 전체 높이 띠와 남·북 띠를 만들어 모서리를 한 번씩만 덮는다.
- `WithExpansionCount`, `PreviewLabelPlacement`는 그대로.

### 5.3 공간 Actor API (`BathhouseSpaceActor.h`, `BathhouseSpaceExpansion.cpp`)

- `CollectStepSnapshots`가 `Sides`·`Price`를 옮긴다.
- 신규 public: `int32 GetNextExpansionPrice() const`(다음 줄이 있으면 그 `Price`, 없으면 0), `bool IsAtExpansionLimit() const`(`AppliedExpansionCount >= ExpansionSteps.Num()`).
- `CanApplyNextExpansion`(부작용 없음) 조건: game world·begun play, `AppliedExpansionCount < ExpansionSteps.Num()`, **다음 줄 `IsStepApplicable`**, **다음 줄 `Price > 0`**, Rotation 0·Scale 1, Settings 상자 mesh·두께. 실패 문구(로그용 한국어)에 "물러날 벽이 없음/같은 벽 중복/양이 잘못됨/가격 0 이하"를 구분한다.

### 5.4 runtime 적용·되돌림

- `ApplyNextExpansion` 순서는 U2와 같다(재확인 → Undo 기록·횟수 +1 → `ApplyZoneGeometry` → 이 공간 `RebuildShell`, 실패면 되돌림 → 조각).
- 조각: `CleaningChunkKind != None`이면 `ExpansionBandRects(Base, Steps, 이전 횟수)`의 직사각형마다 `SplitChunks(직사각형, CleaningChunkMaxSizeCm)` 결과를 한 목록에 모아 `FBathhouseCleaningChunkSpawner::Spawn` 한 번으로 `CleaningChunks`에 덧붙인다. class 없음은 기존처럼 오류 로그 후 건너뜀(실패 아님). 기존 조각은 건드리지 않는다.
- `UndoExpansion`은 그대로(추가 조각 전부 파괴, 횟수 복원, 구역·shell 재생성).
- 이웃 공간 shell은 다시 만들지 않는다(U2 근거 유지: `BuildPlan`이 이웃의 안쪽 직사각형을 읽지 않고, 맞닿은 벽은 넓힐 수 없다). 여러 벽이어도 같다.
- 새 형상은 옛 바깥 직사각형 밖에만 생긴다. 움직이지 않는 벽(예: 홀 서쪽 벽)은 같은 자리에 더 길게 다시 생기며, 이미 놓인 Actor는 건드리지 않는다.

### 5.5 편집 미리보기

변경 없음. 미리보기 횟수 n이면 앞 n줄의 모든 벽이 적용된 모습이고 글자·겹침 표시 규칙은 그대로다.

## 6. 검증 (D2 + D3)

### 6.1 `ValidateExpansion` (`BathhouseSpaceValidation.h`, `BathhouseSpaceExpansionValidation.cpp`)

- 인자 `int32 MaxPurchaseCount` → `int32 HallEffectRowCount`. `BathhouseSpaceWorldValidation.cpp`가 world 첫 유효 `ABathhouseExpansionAuthority`의 Definition `Tiers.Num()`을 넘기고, Authority·Definition이 없으면 `INDEX_NONE`(효과 표 검사 생략).
- `EBathhouseProblemCode`(C++ 전용): `ExpansionStepsBelowCap` 삭제, `ExpansionSidesEmpty`, `ExpansionSideDuplicate`, `ExpansionPriceInvalid`, `ExpansionHallEffectShort` 추가.

| 코드 | 판정 | 심각도 | owner | 문구 예 |
|---|---|---|---|---|
| `ExpansionSidesEmpty` | 줄 `Sides`가 비어 있음 | Error | 그 공간 | `홀(Space_Hall) 1번째 넓힘 줄에 물러날 벽이 없습니다.` |
| `ExpansionSideDuplicate` | 같은 줄에 같은 `Side`가 두 번 이상(벽마다 한 번 보고) | Error | 그 공간 | `… 1번째 넓힘 줄에 동(+X) 벽이 두 번 있습니다.` |
| `ExpansionPriceInvalid` | 줄 `Price` ≤ 0 | Error | 그 공간 | `… 2번째 넓힘 줄의 가격이 0 이하입니다.` |
| `ExpansionAmountInvalid` | 벽 항목 양 ≤ 0·비유한(항목마다) | Error | 그 공간 | `… 1번째 넓힘 줄의 남(-Y) 벽: 넓히는 양은 0보다 큰 유한한 값이어야 합니다.` |
| `ExpansionTouchingSide` | 0회에 다른 공간과 맞닿은 변을 벽 항목이 넓힘(항목마다, 기존 판정) | Error | 그 공간 | 기존 문구 + 벽 방향 |
| `ExpansionEntranceSide` | 바깥 출입구가 있는 변을 벽 항목이 넓힘(항목마다) | Warning | 그 공간 | 기존 문구 |
| `ExpansionHallEffectShort` | `HallEffectRowCount != INDEX_NONE`이고 홀 줄 수 + 1 > `HallEffectRowCount` | Error | 홀 | `홀 넓힘 효과 표(DA_BathhouseExpansion_Default Tiers)가 {n}줄입니다. 홀 넓힘 {m}개에는 {m+1}줄이 필요합니다.` |
| `ExpansionOverlap`, `ExpansionOutsideOpeningBlocked`, `ExpansionNavOutside`, `ExpansionNavWorkCovered` | 기존(목록 끝 모습) | Error | 기존 | 기존 |

- 같은 줄의 뒤쪽 중복 항목에는 양·맞닿음·출입구 검사를 하지 않는다(형상에서 건너뛰는 항목이라 중복 오류 하나로 충분).
- `ItemIndex`는 줄 index다. 벽 방향은 문구에 넣는다.
- 위치 제안은 넓힘 문제에 붙이지 않는다(기존).
- `ValidateWorld`는 0회 복사본으로 기존 검사, 이어서 `ValidateExpansion`(기존 흐름). BeginPlay 로그·`IsDataValid`가 같은 결과를 쓴다.

### 6.2 확장 정의·열쇠걸이 (D2, Facility·Interaction)

- `UBathhouseExpansionDefinition`: `MaxPurchaseCount`, `PurchasePrices`, `GetMaxPurchaseCount`, `TryGetPurchasePrice`와 그 규칙을 지운다(deprecated로 두지 않음, 근거 U2 19.1). `ValidatePurchaseData`(이름 유지)는 `Tiers` 규칙(비어 있음, 열쇠 ≥ 한도, 줄마다 줄지 않음)만 본다. `GetHallEffect`·`GetHallEffectIndex` 유지. `Tiers` tooltip: "index = 홀 넓힘 횟수(0회부터). 표 끝을 넘으면 마지막 줄. 줄 수는 홀 넓힘 줄 수 + 1 이상(공간 검사가 알림)."
- `ABathhouseKeyRackActor::IsDataValid`: `GetMaxPurchaseCount` 계산을 없애고 `Tiers` 모든 줄의 최대 `KeyPoolSize`와 `PairTransforms.Num()`을 비교한다(근거 U2 19.1: 열쇠걸이가 Building을 알지 않게 함).
- `ABathhouseExpansionAuthority` 검증은 그대로.

## 7. 구입 subsystem·view (D2)

`Public/Building/BathhouseExpansionTypes.h`, `BathhouseExpansionPurchaseSubsystem.h/.cpp`.

- `EBathhouseExpansionFailure`에서 `MaxPurchasesReached` 삭제.
- `FBathhouseExpansionView`: `PurchaseCount`, `MaxPurchaseCount`, `bMaxReached`, `NextPrice`, `Shortfall` 삭제. `bool bAllAtLimit` 추가 = 등록된 공간이 하나 이상이고 등록된 공간 모두 `IsAtExpansionLimit()`.
- `FBathhouseExpansionOptionView`에 `int32 AppliedCount`, `int32 StepCount`(= 줄 수, 표시 M), `bool bAtLimit`, `int32 NextPrice`(`bCanExpand`일 때만 `GetNextExpansionPrice()`, 아니면 0) 추가. `bCanExpand`·`NextSizeCm`(= `GetInteriorRectForCount(적용 + 1)`, 여러 벽 결과)은 기존.
- `GetPurchaseCount()` 삭제.
- `Resolve`: Definition 규칙은 `Tiers` 규칙만. 나머지 사용 가능 조건(Authority, Definition, 공간 ≥ 1, wallet, tier 일관성)은 그대로.
- `EvaluatePurchase(const APlayerState* Buyer, EBathhouseSpaceKind Kind, int32 ExpectedAppliedCount, int32* OutPrice = nullptr)` 순서: `Busy` → `Unavailable` → `SpaceUnavailable`(그 종류 없음) → `StaleState`(`ExpectedAppliedCount != 그 공간 GetAppliedExpansionCount()`) → `SpaceMaxReached`(`CanApplyNextExpansion` 실패) → `InsufficientMoney`(`!Wallet->CanSpendMoney(Space->GetNextExpansionPrice())`) → 홀이면 다음 tier 사전 검사(기존). 가격은 항상 고른 공간의 `GetNextExpansionPrice()`.
- `TryPurchase(APlayerState* Buyer, EBathhouseSpaceKind Kind, int32 ExpectedAppliedCount)`: transaction 순서·되돌림·방송·실패 주입은 U2 그대로(정본 ExpansionPurchaseSystem Purchase Subsystem 표). 가격 출처만 바뀐다.
- 기대 횟수가 고른 공간의 넓힌 횟수이므로 같은 확인의 두 번째 요청과 확인 뒤 가격이 바뀐 요청을 모두 `StaleState`로 거절한다.
- 효과 표가 홀 줄 수보다 짧은 잘못된 데이터에서는 `GetHallEffectIndex` clamp로 tier가 오르지 않은 채 구입된다(계약 4.7 "표 끝을 넘으면 마지막 값"). 데이터 오류는 공간 검증이 알린다.

## 8. 확장 탭 표시 (D2)

### 8.1 표시 모델 (`Private/UI/ExpansionScreenModel.h/.cpp`)

- `FExpansionScreenDisplay`에서 `Stage`·`bStageVisible`, `Price`·`bPriceVisible` 삭제.
- `FExpansionOptionDisplay`에 `Stage`, `bStageVisible`, `Price`, `bPriceVisible` 추가.
- 규칙:
  - 사용 불가: 기존(문구 `확장을 사용할 수 없습니다`, 잔액만 보임, `bClearSelection`).
  - `bAllAtLimit`: `최대 확장 단계입니다`, 선택지·구입·확인·부족액 숨김, 잔액·락커·완료 문구 보임, `bClearSelection`.
  - 선택지: 공간 있으면 `확장 단계 {AppliedCount}/{StepCount}`(공간 없으면 숨김). `bPresent && bCanExpand`면 크기 `{현재} → {다음}`, `다음 넓힘 {NextPrice}원`(금액 형식 기존), 고를 수 있음, 홀이면 효과 문구. 아니면 크기 `{현재}`(없으면 빈 문구), 가격 숨김, `이 공간은 더 넓힐 수 없습니다`, 비활성. 선택된 것이 비활성이면 `bClearSelection`.
  - 구입 버튼 문구: 고른 선택지가 고를 수 있으면 `확장 구입 ({그 공간 NextPrice}원)`, 아니면 `확장 구입`.
  - 부족액: 고른 선택지가 고를 수 있고 잔액 < 그 가격일 때만 `{가격 − 잔액}원 부족`. 선택 전에는 숨김.
  - 구입 가능 = 사용 가능 && `!bAllAtLimit` && 선택 있음 && 그 선택지 고를 수 있음 && 잔액 ≥ 그 가격. 확인 대기 중 `bConfirmEnabled`도 같은 판정.
  - LOCTEXT(namespace `ExpansionScreen`): `확장 단계 {0}/{1}`, `다음 넓힘 {0}`(`{0}` = `FormatMoney`), `확장 구입`, 기존 문구 유지. `현재 확장 단계: {0}`, `이번 구입 가격 {0}` 삭제.

### 8.2 widget (`ExpansionScreenWidget.h/.cpp`, `ExpansionSpaceOptionWidget.h/.cpp`)

- `UExpansionScreenWidget`: BindWidget `StageText`, `PriceText` **삭제**. 표시 상태 `ConfirmPurchaseCount` → `ConfirmAppliedCount`(확인을 연 순간 `View.Options[선택].AppliedCount`). 확인은 `TryPurchase(사용자, 선택, ConfirmAppliedCount)`. `HandleOptionClicked`는 `bMaxReached` 대신 `bAllAtLimit`을 본다.
- `UExpansionSpaceOptionWidget`: `UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> StageText`, `UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> PriceText` 추가. `ApplyModel`이 null 확인 뒤 문구와 가시성(보임 `HitTestInvisible`, 숨김 `Collapsed`, 기존 `EffectText`와 같은 방식)을 적용한다.
  - Optional 근거(U2 8.1과 같음): 이미 있는 WBP에 필수 binding을 더하면 구현~Editor 사이에 compile 오류가 되고, root가 품은 이 WBP를 로드하는 `ScreenWheelContentContract`가 실패한다. 존재는 content 자동화가 Editor 작업 뒤 단언한다.
- 그리기 조건(FBK-004): 새 글자는 이미 그려지는 게임 world 컴퓨터 화면(`UWidgetComponent`) 안의 `UTextBlock`이다. 새 component·hidden flag·tick 변화가 없고 가시성은 C++가 기존 방식으로만 바꾼다. 글꼴은 다른 화면 글자와 같은 기본 UMG 글꼴(한글 fallback)이다. 남는 위험은 카드 높이 고정(`OptionSize`) 안에서 두 줄이 늘어 잘리는 것이며 Editor 단계(수치)와 사용자 PIE(눈)로 확인한다.

## 9. 상점 — 4·8칸 락커 (데이터만)

- 코드 변경 없음. `FShopProductRules`가 락커(`LockerSlotCount > 0`, `Facility.Discardable` 없음)를 이미 허용한다. 한도 초과 구매는 막지 않고 설치만 `CanInstallLockerSlots`가 막는다.
- Editor: `DA_ShopCatalog` 맨 뒤(`ClothesLocker1` 뒤)에 `ClothesLocker4`, `ClothesLocker8` 줄(13.3).
- 확인할 기존 사실: `DA_FacilityPlacement_ClothesLocker_4`·`_8`은 placement 활성, `LockerSlotCount` 4·8, `Facility.Type.ClothesLocker` 태그, `Facility.Discardable` 없음, `RecoveryItemClass` 공통 item(`.md/Unreal/PlacementSystem.md`). 신규 설치 payload와 락커 bank 등록은 1칸과 같은 경로다.

## 10. 확장 데이터 lifecycle과 전환 순서

- 구현 빌드 직후~Editor 단계 사이에는 세 공간 줄이 새 형식에서 빈 값(벽 없음, 가격 0)이다. 이때 공간 Data Validation 오류(빈 벽·가격)가 나오고, 게임에서는 세 선택지가 모두 `이 공간은 더 넓힐 수 없습니다`가 된다(`IsAtExpansionLimit`는 거짓이라 `최대 확장 단계입니다`는 아님). Editor 단계가 값을 넣으면 해소된다. 정상 전환 상태이며 결함이 아니다.
- `DA_BathhouseExpansion_Default`는 지운 필드 값을 파일에 남긴 채 load돼도 안전하다(건너뜀). Editor 단계가 재저장한다.

## 11. 배치 미리보기 위치 없음 숨김 (D4, Placement)

### 11.1 판정

- "놓을 위치가 없음" = 이번 갱신에서 `ValidateCurrentPlacement`가 **후보 transform을 계산하지 못함**이다. 후보 계산은 `TracePlacementZone` 성공(배치 거리 안에서 배치 trace 채널 첫 hit가 구역 바닥 윗면 `IsZoneSurfaceHit`) → `MakeCandidateTransform` → `BuildPlacedActorTransform` 성공일 때만 된다.
  - 거리 밖, 벽·천장·계단 형상(그 너머 다른 공간 포함), 구역 없는 바닥·하늘은 trace 실패 → 후보 없음 → 숨김(계약 4.11).
  - 공간 불허·락커 한도·구역 밖·겹침·바닥 지지 부족은 후보 계산 뒤의 실패 → 후보 있음 → 조준한 자리에 빨간 미리보기(EXP-052, 지금과 같음).
  - 후보 계산 전의 상태 이상(들고 있는 설비 변경, 미리보기 geometry 변경, 정의 오류)도 후보가 없으므로 숨긴다. 정상 플레이에서 생기지 않는 fail-closed 상태이며, 마지막 위치에 빨간 미리보기를 남기지 않는다는 D4 결과와 같은 방향이다.
- 배치 거리는 기존 `UFacilityPlacementSettings::PlacementTraceDistance`(Project Settings) 그대로다.

### 11.2 구현 지시

- `UPlayerFacilityPlacementComponent::ValidateCurrentPlacement(FTransform& OutCandidate, AFacilityPlacementZoneActor*& OutZone, bool& bOutHasCandidate) const`(private, `PlayerFacilityPlacementValidation.cpp`): 시작에 `bOutHasCandidate = false`. `BuildPlacedActorTransform` 성공 직후 `true`. 이후 실패 경로는 `true`를 유지한다. 다른 판정·문구·순서는 바꾸지 않는다.
- `RefreshPreview`(`PlayerFacilityPlacementComponent.cpp`):

```cpp
AFacilityPlacementZoneActor* Zone = nullptr;
FTransform Candidate = CurrentCandidate;
bool bHasCandidate = false;
CurrentPlacementQuery = ValidateCurrentPlacement(Candidate, Zone, bHasCandidate);
PreviewZone = Zone;
if (bHasCandidate) { CurrentCandidate = Candidate; }
if (AFacilityPlacementPreviewActor* LivePreview = PreviewActor.Get())
{
	if (bHasCandidate)
	{
		LivePreview->SetActorTransform(CurrentCandidate);
		LivePreview->SetPlacementValidity(CurrentPlacementQuery.bSucceeded, CurrentPlacementQuery.FailureReason);
	}
	LivePreview->SetActorHiddenInGame(!bHasCandidate);
}
```

  - 후보가 없을 때는 옮기지 않고 숨기기만 한다. 다시 후보가 생기면 같은 호출에서 옮긴 뒤 보이게 하므로 옛 위치가 한 프레임도 보이지 않는다.
  - 미리보기 시작(`HandleHeldObjectChanged`)이 곧바로 `RefreshPreview`를 부르므로, 조준이 없으면 시작부터 숨는다(아이템 위치에 빨간 미리보기가 잠깐 보이던 현상도 없어진다).
  - `SetActorHiddenInGame`은 값이 바뀔 때만 일하므로 매 tick 호출해도 render state를 매번 다시 만들지 않는다.
- `ConfirmPlacement`는 지역 bool을 넘기고 쓰지 않는다. 후보가 없으면 지금처럼 `설치 가능한 구역을 바라보세요.`로 실패하고 아이템·미리보기 Actor·격자를 유지한다(EXP-051, 계약 4.9).
- 미리보기 Actor를 파괴하지 않는다. 누적 Yaw·LCtrl·격자·`MergeSupplementalInteractionQuery`(안내 문구)는 그대로다.
- `AFacilityPlacementPreviewActor`는 바꾸지 않는다. 미리보기 mesh에 `bCastHiddenShadow`·`bAffectIndirectLightingWhileHidden`·`bRayTracingFarField`를 켜지 않는다(아래 표).

### 11.3 엔진 그리기 조건 (FBK-004, UE 5.8 엔진 소스 확인)

| 조건 | 출처 | 계약 |
|---|---|---|
| `SetActorHiddenInGame(b)`는 `IsHidden() != b`일 때만 `SetHidden` → `UpdateComponentVisibility` → 등록 component마다 `OnActorVisibilityChanged` → `MarkRenderStateDirty` | `Engine/Private/Actor.cpp` `SetActorHiddenInGame`·`UpdateComponentVisibility`, `ActorComponent.h` `OnActorVisibilityChanged` | 매 갱신 호출 허용(변화 없으면 no-op) |
| game world(`UWorld::UsesGameHiddenFlags()` = `IsGameWorld()`)에서 `USceneComponent::ShouldRender()` = `IsVisible() && !Owner->IsHidden() && 보임 flag` | `SceneComponent.cpp` `ShouldRender`, `World.cpp` `UsesGameHiddenFlags` | 숨긴 Actor의 mesh는 그리지 않는다. 다시 false면 원래 조건(복제한 mesh는 보이는 source만)으로 그린다 |
| primitive proxy는 `ShouldRender() \|\| bCastHiddenShadow \|\| bAffectIndirectLightingWhileHidden \|\| bRayTracingFarField`일 때 만든다 | `PrimitiveComponent.cpp` scene proxy 생성 조건 | 세 flag를 켜지 않는다(기본 false, 미리보기 생성 코드가 설정하지 않음). 그래서 숨김 중 그림자·간접광도 없다 |
| 미리보기 Actor는 collision·tick·Nav가 없다 | `FacilityPlacementPreviewActor.cpp` | 숨김이 다른 시스템에 영향 없음 |

## 12. lifecycle·rollback·전역 설정 영향

- Project/World/Input/Collision/Nav 설정 변경 없음. Config 변경 없음.
- 확장 transaction 타임라인·되돌림·방송은 U2와 같다. 여러 벽은 공간 하나의 shell 재생성 한 번 안에서 끝난다.
- Recast Dynamic이 넓힌 공간 tile을 다시 만든다(기존). 넓힌 끝 모습이 Nav 범위 안인지는 공간 검증이 알리고 범위는 Level `NavMeshBounds`(Editor 단계 13.3)가 맞춘다.
- World Partition: 공간 Actor 항상 로드(기존).
- 저장 없음(기존).

## 13. Blueprint/API, Core Redirect, Editor migration

### 13.1 reflected 변경

- 신규: `FBathhouseSpaceExpansionSide`(`Side`, `AmountCm`), `FBathhouseSpaceExpansionStep::Sides`·`Price`, `UExpansionSpaceOptionWidget::StageText`·`PriceText`(optional).
- 삭제: `FBathhouseSpaceExpansionStep::Side`·`AmountCm`, `UBathhouseExpansionDefinition::MaxPurchaseCount`·`PurchasePrices`, `UExpansionScreenWidget::StageText`·`PriceText` binding.
- class·struct·enum rename·삭제 없음 → Core Redirect 불필요. native 부모·custom `Serialize`·default subobject 변경 없음.
- CoreSystem 규칙: Editor 작업 전에 새 빌드로 `DA_BathhouseExpansion_Default`, 공간 external actor 3개, `WBP_ExpansionScreen`, `WBP_ExpansionSpaceOption`, `WBP_ComputerScreenRoot`를 비저장 load해 Fatal·load 오류가 없는지 본다(지운 property 경고만 허용). `WBP_ExpansionScreen`은 지운 binding 이름의 widget이 남아 있어도 compile된다(BindWidget 없는 일반 widget).

### 13.2 구현 단계가 `PROMPT_UNREAL.md`로 넘길 것

13.3 표와 순서를 그대로 옮기고, 구현에서 바뀐 이름이 있으면 함께 적는다.

### 13.3 Editor 단계 변경 (allowlist, exact)

조정값 원본 원칙에 따라 수치는 상위 계약 4.7의 해당 칸을 가리킨다. 넣는 값은 그 칸의 값과 정확히 같다.

1. **사전 load 확인**(13.1). 저장하지 않는다.
2. **`/Game/Bathhouse/Data/Expansion/DA_BathhouseExpansion_Default`**: 새 빌드로 열어 재저장(지운 `MaxPurchaseCount`·`PurchasePrices` 정리). `Tiers`는 그대로(3줄 = 홀 줄 2 + 1). 새 프로세스 재로드, Data Validation 오류 0.
3. **공간 넓힘 줄**(external package는 `.md/Unreal/BuildingSystem.md` 표). 각 공간 `Expansion Steps`를 아래로 바꾼다(줄 2개, `Sides` 항목 순서 그대로).

   | 공간 | 줄 | `Sides`(순서) | 각 `Amount Cm` | `Price` |
   |---|---|---|---|---|
   | `Space_Hall` | `[0]` | 남(-Y), 북(+Y) | 4.7 "한 번 넓어지는 방향과 변별 양 (D3)" 행의 "변마다" 값, 모든 항목 같음 | 4.7 "확장 구입 가격 (D2)" 행 "홀 1번째" |
   | `Space_Hall` | `[1]` | 남(-Y), 북(+Y) | 같음 | 같은 행 "홀 2번째" |
   | `Space_Bath` | `[0]` | 남(-Y), 북(+Y), 동(+X) | 같음 | 같은 행 "목욕공간 1번째" |
   | `Space_Bath` | `[1]` | 남(-Y), 북(+Y), 동(+X) | 같음 | 같은 행 "목욕공간 2번째" |
   | `Space_Work` | `[0]` | 동(+X), 북(+Y) | 같음 | 같은 행 "작업공간 1번째" |
   | `Space_Work` | `[1]` | 동(+X), 북(+Y) | 같음 | 같은 행 "작업공간 2번째" |

4. **끝 모습 확인**: 세 공간 미리보기 횟수 2. 미리보기 글자에 ` · 겹침 있음`이 없고, 지상 넓힘 띠(홀 남·북, 목욕공간 남·북·동, 모서리 포함)에 Level Actor(편집 전용 sprite·frustum 제외)가 없으며, 지형 면이 지상 바닥 판 아래인지 본다. 지형·장애물 판정은 WorldStatic·WorldDynamic object type 단순 충돌 trace(`bTraceComplex=false`)로 한다(FBK-001). 띠에 Level Actor가 있으면 옮기지 말고 멈춰 보고한다(사전 허용 S2: 승인 범위 밖 asset).
5. **`NavMeshBounds`**(package `.md/Unreal/WorldSystem.md`): 4의 끝 모습에서 공간 Data Validation에 `ExpansionNavOutside`가 나오지 않을 때까지 X·Y를 넓힌다. 홀 서쪽 출입구 밖 마당은 계속 덮고, Z 범위는 그대로 두어 작업공간 바닥을 넣지 않는다(`ExpansionNavWorkCovered` 없음). 편집 world에서 끝 모습 미리보기로 손님 생성기 → 홀·목욕공간 넓힘 띠 경로가 있고 작업공간 띠에는 Nav가 없는지 본다(Recast 편집 결과는 저장하지 않음).
6. 세 공간 미리보기 횟수 0으로 돌린 뒤 공간 3개·`NavMeshBounds` external package 저장(Python `EditorLoadingAndSavingUtils.save_packages`), 새 프로세스 재로드. 공간 Data Validation: 오류 0, 경고는 기존 `Space_Bath` opt-out 2개만(출입구 변 경고 없음).
7. **`/Game/Bathhouse/UI/WBP_ExpansionScreen`**: `HeaderRow`의 `StageText`, `InfoColumn`의 `PriceText` 삭제, 남은 배치 정리. Compile 경고 0.
8. **`/Game/Bathhouse/UI/WBP_ExpansionSpaceOption`**: `CardColumn`을 `NameText` → `StageText` → `SizeText` → `PriceText` → `EffectText` → `StatusText` 순서로(두 TextBlock 신규, 기존 문구 style). 필요하면 `OptionSize` 높이를 늘리되 세 카드와 아래 정보·구입 영역이 `RootSize` 1024×576 안에 들어가는지 widget 크기 합으로 확인. Compile 경고 0. 화면 잘림의 눈 확인은 숨김 Editor에서 컴퓨터 화면이 그려지지 않으면 사용자 PIE 체크리스트로 넘긴다(FBK-003).
9. **`/Game/Bathhouse/Data/Shop/DA_ShopCatalog`**: 맨 뒤에 두 줄 추가(기존 21개 순서 유지).

   | ProductId | DisplayName | bForSale | Price | PlacementDefinition | ItemBoxDefinition·Icon |
   |---|---|---|---|---|---|
   | `ClothesLocker4` | `4칸 락커` | true | 4.7 "락커 상품 가격" 행 "4칸" | `/Game/Bathhouse/Data/Placement/DA_FacilityPlacement_ClothesLocker_4` | 비움 |
   | `ClothesLocker8` | `8칸 락커` | true | 같은 행 "8칸" | `/Game/Bathhouse/Data/Placement/DA_FacilityPlacement_ClothesLocker_8` | 비움 |

   두 정의에 `Facility.Discardable`이 없고 `LockerSlotCount`가 4·8인지 확인. Catalog Data Validation 오류 0, 저장·재로드.
10. `ExpansionAuthority`(Definition 연결, `Initial Tier Index` 0)와 `KeyRack` Data Validation 오류 0 확인(저장 안 함).
11. 자동화: `BathhouseSim.Expansion.*`(content 계약 포함), `BathhouseSim.Computer.Input.ScreenWheelContentContract`, 17절의 새 Placement 테스트.
12. Unreal 정본 갱신: `Unreal/BuildingSystem.md`(넓힘 줄 형식 `Sides`·`Price`, 방향, 가격 원본, "합이 Max Purchase Count 이상" 문장 삭제), `Unreal/FacilitySystem.md`(확장 정의 필드, 열쇠걸이 검사 범위), `Unreal/InteractionUISystem.md`(확장 화면 binding), `Unreal/ShopSystem.md`(4·8칸 락커), `Unreal/WorldSystem.md`(Nav 범위).

- 그 밖의 asset(root WBP, 열쇠걸이, 확장 관리자, 설비 정의)은 바꾸지 않는다.

## 14. 조정값 원본과 상수 예외

| 값 | 원본 |
|---|---|
| 넓힘 가격·방향·벽별 양·공간별 상한 | 공간 Level instance `ExpansionSteps`(`Sides`·`Price`, 줄 수) |
| 홀 효과 표 | `DA_BathhouseExpansion_Default` `Tiers` |
| 락커 상품 가격 | `DA_ShopCatalog` `ClothesLocker1/4/8.Price` |
| 배치 거리 | `UFacilityPlacementSettings::PlacementTraceDistance`(기존) |
| 조각 최대 크기, 벽·판 두께 | `UBathhouseBuildingSettings`(기존) |

상수 예외(엔진·표시 의미): cm→m 100, 크기 문구 소수 최대 1자리, 부동소수 허용 오차 `UE_KINDA_SMALL_NUMBER`(기존 `LayoutTolerance` 포함), 효과 표 index clamp. D4는 새 수치가 없다. 자동화 기대값은 fixture 값·Settings에서 계산한다.

## 15. 구현 금지 범위

- Content·Config 수정 금지.
- `BathhouseSpaceLayout.cpp`(419줄), `BathhouseSpaceValidation.cpp`(399줄), `BathhouseSpaceActor.cpp` 변경 금지. 새 helper는 `BathhouseSpaceExpansionLayout.cpp`·`BathhouseSpaceExpansionValidation.cpp`·`BathhouseSpaceExpansion.cpp`에 둔다.
- 전체 구입 횟수·전체 상한·구입 순번 가격을 코드에 남기지 않는다(호환용 deprecated 필드 포함).
- 이웃 공간 shell 재생성, ISM instance 이동, Static component 이동 금지(기존).
- 넓힘 벽을 한 번에 하나씩 차례로 적용해 shell을 여러 번 다시 만드는 구현 금지(중간 모습이 생긴다).
- `PlayerFacilityPlacementComponent.cpp`에는 11.2의 `RefreshPreview`·`ConfirmPlacement` 변경만. 숨김 판정을 위한 trace 추가, 미리보기 Actor 파괴·재생성, 격자 숨김, 배치 거리 변경, 실패 문구 변경 금지.
- `AFacilityPlacementPreviewActor`, `UPlayerInteractionComponent`, Character 변경 금지.
- 확장 탭 widget에 Tick·timer·domain 상태 보관 금지, WBP graph 금지, 실제 사용자 focus 호출 금지(기존).
- 상점 규칙·한도 판정·휠 규칙 변경 금지.

## 16. 수정 파일

- Building: `Public/Building/BathhouseSpaceTypes.h`, `Public/Building/BathhouseSpaceActor.h`, `Private/Building/BathhouseSpaceExpansion.cpp`, `Private/Building/BathhouseSpaceLayout.h`, `Private/Building/BathhouseSpaceExpansionLayout.cpp`, `Private/Building/BathhouseSpaceValidation.h`, `Private/Building/BathhouseSpaceExpansionValidation.cpp`, `Private/Building/BathhouseSpaceWorldValidation.cpp`, `Public/Building/BathhouseExpansionTypes.h`, `Public/Building/BathhouseExpansionPurchaseSubsystem.h`, `Private/Building/BathhouseExpansionPurchaseSubsystem.cpp`.
- Facility·Interaction: `Public/Facility/BathhouseExpansionDefinition.h`, `Private/Facility/BathhouseExpansionDefinition.cpp`, `Private/Interaction/BathhouseKeyRackActor.cpp`.
- UI: `Public/UI/ExpansionScreenWidget.h`, `Private/UI/ExpansionScreenWidget.cpp`, `Public/UI/ExpansionSpaceOptionWidget.h`, `Private/UI/ExpansionSpaceOptionWidget.cpp`, `Private/UI/ExpansionScreenModel.h/.cpp`.
- Placement: `Public/Placement/PlayerFacilityPlacementComponent.h`(시그니처, 새 테스트 friend), `Private/Placement/PlayerFacilityPlacementValidation.cpp`, `Private/Placement/PlayerFacilityPlacementComponent.cpp`.
- 자동화: `Private/Tests/BathhouseExpansionAutomationTestSupport.h`, `Private/Tests/BathhouseExpansionAutomationTests.cpp`, `Private/Tests/ExpansionScreenAutomationTests.cpp`, 신규 권장 `Private/Tests/BathhouseExpansionMultiSideAutomationTests.cpp`(D3), 신규 권장 `Private/Tests/FacilityPlacementPreviewAimAutomationTests.cpp`(D4). 기존 큰 테스트 파일에 넣어도 되지만 테스트 이름은 17절대로 둔다.

## 17. 자동화·빌드·코드 리뷰 기준

빌드: [UE_BUILD_POLICY.md](../../../UE_BUILD_POLICY.md) 고정 명령. 헤더·UPROPERTY가 바뀌므로 같은 프로젝트 Editor를 닫고 빌드한다.

기대값은 fixture 값(넓힘 줄 벽·양·가격, `Tiers`, Settings)에서 계산하고 리터럴로 복제하지 않는다.

| 영역 | 확인 |
|---|---|
| 순수 layout (D3) | `ExpandInterior`: 남+북, 남+북+동, 동+북을 한 줄에 적용, 벽별 다른 양(북 a·남 b), 항목 순서를 바꿔도 같음, 같은 줄 뒤쪽 중복 무시, 양 잘못된 항목 건너뜀, 빈 줄은 변화 없음. `IsStepApplicable` 표(빈·중복·양 잘못·정상). `ExpansionBandRects`: 직사각형끼리 겹치지 않음, 넓이 합 = 넓힌 뒤 − 넓히기 전, 두 벽이 만나는 모서리 중심점이 정확히 한 직사각형에 듦, 범위 밖 index·빈 줄은 빈 목록 |
| 검증 (D2·D3) | 6.1 표 코드별 양성·음성: 빈 벽, 중복(오류 1개), 가격 0, 항목별 양, 홀 동쪽(통로 변) 항목 → `ExpansionTouchingSide`, 같은 줄 남 항목은 통과, 서쪽 항목 → `ExpansionEntranceSide` 경고, `ExpansionHallEffectShort`(효과 표 = 홀 줄 + 1은 통과, Authority 없으면 생략), 여러 벽 끝 모습 겹침 → `ExpansionOverlap`, `ExpansionStepsBelowCap` 없음, 미리보기 횟수와 무관한 `ValidateWorld` |
| Definition·열쇠걸이 (D2) | `ValidatePurchaseData`가 `Tiers` 규칙만 본다. 열쇠걸이가 쓰지 않는 줄까지 포함해 최대 열쇠 수 검사 |
| 공간 runtime | 남+북 줄 적용 뒤 `ZoneBounds` extent·상대 위치가 두 변 모두 반영, shell 바깥 직사각형이 새 직사각형을 덮음, `Stain` 공간 남+북+동 적용 뒤 새 조각 사각형 넓이 합 = 띠 넓이·모서리 덮음·기존 조각 identity 유지, `None` 공간 조각 0, `UndoExpansion` 원상, 이웃 shell identity 유지. `CanApplyNextExpansion` 거짓: 빈 벽·중복·양 잘못·가격 0. `GetNextExpansionPrice`·`IsAtExpansionLimit` |
| 구입 (D2) | 홀 1번째 = 홀 줄 0 가격, 목욕 1번째 = 목욕 줄 0 가격(fixture에서 서로 다름), 목욕 구입 뒤 홀 2번째 = 홀 줄 1 가격, 다른 공간 횟수·가격 불변, 여러 벽 홀 구입에 tier +1 한 번·열쇠 수 = 새 효과 열쇠 수, 홀 상한 뒤 홀 `SpaceMaxReached`·목욕 구입 가능, 모두 상한이면 `bAllAtLimit`, 같은 기대 횟수 두 번째 `StaleState`, 잔액 부족은 고른 공간 가격 기준, view 선택지 `AppliedCount`·`StepCount`·`NextPrice`·`NextSizeCm`(여러 벽 결과), 기존 실패 주입 되돌림 |
| 표시 모델 (D2) | 선택지 `확장 단계 N/M`·`다음 넓힘 N원`, 상한 선택지 가격 숨김·상태 문구, 선택 전 버튼 `확장 구입`·부족액 없음, 선택에 따라 버튼 가격·부족액이 바뀜, 일부 상한이면 최대 문구 없음, 모두 상한이면 최대 문구, 화면 헤더에 단계·가격 문구 없음 |
| widget | 확인이 `ConfirmAppliedCount`로 1회 구입, 연타 1회, 취소 경로(탭·사용 종료·취소) 기존 유지. `StageText`·`PriceText` 없는 option widget에서도 `ApplyModel` 안전 |
| 배치 미리보기 (D4) `BathhouseSim.Placement.PreviewHiddenWithoutAim` | 같은 세션에서: 구역 바닥 조준 → 미리보기 보임·후보 위치. (a) 하늘(hit 없음) (b) 배치 거리(Settings 값) 밖의 구역 (c) 배치 trace 채널을 막는 비구역 물체(벽 대용) 조준 → 미리보기 Actor 유효·`IsHidden()`·transform 불변, game world면 미리보기 mesh `ShouldRender()` 거짓, 격자 그대로, 쿼리 실패 문구 = 기존 `설치 가능한 구역을 바라보세요.`, `ConfirmPlacement` 실패·아이템 손에 있음·미리보기 Actor identity 유지. 다시 구역 바닥 → 보임·새 후보 위치·회전 입력 전과 같은 누적 Yaw. 공간 불허 구역·겹침 자리 조준 → 보임·invalid material(숨지 않음). 미리보기 시작 시 조준 없음 → 처음부터 숨김 |
| content(Editor 뒤) | `WBP_ExpansionSpaceOption`에 `StageText`·`PriceText`, `WBP_ExpansionScreen`에 `StageText`·`PriceText` 없음, `DA_ShopCatalog` 마지막 세 상품이 1·4·8칸 락커 정의 순서이고 판매·가격 > 0, 4·8칸 정의 `LockerSlotCount` 4·8·`Facility.Discardable` 없음, `DA_BathhouseExpansion_Default` `ValidatePurchaseData` 통과(옛 "Max Purchase Count ≥ 1" 단언 삭제). Level 넓힘 줄 값은 Editor 단계 Data Validation이 확인한다 |

회귀: 기존 Building·Expansion·Computer(`Computer.Input.*`)·Shop·FacilityPlacement·Economy 자동화. `ScreenWheelContentContract`는 Editor 작업 전후 모두 통과(optional binding 근거).

코드 리뷰 기준:

- 넓힘 줄 하나의 모든 벽이 한 번의 shell 재생성으로 적용되고 이웃 shell을 다시 만들지 않는다.
- 가격은 항상 고른 공간의 다음 줄 `Price`에서 오고 전체 구입 횟수·상한·순번 가격이 코드에 없다.
- 형상(`ExpandInterior`)과 적용 조건(`IsStepApplicable`)이 같은 규칙(첫 등장 벽, 양 유효)을 쓴다.
- 조각 띠가 모서리를 포함하고 겹치지 않는다.
- D4: 미리보기 숨김 판정이 후보 계산 여부 하나이고, 숨길 때 옮기지 않으며, Actor를 파괴하지 않는다. 미리보기 mesh에 hidden 상태 그림자 flag를 켜지 않는다.
- 400줄 파일 무변경(`BathhouseSpaceLayout.cpp`, `BathhouseSpaceValidation.cpp`), `PlayerFacilityPlacementComponent.cpp` 변경이 11.2 범위.
- 새 binding이 Optional이고 content 계약이 존재를 단언한다.
- 문구가 계약과 같다(`확장 단계 N/M`, `확장 구입 (N원)`, `N원 부족`, `이 공간은 더 넓힐 수 없습니다`, `최대 확장 단계입니다`, `확장을 사용할 수 없습니다`, `설치 가능한 구역을 바라보세요.`).

## 18. 사용자 PIE에서 관찰할 시나리오 (마스터 `PIE_CHECKLIST.md` 작성용)

- 대표: EXP-042, EXP-047, EXP-050.
- 나머지: EXP-021·022·023(재확인), 040·041·043·044·045·046·048, 051·052. 회귀: EXP-024~032 중 EXP-024·025·027·028·029.
- 관찰 메모:
  - 가격 합이 크다. 필요하면 PlayerState Blueprint `StartingMoney`를 저장하지 않고 임시로 올린다(U2와 같음).
  - EXP-047: 목욕공간 남동·북동 모서리 바닥·천장 이음새, 벽 틈, 깜빡이는 면, 모서리까지 욕탕 미리보기가 초록인지. 물 얼룩은 손님이 있는 곳에만 생기므로 모서리 생성은 시간이 걸린다.
  - EXP-045·048은 Editor에서 저장하지 않고 값을 바꾼 뒤 PIE, 끝나면 되돌린다. EXP-045의 홀 3번째 줄은 `Tiers` 4번째 줄(열쇠 수 ≤ 열쇠걸이 자리 수)도 함께 넣는다. 3번째 넓힘 끝 모습이 손님 길 범위 밖이면 검증 오류 로그가 남지만 구입·가격 확인에는 영향이 없다. EXP-048의 오류·경고는 `Space_Hall`을 저장하거나 우클릭 Validate로 본다.
  - EXP-046: 홀 2회(한도 8칸)에서 1칸 락커 2개가 설치돼 있으면 4칸은 설치되고(6/8) 8칸은 한도로 막힌다. 8칸 설치를 보려면 1칸 락커를 Q 길게 눌러 회수한 뒤(설치 0칸) 놓는다.
  - EXP-050·051: 시선을 들어 거리 밖, 계단 쪽 벽, 천장, 출입구 밖 지형, 하늘을 차례로 본다. 사라질 때 마지막 자리에 빨간 잔상이 없는지, 돌아올 때 회전이 유지되는지, 좌클릭해도 설치되지 않고 안내 문구가 나오는지.
  - 카드 문구(`확장 단계`, `다음 넓힘`, 홀 효과 두 줄)가 1024×576 화면에서 잘리지 않는지(Editor 단계가 눈으로 확인하지 못한 경우).
  - 손님이 넓어진 남·북·동 바닥까지 다니는지(몇 초 뒤, Recast Dynamic).

## 19. 운영 메모

- 구현 단계는 헤더·UPROPERTY를 바꾸므로 같은 프로젝트 Unreal Editor를 닫고 빌드한다(Live Coding 불가). 사용자가 BeekeepingSim Editor를 열어 둔 경우 실행 인자의 uproject 경로로 BathhouseSim Editor만 확인한다.
- 빌드 직후부터 Editor 단계 전까지 공간 Data Validation 오류와 "더 넓힐 수 없음" 표시는 정상 전환 상태다(10절).
- Editor 단계는 새 빌드로 MCP Editor를 띄운다(`-ModelContextProtocolStartServer`). 미리보기 횟수를 0으로 돌린 뒤 저장한다. 미리보기 글자 화면 판정은 숨김 Editor에서 하지 않는다(FBK-003).
- 아키텍처 단계에서 추가 Editor 조사는 필요 없었다. Nav 범위·끝 모습 띠의 Level Actor 유무는 값 입력 뒤에만 알 수 있어 Editor 단계 확인 항목으로 두었다(13.3의 4·5).
