# Unreal Editor 인계 — 상점 확장

## 현재 상태와 단계

- R1/P3 구현 후 UE 5.8 Editor 빌드, Shop 집중 자동화 11/11, 전체 회귀 69/69(실패 0)가 통과했다. 전체 회귀는 경고가 있는 성공 7건을 포함한다.
- Physics gate는 코드 리뷰에서 통과했고 이번 Editor 작업 대기는 해제됐다. 요청 D=2/8/20의 실제 초기 깊이는 2/8/15cm, 최고 분리 속도는 9.319/40.096/22.022cm/s이며 D20>D2와 닫힌 방 정착을 통과 근거로 삼았다. 세 값의 단조 증가는 확인하지 않았다.
- MCP로 `DA_ShopCatalog`의 Shower를 보존하고 6종을 추가해 개별 저장했다. `BP_ShopDeliveryBox`의 root scale과 `HeldTransform`을 authoring하고 warnings-as-errors Compile 및 개별 저장을 마쳤다.
- Project Settings의 Catalog, DeliveryBoxClass, UnboxOverlapDepthCm=8은 이미 계약값이었다. Config는 변경하지 않았다. WBP hierarchy는 현재 MCP에서 읽을 수 없어 수정하지 않았다.
- 저장 후 새 Editor PID 24300은 Turnkey SDK 확인 지점에서 10분간 로그·MCP 포트가 진행되지 않아 종료했다. 새 프로세스 reload는 미완료다. 저장된 값과 같은 세션 readback을 fresh-process 확인으로 간주하지 않는다.
- 사용자 지시에 따라 PIE와 명시적 Data Validation은 실행하지 않았다. 이번 저장에서 엔진 로그의 `AssetCheck: Validating asset` 메시지가 발생했으나 별도 Data Validation 실행 결과로 취급하지 않는다.
- Content 변경은 `DA_ShopCatalog`과 `BP_ShopDeliveryBox` 두 자산뿐이다. Config, Level, WBP는 변경하지 않았다. load gate 임시 BP 사본은 제거했고 원본 백업 SHA-256은 `193B9701268E607C9574F4E12E0BA1B3F173C967F052A84C42CF4DE4561047D7` 그대로다.

## 완료된 Source 검증

| 확인 | 실제 결과 |
|---|---|
| UE 5.8 BathhouseSimEditor Win64 Development | Build.bat 성공 |
| 복사본 load gate | Template_Default에서 1/1 성공, 이후 복사본 제거 |
| 원본 load gate | Template_Default 1/1 성공 |
| 원본 load gate | /Game/Maps/DefaultMap 1/1 성공 |
| Shop focused automation | 11 total, 11 pass, 0 warning, 0 fail, 0 notRun |
| physics endpoint gate | sanity pass; effective D=2/8/15cm, D20 peak 22.022 > D2 9.319cm/s; intermediate D8 peak is higher, so monotonic trend remains unverified |
| 전체 Automation RunTests BathhouseSim | 69 total, 62 success, 7 success with warnings, 0 fail, 0 notRun |

세 load gate 자동화 보고서에는 경고·실패가 없었다. BP native parent, BoxMesh root, CDO 양수 finite root scale 및 HeldTransform property가 확인됐다. Fatal, asset load failure 또는 Serial size mismatch는 확인되지 않았다.

백업은 Saved/MigrationBackup/20260928_shop_ext/BP_ShopDeliveryBox.uasset에 보관했다. SHA-256: 193B9701268E607C9574F4E12E0BA1B3F173C967F052A84C42CF4DE4561047D7.

## 후속 Editor authoring 계약

아래는 기존 승인 범위의 Editor authoring 계약이다. 이번 실행에서 Catalog와 Blueprint를 authoring·저장하고 Compile했으며, 새 프로세스 reload는 미완료다. PIE와 명시적 Data Validation은 사용자 지시로 제외했다.

### Catalog와 상품

- Asset: /Game/Bathhouse/Data/Shop/DA_ShopCatalog (DA_ShopCatalog). 기존 Shower entry를 보존한다.
- Bath, Washer, Dryer, Boiler, Cooler, Circulator 6개 entry를 추가한다. 각 entry의 ProductId는 영문 고유 ID, bForSale=true, DisplayName은 해당 설비명, PlacementDefinition은 아래 기존 Definition을 연결한다. 가격은 양의 정수로 Editor에서 결정한다. Icon은 선택이다.
- Definition asset 경로:
  - /Game/Bathhouse/Data/Placement/DA_FacilityPlacement_Bath
  - /Game/Bathhouse/Data/Placement/DA_FacilityPlacement_Washer
  - /Game/Bathhouse/Data/Placement/DA_FacilityPlacement_Dryer
  - /Game/Bathhouse/Data/Placement/DA_FacilityPlacement_Boiler
  - /Game/Bathhouse/Data/Placement/DA_FacilityPlacement_Cooler
  - /Game/Bathhouse/Data/Placement/DA_FacilityPlacement_Circulator
  - 기존 Shower: /Game/Bathhouse/Data/Placement/DA_FacilityPlacement_Shower
- Save allowlist: DA_ShopCatalog만 해당 변경이 있을 때 개별 저장한다. Data Validation에서 중복 ID, 양수 가격, discard tag와 locker 조건을 확인한다.
- 현재 결과: Shower 항목을 유지하고 Bath, Washer, Dryer, Boiler, Cooler, Circulator를 순서대로 추가했다. ID는 각 영문 설비명, 표시 이름은 욕조·세탁기·건조기·보일러·쿨러·순환기, 모두 판매 중·가격 10,000·Icon 없음이며 각각 대응 Definition을 참조한다. Catalog 개별 Save 성공. Data Validation은 사용자 지시로 실행하지 않았다.

### 배송 상자 Blueprint

- Asset: /Game/Bathhouse/Blueprints/Shop/BP_ShopDeliveryBox.
- Parent: AShopDeliveryBoxActor (/Script/BathhouseSim.ShopDeliveryBoxActor). BoxMesh가 root이고 유효한 cube mesh를 유지한다.
- BoxMesh Relative Location/Rotation은 zero, Relative Scale은 (0.8, 0.8, 0.8)로 authoring한다. Collision은 QueryAndPhysics, CCD On, Pawn Ignore를 확인한다. Native validation은 모든 축의 양수 finite scale을 허용한다.
- HeldTransform: Translation (10, 50, -60), Rotation identity, Scale (1, 1, 1). Runtime은 scale을 무시하므로 표시 위치/회전만 조정한다.
- Save allowlist: CDO/default를 실제로 바꾼 경우 BP_ShopDeliveryBox만 개별 저장한다. Parent 변경·새 component·Blueprint logic은 추가하지 않는다.
- 현재 결과: 기존 parent `/Script/BathhouseSim.ShopDeliveryBoxActor`와 BoxMesh root·메시 참조·충돌 설정을 보존했다. BoxMesh relative location/rotation은 0, scale은 0.8, QueryAndPhysics·CCD·Pawn Ignore다. HeldTransform은 계약값이다. Compile 및 개별 Save 성공. 새 프로세스 reload는 미완료다.

### Project Settings와 UI

- Project Settings > Bathhouse Shop에서 Catalog=DA_ShopCatalog, DeliveryBoxClass=BP_ShopDeliveryBox를 확인한다. UnboxOverlapDepthCm 기본값은 8cm이며 허용 범위는 0~50cm다. Config 저장이 필요하면 Editor의 정상 Save Config 흐름으로 Config/DefaultGame.ini만 저장한다.
- 현재 결과: CDO와 `Config/DefaultGame.ini`에서 Catalog/DeliveryBoxClass 참조 및 기본값 8을 확인했다. 모두 이미 일치해 Config는 저장하지 않았다.
- WBP: /Game/Bathhouse/UI/Shop/WBP_ShopScreen, parent UShopScreenWidget. ProductScroll만 상품 목록을 세로 스크롤하고 cart/order panel은 그 바깥 형제로 남아 항상 보이는지 확인한다.
- WBP hierarchy가 계약과 일치하면 수정/저장하지 않는다. 필요한 경우에도 승인된 layout 조정만 하고 Save allowlist에 WBP_ShopScreen을 추가한다. Domain 상태, 가격 계산, 입력·상점 로직은 Blueprint로 옮기지 않는다.
- 현재 결과: parent는 `/Script/BathhouseSim.ShopScreenWidget`으로 확인했다. MCP toolset에 Widget tree 조회·편집 기능이 없고 `WidgetTree`를 generic object query로 읽지 못해 ProductScroll·cart/order 계층은 미확인이다. WBP는 수정·저장하지 않았다.

## Compile, Save, reload 결과

1. UE 5.8 Build와 기존 Shop automation/전체 회귀 gate는 위 표에 기록된 성공 결과를 사용했다. 이번 Editor 단계에서 테스트를 다시 실행하지 않았다.
2. 변경 Blueprint `BP_ShopDeliveryBox`를 warnings-as-errors로 Compile했다. 오류나 warning 없이 성공 응답을 받았다.
3. `DA_ShopCatalog`과 `BP_ShopDeliveryBox`를 각각 개별 Save했다. 두 Save 모두 성공 응답과 package 저장 로그를 반환했다. package 경로 기준 두 asset은 dirty=false였다.
4. 새 Editor PID 24300은 `LogTurnkeySupport`의 SDK 확인 다음 단계로 진행하지 않았다. 10분 동안 `127.0.0.1:8000` listener와 새 Turnkey log가 없었고 Editor log도 갱신되지 않아 task-owned PID를 종료했다. 저장 뒤 fresh-process asset reload는 확인되지 않았다.
5. 사용자 요청 범위에 따라 Data Validation과 PIE 수용 절차는 실행하지 않았다. 이 항목들을 통과했다고 표시하지 않는다.

## 이번 요청에서 제외한 PIE 수용 절차

이번 실행에서는 사용자 지시에 따라 아래 시나리오를 시작하지 않았다.

| 확인 | 절차와 기대 결과 |
|---|---|
| SHOP-018~020, 038~042 | 한 개·혼합 주문을 개봉한다. 정면·벽 앞·사방이 좁은 위치에서 생성물이 벽/천장을 통과하지 않고 guest Pawn을 밀지 않는다. 막힌 경우 안전한 후보 또는 deterministic fallback으로 나온다. |
| SHOP-032 | 상품 목록을 길게 만든 뒤 ProductScroll을 움직인다. Cart와 order/status 영역은 계속 보인다. |
| SHOP-034~035, 044 | 7종을 주문·배송·개봉하고 각 fresh-install 아이템을 기존 Placement에 배치한다. 주문 요약과 Definition 참조가 맞고 신규 Boiler operation 잔량은 0이다. |
| SHOP-043 | Shower 주문으로 overlap depth 2cm, 기본 8cm, 20cm를 비교한다. depth가 커질수록 실제 반발 속도가 강해지는지 확인한다. 튐이 없거나 크기 비교가 성립하지 않으면 depenetration 설정을 바꾸지 말고 멈춘다. |
| 상자 scale/held pose | delivery spawn, pickup, free drop, last-safe recovery 뒤 root scale이 0.8로 유지되고 HeldTransform 위치/회전만 반영되는지 확인한다. |

저장 후 정본 갱신은 실제 저장 asset의 새 프로세스 reload 근거가 생긴 다음에만 .md/Unreal/ShopSystem.md에 반영한다. 이번 reload는 미완료다. Catalog·BP CDO와 Project Settings는 같은 세션 readback만 있으며 WBP scroll hierarchy와 PIE 결과는 확인하지 않았다.
