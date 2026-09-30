# 코드 리뷰 프롬프트 — 수건 cue 갱신 재작업

## 현재 단계와 입력

서비스 2단위 완료 뒤 보고된 수건 cue 갱신 결함의 C++ 재작업이다. AGENT_WORKFLOW.md → AGENT_REVIEW.md 순서로 코드 리뷰한다. 현재 보고는 코드 리뷰 승인이나 PIE 통합 승인 자체가 아니다.
입력: [재작업 findings](PROMPT_IMPLEMENTATION_R.md), [구현 프롬프트 맨 앞 재작업 절](PROMPT_IMPLEMENTATION.md), [기능 계약](PROMPT_ARCHITECTURE.md), [수건 cue 재계산 정본](Architecture/TowelSystem.md), [Interaction PresentationRevision 정본](Architecture/InteractionSystem.md), [결정적 자리 표현](Architecture/TowelPresentationSystem.md).
기존 2단위 Source·F1~F4 결과·Editor 작업물을 유지했다. 최초 구현/F1~F4를 다시 수행하지 않았다. 이번 변경은 아래 8개 Source 파일과 PROMPT_REVIEW.md·PROMPT_UNREAL.md뿐이다. 기존 인계 문서는 Saved/ImplementationUnit2/CueRefresh/Before/.md/에 보관했다.

## 선택된 경로와 구현

- 정본의 C안: `FPlayerInteractionQuery` 끝에 `UPROPERTY() int64 PresentationRevision = 0`을 추가하고 Equals에 포함했다. Blueprint 비노출이며 HUD 이름·행동명·이유·가능 여부·mode는 그대로다.
- `TowelDisplayCueUtils::GetPresentationRevision` 한 곳에서 대상 inventory Revision + 든 ATowelBasketActor inventory Revision을 계산한다. 없는 inventory·바구니가 아닌 물건은 해당 항 0이다.
- 선반·사용 수건통·기계 transfer port query의 정상·불가·inactive fallback·ownerless 빈 반환에 같은 규칙을 채운다. 이동 조건·정원·기계 상태를 바꾸지 않는다.
- 매 transfer 뒤 기존 HeldUse의 RefreshInteractionQuery → CommitQuery → SyncFocusObservers가 revision 차이를 감지해 기존 owner focus 알림을 보낸다. cue는 기존 Update에서 authoritative Count/Count−1로 다시 계산된다. 플레이어 연속 입력은 같은 프레임, 손님/기계 등 외부 변화는 늦어도 다음 query tick에 반영된다.
- PlayerInteractionComponent·PlayerHeldTargetUseComponent는 수정하지 않았다. 반복 guard는 기존 target/key 조건이며 revision이 바뀌어도 반복을 중단하지 않는다. owner inventory 구독·query 캐시·새 lifecycle을 추가하지 않았다.
- NotifyInteractionFocusChanged·TowelDisplayCueUtils::Update·뚜껑 요청은 변경하지 않았다. 기존 SetSourceInsertable은 같은 source key에 bool을 덮어쓰므로 같은 열림 방향의 요청 반복은 중복 source를 만들지 않는다. 기존 결정적 Pile·뚜껑 자동화도 유지했다.
- 진열 공간·설비 router·냉장고 runtime query와 HUD는 변경하지 않았다. 새 필드의 기본값 0을 유지한다.

## 변경 파일과 클래스 성장

Source/BathhouseSim 기준 줄 수다. 이번 시작 상태와 비교한다.

| 파일 | 변경 전 → 후 |
|---|---|
| `Private/Tests/ServiceBlueprintLoadAutomationTests.cpp` | 227 → 227 |
| `Private/Tests/TowelDisplayCueAutomationTests.cpp` | 236 → 539 |
| `Private/Towel/CleanTowelStackActor.cpp` | 145 → 149 |
| `Private/Towel/TowelDisplayCueUtils.cpp` | 45 → 55 |
| `Private/Towel/TowelDisplayCueUtils.h` | 14 → 16 |
| `Private/Towel/TowelTransferPortComponent.cpp` | 151 → 153 |
| `Private/Towel/UsedTowelBinActor.cpp` | 220 → 224 |
| `Public/Interaction/InteractionTypes.h` | 279 → 283 |

production은 query 값 1개와 stateless helper 1개 추가뿐이다. 새 UFUNCTION·delegate·Tick·bind/unbind·default subobject·독립 상태 책임은 없다. 큰 Interaction 클래스의 구현 성장 없이 승인된 query 계약을 따랐다. 테스트 파일 증가는 네 대상 실제 입력 경로의 fixture·회귀 검증이다.
`ServiceBlueprintLoadAutomationTests.cpp`는 기존 DA_ShopCatalog 상품 수 기대값 한 줄만 9→16으로 맞췄다. [현재 Shop Editor 정본](Unreal/ShopSystem.md)에 기존 9 + 2단위 7 = 16이 저장·재로드된 상태다. 실제 Content를 바꾸거나 기존 상품 보존 검사를 제거하지 않았다.

## 새 실제 경로 자동화

`BathhouseSim.Towel.Display.RefreshFollowsTransfersAndRepeats`를 기존 TowelDisplayCueAutomationTests.cpp에 추가했다. 새 테스트는 NotifyInteractionFocusChanged를 직접 호출하지 않는다. 실제 locally controlled player·camera trace·held basket·Interaction Refresh·HeldUse BeginUse/TickComponent를 사용한다.

| 대상/시나리오 | 경로와 기대값 |
|---|---|
| 선반 TOWL-001·004·005·013·016 | 조준 고정 → LMB 즉시 1장 + Tick 반복 2장 → RMB 즉시 1장 + Tick 반복 2장. 매 장 exactly 1 transfer, preview=GetIndexPresentation(Count), highlight=GetIndexPresentation(Count−1) |
| 사용 수건통 TOWL-004·005·016 | Apply·프리뷰 없음. RMB 즉시+반복 총 3장마다 강조가 새 맨 위로 이동 |
| Waiting 세탁기 TOWL-006·010·013 | Used 바구니로 LMB/RMB 각각 3장, 결정적 Pile의 새 index transform과 cue 일치 |
| Waiting 건조기 TOWL-006·017 | Wet 바구니로 같은 LMB/RMB 3장 검증 |
| 외부 선반 변화 TOWL-018 | 실제 UTowelTransferSubsystem으로 선반→손님 inventory 1장 이동 → RefreshInteractionQuery 1회로 새 자리 반영 |
| 바구니만 변경 | 네 대상에서 basket→외부 inventory 1장 이동, 대상 revision 불변인데 query 합은 변경되고 cue를 다시 계산 |
| query/HUD | 매 count-only 이동에서 Equals false. revision만 맞추면 Equals true이므로 모든 기존 HUD/가능 여부 필드는 동일 |
| fallback/default | inactive 대상·ownerless port·null inventory/바구니 없음의 합, BlueprintVisible flag 없음, 진열 공간·router·냉장고 revision 0 |
| 숨김 | 실제 suppression과 focus exit에서 양 cue 숨김, suppression 해제 후 실제 조준·표현 복구 |

모든 cue 검사에서 실제 target focus도 확인한다. 사용 수건통의 Apply 예외를 제외하면 각 장 이동 직후 보이는 cue의 relative transform을 검사하며 수량 경계에 의한 우연한 query 변화에 기대지 않는다.
기존 `BathhouseSim.Towel.Display.CuesDeterministicPileAndLid` 본문은 수정하지 않았다. 기존 Towel·Service·Interaction held-use·Utility 자동화를 제외하지 않고 전체 실행했다.

## 빌드·로드·회귀 결과

모든 headless는 AGENT_WORKFLOW 형식과 -DDC-ForceMemoryCache를 사용했다. 빌드는 UE 5.8 Build.bat / BathhouseSimEditor Win64 Development / -WaitMutex -NoHotReloadFromIDE다.

| 검증 | 실제 결과 | 로그 / report (Saved 기준) |
|---|---|---|
| Build 01~04 | 4회 Succeeded, 최종 9.56초. 최종 빌드 뒤 Source 수정 없음 | ImplementationUnit2/CueRefresh/build_01.txt ~ build_04.txt |
| DefaultMap 로드 + Towel.Display 최종 | 2/2 성공, test warning/error 0, exit 0 | ImplementationUnit2/CueRefresh/defaultmap_03.log; Automation/Reports/20260930/unit2_cue_defaultmap_03/index.json |
| Template 전체 BathhouseSim | **100/100 성공**, 실패 0, 미실행 0, test warnings 22, errors 0, exit 0 | ImplementationUnit2/CueRefresh/all.log; Automation/Reports/20260930/unit2_cue_all/index.json |
| 정적 검사 | git --no-optional-locks diff --check 통과, 새 테스트 direct Notify 호출 없음 | ImplementationUnit2/CueRefresh/static_checks.txt |

전체 100개 중 warning 없는 성공은 90개, warning을 동반한 성공은 10개다. warning 22개는 기존 invalid-authoring fixture·판매 pool/수거함·preview material 미지정·수건 slot/payload 거부와 Template의 Boiler/Cooler/Circulator instance 없음(CDO 호환성 확인), 이동 불가 static mesh 등을 보고했다. 새 cue 테스트는 warning/error 0이다.
전체에 Towel 4개, Service 22개, Interaction 16개(held-use 포함), Utility 10개가 포함된다. 이전 F1~F4의 99개에 이번 새 테스트 1개가 추가됐다. 이전 빌드/회귀 상세는 Saved/ImplementationUnit2/Rework/ 및 CueRefresh/Before/.md/PROMPT_REVIEW.md에 유지돼 있다.
초기 DefaultMap 테스트 01·02는 새 테스트 1개가 실패했다(각 error 2): RefreshFollowsTransfersAndRepeats의 Shelf/UsedBin query revision 14/7 기대값에 0이 반환됐다. 비활성 query fixture가 PackagePhysicalRoot trace 충돌을 끈 뒤 복구하지 않아 suppression 해제에서 조준을 잃은 것이었다. 충돌 복구와 매 cue 검사 실제 focus 확인을 추가한 뒤 03에서 전부 통과했다. 냉장고 default query probe도 조준선 밖에 둔다. 실패 로그를 삭제하거나 테스트를 제외하지 않았다.
startup의 optional profiling DLL·Zen/DDC·Rider/EOS 환경 메시지는 전체 로그에 남겼다. DDC는 명시한 memory fallback으로 초기화돼 테스트까지 실행됐으며 fatal 없이 정상 종료했다. Automation test event와 startup 메시지를 구별한다.

## 보존 확인과 API/Editor 영향

- 시작 전 UnrealEditor 종료를 확인했고 검증 종료 뒤 headless process도 종료됐다.
- Content/Config status 전체 문자열과 변경 13개 uasset SHA256이 시작/종료에 동일하다. Content가 clean이었다는 뜻이 아니다. 기존 Shower·Washer·Dryer·상품·DefaultMap external actor 및 신규 Vanity/DA 작업물을 그대로 유지했다. Config 변경 없음.
- 시작 snapshot: Saved/ImplementationUnit2/CueRefresh/content_before.json·files_before.json. 종료 확인: verification_final.json. 기존 Source 중 위 8개 외 파일은 SHA256 동일하며 신규 Source 파일도 없다.
- 6개 production 변경에서 지정 revision 추가 부분만 제거하면 시작 SHA256과 일치한다(production_delta_verified.json). 따라서 cue Update·owner observer·뚜껑/HUD/transfer 조건의 기존 구현은 보존했다.
- 종료 검사 중 FEEDBACK_BACKLOG.md의 별도 변경을 감지했다. 구현 단계는 이 파일을 수정하지 않았고 현재 내용을 보존했다. 이 별도 변경은 verification_final.json에서 구현 변경과 구분한다.
- Architecture·Unreal 정본·구현/재작업 입력·QNA는 수정하지 않았다. 정본은 이미 선택된 경로를 확정한 입력이므로 이번 결과를 중복 기록하지 않는다.
- additive query UPROPERTY뿐이며 reflected rename/삭제·parent/subobject 변경·Core Redirect·BP migration이 없다. 구현 프롬프트가 정한 DefaultMap 단일 로드 gate를 최종 실행으로 통과했다. asset Save·commit·push는 수행하지 않았다.

## 리뷰 중점과 남은 PIE

revision 합이 모든 반환 경로에 채워지는지, Equals 외 실행/반복 조건을 바꾸지 않는지, 바구니만 변하는 경우도 갱신되는지, 새 자동화가 observer 생략을 우회하지 않는지를 리뷰한다.
[PROMPT_UNREAL.md](PROMPT_UNREAL.md) 맨 앞에 네 수건 대상 연속 LMB/RMB 매 장 cue 이동과 조준 중 실제 손님 획득을 추가했다. 이번 새 authoring·저장 대상은 없다.
headless transform 검증은 실제 화면 반투명/외곽선·lid animation·authored mesh bounds·실제 손님 이동 PIE 확인을 대체하지 않는다. 이 시각/통합 확인은 다음 단계에 남긴다.
