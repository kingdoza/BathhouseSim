# Unreal Editor 인계 — 상점 확장

## 현재 상태와 단계

- R1/P3 구현 후 UE 5.8 Editor 빌드, Shop 집중 자동화 11/11, 전체 회귀 69/69(실패 0)가 통과했다. 전체 회귀는 경고가 있는 성공 7건을 포함한다.
- Physics sanity는 통과했다. 요청 D=2/8/20의 실제 초기 깊이는 정본의 쌍별 상한으로 2/8/15cm, 최고 분리 속도는 9.319/40.096/22.022cm/s였다. 자동화의 D20>D2 endpoint gate는 통과했지만 세 값의 단조 증가는 확인되지 않았다.
- depenetration 설정과 개봉 impulse는 바꾸지 않았다. 이 실행에서는 Editor authoring, Blueprint compile/save, PIE를 수행하지 않았다. 기존 승인된 Editor/PIE 계약은 아래에 유지한다.
- 최종 Content/Config/Level 변경은 없다. load gate 임시 BP 사본은 제거했고 원본 Blueprint SHA-256은 193B9701268E607C9574F4E12E0BA1B3F173C967F052A84C42CF4DE4561047D7로 유지됐다.

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

아래는 기존 승인 범위의 Editor authoring 계약이다. 이번 실행에서는 Source 구현과 자동화만 수행했으며 asset 저장·재로드와 PIE는 진행하지 않았다.

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

### 배송 상자 Blueprint

- Asset: /Game/Bathhouse/Blueprints/Shop/BP_ShopDeliveryBox.
- Parent: AShopDeliveryBoxActor (/Script/BathhouseSim.ShopDeliveryBoxActor). BoxMesh가 root이고 유효한 cube mesh를 유지한다.
- BoxMesh Relative Location/Rotation은 zero, Relative Scale은 (0.8, 0.8, 0.8)로 authoring한다. Collision은 QueryAndPhysics, CCD On, Pawn Ignore를 확인한다. Native validation은 모든 축의 양수 finite scale을 허용한다.
- HeldTransform: Translation (10, 50, -60), Rotation identity, Scale (1, 1, 1). Runtime은 scale을 무시하므로 표시 위치/회전만 조정한다.
- Save allowlist: CDO/default를 실제로 바꾼 경우 BP_ShopDeliveryBox만 개별 저장한다. Parent 변경·새 component·Blueprint logic은 추가하지 않는다.

### Project Settings와 UI

- Project Settings > Bathhouse Shop에서 Catalog=DA_ShopCatalog, DeliveryBoxClass=BP_ShopDeliveryBox를 확인한다. UnboxOverlapDepthCm 기본값은 8cm이며 허용 범위는 0~50cm다. Config 저장이 필요하면 Editor의 정상 Save Config 흐름으로 Config/DefaultGame.ini만 저장한다.
- WBP: /Game/Bathhouse/UI/Shop/WBP_ShopScreen, parent UShopScreenWidget. ProductScroll만 상품 목록을 세로 스크롤하고 cart/order panel은 그 바깥 형제로 남아 항상 보이는지 확인한다.
- WBP hierarchy가 계약과 일치하면 수정/저장하지 않는다. 필요한 경우에도 승인된 layout 조정만 하고 Save allowlist에 WBP_ShopScreen을 추가한다. Domain 상태, 가격 계산, 입력·상점 로직은 Blueprint로 옮기지 않는다.

## 재진입 후 Compile, Save, reload

physics 동작 계약에 대한 다음 지시 전에는 이 절차를 시작하지 않는다. 재개가 승인되면 아래 순서로 진행한다.

1. UnrealEditor를 종료한 상태에서 UE 5.8 Build.bat 결과와 재검토된 automation을 확인한다.
2. dirty package를 확인한다. 이번 단계의 save allowlist에 있는 승인 변경만 대상으로 삼고 Save All, Level 배치, 광범위한 Content 변경은 하지 않는다.
3. 승인된 변경 Blueprint를 개별 Compile한다. BP_ShopDeliveryBox의 native parent, root, scale, HeldTransform과 Catalog reference를 확인한다.
4. 실제 변경한 allowlist asset만 개별 Save한다. Project Settings 변경은 필요할 때만 Config에 저장한다.
5. Editor를 재시작해 저장한 asset을 다시 로드한다. Fatal, Failed to load, Serial size mismatch가 나오면 중단하고 로그를 남긴다.

## 재진입 후 PIE 수용 절차

| 확인 | 절차와 기대 결과 |
|---|---|
| SHOP-018~020, 038~042 | 한 개·혼합 주문을 개봉한다. 정면·벽 앞·사방이 좁은 위치에서 생성물이 벽/천장을 통과하지 않고 guest Pawn을 밀지 않는다. 막힌 경우 안전한 후보 또는 deterministic fallback으로 나온다. |
| SHOP-032 | 상품 목록을 길게 만든 뒤 ProductScroll을 움직인다. Cart와 order/status 영역은 계속 보인다. |
| SHOP-034~035, 044 | 7종을 주문·배송·개봉하고 각 fresh-install 아이템을 기존 Placement에 배치한다. 주문 요약과 Definition 참조가 맞고 신규 Boiler operation 잔량은 0이다. |
| SHOP-043 | Shower 주문으로 overlap depth 2cm, 기본 8cm, 20cm를 비교한다. depth가 커질수록 실제 반발 속도가 강해지는지 확인한다. 튐이 없거나 크기 비교가 성립하지 않으면 depenetration 설정을 바꾸지 말고 멈춘다. |
| 상자 scale/held pose | delivery spawn, pickup, free drop, last-safe recovery 뒤 root scale이 0.8로 유지되고 HeldTransform 위치/회전만 반영되는지 확인한다. |

저장 후 정본 갱신은 실제 저장·재로드 근거가 생긴 다음에만 .md/Unreal/ShopSystem.md에 반영한다. CDO hierarchy, catalog 판매 상태, Project Settings, WBP scroll hierarchy와 PIE 결과를 확인하지 않았다.
