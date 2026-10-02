# PROMPT_IMPLEMENTATION — EXP-U2 확장 구입 수직(홀 1회)

- 작업 ID: `EXP-U2`
- 단계: 아키텍처
- 상태: 완료
- 복귀 재설계(2026-10-02, Editor 작업 R1, RET-003): 넓힘 미리보기 글자의 위치·글꼴·지하 공간 표시. 18절. 6.4 해당 문장을 18절로 대체한다. 사용자 동작 변경 없음(글자 문구 `넓힘 미리보기 N회`·` · 겹침 있음` 유지)
- 복귀 재설계 2(2026-10-02, Editor 재확인 `REPORT_UNREAL_EDITOR.md` 10.3 R3, RET-004): 글자 component의 hidden-in-game 제거와 그리기·갱신 조건. 18.5절. 18.2·18.4의 해당 항목을 18.5가 대체한다
- 재설계(2026-10-02, 사용자 결정 D2, 상위 계약 `ef4d1db`): 전체 구입 상한 제거, 가격을 공간 넓힘 줄마다, 선택지별 `확장 단계 N/M`·가격 표시. **19절**. U2에서는 구현하지 않음. 사용자 결정으로 U2는 D2 없이 PIE 승인됐고(현재 Source `0536304`/`451ae42` 기준), 19절은 다음 단위(U3 묶음) 아키텍처의 입력이다. 0.1~0.5는 현재 상태, 0.6은 D2 예정이다. 5.1·6.6·7·8.3·8.5·11·15절은 현재 구현 기준이고, 19절과 다른 내용은 다음 단위에서 19절대로 바뀐다

- 상위 계약: [../PROMPT_ARCHITECTURE.md](../PROMPT_ARCHITECTURE.md)(상태 완료). 이번 범위는 EXP-020~032, 대표 흐름은 EXP-023(홀 확장 구입) → EXP-028(늘어난 한도로 1칸 락커 설치)이다.
- 선행 결과: `EXP-U1` 병합 `8ca6a24`. U1이 확정한 넓힘 목록·편집 미리보기 형태는 [../../../Architecture/BuildingSystem.md](../../../Architecture/BuildingSystem.md) Expansion 절이 정본이다. U1 Editor 데이터 관리 개요는 `git show 8ca6a24:.md/Work/EXPANSION-PURCHASE/EXP-U1/PROMPT_IMPLEMENTATION.md` 0절이다.
- 구조 정본: [../../../Architecture/ExpansionPurchaseSystem.md](../../../Architecture/ExpansionPurchaseSystem.md)(신규, 구입 상태·transaction·확장 탭·락커 판매), BuildingSystem.md(공간 넓힘 적용·검증·미리보기). 이 문서는 정본 내용을 반복하지 않고 구현 지시와 검증 기준을 적는다.
- 단계 시작 커밋 `6ef2200`, 브랜치 `work/EXP-U2`.

---

## 0. 사용자 확인용 — Editor 데이터 관리 변화 (U2)

U1 개요 0.2 표의 "U2 예정" 행이 이번에 실제가 된다. 수치는 적지 않는다. 기본 제안값은 상위 계약 4.7 표에 있고 Editor 작업 단계가 그 값으로 넣는다.

### 0.1 새로 생기거나 뜻이 바뀌는 값

| 값 | 원본(어디에) | Editor에서 바꾸는 법 | 단위·기준 |
|---|---|---|---|
| 확장 구입 가격 | `DA_BathhouseExpansion_Default` `Purchase Prices`(신규). 1번째 줄 = 1번째 구입 가격, 2번째 줄 = 2번째 구입 가격 | DataAsset 열기 → 줄 수정 | 원. 고른 공간과 무관 |
| 전체 구입 횟수 상한 | `DA_BathhouseExpansion_Default` `Max Purchase Count`(신규) | DataAsset 열기 | 회. 이 값만큼 가격 줄이 있어야 하고, 효과 표는 이 값 + 1줄(0회 포함)이 있어야 한다 |
| 홀 넓힘 횟수별 열쇠 수·락커 칸 한도 | `DA_BathhouseExpansion_Default` 기존 `Tiers`. 뜻이 "확장 단계별"에서 **"홀 넓힘 횟수별"**로 바뀐다(1번째 줄 = 홀 0회, 2번째 줄 = 홀 1회 …). 값과 줄 수는 그대로 쓴다 | 기존과 같음 | 개·칸. 홀을 넓힐 때만 다음 줄로 간다. 표 끝을 넘으면 마지막 줄 |
| 공간별 넓힘 방향·양 | 공간 Actor `Expansion Steps`(U1에서 형태 확정, 이번에 구현). 줄 = 그 공간의 몇 번째 넓힘(`Side`, `Amount Cm`) | `Space_Hall` 등 선택 → Details `Bathhouse Space › Expansion` | cm. 그 방향 벽 한 면만 그만큼 물러난다 |
| 공간별 넓힘 횟수 상한 | 같은 `Expansion Steps`의 줄 수 | 줄을 더하거나 지움 | 회 |
| 넓힘 미리보기 횟수 | 공간 Actor `Editor Preview Expansion Count`(편집 화면 전용, 저장 안 됨) | Details 숫자 입력 | 회, 0~줄 수 |
| 미리보기 글자 크기·높이·해상도 | Project Settings > Game > Bathhouse Building `Editor Preview Label World Size Cm`(글자 높이), `Editor Preview Label Height Cm`(가장 높은 천장 위 여유, 18절 신규), `Editor Preview Label Font Size`(글자 렌더 해상도, 18절 신규) | 설정 창 | cm, cm, 글꼴 크기. 편집 화면 표시 전용. 글꼴은 프로젝트 화면과 같은 엔진 기본 UMG 글꼴이라 따로 고르지 않는다 |
| 1칸 락커 상품 가격 | `DA_ShopCatalog`에 새 상품 줄 `ClothesLocker1`(`1칸 락커`, 목록 맨 뒤) | DataAsset 열기 → 그 줄 `Price` | 원 |

- 시작 상태(확장 0회, 열쇠 3개, 한도 2칸, 1칸 락커 2개)는 바뀌지 않는다. 확장 관리자 Level instance `ExpansionAuthority`의 `Initial Tier Index`는 0이어야 한다(0이 아니면 오류로 알린다. 시작은 항상 홀 0회다).
- "현재 확장 단계"는 지금까지 산 횟수이며 따로 저장하는 값이 없다(세 공간의 넓힌 횟수 합).

### 0.2 작업 흐름 예

- **2번째 구입 가격을 바꾸고 싶다:** `DA_BathhouseExpansion_Default` → `Purchase Prices` 2번째 줄.
- **확장을 3번까지 살 수 있게:** `Max Purchase Count`를 3으로 → `Purchase Prices`에 3번째 줄 추가 → `Tiers`에 홀 3회 줄(4번째 줄) 추가 → 공간들의 `Expansion Steps` 줄 수 합이 3 이상인지 확인. 빠뜨리면 저장할 때 오류·경고가 뜬다.
- **홀을 넓혔을 때 열쇠를 더 많이:** `Tiers`의 해당 줄 `Key Pool Size`. 열쇠걸이 자리 수(`BP_BathhouseKeyRack` `Pair Transforms` 개수)를 넘으면 `KeyRack`을 저장·검사할 때 오류가 뜬다.
- **홀 1번째 넓힘을 남쪽으로:** `Space_Hall` → `Expansion Steps` 1번째 줄 `Side` = 남(-Y). 목욕공간과 맞닿은 동쪽 벽을 고르면 오류, 출입구가 있는 서쪽 벽을 고르면 경고가 뜬다.
- **넓힌 모습을 미리 보기:** `Space_Hall` → `Editor Preview Expansion Count` = 1. 벽·바닥·천장·조명·배치 격자·조각 미리보기 선이 넓힌 모습을 따르고 공간 위에 `넓힘 미리보기 1회`가 뜬다. 0으로 돌리거나 레벨을 다시 열면 원래대로다. PIE는 항상 0회에서 시작한다. 미리보기를 켠 채 저장해도 게임에는 영향이 없지만, 저장 전에 0으로 돌리는 것을 권장한다.
- **1칸 락커 가격:** `DA_ShopCatalog` → `ClothesLocker1` 줄 `Price`.

### 0.3 잘못된 설정은 어떻게 알려 주나 (U2에서 추가)

- 확장 정의(`DA_BathhouseExpansion_Default` 저장·검사): 가격 줄이 전체 상한보다 적음, 가격이 0 이하, 효과 표 줄이 전체 상한 + 1보다 적음(기존: 효과 표 비어 있음, 줄끼리 줄어듦, 열쇠 < 락커 한도).
- 확장 관리자(`ExpansionAuthority` 검사): 확장 정의가 비었음, `Initial Tier Index` ≠ 0.
- 열쇠걸이(`KeyRack` 검사): 도달할 수 있는 효과 표 줄의 열쇠 수가 자리 수보다 많음.
- 공간 Actor(저장·검사, 미리보기 횟수와 무관하게 0회와 목록 끝 모습을 검사):
  - 오류: 넓힘 양이 0 이하, 0회에 다른 공간과 맞닿은 벽(통로 벽 포함)을 넓히는 줄, 모든 공간을 목록 끝까지 넓힌 모습끼리 겹침, 끝 모습에서 바깥 출입구 앞을 다른 공간이 막음, 넓힌 끝 손님 공간 바닥이 손님 길 범위 밖, 넓힌 끝 작업공간 바닥이 손님 길 범위 안
  - 경고: 바깥 출입구가 있는 벽을 넓히는 줄(출입구는 벽을 따라가지만 바깥 물건은 따라가지 않는다), 공간별 줄 수 합이 전체 상한보다 작음(전체 상한 전에 모든 선택지가 막힌다)
- 게임 중 확장 데이터가 없거나 잘못되면 확장 탭에 `확장을 사용할 수 없습니다`가 보이고 Output Log에 `LogBathhouseExpansion` Error가 원인별로 한 번 남는다.

### 0.4 새로 만들거나 바뀌는 asset

| 구분 | 대상 | 이번 단위에서 |
|---|---|---|
| 기존 DataAsset | `/Game/Bathhouse/Data/Expansion/DA_BathhouseExpansion_Default` | `Max Purchase Count`, `Purchase Prices` 채움. `Tiers`는 그대로 |
| 기존 DataAsset | `/Game/Bathhouse/Data/Shop/DA_ShopCatalog` | 맨 뒤에 `ClothesLocker1` 상품 줄 추가(설비 정의 `DA_FacilityPlacement_ClothesLocker_1`) |
| 기존 Level Actor | `Space_Hall`, `Space_Bath`, `Space_Work` | `Expansion Steps` 각 2줄(상위 계약 4.7 공간별 상한 기본 2) |
| 기존 Level Actor | `NavMeshBounds` | 넓힌 끝 홀·목욕공간을 덮지 않는다는 오류가 나오면만 크기 조정 |
| 새 Widget Blueprint | `/Game/Bathhouse/UI/WBP_ExpansionScreen`(parent `UExpansionScreenWidget`), `/Game/Bathhouse/UI/WBP_ExpansionSpaceOption`(parent `UExpansionSpaceOptionWidget`) | 만든다 |
| 기존 Widget Blueprint | `/Game/Bathhouse/UI/WBP_ComputerScreenRoot` | `확장` 탭 버튼과 Switcher 세 번째 화면 추가 |
| Project Settings | Bathhouse Building 미리보기 글자 값 3개(18절) | 원본 `Config/DefaultGame.ini`, 필요하면 조정 |

### 0.5 핵심 결정과 대안

| 결정 | 채택 | 버린 대안과 이유 |
|---|---|---|
| 넓힌 횟수 owner | 각 공간 Actor(runtime, 저장 안 함). 전체 구입 횟수는 세 공간 횟수의 합으로 계산 | 확장 관리자에 공간별 횟수 표: 공간 형상이 쓰는 횟수와 두 곳에 같은 값이 생겨 어긋날 수 있다 |
| 구입 조율 | Building의 새 world subsystem `UBathhouseExpansionPurchaseSubsystem` | 확장 관리자(Facility Actor)에 넣기: Facility가 Building·Economy를 알게 되고 Actor가 transaction까지 맡는다 |
| 열쇠·한도 | 기존 확장 관리자 tier를 "홀 넓힘 횟수"로 재해석, 홀 구입 때만 다음 tier | 새 표·새 관리자: 열쇠걸이·락커 한도가 이미 tier를 읽으므로 같은 경로를 다시 만들 이유가 없다 |
| 값 위치 | 가격·전체 상한·효과 표 = `DA_BathhouseExpansion_Default`, 공간 넓힘 = 공간 Actor, 락커 가격 = `DA_ShopCatalog`(U1 0.2 약속 그대로) | — |
| 확장 탭 화면 상태 | 위젯 표시 상태(선택·확인 대기·완료 표시). 구입 가능 판단은 매번 subsystem에 묻는다 | 화면이 가격·횟수를 보관: 손님 결제·다른 변화와 어긋난다 |
| 빠른 중복 클릭 | 화면은 확인 대기일 때만 구입을 요청하고, 요청에는 확인을 연 순간의 구입 횟수를 함께 보낸다. 횟수가 바뀌었으면 subsystem이 거절 | 화면 guard만: 같은 프레임 중복 이벤트·다른 경로 호출을 막지 못한다 |
| 목욕·작업공간 | 같은 구입 경로로 동작(공간 종류별 분기 없음, 홀만 효과 표 적용). U2는 세 공간 `Expansion Steps`를 모두 넣는다(EXP-021이 세 선택지의 전후 크기를 요구) | U2에서 홀만 허용: 임시 차단 코드를 넣었다가 U3에서 빼야 하고 EXP-021 표시를 만족할 수 없다 |

---

### 0.6 다음 단위 예정 변경 (사용자 결정 D2, 아직 Editor·Source에 없음)

위 0.1~0.5는 **지금 Editor·Source의 실제 상태**다(U2는 D2 없이 사용자 PIE 승인). 지금 가격은 `DA_BathhouseExpansion_Default` `Purchase Prices`(몇 번째 구입별)이고 전체 상한은 `Max Purchase Count`다. 다음 단위에서 19절대로 구현하면 이렇게 바뀐다.

| 값 | 지금 | 다음 단위(D2) 뒤 |
|---|---|---|
| 확장 구입 가격 | `DA_BathhouseExpansion_Default` `Purchase Prices`(몇 번째 구입별, 공간 무관) | 각 공간 Actor `Expansion Steps` 줄마다 `Price`(그 공간의 몇 번째 넓힘별). `Space_Hall` → Details `Bathhouse Space › Expansion › Expansion Steps` → 줄 펼쳐 `Price` |
| 상한 | 전체 `Max Purchase Count`와 공간별 줄 수 중 먼저 닿는 쪽 | 공간별 줄 수만. `Max Purchase Count`·`Purchase Prices`는 확장 정의에서 지운다 |
| 홀 효과 표 길이 | 전체 상한 + 1줄 이상 | 홀 넓힘 줄 수 + 1줄 이상(`Space_Hall` 검사가 알림) |
| 화면 | `현재 확장 단계: N`(전체 구입 횟수), 화면 아래 이번 구입 가격 | 선택지마다 `확장 단계 N/M`과 그 공간의 다음 넓힘 가격. 버튼 가격·부족액은 고른 공간 기준. `최대 확장 단계입니다`는 모든 공간이 상한일 때만 |

## 1. 기능 계약과 범위

- 현재 단계: 수직 구현(상위 작업의 두 번째 단위). 대표 흐름: 확장 탭에서 `홀`을 골라 1회 구입 → 홀이 넓어지고 열쇠·한도 상승 → 상점에서 1칸 락커를 사서 넓어진 홀에 설치.
- 시나리오: EXP-020~032 전부. 상위 계약 4.3~4.9의 U2 해당 동작, 4.6의 1칸 락커만.
- U3(EXP-040~046)는 설계하지 않는다. 다만 아래 구조는 U3가 데이터 추가와 검증만으로 얹을 수 있게 한다(16절 U3 확장 지점).

## 2. 목적·수용 기준·비목표

목적: 컴퓨터 `확장` 탭에서 돈을 내고 공간 하나를 넓히고, 홀이면 열쇠·락커 한도가 오르며, 상점에서 1칸 락커를 사서 설치할 수 있게 한다.

수용 기준(시나리오별 관찰 결과):

- EXP-020: 탭 순서 `관리 · 상점 · 확장`, 처음 진입 기본 탭 `관리`.
- EXP-021: 0회·잔액 충분 → `현재 확장 단계: 0`, `설치된 락커 칸 2/2`, 가격, 잔액, 세 선택지(전후 크기), 홀의 `열쇠 3개 → 4개`, `락커 칸 한도 2칸 → 4칸`(수치는 데이터에서), 구입 버튼 꺼짐.
- EXP-022: 잔액 < 가격 → 구입 버튼 꺼짐 + `N원 부족`. 손님 결제로 잔액이 가격 이상이 되면 다시 열지 않아도 켜짐.
- EXP-023: `확인` 순간 돈 1회 차감(HUD 변화량), 홀 바깥벽 물러남·바닥·천장·조명이 넓어진 홀을 덮음, 4번 열쇠 등장, 탭이 `확장 완료`, `현재 확장 단계: 1`, `설치된 락커 칸 2/4`.
- EXP-024: 넓어진 바닥을 걷고 홀 설비 미리보기 가능, 손님이 지나다니고 쓰레기가 생길 수 있음.
- EXP-025: 옛 홀 바깥벽 바로 안쪽의 플레이어·손님·물건이 밀리거나 끼이거나 사라지지 않음.
- EXP-026: 확인 대기 중 `취소`·다른 탭·컴퓨터 이탈 → 아무것도 안 바뀌고 다시 오면 구입 버튼부터.
- EXP-027: `확인` 연타 → 결제·넓힘 각 한 번.
- EXP-028: 확장 1회에서 1칸 락커 구입·배송·개봉·설치 → `설치된 락커 칸 3/4`, 손님 3명 이상 동시 락커 사용.
- EXP-029: 0회에서 1칸 락커 구매 가능, 설치는 `현재 확장 단계의 설치 가능한 락커 칸 수를 초과합니다`로 막히고 손에 남음. 홀 1회 뒤 다시 조준하면 설치됨.
- EXP-030: 락커 아이템은 목욕공간·지하에서 공간 불가 문구, 쓰레기 수거 구역에서 사라지지 않음.
- EXP-031: 확장 관리 데이터가 없으면 `확장을 사용할 수 없습니다`, 돈 불변.
- EXP-032: 관리·상점 탭 동작과 휠 스크롤이 지금과 같음.

비목표: 상위 계약 11절. 추가로 U3 범위인 4·8칸 락커 상품, 목욕·작업공간 넓힘의 수용 검증, 두 번째 구입·최대 표시·상한 조정의 수용 검증은 이번 PIE 대상이 아니다(같은 코드로 동작하지만 U3가 확인한다).

## 3. 설계에 맡김 항목의 결정 (상위 계약 12절)

| 항목 | 결정 | 근거 |
|---|---|---|
| 확장 횟수·공간별 넓힘 상태 owner | 공간 Actor `AppliedExpansionCount`(Transient). 전체 횟수 = 등록된 공간 합 | 단일 정본, 형상 owner와 같은 곳 |
| 결제·넓힘·열쇠·한도 묶기와 되돌림 | subsystem transaction: 사전 검사 → 공간 적용(되돌림 가능) → 돈 차감 → 홀이면 tier 상승 → 방송 1회. 7절 | 되돌릴 수 없는 열쇠 생성(tier)을 마지막에 둔다 |
| 기존 확장 단계 데이터와 홀 넓힘 횟수 연결 | tier index = min(홀 넓힘 횟수, 표 끝) | 열쇠걸이·락커 한도 경로 재사용 |
| 확장 탭 widget 구조·탭 root 확장·갱신 | root 탭 enum 3개, 새 native 화면·선택지 widget, 순수 표시 모델, delegate 구독 갱신. 8절 | Native Widget Policy |
| 상점 락커 판매 규칙 변경 | `FShopProductRules` 한 곳: non-locker는 `Facility.Discardable` 필수, locker는 그 태그 금지 | 기존 단일 규칙 위치 |
| 넓어질 때 끼임 방지 | 넓힘은 0회에 맞닿지 않은 바깥벽만, 새 형상은 옛 바깥 직사각형 밖에만 생김(형상 규칙과 검증). 넓힌 공간 shell만 같은 프레임에 다시 만든다 | U1 형상 규칙의 결과 |
| 공간 형상 갱신 방식 | 넓힌 공간 하나의 shell 전체 재생성(U1 계약 "전부 파괴 후 새로 만듦" 유지). 이웃 공간 계획은 넓힌 공간의 안쪽 직사각형을 읽지 않으므로 다시 만들지 않는다 | `FBathhouseSpaceLayout::BuildPlan`이 이웃의 Actor 위치·개구부·계단·바닥 Z만 읽음(Source 확인) |
| 손님 길 갱신 | Recast Dynamic이 생성 component 등록·파괴를 반영. 별도 호출 없음. BeginPlay에 목록 끝 범위 dirty 등록 1회(6.5) | U1 결정 유지 |
| 생성 조각 추가 | 넓힘 줄마다 늘어난 띠를 같은 분할 규칙으로 나눠 그 공간 조각 종류로 추가, 기존 조각 불변 | U1 확정 계약 |

## 4. 대상 시스템·파일과 책임 변화

| 대상 | 기존 책임 | 신규 책임 | 판단 |
|---|---|---|---|
| `ABathhouseSpaceActor`(Building) | 공간 authoring, 형상·조각 생성, 0회 구역 | `ExpansionSteps`, `EditorPreviewExpansionCount`, `AppliedExpansionCount`, 넓힘 적용·되돌림, 효과 횟수별 안쪽 직사각형 | 기존 확장. 새 메서드는 별도 cpp `BathhouseSpaceExpansion.cpp`(현재 cpp 269줄) |
| `FBathhouseSpaceLayout`(순수) | 형상 계획 | 넓힌 안쪽 직사각형·넓힘 띠 계산 | 새 cpp `BathhouseSpaceExpansionLayout.cpp`(현재 cpp 419줄, 추가 금지) |
| `FBathhouseSpaceValidation` | 0회 layout·Nav·world 검사 | 넓힘 검사, 0회·끝 모습 snapshot 분리 | 새 cpp `BathhouseSpaceExpansionValidation.cpp`(현재 `BathhouseSpaceValidation.cpp` 399줄, 추가 금지) |
| `UBathhouseSpaceShellComponent` | 생성 component 소유 | 편집 world 미리보기 글자(Transient `UWidgetComponent` + 코드로 만든 글자 widget, 18절) | 기존 확장 |
| `UBathhouseBuildingSettings` | 공용 형상 값 | `EditorPreviewLabelWorldSizeCm`, `EditorPreviewLabelHeightCm`, `EditorPreviewLabelFontSize`(18절) | 기존 확장 |
| `UBathhouseExpansionPurchaseSubsystem`(Building, 신규) | 없음 | 공간 등록부, 구입 화면 view, 구입 transaction, 변경 방송 | 신규 `UWorldSubsystem` |
| `UBathhouseExpansionDefinition`(Facility) | tier 표 | 가격 목록·전체 상한·검증 helper, tier = 홀 효과 표 | 기존 확장 |
| `ABathhouseExpansionAuthority`(Facility) | tier 상태·변경 방송 | Definition getter, Data Validation | 기존 확장(81줄) |
| `ABathhouseKeyRackActor`(Interaction) | tier에 맞춰 열쇠 생성 | Data Validation(열쇠 수 ≤ 자리 수) | 기존 확장 |
| `IComputerScreenContextReceiver`(Computer) | context·사용자 전달 | `NotifyComputerUseEnded()`(기본 빈 구현) | 기존 확장 |
| `ABathhouseComputerActor` | 예약·화면 context | 예약 해제 시 화면에 사용 종료 알림 | 기존 확장(216줄) |
| `UComputerScreenRootWidget`(UI) | 관리·상점 탭(bool) | 탭 3개 enum, 확장 화면 조립, 탭 이탈·사용 종료 시 확인 취소 | 기존 확장 |
| `UExpansionScreenWidget`, `UExpansionSpaceOptionWidget`(UI, 신규) | 없음 | 확장 탭 표시·입력 의도 | 신규 native widget |
| `FExpansionScreenModel`(UI private, 신규) | 없음 | view + 표시 상태 → 문구·활성·표시 여부(순수) | 신규, 자동화 대상 |
| `FShopProductRules`(Shop) | 설비 상품 = non-locker + Discardable | locker 상품 허용(Discardable 금지) | 규칙 한 줄 변경 |

새 의존: Building → Facility(Authority·Definition·FacilitySubsystem·LockerCapacitySubsystem), Building → Economy(wallet), UI → Building(구입 subsystem·view type). 순환은 없다(Facility·Economy는 Building을 모른다). 새 module·plugin은 없다(미리보기 글자는 기존 `UMG` 의존을 편집 world에서만 쓴다, 18절).

## 5. 확장 데이터 — Definition·Authority·열쇠걸이

### 5.1 `UBathhouseExpansionDefinition`

(D2 예정, 다음 단위 구현: `MaxPurchaseCount`·`PurchasePrices`와 그 규칙·API는 19.2가 삭제한다. 지금은 아래가 현재 구현이다.)

- 기존 `Tiers`(`FBathhouseExpansionTier` `KeyPoolSize`, `MaxInstalledLockerSlots`) 이름·형식 유지. tooltip을 "index = 홀 넓힘 횟수(0회부터). 표 끝을 넘으면 마지막 줄"로 바꾼다. 이름을 바꾸지 않으므로 redirect·migration이 없다.
- 신규 UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category `Expansion`, Korean tooltip):
  - `int32 MaxPurchaseCount`(ClampMin 0): 전체 확장 구입 횟수 상한. C++ 기본 0(값은 asset이 정본).
  - `TArray<int32> PurchasePrices`: index k = k+1번째 구입 가격.
- 신규 C++ API:
  - `int32 GetMaxPurchaseCount() const`
  - `bool TryGetPurchasePrice(int32 PurchaseIndex, int32& OutPrice) const`(index 범위 안이고 값 > 0일 때만 참)
  - `const FBathhouseExpansionTier* GetHallEffect(int32 HallExpansionCount) const` = `Tiers[Clamp(count, 0, Num-1)]`, 표가 비면 null
  - `int32 GetHallEffectIndex(int32 HallExpansionCount) const` = 같은 clamp, 표가 비면 `INDEX_NONE`
  - `bool ValidatePurchaseData(FText& OutFailureReason) const`: 아래 규칙을 runtime과 `IsDataValid`가 함께 쓴다.
- 규칙(모두 오류): `Tiers` 비어 있음, 기존 tier 규칙(열쇠 ≥ 한도, 줄마다 줄어들지 않음), `PurchasePrices.Num() < MaxPurchaseCount`, `0..MaxPurchaseCount-1` 가격 중 0 이하, `Tiers.Num() < MaxPurchaseCount + 1`. 상한보다 긴 가격 줄·효과 줄은 허용(쓰이지 않음).
- `IsDataValid`는 문제마다 별도 오류 문구(한국어)를 낸다. 기존 tier 오류 문구 형식을 따른다.

### 5.2 `ABathhouseExpansionAuthority`

- `UBathhouseExpansionDefinition* GetExpansionDefinition() const` 추가(C++ public).
- `IsDataValid`(WITH_EDITOR, CDO 제외): `ExpansionDefinition` 없음 → 오류, `InitialTierIndex != 0` → 오류("시작 확장 상태는 홀 넓힘 0회입니다"). `InitialTierIndex` property는 기존 asset 호환을 위해 지우지 않는다.
- `TryAdvanceToTier`, `OnExpansionTierChanged`, tier 상태와 subsystem 방송은 그대로다. 호출자는 구입 transaction뿐이다(BlueprintCallable은 유지하되 Content 사용처가 없다. 다른 경로로 tier가 바뀌면 7.2의 일관성 검사가 구입을 막는다).

### 5.3 `ABathhouseKeyRackActor`

- `IsDataValid`(WITH_EDITOR, CDO 제외, world 있음): 같은 world의 `ABathhouseExpansionAuthority`마다 Definition을 읽어 도달 가능한 효과 줄(index `0..Min(MaxPurchaseCount, Tiers.Num()-1)`)의 최대 `KeyPoolSize`가 `PairTransforms.Num()`보다 크면 오류("열쇠 수 {0}개가 열쇠걸이 자리 {1}개보다 많습니다"). Authority·Definition이 없으면 검사하지 않는다(Authority 쪽 오류가 알린다).
- runtime 열쇠 생성(`MaterializeToPoolSize`)은 바꾸지 않는다.

## 6. 공간 넓힘 — Building

### 6.1 데이터 (U1 확정 형태 그대로)

- `BathhouseSpaceTypes.h`에 `USTRUCT(BlueprintType) FBathhouseSpaceExpansionStep { EBathhouseSpaceSide Side; float AmountCm (ClampMin 0, ForceUnits cm); }`, Korean tooltip.
- `ABathhouseSpaceActor`:
  - `UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Bathhouse Space|Expansion") TArray<FBathhouseSpaceExpansionStep> ExpansionSteps;` tooltip: "줄 = 이 공간의 몇 번째 넓힘. 줄 수 = 공간별 넓힘 횟수 상한."
  - `#if WITH_EDITORONLY_DATA` 안 `UPROPERTY(Transient, EditInstanceOnly, Category = "Bathhouse Space|Expansion", meta = (ClampMin = "0")) int32 EditorPreviewExpansionCount = 0;` 읽을 때 항상 `[0, ExpansionSteps.Num()]` clamp. `PostEditChangeProperty`에서도 clamp해 Details 값이 줄 수를 넘지 않게 한다.
  - `UPROPERTY(Transient, VisibleInstanceOnly, Category = "Bathhouse Space|Expansion") int32 AppliedExpansionCount = 0;` runtime 정본. PIE Details에서 보기만 한다.

### 6.2 효과 횟수와 안쪽 직사각형

- 순수 helper(`FBathhouseSpaceLayout`, 새 cpp):
  - `static FBox2D ExpandInterior(const FBox2D& Base, const TArray<FBathhouseExpansionStepSnapshot>& Steps, int32 Count)`: Count를 `[0, Steps.Num()]`로 clamp하고 index `0..Count-1` 줄을 순서대로 적용. 줄 하나는 `Side` 변만 바깥으로 `AmountCm` 옮긴다. `AmountCm`가 0 이하·비유한인 줄은 형상에서 건너뛴다(검증 오류가 알린다).
  - `static FBox2D ExpansionBand(const FBox2D& Base, const TArray<...>& Steps, int32 StepIndex)`: `ExpandInterior(StepIndex)`와 `ExpandInterior(StepIndex+1)`의 차(한 변 쪽 띠 직사각형). 건너뛴 줄이면 면적 0.
- snapshot(`FBathhouseSpaceSnapshot`) 추가 필드: `FBox2D BaseInterior`(0회), `TArray<FBathhouseExpansionStepSnapshot> Steps`(`Side`, `AmountCm`), `int32 ExpansionCount`(이 snapshot이 뜻하는 횟수). `Interior`는 항상 `ExpandInterior(BaseInterior, Steps, ExpansionCount)`다.
- 순수 helper `static FBathhouseSpaceSnapshot WithExpansionCount(const FBathhouseSpaceSnapshot&, int32 Count)`: 횟수와 `Interior`만 바꾼 복사본.
- `ABathhouseSpaceActor`:
  - `int32 GetEffectiveExpansionCount() const`: world가 `EWorldType::Editor`면 clamp한 `EditorPreviewExpansionCount`, 그 밖(PIE·게임)은 `AppliedExpansionCount`. editor-only property는 `WITH_EDITORONLY_DATA`로 감싼다.
  - `FBox2D GetBaseInteriorRect() const`(0회, 기존 계산), `FBox2D GetInteriorRectForCount(int32) const`, `GetInteriorRect()` = 효과 횟수의 직사각형(주석 갱신).
  - `int32 GetAppliedExpansionCount() const`, `int32 GetExpansionStepCount() const`.
  - `FillSnapshot`은 위 필드를 채우고 `ExpansionCount = GetEffectiveExpansionCount()`.
  - `ApplyZoneGeometry`는 기존대로 `GetInteriorRect()`를 쓴다(그래서 편집 world 미리보기와 runtime 넓힘 모두 `ZoneBounds`가 따라간다).
- 위치 제안 `ApplyMove`는 `ActorXY`, `Interior`와 함께 `BaseInterior`도 옮긴다.

### 6.3 runtime 넓힘 적용·되돌림

```cpp
struct FBathhouseSpaceExpansionUndo
{
	int32 PreviousCount = INDEX_NONE;
	int32 PreviousChunkCount = 0;
	bool IsSet() const { return PreviousCount != INDEX_NONE; }
};

bool CanApplyNextExpansion(FText& OutFailureReason) const;
bool ApplyNextExpansion(FBathhouseSpaceExpansionUndo& OutUndo, FText& OutFailureReason);
void UndoExpansion(const FBathhouseSpaceExpansionUndo& Undo);
```

- `CanApplyNextExpansion`(부작용 없음): game world, `HasActorBegunPlay`, `AppliedExpansionCount < ExpansionSteps.Num()`, 다음 줄 `AmountCm` 유한 > 0, Rotation 0·Scale 1, Settings 상자 mesh load 가능, Settings 두께가 양수·유한. 실패 문구는 로그용 한국어.
- `ApplyNextExpansion`:
  1. `CanApplyNextExpansion` 재확인. 실패면 아무것도 바꾸지 않고 false.
  2. `OutUndo = {AppliedExpansionCount, CleaningChunks.Num()}`, `++AppliedExpansionCount`.
  3. `ApplyZoneGeometry()` → `RebuildShell()`(이 공간만). false면 `UndoExpansion(OutUndo)` 후 false.
  4. `CleaningChunkKind != None`이면 `ExpansionBand(Base, Steps, 이전 횟수)`를 `SplitChunks(띠, ChunkMaxSizeCm)`로 나눠 `FBathhouseCleaningChunkSpawner::Spawn`으로 `CleaningChunks`에 덧붙인다. class 없음은 기존처럼 오류 로그 후 건너뛴다(실패로 보지 않음, U1 BeginPlay와 같은 규칙).
  5. true.
- `UndoExpansion`: `CleaningChunks`의 `PreviousChunkCount` 이후 Actor를 파괴·제거, `AppliedExpansionCount = PreviousCount`, `ApplyZoneGeometry()` → `RebuildShell()`. 반복 호출에 안전하게 쓴다(Undo가 비었으면 no-op).
- 이웃 공간 shell은 다시 만들지 않는다(3절 근거). 자동화가 이웃 계획 불변을 확인한다.
- 넓힌 공간 shell은 같은 함수 호출 안에서 파괴 후 재생성되므로 물리 step 사이에 바닥이 비는 프레임이 없다. 형상은 옛 안쪽 직사각형 위치에서 같고 새 벽은 옛 바깥 직사각형 밖에만 생긴다(BuildingSystem Geometry Rules). 이미 놓인 설비·손님·물건 Actor는 건드리지 않는다.
- 배치 격자: `ZoneBounds` extent·상대 위치가 바뀌고, 다음 미리보기 세션의 `SetGridVisible(true)`가 격자 크기·DMI 값을 다시 계산한다(기존 코드). 추가 작업 없음.

### 6.4 편집 미리보기

- `RebuildShell`이 편집 world(`EWorldType::Editor`)에서 효과 횟수 > 0이면 `FBathhouseShellVisualInputs.PreviewLabel`에 `넓힘 미리보기 {N}회`를 넣고, 같은 snapshot 목록(효과 횟수 기준)에서 이 공간이 관련된 부피 겹침이 있으면 ` · 겹침 있음`을 덧붙인다. 겹침 판정은 내부 header `BathhouseSpaceValidationInternal.h`의 기존 `BathhouseSpaceValidationDetail::VolumesOverlap`을 재사용한다(새 cpp `BathhouseSpaceExpansion.cpp`에서 include, 400줄 파일 변경 없음).
- (2026-10-02 18절로 대체) shell이 만드는 글자 component·글꼴·위치는 18절이 정본이다. game world에서는 만들지 않고 default subobject를 추가하지 않는 계약은 유지한다.
- 미리보기 값 변경 → OnConstruction → 기존 `FBathhouseSpaceEditorSync::RequestRebuild`가 모든 공간을 다시 짓는다(통로 구멍 포함). 추가 작업 없음.

### 6.5 lifecycle 변경

- `BeginPlay`(game world): 기존 순서(`ApplyZoneGeometry` → `RebuildShell` → 조각 → 검증 로그) 뒤에 `UBathhouseExpansionPurchaseSubsystem::RegisterSpace(this)`. `AppliedExpansionCount`는 0으로 시작한다(편집 값·직렬화된 `ZoneBounds`와 무관).
- `BeginPlay`에서 이 공간을 목록 끝까지 넓힌 바깥 직사각형 × Z `[Zf − s, Zc + s]` 상자를 `UNavigationSystemV1::AddDirtyArea(…, ENavigationDirtyFlag::All)`로 한 번 등록한다. 이유: 편집 중 미리보기를 켠 상태의 Nav 데이터가 저장·PIE 복제되면 넓힘 띠에 낡은 Nav가 남을 수 있다. 게임 world의 실제 형상으로 다시 만들게 하는 보험이다. Nav system이 없으면 건너뛴다.
- `EndPlay`: `UnregisterSpace(this)` 후 기존 조각 파괴.
- 조각 spawn(`SpawnCleaningChunks`)은 BeginPlay에서 효과 횟수(=0) 안쪽 직사각형 전체를 쓴다(기존).

### 6.6 검증 (BuildingSystem Validation 표에 추가)

- `ValidateWorld`는 snapshot을 모은 뒤 **0회 복사본**(`WithExpansionCount(…, 0)`)으로 기존 `ValidateLayout`·`ValidateNavigation`을 돌린다(미리보기 횟수와 무관, 위치 제안도 0회 기준). 이어서 `ValidateExpansion`을 돌린다.
- `ValidateExpansion(Snapshots, NavBoxes, Inputs, MaxPurchaseCount, OutProblems)`(순수, 새 cpp). `MaxPurchaseCount`는 world의 Authority Definition 값, 없으면 `INDEX_NONE`(합계 경고 생략). 문제 코드 추가:

| 코드 | 판정 | 심각도 | owner |
|---|---|---|---|
| `ExpansionAmountInvalid` | 줄 `AmountCm` ≤ 0 또는 비유한 | Error | 그 공간 |
| `ExpansionTouchingSide` | 0회에 다른 공간과 바깥 직사각형이 그 변에서 닿음(두 공간 부피 Z 구간 `[Zf−s, Zc+s]`가 양의 길이로 겹치고, 변 좌표가 허용 오차 안에서 같고, 변 길이 방향 구간이 양의 길이로 겹침)인데 그 변을 `Side`로 쓰는 줄 | Error | 그 공간 |
| `ExpansionOverlap` | 모든 공간을 목록 끝까지 넓힌 복사본끼리 부피 겹침(닿음 허용, 기존 겹침 판정 재사용) | Error | 두 공간 중 index 작은 쪽, 상대 = 다른 쪽 |
| `ExpansionOutsideOpeningBlocked` | 끝 복사본에서 바깥 출입구 구간에 다른 공간이 닿음(기존 판정 재사용) | Error | 출입구 공간 |
| `ExpansionNavOutside` / `ExpansionNavWorkCovered` | 끝 복사본에 기존 Nav 규칙(손님 공간 바닥·출입구 앞 바깥 지점이 범위 안, Work 바닥이 범위 밖)을 적용 | Error | 그 공간 |
| `ExpansionEntranceSide` | 바깥 출입구가 있는 변을 `Side`로 쓰는 줄 | Warning | 그 공간 |
| `ExpansionStepsBelowCap` | `Σ ExpansionSteps.Num() < MaxPurchaseCount` | Warning | 홀 공간(없으면 첫 공간) |

- 직사각형은 커지기만 하므로 끝 모습끼리 검사로 모든 중간 조합의 겹침·막힘을 덮는다. 위치 제안은 넓힘 문제에 붙이지 않는다(고칠 곳이 `Expansion Steps`라서).
- 문구는 기존처럼 한국어이고 "목록 끝까지 넓힌 모습" 또는 "{k}번째 넓힘 줄"을 밝힌다. 문구 예: `홀(Space_Hall) 1번째 넓힘 줄이 목욕공간과 맞닿은 동(+X) 벽을 넓힙니다. 공간 사이 벽은 움직일 수 없습니다.`
- BeginPlay 로그(`LogValidationProblems`)도 같은 `ValidateWorld`를 쓰므로 새 문제를 함께 남긴다.

## 7. 구입 — `UBathhouseExpansionPurchaseSubsystem`

### 7.1 타입 (`Public/Building/BathhouseExpansionTypes.h`, non-reflected C++)

(D2 예정, 다음 단위 구현: 7.1~7.3의 전체 구입 횟수·전체 상한·구입 순번 가격·`ExpectedPurchaseCount`는 19.4대로 바뀐다. 지금은 아래가 현재 구현이다.)

```cpp
enum class EBathhouseExpansionFailure : uint8
{
	None, Unavailable, Busy, StaleState, MaxPurchasesReached,
	SpaceUnavailable, SpaceMaxReached, InsufficientMoney, ApplyFailed
};

struct FBathhouseExpansionOptionView
{
	EBathhouseSpaceKind Kind = EBathhouseSpaceKind::Hall;
	bool bPresent = false;            // 그 종류의 공간이 등록됨
	bool bCanExpand = false;          // 공간별 상한 전이고 다음 줄 적용 가능
	FVector2D CurrentSizeCm = FVector2D::ZeroVector;
	FVector2D NextSizeCm = FVector2D::ZeroVector;
	bool bHasHallEffect = false;      // 홀만
	int32 KeysNow = 0, KeysNext = 0, LockerLimitNow = 0, LockerLimitNext = 0;
};

struct FBathhouseExpansionView
{
	bool bAvailable = false;
	bool bBusy = false;               // transaction 진행 중(화면은 이 view를 적용하지 않음)
	int32 PurchaseCount = 0;
	int32 MaxPurchaseCount = 0;
	bool bMaxReached = false;
	int32 NextPrice = 0;
	int32 Balance = 0;
	int32 Shortfall = 0;              // max(0, NextPrice - Balance), 최대 도달·사용 불가면 0
	int32 InstalledLockerSlots = 0;
	int32 LockerSlotLimit = 0;
	FBathhouseExpansionOptionView Options[3]; // Hall, Bath, Work 순서
};
```

### 7.2 API와 상태

- `UBathhouseExpansionPurchaseSubsystem : UWorldSubsystem`(`Public/Building/BathhouseExpansionPurchaseSubsystem.h`). `Initialize`에서 `Collection.InitializeDependency<UBathhouseFacilitySubsystem>()` 후 `OnExpansionAuthorityChanged`를 구독해 `OnExpansionChanged`로 전달한다. `Deinitialize`에서 해제.
- 상태: 공간 등록부(종류별 weak 하나, 같은 종류 두 번째 등록은 `LogBathhouseExpansion` Error 후 무시), `bPurchasing`, 원인별 한 번 로그용 mutable 플래그.
- `void RegisterSpace(ABathhouseSpaceActor&)`, `void UnregisterSpace(ABathhouseSpaceActor&)`: 변경 시 `OnExpansionChanged` 방송.
- `int32 GetPurchaseCount() const` = 등록 공간 `GetAppliedExpansionCount()` 합.
- `FBathhouseExpansionView BuildView(const APlayerState* Buyer) const`
  - `bBusy = bPurchasing`.
  - 사용 가능 조건(하나라도 아니면 `bAvailable=false`, 원인별 Error 1회): Facility subsystem의 Authority가 있음, Definition 있음·`ValidatePurchaseData` 통과, 등록 공간이 하나 이상, Buyer의 `ABathhousePlayerState::GetWallet()` 있음, **tier 일관성**: `Authority->GetCurrentTierIndex() == Definition->GetHallEffectIndex(홀 적용 횟수)`(홀이 없으면 0회 기준).
  - `PurchaseCount`, `MaxPurchaseCount`, `bMaxReached = PurchaseCount >= MaxPurchaseCount`, `NextPrice`(최대 도달 아니면 `TryGetPurchasePrice(PurchaseCount)`), `Balance`, `Shortfall`.
  - 락커: `ULockerCapacitySubsystem::GetInstalledLockerCapacity()`, `UBathhouseFacilitySubsystem::GetMaxInstalledLockerSlots()`.
  - 선택지: 종류마다 등록 공간이 있으면 `CurrentSizeCm` = `GetInteriorRect()` 크기, `bCanExpand` = `CanApplyNextExpansion`, 가능하면 `NextSizeCm` = `GetInteriorRectForCount(적용+1)` 크기. 홀은 `bHasHallEffect=true`, `GetHallEffect(h)`·`GetHallEffect(h+1)`의 열쇠·한도.
- `EBathhouseExpansionFailure EvaluatePurchase(const APlayerState* Buyer, EBathhouseSpaceKind Kind, int32 ExpectedPurchaseCount, int32* OutPrice = nullptr) const`: 순서대로 `Busy`(bPurchasing) → `Unavailable` → `StaleState`(`ExpectedPurchaseCount != GetPurchaseCount()`) → `MaxPurchasesReached` → `SpaceUnavailable`(그 종류 없음) → `SpaceMaxReached`(`CanApplyNextExpansion` 실패) → `InsufficientMoney`(`!Wallet->CanSpendMoney(Price)`) → 홀이면 다음 tier 사전 검사(Authority 등록, `GetHallEffectIndex(h+1)` 유효, `> 현재`면 `TryAdvanceToTier`가 받아들일 index인지 `GetTier`로 확인. 실패는 `Unavailable`).
- `EBathhouseExpansionFailure TryPurchase(APlayerState* Buyer, EBathhouseSpaceKind Kind, int32 ExpectedPurchaseCount)`(동기).
- `FSimpleMulticastDelegate OnExpansionChanged`(C++ 전용): 구입 commit, 공간 등록·해제, Authority 등록 변경 때 한 번.
- 로그 category `LogBathhouseExpansion`(Building, 신규).

### 7.3 transaction 타임라인

| 순서 | 동작 | 실패 시 |
|---|---|---|
| 1 | `bPurchasing`이면 `Busy` | 변화 없음 |
| 2 | `EvaluatePurchase` | 그 실패 코드, 변화 없음 |
| 3 | `TGuardValue(bPurchasing, true)` | — |
| 4 | `Space->ApplyNextExpansion(Undo)`(형상·구역·조각) | 함수가 스스로 되돌림 → `ApplyFailed` |
| 5 | `Wallet->TrySpendMoney(Price)`(`OnMoneyChanged` 1회 → HUD 변화량) | `Space->UndoExpansion(Undo)` → `InsufficientMoney` |
| 6 | 홀이고 `GetHallEffectIndex(h+1) > 현재 tier`면 `Authority->TryAdvanceToTier(새 index)`(열쇠걸이가 새 열쇠·고리 생성, 락커 한도 상승) | 프로그래밍 오류 경로: Error 로그, `Wallet->TryAddMoney(Price)`, `UndoExpansion` → `ApplyFailed` |
| 7 | guard 해제 후 `OnExpansionChanged` 1회 | — |
| 8 | `None` 반환 | — |

- 되돌릴 수 없는 단계(tier 상승 → 열쇠 생성)를 마지막에 둔다. 2단계 사전 검사가 4·6단계 성공을 보장하므로 6단계 실패는 정상 데이터에서 일어나지 않는다. 그 경로의 환불은 HUD에 `+` 변화량을 한 번 더 보일 수 있으며 Error 로그로 남긴다.
- 5단계 `OnMoneyChanged` 동기 callback 중 화면이 `BuildView`를 부르면 `bBusy=true`라 화면은 적용하지 않는다(8.3). 같은 callback에서 `TryPurchase`를 다시 부르면 `Busy`다.
- 돈 차감 실패(4단계 뒤 잔액이 줄어든 재진입 등)는 형상을 되돌리므로 돈·공간·열쇠·한도·구역이 모두 구입 전과 같다(계약 4.9).
- `ExpectedPurchaseCount`가 같은 확인에서 온 두 번째 요청을 `StaleState`로 거절한다(EXP-027 domain 보장).

## 8. 확장 탭 UI

### 8.1 root `UComputerScreenRootWidget`

- `bShopSelected`를 private `enum class EComputerScreenTab : uint8 { Management, Shop, Expansion }` 상태로 바꾼다(기본 `Management`). Switcher index = enum 값(0·1·2).
- 신규 `UPROPERTY(meta = (BindWidgetOptional)) UButton* ExpansionTabButton`, `UPROPERTY(meta = (BindWidgetOptional)) UExpansionScreenWidget* ExpansionScreen`.
  - Optional 근거: `WBP_ComputerScreenRoot`는 이미 있는 asset이다. 필수 binding을 더하면 구현 ~ Editor 작업 사이에 이 WBP가 compile 오류가 되고, 이 WBP를 로드하는 기존 자동화(`ScreenWheelContentContract`)가 실패한다. 존재 여부는 새 content 자동화(15절)가 Editor 작업 뒤 확인한다. 기존 5개 binding은 그대로 필수다.
- 탭 버튼 3개는 기존처럼 선택된 탭만 비활성. 탭을 `Expansion`에서 다른 탭으로 바꾸면 먼저 `ExpansionScreen->CancelPendingConfirm()`. `Expansion`으로 바꾸면 `ExpansionScreen->RefreshFromDomain()`.
- `InitializeComputerScreen`·`NotifyComputerUserChanged`·`NativeConstruct`는 기존 두 화면과 같은 방식으로 `ExpansionScreen`에도 전달한다.
- `NotifyComputerUseEnded()` override: `ExpansionScreen->CancelPendingConfirm()`. 탭·선택 등 나머지 화면 상태는 유지(계약 4.8).

### 8.2 컴퓨터 사용 종료 알림

- `IComputerScreenContextReceiver`에 `virtual void NotifyComputerUseEnded() {}`(순수 가상 아님) 추가. 기존 구현(`UBathWaterManagementScreenWidget`, `UShopScreenWidget`, root)은 바꾸지 않아도 컴파일된다.
- `ABathhouseComputerActor::ReleaseReservation`이 실제로 `CurrentUser`를 지웠을 때만 screen user widget이 receiver면 `NotifyComputerUseEnded()`를 호출한다. 정상 이탈(blend 완료)과 강제 정리(예약 해제 포함) 모두 이 함수를 지난다. focus-out 시작 시점에 이미 hit testing이 꺼지므로 blend 동안 확인을 누를 수 없다.

### 8.3 (현재 구현. D2 예정 변경은 19.5·19.6) `UExpansionScreenWidget` (`Public/UI/ExpansionScreenWidget.h`, `UCLASS(Abstract, Blueprintable)`, `IComputerScreenContextReceiver`)

필수 BindWidget(새 asset이므로 모두 필수):

| 이름 | 타입 | 용도 |
|---|---|---|
| `StageText` | UTextBlock | `현재 확장 단계: {N}` |
| `BalanceText` | UTextBlock | `잔액 {N}원` |
| `LockerText` | UTextBlock | `설치된 락커 칸 {설치}/{한도}` |
| `OptionsPanel` | UPanelWidget | 세 선택지 묶음(최대·사용 불가면 Collapsed) |
| `HallOption`, `BathOption`, `WorkOption` | UExpansionSpaceOptionWidget | 종류는 C++가 NativeConstruct에서 지정 |
| `PriceText` | UTextBlock | `이번 구입 가격 {N}원` |
| `PurchasePanel` | UPanelWidget | 구입 버튼 묶음 |
| `PurchaseButton` | UButton | 클릭 → 확인 대기 |
| `PurchaseButtonText` | UTextBlock | `확장 구입 ({N}원)` |
| `ConfirmPanel` | UPanelWidget | `정말 구입할까요?`(WBP 고정 문구)와 두 버튼, PurchasePanel과 같은 자리 |
| `ConfirmButton`, `CancelButton` | UButton | 확인·취소 |
| `ShortfallText` | UTextBlock | `{N}원 부족` |
| `ResultText` | UTextBlock | `확장 완료` |
| `MessageText` | UTextBlock | `최대 확장 단계입니다` / `확장을 사용할 수 없습니다` |

- 표시 상태(widget 소유, domain 아님): `TOptional<EBathhouseSpaceKind> SelectedKind`, `bool bConfirmPending`, `int32 ConfirmPurchaseCount`(확인을 연 순간의 `PurchaseCount`), `bool bShowCompleted`.
- 구독: 현재 사용자 wallet `OnMoneyChanged`(dynamic), `ULockerCapacitySubsystem::OnLockerCapacityChanged`(dynamic), 구입 subsystem `OnExpansionChanged`(native handle). 사용자 변경 때 wallet만 다시 묶는다. `NativeDestruct`에서 모두 해제(대칭).
- `RefreshFromDomain()`: `BuildView(CurrentUser)` → `bBusy`면 아무것도 하지 않음 → 아니면 `FExpansionScreenModel::Build(View, 표시 상태)` 결과를 widget에 적용. 모델이 선택을 지워야 한다고 하면(선택지가 더 이상 고를 수 없음, 최대·사용 불가) `SelectedKind`를 지우고 확인 대기를 끈다.
- 입력:
  - 선택지 클릭(선택지 widget이 parent에 C++ delegate로 알림): 고를 수 있으면 `SelectedKind` 설정, `bShowCompleted=false`, 확인 대기 중이면 끔(다른 공간을 고르면 다시 구입 버튼부터).
  - `PurchaseButton`: 모델이 구입 가능이라고 할 때만 `bConfirmPending=true`, `ConfirmPurchaseCount = View.PurchaseCount`, `bShowCompleted=false`.
  - `ConfirmButton`: `bConfirmPending`이 아니면 무시. `bConfirmPending=false`로 먼저 내린 뒤 `TryPurchase(User, *SelectedKind, ConfirmPurchaseCount)`. `None`이면 `SelectedKind` 해제, `bShowCompleted=true`. 실패면 선택 유지·완료 표시 없음(새 view가 부족액 등을 보여 준다). 마지막에 `RefreshFromDomain()`.
  - `CancelButton`·`CancelPendingConfirm()`: `bConfirmPending=false` → refresh.
- `NotifyComputerUseEnded()`: `CancelPendingConfirm()`.
- Tick·timer·NativeTick override 없음.

### 8.4 `UExpansionSpaceOptionWidget` (`Public/UI/ExpansionSpaceOptionWidget.h`, `UCLASS(Abstract, Blueprintable)`)

- 필수 BindWidget: `SelectButton`(UButton, 카드 전체), `NameText`, `SizeText`, `EffectText`, `StatusText`(UTextBlock), `SelectionHighlight`(UWidget, 선택 표시).
- API: `void SetSpaceKind(EBathhouseSpaceKind)`, `void ApplyModel(const FExpansionOptionDisplay&)`, C++ delegate `FOnExpansionOptionClicked(EBathhouseSpaceKind)`. 버튼 바인딩은 NativeConstruct/Destruct 대칭.
- 비활성은 `SelectButton->SetIsEnabled(false)`, 선택은 `SelectionHighlight` 가시성(Visible/Collapsed, hit test 없음 `HitTestInvisible`). WBP graph 없음.

### 8.5 (현재 구현. D2 예정 변경은 19.5) `FExpansionScreenModel` (`Private/UI/ExpansionScreenModel.h/.cpp`, 순수)

- 입력: `FBathhouseExpansionView`, `TOptional<EBathhouseSpaceKind> Selected`, `bool bConfirmPending`, `bool bShowCompleted`.
- 출력 `FExpansionScreenDisplay`: 헤더 세 문구와 가시성, 선택지 3개 `FExpansionOptionDisplay{Name, Size, Effect, bEffectVisible, Status, bStatusVisible, bEnabled, bSelected}`, `bOptionsVisible`, `Price`·`bPriceVisible`, `bPurchasePanelVisible`, `PurchaseButtonText`, `bPurchaseEnabled`, `bConfirmPanelVisible`, `bConfirmEnabled`, `Shortfall`·`bShortfallVisible`, `bResultVisible`, `Message`·`bMessageVisible`, `bClearSelection`.
- 규칙:
  - 사용 불가: `MessageText` = `확장을 사용할 수 없습니다`, 선택지·가격·구입·확인·부족·단계·락커 문구 숨김, 잔액은 보임, `bClearSelection`.
  - 최대 도달: `MessageText` = `최대 확장 단계입니다`, 선택지·가격·구입·확인·부족 숨김, 단계·잔액·락커·완료 문구는 보임, `bClearSelection`.
  - 선택지: 공간 있고 `bCanExpand`면 Size = `{현재} → {다음}`, 고를 수 있음. 아니면 Size = `{현재}`(없으면 빈 문구), Status = `이 공간은 더 넓힐 수 없습니다`, 비활성. 선택된 것이 비활성이면 `bClearSelection`.
  - 크기 문구: `{X}m×{Y}m`, world X 먼저, cm ÷ 100, 소수 최대 1자리(`FNumberFormattingOptions` `MaximumFractionalDigits = 1`, 천 단위 구분 없음).
  - 홀 효과: `열쇠 {현재}개 → {다음}개` 줄바꿈 `락커 칸 한도 {현재}칸 → {다음}칸`. 홀이 아니거나 넓힐 수 없으면 숨김.
  - 공간 이름: `홀`·`목욕공간`·`작업공간` LOCTEXT 표(UENUM DisplayName은 game build에서 비므로 쓰지 않음).
  - 금액: `FText::AsNumber`(천 단위 구분) + `원`. 부족: `{Shortfall}원 부족`, `Shortfall > 0`일 때만(선택과 무관).
  - 구입 가능 = 사용 가능 && !최대 && 선택 있음 && 그 선택지 고를 수 있음 && `Shortfall == 0`.
  - 확인 대기면 PurchasePanel 숨김·ConfirmPanel 보임, `bConfirmEnabled` = 구입 가능. 아니면 반대이고 `bPurchaseEnabled` = 구입 가능.
  - 완료 문구 `확장 완료`는 `bShowCompleted`일 때(사용 불가가 아니면).
- 문구는 모두 C++ LOCTEXT(namespace `ExpansionScreen`). WBP 고정 문구는 `정말 구입할까요?`, `확인`, `취소`, 탭 라벨 `확장`뿐이다.

### 8.6 WBP 계약 (Editor 작업 단계)

- `WBP_ComputerScreenRoot`: `TabRow`의 `ShopTabButton` 뒤에 `ExpansionTabButton`(라벨 `확장`, 기존 탭 버튼과 같은 크기·간격), `ScreenSwitcher` child [2]에 `ExpansionScreen`(`WBP_ExpansionScreen_C`). 기존 child 0·1과 계층은 그대로.
- `WBP_ExpansionScreen`: 1024×576 화면 안(탭 바 아래 영역)에 8.3 binding을 배치. 헤더(단계·잔액·락커) → 세 선택지(가로 3열 권장) → 가격·부족·완료·메시지 → 구입/확인 자리. `ConfirmPanel`은 `PurchasePanel`과 같은 자리이며, `ConfirmButton`이 구입 버튼 바로 그 위치에 오지 않게 둔다(구입 버튼 더블클릭이 확인으로 이어지지 않게: 그 위치에는 `취소` 또는 안내 문구). 컨테이너는 `SelfHitTestInvisible` 또는 Visible, 버튼은 Visible.
- `WBP_ExpansionSpaceOption`: `SelectButton`이 카드 전체를 덮고 그 안에 문구 4개와 `SelectionHighlight`.
- 스크롤 영역은 필요 없다. 만들면 `COMPUTER-WHEEL-SCROLL` 계약(`ConsumeMouseWheel=WhenScrollingPossible`, `AnimateWheelScrolling=false`)을 따른다.
- WBP graph로 상태·domain 호출·선택 표시를 만들지 않는다.

## 9. 상점 — 1칸 락커 판매

- `FShopProductRules::ValidateDefinitions`의 설비 규칙을 바꾼다: `LockerSlotCount == 0`이면 `Facility.Discardable` 필수(기존), `LockerSlotCount > 0`이면 그 태그가 **없어야** 한다(Placement Data Validation과 같은 뜻). 문구: 비락커 = "설비 상품은 Facility.Discardable 설비여야 합니다.", 락커 = "락커 상품에는 Facility.Discardable 태그를 둘 수 없습니다." 그 뒤 `Facility->ValidateRuntime`은 그대로.
- 카트·주문·배송·개봉·운반·배치·회수 경로는 바꾸지 않는다. 락커 신규 설치 payload(`InstanceData` null)는 기존 `ABathhouseFacilityActor::ImportPlacementPayload` 신규 설치 분기와 락커 bank 등록 경로로 처리된다(Source 확인). 한도 초과 설치 거부도 기존 `CanInstallLockerSlots` 문구 그대로다.
- 락커 아이템은 `Facility.Discardable`이 없어 쓰레기 수거 구역·쓰레기통에서 버려지지 않는다(기존).
- Catalog 데이터(Editor): `DA_ShopCatalog` 맨 뒤에 `ProductId=ClothesLocker1`, `bForSale=true`, `DisplayName=1칸 락커`, `Price`(상위 계약 4.7 제안값), `PlacementDefinition=DA_FacilityPlacement_ClothesLocker_1`, `ItemBoxDefinition` 비움. 기존 20개 순서 유지.

## 10. lifecycle·전역 설정 영향

- Project/World/Input/Collision/Nav 설정 변경 없음. 미리보기 Settings 값의 원본은 `Config/DefaultGame.ini` `[/Script/BathhouseSim.BathhouseBuildingSettings]`다(18.2).
- 런타임 형상 재생성은 넓힌 공간 하나의 Static ISM·Movable 조명을 다시 만든다. Recast Dynamic이 그 범위 tile을 다시 만든다(손님 길 갱신). 나머지 공간·설비·손님·물리 물체에는 호출이 없다.
- World Partition: 공간 Actor는 항상 로드(`bIsSpatiallyLoaded=false`, U1)라 등록부가 셋을 모두 받는다.
- 저장 없음. 게임을 다시 시작하면 0회(계약 4.5).

## 11. Blueprint/API, Core Redirect와 Editor migration

- 신규 reflected: `FBathhouseSpaceExpansionStep`, 공간 `ExpansionSteps`·`EditorPreviewExpansionCount`(editor-only)·`AppliedExpansionCount`, Definition `MaxPurchaseCount`·`PurchasePrices`, Settings `EditorPreviewLabelWorldSizeCm`·`EditorPreviewLabelHeightCm`·`EditorPreviewLabelFontSize`(18절), 편집 전용 글자 widget class `UBathhouseSpacePreviewLabelWidget`(18절), root `ExpansionTabButton`·`ExpansionScreen`(optional), `UExpansionScreenWidget`·`UExpansionSpaceOptionWidget`과 그 BindWidget, `UBathhouseExpansionPurchaseSubsystem`.
- rename·삭제 없음 → Core Redirect 불필요. default subobject 추가 없음(미리보기 글자는 shell Transient 생성물). 모두 tagged property 추가라 기존 export와 호환된다. CoreSystem 규칙에 따라 Editor 작업 전 `DA_BathhouseExpansion_Default`, `WBP_ComputerScreenRoot`, 공간 external actor 3개를 새 빌드로 비저장 로드해 오류가 없는지 먼저 확인한다(기존 Blueprint load 자동화 또는 headless load).
- Editor 작업 단계 할 일(구현 단계가 `PROMPT_UNREAL.md`로 정리):
  1. `DA_BathhouseExpansion_Default`: `MaxPurchaseCount`, `PurchasePrices`(상한 수만큼)를 상위 계약 4.7 제안값으로. `Tiers` 그대로. 저장·재로드, Data Validation 오류 0.
  2. `Space_Hall`·`Space_Bath`·`Space_Work` `ExpansionSteps` 각 2줄, `AmountCm` = 4.7 "한 번 넓어지는 양" 제안값. `Side`는 0회에 맞닿은 변이 아니고 출입구 변이 아닌 쪽(홀은 남·북 중, 목욕공간은 동·남·북 중, 작업공간은 아무 변). 미리보기로 끝 모습을 확인하고, 끝 모습 바닥·새 벽 자리에 Level Actor(설비·장식·마당 물건)가 없는지 확인한다. 공간 3개 Data Validation 오류 0(경고는 기존 opt-out 2개만). 미리보기 0으로 돌린 뒤 external package 저장(Python `save_packages`), 새 프로세스 재로드.
  3. Nav 범위 오류가 나오면 `NavMeshBounds` 크기 조정(지하 제외 계약 유지). 편집 world에서 끝 모습 미리보기로 손님 길이 덮이는지 확인 후 미리보기 0.
  4. `WBP_ExpansionSpaceOption`, `WBP_ExpansionScreen` 생성, `WBP_ComputerScreenRoot` 수정(8.6). Compile 경고 없음.
  5. `DA_ShopCatalog` 1칸 락커 줄 추가(9절). `DA_FacilityPlacement_ClothesLocker_1`에 `Facility.Discardable`이 없음을 확인.
  6. `ExpansionAuthority` instance `InitialTierIndex` 0, `KeyRack` Data Validation 오류 0 확인.
  7. Unreal 정본 갱신: `Unreal/BuildingSystem.md`(넓힘 줄 원본 위치), `Unreal/ShopSystem.md`(락커 상품), `Unreal/InteractionUISystem.md`(탭 3개·새 WBP, 낡은 WidgetClass 기록 정정), `Unreal/FacilitySystem.md`(확장 정의 필드 원본 위치).

## 12. 조정값 원본과 상수 예외

| 값 | 원본 |
|---|---|
| 구입 가격, 전체 상한, 홀 효과 표 | `DA_BathhouseExpansion_Default` `PurchasePrices`·`MaxPurchaseCount`·`Tiers` |
| 넓힘 방향·양, 공간별 상한 | 공간 Level instance `ExpansionSteps` |
| 조각 최대 크기, 벽·판 두께, 미리보기 글자 크기·높이 여유·렌더 해상도 | `UBathhouseBuildingSettings` |
| 락커 상품 가격 | `DA_ShopCatalog` `ClothesLocker1.Price` |
| 열쇠걸이 자리 수 | `BP_BathhouseKeyRack` `PairTransforms` |
| HUD 변화량 표시 시간 | 기존 `WBP_MoneyHud` `DeltaDisplaySeconds` |

상수 예외(엔진 의미): cm→m 변환 100, 크기 문구 소수 최대 1자리(표시 형식), 부동소수 비교 `UE_KINDA_SMALL_NUMBER`, 효과 표 index clamp. 그 밖의 동작·감각 수치를 코드에 두지 않는다. 자동화는 기대값을 같은 원본(fixture Definition·snapshot 값)에서 계산한다.

## 13. 구현 금지 범위

- Content·Config 수정 금지(Editor 단계 몫). `PROMPT_UNREAL.md`에 위 Editor 할 일을 정리한다.
- U3 데이터(4·8칸 락커 상품) 추가 금지. 목욕·작업공간을 막는 임시 분기 금지(같은 경로).
- 이웃 공간 shell 재생성, 형상 부분 갱신(ISM instance 이동), Static component 이동 금지.
- 기존 `TryAdvanceToTier`·tier 의미 외 Authority 상태 추가 금지(전체 구입 횟수를 Authority에 저장하지 않는다).
- 휠 규칙·상점의 다른 규칙·락커 한도 판정 코드 변경 금지. 상점 화면에서 한도 초과 구매를 막지 않는다(계약 Q4).
- 확장 탭 widget에 Tick·timer·world scan·domain 상태 보관 금지. WBP graph에 상태·domain 호출 금지.
- 실제 사용자 focus 호출(`SetFocus`·`SetKeyboardFocus`·`SetUserFocus`·`SetWidgetToFocus`) 금지(ComputerSystem Keyboard Focus Invariant).
- 이미 400줄을 넘은 `BathhouseSpaceLayout.cpp`, 399줄 `BathhouseSpaceValidation.cpp`에는 새 규칙·helper를 넣지 않는다. 새 helper는 새 cpp, 공유 내부 판정은 `BathhouseSpaceValidationInternal.h`를 재사용한다. `BathhouseSpaceWorldValidation.cpp`(262줄)는 0회 복사본·`ValidateExpansion` 호출·Authority 상한 읽기만 더한다.

## 14. 사용자 PIE에서 관찰할 시나리오

- 대표: EXP-023 → EXP-028(홀 구입 후 1칸 락커 구입·배송·개봉·설치, `3/4`, 손님 3명 이상 동시 락커).
- 나머지: EXP-020~022, 024~027, 029~032.
- 관찰 메모(마스터 `PIE_CHECKLIST.md` 작성용):
  - EXP-022·029는 시작 잔액이 구입 가격과 락커 가격 합보다 적으면 손님 결제로 모은다. 필요하면 PlayerState Blueprint `StartingMoney`를 저장하지 않고 임시로 올린다.
  - EXP-024·025: 넓히는 순간 홀 안의 손님이 멈추거나 떨어지지 않는지, 옛 벽 자리 근처에 둔 물건이 그대로인지, 몇 초 뒤 손님이 넓어진 바닥을 지나는지(Recast Dynamic 갱신).
  - EXP-031: Editor에서 `ExpansionAuthority`의 `Expansion Definition`을 저장하지 않고 비운 채 PIE → `확장을 사용할 수 없습니다`, 끝나면 되돌림(저장 금지).
  - EXP-027: 확인 버튼 연타 후 돈 HUD 변화량 1회, 열쇠 하나만 추가.
- U2 PIE 범위 밖(U3): 목욕·작업공간 구입, 두 번째 구입, 최대 표시, 4·8칸 락커.

## 15. 자동화·빌드·코드 리뷰 기준

빌드: [UE_BUILD_POLICY.md](../../../UE_BUILD_POLICY.md) 고정 명령. 같은 프로젝트 Editor가 열려 있으면 빌드하지 않는다.

새 자동화(파일 `Private/Tests/BathhouseExpansionAutomationTests.cpp`, UI 모델은 `Private/Tests/ExpansionScreenAutomationTests.cpp`). 기대값은 fixture 값에서 계산한다.

| 영역 | 확인 |
|---|---|
| 순수 layout | `ExpandInterior` 0·1·2줄(각 변 방향), 잘못된 양 건너뜀, `ExpansionBand`가 두 직사각형 차이와 같음, 띠 조각 분할이 띠를 덮고 겹치지 않음, 넓힌 뒤 이웃 공간 `BuildPlan` 결과가 넓히기 전과 같음 |
| 검증 | 표 6.6 코드별 양성·음성(맞닿은 변·통로 변 오류, 반대 변 허용, 끝 모습 겹침, 끝 모습 출입구 막힘, Nav 끝 범위, 출입구 변 경고, 합계 경고와 상한 없음 생략), 미리보기 횟수를 바꿔도 `ValidateWorld` 결과 불변, 위치 제안이 0회 기준 |
| Definition | 가격 줄 < 상한, 가격 ≤ 0, 효과 줄 < 상한+1, 기존 tier 규칙, 상한보다 긴 표 허용, `GetHallEffect` clamp |
| Authority·열쇠걸이 | Definition 없음·`InitialTierIndex≠0` 오류, 도달 가능 열쇠 > 자리 수 오류, 도달 불가 줄 무시 |
| 공간 runtime | game world에서 효과 횟수 = 적용 횟수, 편집 world는 미리보기(clamp), `ApplyNextExpansion` 뒤 `ZoneBounds` extent·상대 위치, shell part가 넓힌 바깥 직사각형을 덮음, 띠 조각 수·종류(`None`이면 0), 기존 조각 identity 유지, 이웃 shell component identity 유지, `UndoExpansion` 뒤 횟수·구역·조각 수·형상 원상 |
| 구입 transaction | 성공: 돈 정확히 가격만큼 1회(`OnMoneyChanged` 1회), 고른 공간만 +1, 홀이면 tier +1·열쇠걸이 열쇠 수 = 새 효과 열쇠 수·한도 = 새 효과 한도, 다른 공간이면 tier·열쇠 불변, `OnExpansionChanged` 1회, `GetPurchaseCount` +1 |
| 실패·되돌림 | 잔액 부족, `StaleState`(같은 기대 횟수 두 번째 호출), 공간별 상한, 전체 상한, Authority 없음·Definition 무효·tier 불일치 → `Unavailable`, `OnMoneyChanged` callback 안 재호출 → `Busy`, 강제 실패 주입(자동화 friend로 4·6단계 실패) → 돈·횟수·구역·조각·형상·tier 원상 |
| 락커 | 0회 한도에서 신규 1칸 락커 설치 거부·아이템 유지, 홀 구입 뒤 같은 아이템 설치 성공·설치 칸 +1 |
| 상점 규칙 | 락커(Discardable 없음) 상품 유효·카트 담기·주문·개봉 성공, 락커+Discardable 무효, 비락커 Discardable 없음 무효(기존 `ShopAutomationTests` catalog 검사에 단언 추가) |
| UI 모델 | 사용 불가·최대·선택 전·선택 후·부족·확인 대기·확인 중 부족·완료·고를 수 없는 선택지·선택 해제, 크기 문구 형식(정수·소수 1자리), 홀 효과 문구, 금액 형식 |
| UI widget(native, WBP 없이) | 확장 화면 확인 대기 → `CancelPendingConfirm`·`NotifyComputerUseEnded`로 취소, root 탭 전환이 취소 호출(friend로 native 화면 주입), 확인 두 번 → 구입 1회 |
| content(Editor 작업 뒤 실행) | `ExpansionScreenContentContract`: `WBP_ComputerScreenRoot`에 `ExpansionTabButton`·`ExpansionScreen`, Switcher child 3개, `WBP_ExpansionScreen`·`WBP_ExpansionSpaceOption` load, `DA_ShopCatalog` 마지막 상품이 1칸 락커 정의, `DA_BathhouseExpansion_Default` `ValidatePurchaseData` 통과 |

회귀: 기존 Building·Computer(`Computer.Input.*` 포함)·Shop·FacilityPlacement·Economy 자동화 통과. `ScreenWheelContentContract`는 Editor 작업 전후 모두 통과해야 한다(optional binding 근거).

코드 리뷰 기준:

- 7.3 순서(되돌릴 수 없는 tier 상승이 마지막), 실패 경로가 부분 변경을 남기지 않음, 방송 횟수.
- 공간 횟수가 공간 Actor 한 곳에만 있고 전체 횟수를 저장하지 않음.
- 편집 미리보기가 game world에 새지 않음(Transient, editor-only, 효과 횟수 분기).
- 이웃 shell을 다시 만들지 않음, 이미 놓인 Actor를 건드리지 않음.
- 화면이 domain 상태를 보관하지 않고 view를 매번 subsystem에서 받음, delegate 대칭 해제, Tick 없음.
- 문구가 계약과 같음(`현재 확장 단계: N`, `설치된 락커 칸 a/b`, `확장 구입 (N원)`, `정말 구입할까요?`, `N원 부족`, `확장 완료`, `이 공간은 더 넓힐 수 없습니다`, `최대 확장 단계입니다`, `확장을 사용할 수 없습니다`).
- 400줄 파일에 새 규칙을 넣지 않음, 새 의존이 4절 목록 안.

## 16. U3 확장 지점 (설계하지 않음, 막지 않음)

- 목욕·작업공간 넓힘, 두 번째 구입, 최대 표시, 공간별·전체 상한 조정은 이번 경로와 데이터로 성립한다. U3는 데이터 확인과 PIE 수용(EXP-040~045)을 맡는다.
- 4·8칸 락커는 `DA_ShopCatalog`에 줄 두 개 추가(9절 규칙이 이미 허용). 설치 칸 수는 기존 락커 bank 등록이 센다.
- 홀 2회 효과(열쇠 5~8)는 기존 `Tiers` 3번째 줄과 열쇠걸이 자리 8개로 성립한다.

## 17. 운영 메모

- 구현 단계는 헤더·UPROPERTY를 바꾸므로 같은 프로젝트 Unreal Editor를 닫고 빌드한다(Live Coding 불가). 사용자가 BeekeepingSim Editor를 열어 둔 경우 실행 인자의 uproject 경로로 구분해 BathhouseSim Editor만 확인한다.
- Editor 작업 단계는 새 빌드로 MCP Editor를 띄운다(`-ModelContextProtocolStartServer`). 미리보기 횟수를 0으로 돌린 뒤 저장한다.
- Editor 사실 추가 조사는 필요 없다(사전 조사 1·2차와 U1 Unreal 정본으로 충분).

## 18. 복귀 재설계 — 넓힘 미리보기 글자 (2026-10-02, Editor 작업 R1, RET-003)

근거: [REPORT_UNREAL_EDITOR.md](REPORT_UNREAL_EDITOR.md) 4절 R1. 문제는 셋이다. (a) 글자가 천장 판 윗면과 같은 평면이라 렌더되지 않는다. (b) `UTextRenderComponent`가 엔진 기본 `RobotoDistanceField`를 써서 한글이 네모로 나온다. (c) 작업공간(지하) 글자가 홀 바닥·지형 아래라 위에서 보이지 않는다. 방향(위에서 읽힘, 글자 위쪽 = 북)은 맞았다.

### 18.1 결정

| 문제 | 결정 | 버린 대안 |
|---|---|---|
| (b) 글꼴 | `UTextRenderComponent`를 쓰지 않는다. 대신 editor-only Transient `UWidgetComponent`(World space)에 코드로 만든 native 글자 widget(`UTextBlock` 하나)을 띄운다. 글꼴은 `UTextBlock` 기본 글꼴을 그대로 쓴다. 프로젝트의 모든 WBP가 쓰는 엔진 기본 UMG 글꼴(`/Engine/EngineFonts/Roboto` composite)이고, 한글은 엔진 fallback `Faces/DroidSansFallback`이 그린다. Content에 글꼴 asset이 없는데도 컴퓨터 화면이 이 글꼴로 한글을 표시하므로 같은 원본을 쓴다. 글꼴 object는 바꾸지 않고 크기만 정한다 | 한글 offline 글꼴 asset 새로 만들기: `UTextRenderComponent`는 offline 글꼴만 그린다(엔진 `TextRenderComponent.cpp`가 runtime 글꼴을 거부한다). Hangul 전체를 굽는 큰 atlas asset이 필요하고 Content 변경이다. 문구를 영문으로 바꾸기: U1에서 사용자가 승인한 표시 문구가 바뀐다 |
| (a) 높이 | 모든 공간 글자를 같은 높이에 둔다. Z = (현재 효과 횟수 snapshot에서 **모든 공간 중 가장 높은 천장 판 윗면** `Zc + s`) + Settings `EditorPreviewLabelHeightCm`. 여유가 0보다 크므로 어떤 판 윗면과도 같은 평면이 아니다 | 판 두께만큼 띄우기: 판 두께는 형상 값이라 글자 여유와 뜻이 다르고, 판 두께를 바꾸면 글자 위치도 바뀐다 |
| (c) 지하 | 위 높이 규칙이 곧 "위로 투영"이다. 지하 공간 글자도 가장 높은 지상 천장 위에 뜬다. 두 글자가 겹치지 않게 붙는 쪽을 나눈다. XY 바깥 직사각형이 양의 넓이로 겹치는 다른 공간 중 바닥 Z가 더 높은 공간이 있으면 아래층이다. 아래층 글자는 안쪽 직사각형 중심의 **남쪽**에, 위층 글자는 **북쪽**에 붙인다(글자 가장자리가 중심점에 닿는다). 문구는 바꾸지 않는다 | 지하 글자를 자기 천장에 두고 깊이 무시(전경)로 렌더: deferred 렌더에서 일반 primitive의 전경 DPG·깊이 무시가 보장되지 않는다. 글자에 공간 이름 붙이기: 승인 문구가 바뀐다 |

### 18.2 변경 범위와 구현 지시

- 새 class `UBathhouseSpacePreviewLabelWidget : UUserWidget`(`Private/Building/BathhouseSpacePreviewLabelWidget.h/.cpp`, `UCLASS(NotBlueprintable)`, 편집 world에서만 생성).
  - `Initialize()` override: `Super::Initialize()` 뒤 `WidgetTree->RootWidget`이 없으면 `WidgetTree->ConstructWidget<UTextBlock>()`을 root로 둔다. native class라 WidgetTree가 비어 있고, `NativeOnInitialized`는 편집 world(player context 없음)에서 불리지 않으므로 쓰지 않는다.
  - `void SetLabel(const FText& Text, int32 FontSize)`: root `UTextBlock`에 문구를 넣고 가운데 정렬한다. 글꼴은 그 `UTextBlock`의 현재 `FSlateFontInfo`를 복사해 `Size`만 바꿔 `SetFont`한다(글꼴 object 유지).
  - UMG API만 쓴다. Slate 직접 사용은 금지다(CoreSystem). WBP를 만들지 않는다(Content 변경 금지가 근거이며, UISystem Native Widget Policy의 편집 전용 예외로 기록).
- `UBathhouseBuildingSettings`(Category `Editor Preview`, Config, 한국어 tooltip에 게임 영향 없음 명시):
  - 기존 `EditorPreviewLabelWorldSizeCm`: tooltip 뜻을 "글자 높이(대략, cm)"로 갱신.
  - 신규 `float EditorPreviewLabelHeightCm`(ClampMin 1): 가장 높은 천장 판 윗면에서 글자까지의 여유.
  - 신규 `int32 EditorPreviewLabelFontSize`(ClampMin 1): 글자를 그리는 글꼴 크기(렌더 해상도).
  - 세 값의 원본은 `Config/DefaultGame.ini` `[/Script/BathhouseSim.BathhouseBuildingSettings]`다(코드 리뷰 3회차 결정, U1 Building Settings 관례·Project Settings 저장 위치). C++ 초기값은 Config 키가 없을 때의 예비값이다.
- 순수 helper(`FBathhouseSpaceLayout`, `BathhouseSpaceExpansionLayout.cpp`): `static FBathhousePreviewLabelPlacement PreviewLabelPlacement(const TArray<FBathhouseSpaceSnapshot>& Snapshots, int32 Index, double SlabThicknessCm, double HeightCm)` → `{ FVector Location; bool bSouthOfCenter; }`.
  - Location XY = `Snapshots[Index].Interior` 중심. Z = 사용 가능한 모든 snapshot의 `CeilingZ + SlabThicknessCm` 최댓값 + `HeightCm`.
  - `bSouthOfCenter` = 다른 snapshot 중 `OuterRect`가 XY에서 양의 넓이로 겹치고 `FloorZ`가 더 높은 것이 있음(허용 오차 `UE_KINDA_SMALL_NUMBER`).
- `ABathhouseSpaceActor::RebuildShell`(편집 world, 효과 횟수 > 0): 지금의 위치 계산(`CeilingZ + SlabThickness`)을 위 helper로 바꾼다. `FBathhouseShellVisualInputs`에 `PreviewLabelLocation`, `bPreviewLabelSouthOfCenter`, `PreviewLabelWorldSizeCm`, `PreviewLabelFontSize`를 채운다. 문구 생성(`BuildExpansionPreviewLabel`)은 그대로다.
- `UBathhouseSpaceShellComponent`: `UTextRenderComponent` 생성 코드를 `UWidgetComponent` 생성으로 바꾼다.
  - 기존 계약 유지: `RF_Transient`, `CreationMethod = UserConstructionScript`, `bIsEditorOnly = true`, Movable, `NoCollision`, `SetCanEverAffectNavigation(false)`, shell에 attach, 같은 정리 경로(`PreviewLabelComponent`), `bPreviewChunks`(편집 world)일 때만. (18.5로 정정: `SetHiddenInGame(true)`는 쓰지 않는다. 그리기 설정은 18.5.)
  - 위젯 설정: `Space = World`, `bDrawAtDesiredSize = true`, `bReceiveHardwareInput = false`, `WidgetClass = UBathhouseSpacePreviewLabelWidget`, 위젯 재질은 엔진 기본값 유지. Pivot은 북쪽 붙임이면 글자 아래 가장자리 가운데, 남쪽 붙임이면 위 가장자리 가운데를 component 원점에 맞춘다. 등록 뒤 `GetUserWidgetObject()`에 `SetLabel(문구, PreviewLabelFontSize)`를 호출한다.
  - 방향: 글자 앞면이 world +Z를 보고 글자 위쪽이 world +Y(북)를 향한다(위에서 읽힘, 지금과 같은 결과). `UWidgetComponent`의 앞면·위쪽 축은 `UTextRenderComponent`와 다르므로 회전값은 구현이 엔진 축에서 정하고 자동화로 확인한다.
  - 크기: component relative scale = `PreviewLabelWorldSizeCm / PreviewLabelFontSize`(균일, 글꼴 크기 1당 cm). 0 이하·비유한이면 글자를 만들지 않는다.
  - `GetPreviewLabelText()`는 shell이 저장한 마지막 문구를 돌려준다(Slate가 없는 headless에서도 확인할 수 있게).
- 파일: `Public/Building/BathhouseBuildingSettings.h`, `Public/Building/BathhouseSpaceShellComponent.h`, `Private/Building/BathhouseSpaceShellComponent.cpp`, `Private/Building/BathhouseSpaceActor.cpp`(위치 계산 몇 줄), `Private/Building/BathhouseSpaceLayout.h`(helper 선언·결과 struct), `Private/Building/BathhouseSpaceExpansionLayout.cpp`, 신규 `Private/Building/BathhouseSpacePreviewLabelWidget.h/.cpp`, 자동화 `Private/Tests/BathhouseExpansionAutomationTests.cpp`. `BathhouseSpaceLayout.cpp`·`BathhouseSpaceValidation.cpp`는 바꾸지 않는다.

### 18.3 영향 시나리오와 유지 범위

- 영향: 사용자 PIE 시나리오는 없다(편집 world 전용 표시). Editor 작업 단계의 편집 world 미리보기 확인 항목만 다시 본다.
- 유지: 넓힘 데이터·runtime 넓힘·구입·확장 탭·락커 판매·검증, Editor 단계에서 저장한 모든 Content(`22e20c7`), 미리보기 문구, 겹침 표시 규칙, game world에 글자가 없다는 계약. R2(`Service.BlueprintLoad` 상품 수 리터럴)는 이 설계 범위 밖이며 구현이 따로 고친다.
- Content 변경 없음. Editor 단계가 바꿀 수 있는 것은 위 세 Settings 값의 원본(`Config/DefaultGame.ini`)뿐이고, 지금 값으로 잘 보이면 바꾸지 않는다.

### 18.4 자동화·검증 기준

| 확인 | 기준 |
|---|---|
| 순수 placement | 공간 하나: Z = 자기 `CeilingZ + s + HeightCm`. 홀·목욕(같은 바닥)·지하: 세 글자 Z가 모두 (가장 높은 천장 판 윗면 + HeightCm)로 같음. 지하는 남쪽, 홀·목욕은 북쪽. XY = 효과 횟수 안쪽 중심. 기대값은 fixture snapshot과 인자에서 계산 |
| 편집 world shell(18.5 그리기 조건 기준 추가) | 미리보기 > 0이면 `UWidgetComponent`가 하나 있다: editor-only, Transient, NoCollision, Nav 비관련, `WidgetClass` = 글자 widget class, 위치 = helper 결과, 앞면 축 = +Z·위쪽 축 = +Y, scale = 크기/글꼴 크기, `GetPreviewLabelText()` = `넓힘 미리보기 N회`(겹침 시 ` · 겹침 있음`). Slate가 초기화된 환경이면 user widget의 `UTextBlock` 문구가 같고 글꼴 object가 `UTextBlock` CDO 기본 글꼴과 같은지(override 없음)도 확인 |
| game world | 같은 공간에 글자 component 없음(기존) |
| 코드 리뷰 | `UTextRenderComponent` 사용 없음, Slate 직접 사용 없음, 글꼴 object 변경 없음, 높이·크기·해상도 리터럴 없음(Settings), 400줄 파일 무변경 |
| Editor 재확인(asset 변경 없음) | 편집 world에서 홀 미리보기 1: 홀 위에 한글 `넓힘 미리보기 1회`가 위에서 읽힌다. 작업공간 미리보기 1: 홀 지붕 위(중심 남쪽)에 보인다. 홀·작업공간 동시: 두 글자가 겹치지 않는다. 0으로 돌린 뒤 저장할 asset이 없다 |

### 18.5 복귀 재설계 2 — 그리기·갱신 조건 (2026-10-02, RET-004)

근거: [REPORT_UNREAL_EDITOR.md](REPORT_UNREAL_EDITOR.md) 10.3. 위치·pivot·방향·scale은 계약대로였지만 render target이 만들어지지 않았다. 18.2가 `SetHiddenInGame(true)`를 유지 항목으로 남긴 것이 원인이다. 엔진 `USceneComponent::IsVisible()`은 `bHiddenInGame`이면 world 종류와 무관하게 false다(`SceneComponent.cpp` `IsVisible`). `UWidgetComponent::ShouldDrawWidget()`은 `IsVisible()`일 때만 그린다(`WidgetComponent.cpp` `ShouldDrawWidget`). 18.4 자동화는 component 구조만 보고 그리기 조건을 보지 않아 놓쳤다.

#### 결정

- hidden-in-game을 쓰지 않는다. 글자는 `bPreviewChunks`(편집 world)일 때만 만들어지고 Transient·editor-only라 PIE 복제·cook에 나가지 않는다. 그래서 "game world에 글자 없음"은 생성 조건으로 지켜진다.
- 갱신을 화면 렌더 기록에 의존시키지 않는다. `TickWhenOffscreen = true`, `TickMode = Enabled`, 자동 redraw(`bManuallyRedraw = false`, `RedrawTime = 0`)로 두고, `SetLabel` 뒤 `RequestRedraw()`를 한 번 부른다. 첫 그리기는 `LastWidgetRenderTime == 0`이라 허용되지만, 이후는 `TickWhenOffscreen`이 `WasRecentlyRendered`(편집 world의 world 시간·viewport 렌더 기록) 판단을 건너뛰게 한다. 매 편집 tick 다시 그리는 비용은 미리보기를 켠 공간당 작은 글자 하나뿐이다(편집 전용, 미리보기 0이면 component 없음).
- 아래 엔진 그리기 조건을 component 생성 계약으로 모두 만족시키고 자동화가 확인한다.

#### 엔진 소스로 확인한 그리기 조건(UE 5.8 `UWidgetComponent`)

| 조건 | 출처 | 계약 |
|---|---|---|
| 편집 world에서 component tick이 돈다 | `ActorComponent.cpp` `RegisterComponentWithWorld`가 비게임 world에서도 tick 함수 등록, `FActorComponentTickFunction::ExecuteTick`이 `bTickInEditor`로 `LEVELTICK_ViewportsOnly` 허용, `UWidgetComponent` 생성자가 `bCanEverTick`·`bTickInEditor` true | 바꾸지 않는다. tick을 끄지 않는다 |
| `TickMode != Disabled` | `TickComponent`가 Disabled면 tick을 끔. 기본값은 cvar에 따라 Enabled/Automatic | `SetTickMode(ETickMode::Enabled)` 명시 |
| 위젯 생성 | `OnRegister`가 편집 world에서 `InitWidget()`. `InitWidget`은 `WidgetClass`가 있고 `FSlateApplication`이 초기화돼 있을 때만 `CreateWidget` | `WidgetClass`를 `RegisterComponent` **전에** 지정 |
| `WidgetRenderer` 존재 | `OnRegister`가 `Space != Screen`이고 `!GUsingNullRHI`일 때만 생성 | `SetWidgetSpace(World)`를 등록 전에 지정. `-nullrhi`에서는 그리지 않음(엔진) |
| `SlateWindow` 존재 | `UpdateWidget`(매 tick)이 Slate 초기화 때 생성하고 `TakeWidget`으로 내용 지정 | 조치 없음 |
| `IsVisible()` | `bHiddenInGame` false, `bVisible` true, level collection 보임 | hidden-in-game 쓰지 않음, visibility 끄지 않음 |
| 위젯 보임 | `IsWidgetVisible`: World space면 component·`SlateWindow` 보임, 그리고 `UUserWidget::IsVisible()`(cached Slate widget 보임). Automatic tick에서 안 보이면 tick을 끔 | root `UTextBlock`과 user widget visibility를 Collapsed·Hidden으로 두지 않음. `WindowVisibility` 기본값 유지 |
| 그리기 시점 | `ShouldDrawWidget`: `IsVisible()` && (`TickWhenOffscreen` \|\| `WasRecentlyRendered(0.5)` \|\| `LastWidgetRenderTime == 0`) && 경과 ≥ `RedrawTime` && (manual이면 `bRedrawRequested`) | `TickWhenOffscreen` true, `RedrawTime` 0, manual 아님 |
| draw size | `DrawWidgetToRenderTarget`은 `DrawSize`가 0 이하이거나 최대 2D texture 크기를 넘으면 그리기 전에 return한다(`bDrawAtDesiredSize`여도 먼저 검사) | `DrawSize`는 엔진 기본값(양수)을 그대로 둔다. 0으로 바꾸지 않는다 |
| desired size | `bDrawAtDesiredSize`면 prepass 뒤 desired size로 render target 크기를 정하고, 0 이하면 render target이 없다 | 문구가 비면 component를 만들지 않음(기존). 글꼴 크기 Settings는 1 이상(ClampMin) |
| render target | `UpdateRenderTarget`이 크기가 양수면 `UTextureRenderTarget2D`를 만들고 위젯 재질 parameter에 넣음 | 조치 없음. Editor 재확인 대상 |
| 앞면 | 위젯 재질은 기본 one-sided(`bIsTwoSided` false). 앞면이 +Z | 18.2 방향 계약 유지 |

#### 변경 범위·수정 파일·유지 범위

- 수정 파일: `Private/Building/BathhouseSpaceShellComponent.cpp`(글자 component 설정: hidden-in-game 제거, `SetTickMode(Enabled)`, `SetTickWhenOffscreen(true)`, 등록 전 `WidgetClass`·Space 지정 확인, `SetLabel` 뒤 `RequestRedraw()`), `Private/Tests/BathhouseExpansionAutomationTests.cpp`(아래 기준). 다른 파일 변경 없음. Content·Config 변경 없음. Settings 값은 그대로(원본 `Config/DefaultGame.ini`).
- 영향 시나리오: 사용자 PIE 없음(편집 world 전용). Editor 단계의 편집 world 미리보기 재확인만 다시 한다.
- 유지: 18.1~18.3 결정(글꼴·높이·남북 붙임·문구), 18.2 나머지 component 계약, 넓힘·구입·확장 탭·락커 판매·검증, Editor 단계에서 저장한 Content(`22e20c7`), game world에 글자 없음.

#### 자동화 기준(18.4에 추가)

| 확인 | 기준 |
|---|---|
| 그리기 조건(headless 가능) | 편집 world 글자 component에서 `bHiddenInGame` false, `IsVisible()` true, `GetWidgetSpace() == World`, `GetTickWhenOffscreen()` true, `IsComponentTickEnabled()` true, `bTickInEditor` true, `PrimaryComponentTick.IsTickFunctionRegistered()` true, `GetDrawSize()` 두 축 양수, `GetWidgetClass()` = 글자 widget class. 엔진 `ShouldDrawWidget`은 protected라 이 공개 조건 묶음을 같은 판정으로 본다 |
| 위젯 보임(Slate 초기화 환경) | user widget이 있고 root `UTextBlock`·user widget visibility가 Collapsed·Hidden이 아님 |
| render target(RHI 필요, Editor 재확인) | 편집 world에서 몇 tick 뒤 `GetRenderTarget()`이 null이 아니고 크기가 양수, `GetMaterialInstance()`가 있음, 화면에 한글 문구가 보임. `-nullrhi` headless에서는 엔진이 그리지 않으므로 자동화 단언 대상이 아니다 |
| 코드 리뷰 | 글자 component에 `SetHiddenInGame`·visibility 끄기·`SetTickMode(Disabled)`·`DrawSize` 0 지정 없음 |

## 19. 재설계 — 사용자 결정 D2 (2026-10-02)

**U2에서는 구현하지 않음. 사용자 결정으로 U2는 D2 없이 PIE 승인됐고, 이 절은 다음 단위 아키텍처의 입력이다.** 다음 단위는 U3와 사용자 추가 지시(여러 방향 넓힘, 배치 미리보기 거리 밖 숨김)를 함께 다루며, 그 단위 아키텍처가 이 절을 다시 검토해 확정한다. 아래 "수정 파일"·"Editor 단계 변경"·"자동화 기준"은 그때의 출발점이다.

입력: 상위 [../PROMPT_ARCHITECTURE.md](../PROMPT_ARCHITECTURE.md) 3절 D2 행, 2.2·4.3·4.4·4.5·4.7·12절, EXP-021·022·023·042~045(커밋 `ef4d1db`). 사용자 선택은 "공간별 상한만"과 "공간별·횟수별 가격"이고, 선택지 설명은 "공간 Actor의 넓힘 줄마다 가격"이었다. 새로 정할 사용자 동작은 없다. 이 절은 D2 이후 이름·규칙·값 위치의 정본이다(FBK-006). 패킷이나 다른 절과 다르면 이 절을 따른다.

### 19.1 결정

| 항목 | 결정 | 버린 대안과 이유 |
|---|---|---|
| 가격 위치 | U1 승인 형태 `FBathhouseSpaceExpansionStep`(`Side`, `AmountCm`)에 `int32 Price`를 더한다. 줄 하나 = 그 공간의 k번째 넓힘의 방향·양·가격(계약 4.7 "넓힘마다 양·방향과 함께 정한다") | DataAsset에 공간별 가격 표: 넓힘 줄 수와 가격 줄 수를 따로 맞춰야 하고, 한 공간의 값이 Level과 DataAsset 두 곳으로 갈린다 |
| 상한 | 공간별 상한 = `ExpansionSteps.Num()`만 남긴다. 전체 상한과 구입 순번은 쓰지 않는다 | — |
| 확장 정의 필드 | `MaxPurchaseCount`·`PurchasePrices`를 **삭제**한다(deprecated로 두지 않음). 이름 있는 tagged property라 옛 asset을 load하면 없는 property 값은 건너뛴다. 이름 변경이 아니라 Core Redirect가 필요 없다. 사용처는 이 DA 하나이고 병합 전이다. Editor 단계가 새 빌드로 load·재저장해 남은 값을 파일에서 없앤다 | deprecated 유지: Details에 쓰이지 않는 가격·상한 칸이 남아 사용자가 어디서 가격을 바꾸는지 헷갈린다 |
| 홀 효과 표 길이 | 홀 `ExpansionSteps.Num()` + 1 이상(계약 4.7 D2). DA와 Level을 함께 봐야 하므로 공간 world 검증이 `Space_Hall` 오류로 알린다 | DA 검증: DA는 Level의 홀 줄 수를 모른다 |
| 중복 확인 방지 | 기대값을 "고른 공간의 넓힌 횟수"로 바꾼다. 그 횟수가 가격도 정하므로 확인을 연 뒤 가격이 바뀐 요청도 거절된다 | 전체 구입 횟수: D2로 의미가 없다 |
| 열쇠걸이 검증 | 효과 표 **모든 줄**의 열쇠 수 ≤ 자리 수 | 도달 가능한 줄만: 열쇠걸이(Interaction)가 홀 줄 수(Building)를 알게 되는 새 의존이 생긴다. 쓰지 않는 줄까지 막는 것은 보수적이라 허용한다 |

### 19.2 Building·Facility 데이터 변경

- `BathhouseSpaceTypes.h` `FBathhouseSpaceExpansionStep`에 `UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Bathhouse Space", meta = (ClampMin = "0", ToolTip = "이 넓힘의 구입 가격(원). 그 공간의 몇 번째 넓힘인지별, 전체 구입 순번과 무관.")) int32 Price = 0;`을 추가한다. C++ 기본 0은 "입력 안 됨"이고 검증 오류로 알린다(값 원본은 Level instance).
- `FBathhouseExpansionStepSnapshot`에 `int32 Price`. `FillSnapshot`이 채운다.
- `ABathhouseSpaceActor`:
  - `int32 GetNextExpansionPrice() const`: 다음 줄이 있으면 그 `Price`, 없으면 0.
  - `bool IsAtExpansionLimit() const` = `AppliedExpansionCount >= ExpansionSteps.Num()`.
  - `CanApplyNextExpansion`에 "다음 줄 `Price` > 0" 조건을 더한다.
- `UBathhouseExpansionDefinition`: `MaxPurchaseCount`, `PurchasePrices`, `GetMaxPurchaseCount`, `TryGetPurchasePrice`와 그 검증 규칙을 지운다. `ValidatePurchaseData`(이름 유지)는 `Tiers` 규칙(비어 있음, 열쇠 ≥ 한도, 줄마다 줄지 않음)만 본다. `GetHallEffect`·`GetHallEffectIndex`는 유지한다. `Tiers` tooltip에서 "전체 상한" 문구를 빼고 "줄 수는 홀 넓힘 줄 수 + 1 이상"으로 바꾼다.
- `ABathhouseKeyRackActor::IsDataValid`: 도달 가능 줄 계산(`GetMaxPurchaseCount` 사용)을 없애고 `Tiers` 모든 줄의 최대 `KeyPoolSize`와 `PairTransforms.Num()`을 비교한다.

### 19.3 검증 변경

- `ValidateExpansion`의 인자 `int32 MaxPurchaseCount`를 `int32 HallEffectRowCount`로 바꾼다. world의 Authority Definition `Tiers.Num()`이고, Authority나 Definition이 없으면 `INDEX_NONE`(검사 생략). `BathhouseSpaceWorldValidation.cpp`가 이 값을 읽는다.
- 문제 코드:
  - 삭제: `ExpansionStepsBelowCap`.
  - 추가 `ExpansionPriceInvalid`(Error, owner = 그 공간): 줄 `Price` ≤ 0. 문구 예 `홀(Space_Hall) 2번째 넓힘 줄의 가격이 0 이하입니다.`
  - 추가 `ExpansionHallEffectShort`(Error, owner = 홀): `HallEffectRowCount != INDEX_NONE`이고 `홀 줄 수 + 1 > HallEffectRowCount`. 문구 예 `홀 넓힘 효과 표(DA_BathhouseExpansion_Default Tiers)가 {n}줄입니다. 홀 넓힘 {m}개에는 {m+1}줄이 필요합니다.`
- 나머지 넓힘 검사(6.6)는 그대로다.

### 19.4 구입 subsystem·view 변경

- `EBathhouseExpansionFailure`에서 `MaxPurchasesReached`를 지운다(C++ 전용 enum).
- `FBathhouseExpansionView`: `PurchaseCount`, `MaxPurchaseCount`, `bMaxReached`, `NextPrice`, `Shortfall`을 지우고 `bool bAllAtLimit`(등록된 공간이 하나 이상이고 모두 `IsAtExpansionLimit`)을 더한다.
- `FBathhouseExpansionOptionView`에 `int32 AppliedCount`, `int32 StepCount`, `bool bAtLimit`, `int32 NextPrice`(넓힐 수 있을 때만 양수)를 더한다.
- `UBathhouseExpansionPurchaseSubsystem`:
  - `GetPurchaseCount()`를 지운다.
  - `BuildView`: 사용 가능 조건에서 Definition 규칙은 `Tiers` 규칙만이다(나머지 조건은 7.2 그대로). 선택지마다 위 필드를 채운다.
  - `EvaluatePurchase(const APlayerState* Buyer, EBathhouseSpaceKind Kind, int32 ExpectedAppliedCount, int32* OutPrice = nullptr)` 순서: `Busy` → `Unavailable` → `SpaceUnavailable`(그 종류 없음) → `StaleState`(`ExpectedAppliedCount != 그 공간 GetAppliedExpansionCount()`) → `SpaceMaxReached`(`CanApplyNextExpansion` 실패) → `InsufficientMoney`(`!Wallet->CanSpendMoney(Space->GetNextExpansionPrice())`) → 홀 다음 tier 사전 검사(7.2 그대로). 가격은 항상 `GetNextExpansionPrice()`다.
  - `TryPurchase(APlayerState* Buyer, EBathhouseSpaceKind Kind, int32 ExpectedAppliedCount)`. 7.3 타임라인은 그대로이고 가격 출처만 바뀐다.
- 자동화 실패 주입·방송 규칙은 그대로다.

### 19.5 확장 탭 표시 변경 (`FExpansionScreenModel`)

- 표시 상태: `ConfirmPurchaseCount`를 `ConfirmAppliedCount`(확인을 연 순간 고른 공간의 넓힌 횟수)로 바꾼다. 확인은 `TryPurchase(사용자, 선택, ConfirmAppliedCount)`.
- 화면 헤더: `현재 확장 단계: N`과 화면 단위 `이번 구입 가격 N원`을 없앤다. 잔액·`설치된 락커 칸 a/b`는 그대로.
- 선택지:
  - `확장 단계 {AppliedCount}/{StepCount}`. 공간이 없으면 숨긴다.
  - 넓힐 수 있으면 `다음 넓힘 {NextPrice}원`과 크기 `{현재} → {다음}`. 고를 수 있다.
  - 넓힐 수 없으면 가격을 숨기고 크기는 현재만, `이 공간은 더 넓힐 수 없습니다`, 비활성. 상한 도달(`2/2`)과 데이터 문제 모두 같은 문구다.
  - 홀 효과 문구는 그대로다.
- 구입 버튼: 고른 선택지가 고를 수 있으면 `확장 구입 ({그 공간 NextPrice}원)`, 아니면 `확장 구입`(꺼짐).
- 부족액: 고른 선택지가 고를 수 있고 잔액 < 그 가격일 때만 `{가격 − 잔액}원 부족`. 선택 전에는 보이지 않는다. 선택을 바꾸면 그 공간 가격으로 다시 계산한다.
- 구입 가능 = 사용 가능 && `!bAllAtLimit` && 선택 있음 && 그 선택지 고를 수 있음 && 잔액 ≥ 그 가격. 확인 대기 중 `bConfirmEnabled`도 같은 판정이다.
- `bAllAtLimit`이면 `최대 확장 단계입니다`가 보이고 선택지·구입·확인·부족액이 숨는다. 잔액·락커·완료 문구는 보인다. 일부 공간만 상한이면 이 문구는 나오지 않는다.
- 사용 불가 표시는 8.5 그대로다(단계 문구가 없어졌을 뿐).

### 19.6 Widget binding 변경

- `UExpansionScreenWidget`: BindWidget `StageText`, `PriceText`를 **지운다**.
- `UExpansionSpaceOptionWidget`: `UPROPERTY(meta = (BindWidgetOptional)) UTextBlock* StageText`(`확장 단계 N/M`), `UPROPERTY(meta = (BindWidgetOptional)) UTextBlock* PriceText`(`다음 넓힘 N원`)를 더한다. 표시 struct `FExpansionOptionDisplay`에 `Stage`, `bStageVisible`, `Price`, `bPriceVisible`.
  - Optional 근거(8.1과 같음): 이미 있는 `WBP_ExpansionSpaceOption`에 필수 binding을 더하면 구현~Editor 사이에 compile 오류가 난다. 그러면 root가 품은 이 WBP를 로드하는 `ScreenWheelContentContract`도 실패한다. 존재는 content 자동화가 Editor 작업 뒤 확인한다.
- 표시 조건 확인(FBK-004): 새 글자는 이미 그려지고 있는 컴퓨터 화면(게임 world `UWidgetComponent`) 안의 `UTextBlock`이다. 새 component·숨김 flag·tick 변화가 없다. 가시성은 C++가 Visible/Collapsed로만 바꾼다. 글꼴은 다른 화면 글자와 같은 기본 UMG 글꼴이라 한글이 나온다. 남는 위험은 선택지 카드에 두 줄이 늘어 1024×576 안에서 잘리는 것이다. Editor 단계가 화면 캡처로 확인한다.

### 19.7 수정 파일

- Building: `Public/Building/BathhouseSpaceTypes.h`, `Public/Building/BathhouseSpaceActor.h`, `Private/Building/BathhouseSpaceExpansion.cpp`, `Private/Building/BathhouseSpaceActor.cpp`(`FillSnapshot`의 가격 한 줄), `Private/Building/BathhouseSpaceLayout.h`(snapshot 필드), `Private/Building/BathhouseSpaceValidation.h`(코드·인자), `Private/Building/BathhouseSpaceExpansionValidation.cpp`, `Private/Building/BathhouseSpaceWorldValidation.cpp`, `Public/Building/BathhouseExpansionTypes.h`, `Public/Building/BathhouseExpansionPurchaseSubsystem.h`, `Private/Building/BathhouseExpansionPurchaseSubsystem.cpp`.
- Facility·Interaction: `Public/Facility/BathhouseExpansionDefinition.h`, `Private/Facility/BathhouseExpansionDefinition.cpp`, `Private/Interaction/BathhouseKeyRackActor.cpp`.
- UI: `Public/UI/ExpansionScreenWidget.h`, `Private/UI/ExpansionScreenWidget.cpp`, `Public/UI/ExpansionSpaceOptionWidget.h`, `Private/UI/ExpansionSpaceOptionWidget.cpp`, `Private/UI/ExpansionScreenModel.h/.cpp`.
- 자동화: `Private/Tests/BathhouseExpansionAutomationTests.cpp`, `Private/Tests/BathhouseExpansionAutomationTestSupport.h`, `Private/Tests/ExpansionScreenAutomationTests.cpp`, (`Expansion.Content.ScreenContract` 포함).
- `BathhouseSpaceLayout.cpp`·`BathhouseSpaceValidation.cpp`(400줄 근처)는 바꾸지 않는다. Config 변경은 없다.

### 19.8 Editor 단계 변경 (allowlist)

1. `/Game/Bathhouse/Data/Expansion/DA_BathhouseExpansion_Default`: 새 빌드로 load해 오류가 없는지 확인한다. 재저장해 지운 `MaxPurchaseCount`·`PurchasePrices` 값을 없앤다. `Tiers`는 그대로 두고(3줄) 새 프로세스에서 재로드한다.
2. `Space_Hall`·`Space_Bath`·`Space_Work`(external package는 `.md/Unreal/BuildingSystem.md`): `ExpansionSteps` 각 줄에 `Price`를 넣는다. 값은 상위 계약 4.7 "확장 구입 가격 (D2)" 행의 제안값이다(공간별·몇 번째 넓힘별). `Side`·`AmountCm`·줄 수는 그대로다. Data Validation 오류 0(가격·홀 효과 표 포함)을 확인하고 저장(`save_packages`)·재로드한다.
3. `/Game/Bathhouse/UI/WBP_ExpansionScreen`: 더 이상 bind되지 않는 `StageText`, `PriceText` widget을 지우고 레이아웃을 정리한다.
4. `/Game/Bathhouse/UI/WBP_ExpansionSpaceOption`: `StageText`, `PriceText`(TextBlock)를 카드 안 `NameText` 근처에 추가한다. Compile 경고 0, 1024×576 화면에서 세 카드 문구가 잘리지 않는지 캡처로 확인한다.
5. `.md/Unreal/` 갱신: `BuildingSystem.md`(넓힘 줄 `Price` 원본), `FacilitySystem.md` 또는 확장 정의 기록(삭제 필드 반영), `InteractionUISystem.md`(binding 변경).
6. 자동화 `Expansion.*`(content 계약 포함)와 `Computer.Input.ScreenWheelContentContract`를 다시 돌린다.
- 그 밖의 asset(`DA_ShopCatalog`, root WBP, Nav 범위, 열쇠걸이)은 바꾸지 않는다.

### 19.9 영향 시나리오와 유지 범위

- U2 PIE 영향: EXP-021(선택지 단계·가격, 전체 단계 문구 없음), EXP-022(홀 가격 기준 부족액), EXP-023(홀 1번째 가격 차감, 홀 `1/2`·2번째 가격, 다른 공간 `0/2` 그대로), EXP-026·027(공간 횟수 기대값). 나머지 U2 시나리오는 그대로다.
- U3 수용 대상(같은 코드로 동작): EXP-042~045(다른 공간 구입 뒤 홀 2번째 가격, 모두 상한일 때만 최대, 홀 상한 뒤 다른 공간 구입, 홀 상한 3 조정과 효과 표 검사).
- 유지: 공간 넓힘 적용·되돌림·이웃 불변, transaction 순서(7.3), tier = 홀 넓힘 횟수, 확장 탭 구조(탭 3개, 확인 2단계, 취소 경로), 락커 상품 규칙, 미리보기 글자(18절), Editor 단계에서 저장한 나머지 Content.

### 19.10 자동화 기준 (15절의 해당 줄을 대체·추가)

| 영역 | 확인 |
|---|---|
| 데이터 | 넓힘 줄 `Price` ≤ 0이면 `ExpansionPriceInvalid`·`CanApplyNextExpansion` 거짓, `GetNextExpansionPrice`가 다음 줄 가격·상한이면 0, `ExpansionHallEffectShort` 양성·음성(효과 표 = 홀 줄 + 1은 통과), Authority 없으면 생략, `ExpansionStepsBelowCap` 없음 |
| Definition·열쇠걸이 | `Tiers` 규칙만 검사(가격·상한 필드 없음), 열쇠걸이가 쓰지 않는 줄까지 포함해 최대 열쇠 수를 검사 |
| 구입 | 홀 1번째 = 홀 줄 0 가격, 목욕 1번째 = 목욕 줄 0 가격(fixture에서 서로 다른 값), 다른 공간을 산 뒤 홀 2번째 = 홀 줄 1 가격, 다른 공간의 횟수·가격 불변, 홀 상한 뒤 홀은 `SpaceMaxReached`이고 목욕은 구입 가능(전체 상한 없음), 모든 공간 상한이면 view `bAllAtLimit`, 같은 기대 횟수 두 번째 호출 `StaleState`, 잔액 부족 판정이 고른 공간 가격 기준 |
| 표시 모델 | 선택지 `확장 단계 N/M`·`다음 넓힘 N원`, 상한 선택지 가격 숨김·상태 문구, 선택 전 버튼 `확장 구입`·부족액 없음, 선택에 따라 버튼 가격·부족액이 바뀜, 일부 상한이면 최대 문구 없음, 모두 상한이면 최대 문구, 헤더에 단계 문구 없음 |
| content(Editor 뒤) | `WBP_ExpansionSpaceOption`에 `StageText`·`PriceText`, `WBP_ExpansionScreen`에 옛 `StageText`·`PriceText` 없음, 세 공간 넓힘 줄 `Price` > 0, `DA_BathhouseExpansion_Default` `ValidatePurchaseData` 통과 |
| 코드 리뷰 | 전체 구입 횟수·전체 상한·구입 순번 가격이 코드에 남지 않음, 가격 리터럴 없음(fixture 값에서 기대값 계산), 400줄 파일 무변경, 새 binding이 Optional이고 content 계약이 존재를 단언 |
