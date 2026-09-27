# 구현 리뷰 입력 — 상점 주문·배송 상자·쓰레기통

## 단계와 범위

- 기능 계약: PROMPT_ARCHITECTURE.md의 상점 Q1~Q25, 구현 정본: PROMPT_IMPLEMENTATION.md와 Architecture/ShopSystem.md.
- 현재 단계: 샤워기 단일 판매 수직 구현. SHOP-001~031, 036, 037 대상. SHOP-032~035는 미구현이다.
- 사용자는 Editor의 열림 여부와 무관하게 Source 작업을 계속하라고 지시했다. Editor는 닫힌 상태로 확인했고 조작하지 않았다.
- UE 5.8 `Build.bat` 빌드가 성공했다. 임시 복사본 4개와 원본 4개(Template_Default, DefaultMap) Blueprint load gate가 각각 성공했다. Shop focused automation 4/4, 전체 `BathhouseSim` 회귀 56 success + 6 succeeded-with-warnings, 실패 0, 미실행 0이다.
- 네 Blueprint 원본을 `Saved/MigrationBackup/20260927_shop/`에 백업했고, 원본과 백업 SHA-256이 모두 일치한다. 임시 `Content/Developers/MigrationCheck/` 복사본은 제거했다.
- Content, Config, Level을 수정·저장하지 않았다. 기존 dirty Content 상태는 보존했다. Blueprint Editor authoring/Compile/Save/PIE는 아직 수행하지 않았다.

## 수정 요약

- Economy: wallet 시작 잔액 100,000과 초기화 무방송, CanSpendMoney/TrySpendMoney 및 잔액 delegate; PlayerState에 ShopCart default subobject.
- Placement: null InstanceData 신규 설치 payload와 SpawnFreshItem factory. Facility, Utility, Towel import는 신규 설치 상태를 보존하며 보일러 operation 잔량은 0으로 시작한다. Facility.Discardable native tag, 락커/태그 Data Validation, 시설 아이템 폐기 구현.
- Carry/Shop domain: DeliveryBox enum append, 범용 CommitConsumeHeldObject와 기존 배치 wrapper, catalog/settings/cart/order subsystem, FIFO 지연 배송, 배송 지점·배송 상자·개봉 transaction·쓰레기통.
- Computer/UI/HUD: IComputerScreenContextReceiver를 통한 screen context/user 전달, 관리 화면 interface 연결, 탭 root·상점·행 3종·잔액 HUD·배송 알림 native widget 계약과 HUD 바인딩 lifecycle.
- 주문 guard는 평가가 끝난 뒤 설정하도록 두어 정상 주문이 자기 자신을 Busy로 막지 않게 했다. UI는 버튼 비활성 상태에서도 잔액 부족액을 다시 평가해 표시한다.

## 시나리오와 자동화 소스

| 시나리오 | 자동화 소스와 커버리지 | 실행 |
|---|---|---|
| SHOP-001, 029 | Shop.WalletAndCart: 시작 잔액·무방송, 지출·입금 이벤트. Economy.CashReentrancy는 configured StartingMoney 위에 지급 1회와 재진입 거부 확인 | 성공. Shop_Focused_Final의 4개 Shop 테스트 무실패·무경고 |
| SHOP-004~008, catalog | Shop.WalletAndCart: 추가/증가/감소/삭제, 상품·전체 수량 상한, 합계 overflow, 부족액 재평가, 판매 중지 줄 보존, 비양수 가격·누락 discard tag·락커·중복 ID Data Validation | 성공. Shop_Focused_Final |
| SHOP-009, 010 | WalletAndCart와 OrderDelayWaitingAndFIFO: 부족액 주문은 잔액/cart 불변, 성공 시 차감 1회·cart 비움, 반복 주문 거부와 snapshot | 성공. Shop_Focused_Final |
| SHOP-011~015, 030 | 같은 주문 자동화: 10초/0초, FIFO, 낮은 천장 대기 후 회복, 같은 tick 도착, 상자 적층과 delivery event | 성공. Shop_Focused_Final |
| SHOP-016~021 | Shop.FreshInstallTrashAndUnboxing: E pickup, summary, free-drop/re-pick 동일 Actor, suppression 거부, 개봉 성공/빈손/소모 이벤트, 트인 바닥·벽 앞·사방 막힌 구석 배치 | 성공. Shop_Focused_Final |
| SHOP-022, 028 | 같은 자동화: 신규 샤워기 factory→기존 placement transaction→기본 번호/가중치→회수·재배치, 신규 보일러 0잔량, Pawn 무시 | 성공. Shop_Focused_Final |
| SHOP-023~027, 036, 037 | 같은 자동화: 샤워기·보일러 폐기, 락커 1/4/8 거부, 열쇠·걸레·렌치·바구니·삽 interface 부재, 빈손 거부 | 성공. Shop_Focused_Final |
| Blueprint load gate | Shop.BlueprintLoad: Computer, PlayerState, HUD, recovery item native parent와 CDO 계약. 네 복사본, 네 원본 Template_Default, 네 원본 DefaultMap | 성공. 각 단일 package gate 1/1, 실패·경고 0 |
| 기존 회귀 | Economy 2개, Placement 5개, Computer 3개, Utility 8개, Interaction 12개 등 전체 `BathhouseSim` 62개 | 성공 56 + 경고 포함 성공 6, 실패 0, 미실행 0. All_Regression_Final3 |
| SHOP-002, 003, 031 및 입력·물리 수용 | WBP tab layout, 실제 컴퓨터 입력 우선순위, viewport HUD와 낙하 외형 | PIE 인계. 미실행 |

SHOP-032~035(7종 판매·혼합 주문)는 이 수직 구현과 자동화에 포함하지 않았다. 개봉 취소·실패 경로는 suppression 거부와 범용 carry consume domain 실패 복원 소스로 연결되며, partial-spawn rollback은 아직 동적 검증되지 않았다.

## 변경 파일

기존 수정:
- Private/Computer/BathhouseComputerActor.cpp
- Private/Economy/BathhousePlayerState.cpp, PlayerWalletComponent.cpp
- Private/Facility/BathWaterUtilityFacilityActor.cpp, BathhouseFacilityPlacementDomain.cpp
- Private/Interaction/PlayerCarryComponent.cpp
- Private/Placement/FacilityPlacementDefinition.cpp, FacilityPlacementPayload.cpp, PlaceableFacilityItemActor.cpp
- Private/Tests/BathhouseEconomyTests.cpp
- Private/Tests/FacilityPlacementAutomationTests.cpp (테스트 geometry의 10cm grid CDO override를 범위 안에서만 적용하고 원래 20cm 설정을 복원)
- Private/Towel/TowelProcessingMachinePlacement.cpp
- Private/UI/BathWaterManagementScreenWidget.cpp, BathhouseHUD.cpp
- Public/Economy/BathhousePlayerState.h, PlayerWalletComponent.h
- Public/Interaction/PhysicalCarryable.h, PlayerCarryComponent.h
- Public/Placement/FacilityPlacementPayload.h, FacilityPlacementTypes.h, PlaceableFacilityItemActor.h
- Public/UI/BathWaterManagementScreenWidget.h, BathhouseHUD.h

신규:
- Private/Placement/FacilityPlacementTypes.cpp
- Private/Shop: BathhouseTrashBinActor, ShopCartComponent, ShopCatalog, ShopDeliveryBoxActor, ShopDeliveryPointActor, ShopOrderSubsystem, ShopSettings, ShopUnboxingPlacement, ShopUnboxingTransaction의 .cpp/.h
- Private/Tests/ShopAutomationTestProbe.cpp/.h, ShopAutomationTests.cpp
- Private/UI: ComputerScreenRootWidget, MoneyHudWidget, ShopCartLineWidget, ShopNoticeWidget, ShopOrderLineWidget, ShopProductCardWidget, ShopScreenWidget의 .cpp
- Public/Computer/ComputerScreenContext.h, Public/Interaction/PhysicalCarryDiscardable.h
- Public/Shop: BathhouseTrashBinActor.h, ShopCartComponent.h, ShopCatalog.h, ShopDeliveryBoxActor.h, ShopDeliveryPointActor.h, ShopOrderSubsystem.h, ShopSettings.h, ShopTypes.h
- Public/UI: ComputerScreenRootWidget.h, MoneyHudWidget.h, ShopCartLineWidget.h, ShopNoticeWidget.h, ShopOrderLineWidget.h, ShopProductCardWidget.h, ShopScreenWidget.h

## 클래스 성장과 API 영향

| 클래스/파일 | 변경 전→후 | 추가 책임 |
|---|---:|---|
| PlayerCarryComponent cpp/header | 610/100→617/101 | 기존 placement consume 경로를 감싸는 generic consume commit |
| PlaceableFacilityItemActor cpp/header | 342/103→417/114 | 명세의 fresh-install factory와 tag 기반 discard lifecycle |
| BathhouseHUD cpp/header | 56/31→132/49 | 두 HUD widget, PlayerState 재시도 timer, delegate 해제 |
| BathhouseComputerActor.cpp | 199→215 | interface context 전달과 사용 시작 후 PlayerState 통지 |
| ShopOrderSubsystem.cpp | 신규 322 | 주문·FIFO·배송 world owner |
| ShopDeliveryBoxActor.cpp | 신규 390 | box lifecycle 및 carry/equipment/discard 계약 |
| ShopScreenWidget.cpp | 신규 379 | domain 구독과 표시 갱신 |

새 reflected type/property와 ShopCart default subobject, Facility.Discardable native tag, DeliveryBox enum append가 추가됐다. 기존 reflected 이름 삭제/변경, Serialize 변경, Core Redirect, Build.cs dependency 변경은 없다. 신규 로직은 승인된 책임 경계에 맞췄으며 Blueprint에는 표시 계층과 asset 연결만 남긴다.

## 검증 결과와 미검증 항목

- UE 5.8 `Build.bat`: 성공. 최종 Source 빌드 후 Shop, Economy, Placement와 전체 자동화를 수행했다. 빌드 로그는 `%LOCALAPPDATA%\UnrealBuildTool\Log.txt`에 있다.
- Shop focused: `Saved/Automation/Reports/20260927/Shop_Focused_Final/index.json` — 4 success, 0 warning, 0 fail.
- Economy focused: `Saved/Automation/Reports/20260927/Economy_Fixture_Final/index.json` — 2 success, 0 warning, 0 fail.
- Placement focused: `Saved/Automation/Reports/20260927/Placement_Final/index.json` — 5 success, 0 fail; 2개 테스트의 의도된 grid geometry/material 경고와 1개 StartupLockerReconciliation 진단을 포함해 총 2개 테스트가 warning 상태다.
- 전체 회귀: `Saved/Automation/Reports/20260927/All_Regression_Final3/index.json` — 56 success, 6 succeeded-with-warnings, 0 fail, 0 notRun. 경고 테스트는 Placement 2, Towel 1, Utility 3이다. 오류 없이 성공 상태인 해당 진단과 상세 이벤트는 JSON 보고서에서 확인한다.
- Blueprint load gate 보고서: `Shop_Copy_Computer`, `Shop_Copy_PlayerState`, `Shop_Copy_HUD`, `Shop_Copy_PlaceableItem`, `Shop_Originals_Template`, `Shop_Originals_DefaultMap` 디렉터리의 `index.json`. 각 1/1 success, 0 warning, 0 fail.
- 4개 원본/백업 SHA-256이 일치한다. backup directory: `Saved/MigrationBackup/20260927_shop/`. `Content/Developers/MigrationCheck/`는 제거됐고 Content의 기존 변경 목록은 이 작업 전과 동일하게 보존됐다.
- `Config/DefaultGame.ini`에는 기존 `GridSizeCm=20` 설정이 있다. Placement 아키텍처의 기본값 10cm를 전제로 하는 세 테스트는 10cm CDO override를 임시 적용해 검증하고, 끝나면 20cm를 복원한다. Config는 수정하지 않았다.

다음은 Editor pass에서 확인해야 한다.

- Blueprint 실제 연결, compile/save/reload와 Project Settings/Level authoring.
- PIE에서 컴퓨터 탭 전환, 주문 UI·HUD, 입력 우선순위, 물리 pickup/drop, 낮은 천장 대기와 재도착 수용.
- ShopUnboxingTransaction의 부분 생성 rollback 동적 fault injection은 자동화에 포함되지 않았다.
- 전체 회귀의 6개 warning 테스트 상세는 `All_Regression_Final3/index.json`에 기록되어 있다. 구현 프롬프트 요구대로 실패는 없고, 경고는 테스트 로그에 명시된 진단이다.