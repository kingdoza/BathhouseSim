# 구현 재작업 프롬프트 — 상점 확장: 물리 측정 harness 수정과 재측정

## 재검토 결론

- 2026-09-28 두 번째 코드 리뷰 결론: **구현 재검토**. 입력은 `.md/PROMPT_REVIEW.md`(19:00 UTC)와 현재 작업 트리다.
- 완료 확인:
  - 백업(`20260928_shop_ext`, SHA 기록).
  - Editor 빌드.
  - copy-first load gate 1~4.
  - P2 `WITH_EDITOR` guard.
  - P3 관찰표(D별 단계, 7종 회전 성분 없음).
- 중단 조건이 발동했다고 보고된 `UnboxingPhysics`의 측정은 **물리가 실제로 진행된 결과로 볼 수 없다.** 기능 명세로 복귀하기 전에 측정 harness부터 바로잡는다. 이번 판단의 근거는 아래와 같다.

## 근거 — 테스트 world에서 물리가 진행되지 않았다

1. **자유낙하 속도가 맞지 않는다.**
   - 두 물품을 Z=5000cm 허공에 만들고 1/60초 × 30 step을 진행했다.
   - 정상 물리라면 자유낙하 속도가 약 490cm/s(980 × 0.5)까지 올라야 한다.
   - 보고된 최고 선속도는 세 D 모두 16.331cm/s다. 중력 한 step(980/60)과 같다. 첫 step 이후 적분이 멈춘 것으로 보인다.
2. **물품이 떠 있다.**
   - 닫힌 방의 바닥 윗면은 Z=0(`ShopRoomFloor` 중심 −50, half 50)이다.
   - 3초 tick 뒤 10개 물품의 Z는 981~1337cm였다. 중력이 작동했다면 모두 바닥 근처에 있어야 한다.
   - 현재 단언("방 안·바닥 위")은 떠 있는 상태도 통과시킨다.
3. **world 생성에 물리 시작 단계가 없다.** `FShopScatterAutomationWorld::Initialize`는 `CreateWorld(Game)`, `InitializeActorsForPlay`만 호출한다. 물리 시뮬레이션 활성화와 `BeginPlay`가 없다.

따라서 "분리 방향 상대속도 0"과 D별 비교는 기능 전제(Q29 B)의 판정 근거가 아니다. depenetration 설정과 unboxing 로직은 계속 바꾸지 않는다.

## 진행 상태 (2026-09-28, 리뷰 에이전트 직접 수정)

사용자 지시로 리뷰 에이전트가 P1을 직접 수정했다. **구현 단계는 P1 코드를 다시 작성하지 말고, 빌드·실행·보고만 한다.** R1(`.md/PROMPT_IMPLEMENTATION.md`) 작업과 같은 차례에 함께 진행한다.

- 수정 파일: `Source/BathhouseSim/Private/Tests/ShopUnboxingScatterAutomationTests.cpp`만. 제품 코드·Content·Config는 무변경.
- 확인된 원인: 물리 테스트가 `World.Tick`만 반복하고 `GFrameCounter`를 올리지 않았다. tick 관리자는 같은 frame 번호에서 tick 함수를 한 번만 실행하므로 물리가 첫 step만 적분했다(최고 속도 16.331cm/s = 980/60). Utility 테스트 helper는 이미 `++GFrameCounter`를 한다.
- 변경 내용:
  1. `FShopScatterAutomationWorld::Initialize(..., bool bSimulatePhysics = false)`. true면 `bShouldSimulatePhysics = true`와 `World->BeginPlay()`를 실행한다. 기본값 false라 다른 Shop 테스트의 동작은 그대로다.
  2. `TickShopPhysicsStep`: `++GFrameCounter` 후 `World.Tick`. 물리 테스트의 모든 tick이 이 helper를 쓴다.
  3. `RunShopFreeFallSanityGate`: 샤워기 한 개를 0.5초 자유낙하시킨다. 속도는 |g|·t ±20%, 낙하 거리는 ½|g|t² ±25%여야 한다. 실패하면 `UnboxingPhysics`가 이후 판정 없이 실패한다.
  4. 튐 측정: 첫 3 step의 분리 방향 최고 상대속도를 따로 기록한다. "nonzero velocity" 단언을 전체 선속도(자유낙하에서는 중력 때문에 항상 참)에서 분리 방향 상대속도로 바꿨다.
  5. 닫힌 방: 각 물품이 생성 바닥면보다 200cm 이상 낙하했는지, 가장 낮은 바닥면이 방 바닥 ±5cm에 정착했는지 단언한다.
- 1차 실행 결과(사용자 실행, 2026-09-28 02:27): sanity 통과(하강 속도 488.736/490, 낙하 126.359/122.5cm). 방 물품 10개가 바닥에 정착했다(lowestBottom −0.03cm). 그러나 D=2/8/20 모두 분리 속도 0.000이고 최종 penetration −44.319/−32.000/−26.319로, 처음부터 겹치지 않았다.
- 2차 수정(테스트 코드만):
  6. 원인: `RunShopOverlapPhysicsCase`가 layout의 **collision shape 중심**을 actor 위치로 그대로 썼다. 메시 bounds origin이 yaw마다 다르게 어긋나 두 물품이 벌어진 채 생성됐다. 근거: 같은 seed에서 D=2와 D=20의 차이가 정확히 18cm이다. 이제 제품(`BuildCandidateAtShapeCenter`)과 같은 변환 `actor = shapeCenter − R·(scale·boundsOrigin)`을 쓰고, CDO scale을 transform에 넣는다.
  7. 초기 겹침 sanity: 생성 직후, tick 전에 실제 두 collision box의 SAT penetration이 D±0.5cm인지 단언하고 `initialPenetration`을 로그에 남긴다.
  8. scale 계약: 겹침 테스트와 닫힌 방 테스트 모두 생성된 actor scale이 CDO(template) scale과 같은지 단언하고 `actorScale/templateScale`을 로그에 남긴다. 제품 경로(`FindSpawnTransforms`가 준 transform에 scale 포함 → `SpawnFreshItem`의 `SpawnActorDeferred`는 `ScaleMethod`를 지정하지 않음)에서 root scale이 두 번 곱해지는지 확인하려는 것이다. **이 단언이 실패하면 테스트를 고치지 말고 그대로 보고한다**(제품 결함 후보).
- 2차 실행 결과(사용자 실행, 2026-09-28 11:38 KST): 빌드 성공. 생성된 물품 actor scale은 겹침·방 테스트 모두 `(0.6,0.6,0.6)`, CDO `GetActorScale3D()`는 `(1,1,1)`. 초기 penetration은 D=8 −32.000, D=2 −44.319, D=20 −26.319cm로 **최종값과 같다**(두 물품이 한 번도 닿지 않음). 분리 속도 0.
- 원인 확정 — **제품 결함**(아래 P3): Blueprint CDO는 component-to-world를 계산하지 않으므로 `ItemCDO->GetActorScale3D()`는 Blueprint가 `ItemRoot`에 넣은 scale과 무관하게 1.0이다. 상점 배치는 이 1.0으로 모든 collision shape를 계산하고, 실제 물품은 root 기본 scale 0.6으로 생성된다. 수치 검산: 실제 penetration = D − (1 − 0.6)·(두 물품의 분리축 반경 합). D=8 → 8 − 0.4·100 = −32. 같은 seed의 D=2·D=20은 반경 합 115.8로 −44.319·−26.319가 둘 다 맞는다.
- 3차 수정(테스트 코드만): 테스트도 같은 방식(`CDO->GetActorScale3D()`)으로 template scale을 읽고 있었다. `GetShopItemTemplateScale()`(CDO root의 `GetRelativeScale3D()`, `FacilityPlacementGeometry`와 같은 규칙)로 바꾸고 `GetDefinitionHalfExtent`, 겹침 테스트, 방 테스트의 기준값에 적용했다. scale 단언은 유지한다. P3 전에는 겹침 테스트 scale 단언이 계속 실패하는 것이 정상이다(0.6 transform × root 0.6 = 0.36 예상).
- 리뷰 에이전트는 Windows UE 빌드를 실행할 수 없어 compile은 미확인이다. 구현 단계가 아래 순서로 실행해 결과를 `.md/PROMPT_REVIEW.md`에 적는다.
  1. `Build.bat BathhouseSimEditor Win64 Development`. compile 오류가 이 파일에서 나면 최소 수정하고 무엇을 고쳤는지 보고한다.
  2. `Automation RunTests BathhouseSim.Shop.UnboxingPhysics`(Headless Policy). 로그의 `UnboxingPhysics sanity`, D=2/8/20 줄(`initialPenetration` 포함), `actorScale` 줄, room 줄을 그대로 옮긴다.
  3. 아래 P2 판정 분기에 따른다. 성립하면 집중 `BathhouseSim.Shop`과 전체 회귀를 R1 결과와 함께 보고한다.

## P1 — 물리 harness 수정(테스트 코드만)

대상: `Source/BathhouseSim/Private/Tests/ShopUnboxingScatterAutomationTests.cpp`의 `FShopScatterAutomationWorld`와 physics 테스트.

1. **물리 시뮬레이션이 실제로 진행되는 world를 만든다.**
   - `UWorld::bShouldSimulatePhysics`를 켠다.
   - 필요한 play 초기화(`BeginPlay` 등)를 수행한다.
   - 프로젝트의 다른 물리 테스트(낙하·래그돌·carry release)에서 physics가 실제로 진행되는 기존 fixture가 있으면 그 방식을 재사용한다. 무엇을 기준으로 삼았는지 보고한다.
2. **sanity gate를 먼저 둔다.** 튐 측정 전에 같은 world에서 샤워기 한 개를 바닥 없는 공중에 두고 0.5초 진행한다.
   - 하강 속도가 980 × t의 ±20% 안이어야 한다.
   - 실패하면 이후 측정을 하지 않고 실패로 보고한다.
3. **닫힌 방 단언을 강화한다.** 3초 뒤 모든 물품이 방 안에 있고, 가장 낮은 collision 바닥면이 방 바닥 윗면 + 5cm 이내(정착)여야 한다.
4. **튐 측정:**
   - 기존 D=2, 8, 20 비교를 유지한다.
   - 매 step(1/60초)마다 분리 방향 상대속도와 전체 선속도의 최대값을 기록한다.
   - 첫 3 step의 값도 따로 적는다(초기 depenetration은 첫 step들에서 일어난다).
   - sanity gate를 통과한 world에서만 결과를 판정한다.

## P2 — 재측정 뒤 판정

- sanity gate 통과 + D가 클수록 분리 속도가 커지면: 기능 전제가 성립한다. 집중 `BathhouseSim.Shop`과 전체 `Automation RunTests BathhouseSim`을 실행하고 보고한다.
- sanity gate 통과 + 분리 속도가 여전히 거의 0: 이것이 진짜 중단 조건이다. 설정을 바꾸지 말고 수치와 함께 멈춰 보고한다(기능 명세 복귀).
- sanity gate 자체가 통과하지 않으면: 물리 fixture 문제로 멈추고 시도한 방법과 로그를 보고한다.

## P3 — 제품 결함: 상점 물품 scale 이중 기준 (구현 재검토)

현상: `FShopUnboxingPlacement`가 계산한 collision shape와 실제로 생성된 물품의 크기가 다르다. shape는 scale 1.0 기준이고 실제 물품은 0.6이다. 결과적으로 다음 계약이 게임에서 성립하지 않는다.

- 무리 겹침 D: 실제로는 겹치지 않고 약 40cm 벌어진다. 튐 연출이 발생하지 않는다.
- 바닥 +20cm: 실제 물품 바닥은 shape 바닥보다 (1−0.6)·Ez만큼 더 높다.
- 환경·Pawn 여유: 실제보다 1/0.6배 큰 상자로 검사한다. 안전 쪽이지만 배치 실패가 과다하다.
- 기존 `ShopAutomationTests`의 "+20cm" 검사는 반환 transform(scale 1)으로만 계산하므로 이 결함을 잡지 못했다.

수정:

1. **scale 원천을 하나로 둔다.** `APlaceableFacilityItemActor`에 static helper(예: `GetDefinitionItemScale(const UFacilityPlacementDefinition&, FVector&, FText&)`)를 추가한다. CDO `GetRootComponent()->GetRelativeScale3D()`를 반환하고, NaN이나 0 이하 성분은 실패로 처리한다(`FacilityPlacementComponent::BuildPlacedActorTransform`과 같은 규칙). `ShopUnboxingPlacement.cpp`의 `ItemCDO->GetActorScale3D()`를 이 helper로 바꾼다.
2. **생성 transform을 최종 world transform으로 만든다.** `SpawnFreshItem`의 `SpawnActorDeferred`와 `FinishSpawning` 모두 `ESpawnActorScaleMethod::OverrideRootScale`을 쓴다(설비 생성 `FacilityActorConversionTransaction.cpp` 313·372행과 같은 방식). 그러지 않으면 transform의 0.6과 root의 0.6이 곱해진다. `SpawnFreshItem` 호출처는 `ShopUnboxingTransaction`과 테스트뿐이다.
3. **생성된 actor로 검증한다.** R1의 `UnboxingDepthPlacementObservation` 단언 전환에서 "바닥 +20±0.5"는 반환 transform이 아니라 `SpawnFreshItem` + `ActivateFreeWorld`로 생성한 actor의 transform으로 계산한 collision 바닥면으로 판정한다. `ShopAutomationTests`의 open-floor "+20cm" 검사에도 생성 actor 기준 검사를 하나 추가한다.
4. 테스트의 `GetShopItemTemplateScale()`은 새 helper를 호출하도록 바꿔도 된다(기준 동일).

완료 기준 — `BathhouseSim.Shop.UnboxingPhysics`:

- actorScale = templateScale = 0.6(현재 Blueprint 값).
- initialPenetration = D ± 0.5.
- 그다음 P2 판정 분기를 적용한다.

R1과 같은 파일(`ShopUnboxingPlacement.cpp`)이므로 R1과 한 차례에 처리한다. 순서: P3 1·2 → R1 → P3 3.

범위 밖 관찰(수정하지 않고 보고만):

- `FacilityActorConversionTransaction.cpp` 75행(설비 회수)도 `ItemCDO->GetActorScale3D()` + 기본 scale method로 같은 패턴이다. 회수 공간 검사가 실제보다 큰 상자로 이뤄진다. 기존 회수 테스트는 constructor에서 scale을 정하는 native fixture를 써서 드러나지 않는다. 별도 과제로 아키텍처에 넘긴다.
- `.md/Unreal/PlacementSystem.md`와 `.md/USER_UNREAL.md`는 `BP_PlaceableFacilityItem.ItemRoot` scale을 `(0.3,0.3,0.3)`으로 기록하고 있다. 실측은 0.6이다. 사용자가 조정한 값이면 Unreal 문서 갱신 대상이다.

## 유지

- P3 관찰 결과(D ≥ 20에서 정면 대신 위 단계 채택)는 **아키텍처 판단 대상**이다. 코드를 바꾸지 않는다.
- 제품 코드 수정은 P3 범위(scale helper, `SpawnFreshItem` scale method, 호출처 교체)와 R1에 한정한다. Content·Config·Level, depenetration 설정, 개봉 속도·impulse는 수정하지 않는다.

## 결과물

- `.md/PROMPT_REVIEW.md` 갱신:
  - harness 수정 방식과 기준 fixture.
  - sanity gate 수치.
  - 튐 측정표(첫 3 step 포함).
  - 닫힌 방 정착 결과.
  - 판정 분기, 자동화 수치.
- `.md/PROMPT_UNREAL.md`는 판정이 성립할 때만 갱신한다.
