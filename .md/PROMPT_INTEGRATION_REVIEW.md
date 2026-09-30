# Unreal MCP 단계 보고 — 서비스 2단위 (2026-09-30)

## 판정

**부분 완료; 통합 승인 보류.** allowlist asset의 authoring·Compile·개별 Save·새 프로세스 재로드 대조(55개 항목 전부 통과)까지 끝났다. `PROMPT_UNREAL.md`의 PIE 대표 시나리오와 관찰 항목은 MCP로 입력·console을 실행할 수 없어 수행하지 못했고 `USER_UNREAL.md`로 넘겼다.
코드 리뷰 승인은 `PROMPT_REVIEW.md`에 기록돼 있지 않았으나, 사용자가 이번 대화에서 직접 진행을 지시해 그 지시를 근거로 실행했다.

## 연결과 세션

- 작업용 Editor(UE 5.8)를 `-ModelContextProtocolStartServer` 인자로 실행해 `127.0.0.1:8000` MCP에 연결했다. 이 프로젝트의 MCP 자동 시작 설정(`bAutoStartServer`)이 꺼져 있으면 인자 없이는 포트가 열리지 않는다(첫 시도는 이 인자 없이 실행해 포트를 기다리며 약 40분을 허비했고, Editor는 이미 동작 중이었다). 연결 절차 문서에 반영이 필요하다.
- 저장 뒤 종료 요청이 저장 확인창에서 멈춰 작업 소유 PID만 종료했고(저장 필요한 변경 없음), 새 Editor로 재로드 검증했다. Computer Use·Python API·binary 편집은 쓰지 않았다.

## 변경 asset (개별 Save, 재로드 값 일치)

신규: `DA_ServiceItem_{HairDryer,SkinLotion,CottonSwab,Comb,Shampoo,BodyWash}`, `BP_Vanity`, `DA_FacilityPlacement_Vanity`.
수정: `BP_Shower`(manager·router·공간 2개 추가), `BP_Washer`·`BP_Dryer`(뚜껑·pile 값), `DA_ShopCatalog`(상품 7개 추가, 총 16개).
변경 없음(상속 확인만): `BP_CleanTowelStack`, `BP_UsedTowelBin`, `BP_DrinkFridge`, `BP_ItemBox`(미리보기 값을 품목별로 바꿔 본 뒤 원복, 값은 저장 전과 동일). 경로·값은 `.md/Unreal/ServiceSystem.md`.

## 검증

- 6개 BP `warnings_as_errors` Compile 성공(저장 전·재로드 후). 저장 시 AssetCheck는 모든 저장 asset에서 오류 0, 경고는 `DA_FacilityPlacement_Vanity`의 RecoveryItemMesh 미설정 하나(다른 Definition과 동일).
- 새 프로세스 readback: DA 6개 값·자리 수, Vanity 컴포넌트 구성·공간/router/manager/slot/footprint/충돌/Definition 연결, Shower manager·router·공간 2개와 기존 slot·Definition 유지, Washer/Dryer 뚜껑·pile·기존 machine 유지, 수건통 cue, 냉장고 manager, 상품 16개·가격·정의 하나씩, dirty 없음.
- DefaultMap의 기존 샤워기 instance가 새 manager·router·두 공간을 상속(override 없음)했고 PIE를 6초 실행하는 동안 Error/Warning 로그가 없었다.
- 조정 사항: Washer/Dryer pile은 컴포넌트 scale이 좌표에 곱해져 프롬프트 값 `(22,18)`/8/2를 그대로 쓸 수 없어 월드 기준으로 환산했다(ServiceSystem.md). 프롬프트의 "MCP가 진열 공간을 `NoCollision`으로 설정" 항목은 `collisionProfileName`만으로는 반영되지 않아 `collisionEnabled`를 명시했다.
- 미검증: 프롬프트의 PIE 절차·DISP/VANI/SHWR/TOWL/F1/F2/SHOP 표 전체, 임시 도형의 화면 모양(박스 안 품목, 진열 자리, 뚜껑 열림/끼임, pile 가시성, 화장대 조준 범위), 명시적 Data Validation 실행(MCP tool 없음, Python API는 이번 작업 승인 밖), 자동화 재실행.

## dirty·미완료

- 종료 전 dirty package 없음. Config·Level·external actor·`.umap` 저장 없음. `BP_ItemBox`는 값이 같은 dirty 표시 때문에 개별 Save 후 종료 요청이 막혀 프로세스만 종료했다.
- `USER_UNREAL.md` 상단 "서비스 2단위" 항목.
