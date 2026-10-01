# 구현 해결 기록 — namespace·상품 수 assertion

- 작업 ID: `SERVICE-U4`
- 단계: 구현
- 상태: 완료

## 이전 namespace 차단 근거 (해결)

- 사용자 PIE 실패 재작업 1회차의 현상 1 Source·자동화 테스트·Scrub Focus Session 문서 수정은 작성했다. 현상 2와 carry 공통 계약은 그대로다.
- 지정한 `Build.bat BathhouseSimEditor Win64 Development -Project='C:\UnrealProjects\BathhouseSim\BathhouseSim.uproject' -WaitMutex -NoHotReloadFromIDE`를 필요한 Engine/Uba 권한으로 실행했다.
- 결과: exit 6, `Failed (OtherCompilationError)`, 124.87초. UHT 성공, 이번 변경 파일 두 개의 컴파일 오류는 보고되지 않았으나 전체 빌드·DLL 링크 완료가 아니다.
- 최초 오류: `Source/BathhouseSim/Private/Tests/ServiceFacilityDisplayAutomationTests.cpp:14`, C2872 `FFixture` 모호함. 같은 파일 137·226행도 무수식 `FFixture`를 쓴다.
- 후보는 `ServiceAmenityTest::FFixture`와 `ServiceFacilityTest::FFixture`다. 기존 `ServiceAmenityChairAutomationTests.cpp:7`, `ServiceAmenityRestAutomationTests.cpp:8`의 global `using namespace ServiceAmenityTest`와 `ServiceFacilityDisplayAutomationTests.cpp:6`의 global `using namespace ServiceFacilityTest`가 unity 컴파일에서 겹친다.
- 이번 변경 테스트 `ServiceAmenityScrubAutomationTests.cpp`는 adaptive build로 unity에서 제외되어 별도로 컴파일됐다. 오류 파일과 위 기존 파일들은 이번 재작업에서 수정하지 않았다.
- 로그: `Saved/Automation/Reports/2026-10-01/SERVICE-U4-R1/build.log`.
- 빌드 Source 식별값: HEAD `3b0e2ca696e89a61a280a82227202427f680b435`, `git diff HEAD -- Source Config` SHA-256 `b31599ae8c9023829f808c8a24c0c206c354c8ce1f588a0a4e54bd4ce8e29756`.
- 파일별 hash: `Saved/Automation/Reports/2026-10-01/SERVICE-U4-R1/build_source_identity.json`.

## 이전 책임·재개 조건 (마스터 승인으로 해결)

- 책임 단계: 구현. 이 재작업은 현상 1만 수정하도록 지정되어 있으므로 별도 서비스 진열 테스트를 임의로 수정하지 않았다. 사용자 질문 없이 차단을 기록하고 중단한다.
- 마스터가 기존 테스트 namespace 충돌 해결을 구현 범위에 포함하거나 별도 수정으로 제공해야 한다. 최소 수정 후보는 `ServiceFacilityDisplayAutomationTests.cpp`의 세 `FFixture` 사용을 `ServiceFacilityTest::FFixture`로 명시하는 것이다. runtime·Content 변경은 필요 없다.
- 충돌 해결 후 같은 UE 5.8 Build Policy 명령으로 전체 빌드를 통과해야 한다. 변경된 Source 식별값을 새로 기록한다.
- 이후 Headless Automation Policy의 템플릿 맵, `-unattended -nullrhi -NoSplash -NoSound -DDC-ForceMemoryCache`, `Automation RunTests`·`-TestExit="Automation Test Queue Empty"`를 사용해 `BathhouseSim.Service.Amenity`, `BathhouseSim.Computer`, `BathhouseSim.Interaction`을 실행한다.
- 빌드 실패 동안 Automation은 실행하지 않았다. 기존 DLL로 이번 변경의 테스트 통과를 주장하지 않는다. PIE는 사용자 담당이다.
- 빌드·Automation 통과 및 최종 diff 검증 뒤 `PROMPT_REVIEW.md`와 `PROMPT_UNREAL.md`를 구현 완료로 바꾼다. 입력 `PROMPT_IMPLEMENTATION_R.md` 상태는 수정하지 않았다.

## 마스터 결정과 namespace 해결 결과

- 마스터가 실제 unity namespace 충돌 지점의 타입 명시를 이번 구현 범위에 포함하도록 승인했다. 테스트 동작·runtime·Content 변경은 허용하지 않았다.
- `ServiceFacilityDisplayAutomationTests.cpp:14/137/226`의 세 사용을 `ServiceFacilityTest::FFixture`로 명시했다.
- 재빌드에서 같은 C2872가 추가 확인됐다: `ServiceFacilityPayloadAutomationTests.cpp:17`, `ServiceFacilityReworkAutomationTests.cpp:38/108/176`, `ServiceFacilityShopAutomationTests.cpp:17`. 이 다섯 사용도 같은 namespace로 명시했다. 네 파일 총 8곳의 타입 이름만 변경했고 테스트 동작은 유지했다.
- 재개 첫 빌드 로그: `Saved/Automation/Reports/2026-10-01/SERVICE-U4-R1-Resume/build.log`(추가 충돌 검출, exit 6, 12.46초).
- 최종 전체 빌드는 같은 Build Policy 명령으로 성공했다(exit 0, DLL 링크 포함, 36.51초). 로그: `Saved/Automation/Reports/2026-10-01/SERVICE-U4-R1-Resume/build_final.log`.
- 최종 빌드 Source: HEAD `3b0e2ca696e89a61a280a82227202427f680b435`, diff SHA-256 `6194a1309f9d690ab6bf6bfd2acdff8162d7e18fe89bb5df7a5f3db9af27e94f`. 파일별 식별값: `Saved/Automation/Reports/2026-10-01/SERVICE-U4-R1-Resume/build_source_identity.json`.
- 지정한 네 필터의 headless Automation을 실행 완료했다. namespace 충돌은 해결됐지만 아래 별도 assertion 실패로 전체 통과 조건은 충족하지 못했다.

## 상품 수 Automation 차단 당시 기록 (해결)

- Headless Automation Policy의 템플릿 맵과 `-DDC-ForceMemoryCache`를 사용했다. 합집합 필터 `BathhouseSim.Service.Amenity+BathhouseSim.Service+BathhouseSim.Computer+BathhouseSim.Interaction`, 미실행 0, exit 255.
- 결과: 54개 중 53 성공(이 중 5개는 warning 포함), 1 실패. Amenity 12/12, Service 33/34(Amenity 포함), Computer 3/3, Interaction 17/17. 신규 `VisibilitySnapshotCarryLossAndEndPlay`와 이번 변경 표시 assertion은 모두 성공했다.
- 유일한 실패: `BathhouseSim.Service.BlueprintLoad`, `Source/BathhouseSim/Private/Tests/ServiceBlueprintLoadAutomationTests.cpp:145`. `Catalog includes the authored unit-two products`가 `Catalog->Products.Num()`을 16으로 기대하지만 실제 로드 값은 20이다. 환경 Fatal·namespace 컴파일 오류가 아니라 실행된 테스트의 assertion 실패다.
- 최신 승인 범위는 무수식 `FFixture` namespace 명시만이다. 따라서 위 assertion 변경은 추가 범위 확정이 필요하다. `ServiceBlueprintLoadAutomationTests.cpp`와 카탈로그 Content는 수정하지 않았다.
- 책임 단계: 구현. 마스터가 현재 상품 수 계약에 맞춘 테스트 기대값 수정 범위를 정하거나 수정본을 제공한 뒤 재개한다. 개수만 바꿔 통과시키기 전에 현재 authored 상품 계약과 대조해야 한다. runtime·Content 변경은 이번 해결에 포함하지 않는다.
- 새 Source 식별값으로 같은 Build Policy 전체 빌드를 통과시키고 같은 네 필터 Automation을 재실행해야 한다. 통과 후 `PROMPT_REVIEW.md`·`PROMPT_UNREAL.md`·이 QNA를 완료로 갱신한다.
- JSON: `Saved/Automation/Reports/2026-10-01/SERVICE-U4-R1-Resume/Automation/index.json`. 전체 Editor 로그: `Saved/Automation/Reports/2026-10-01/SERVICE-U4-R1-Resume/automation_editor.log`. 상세 결과는 [PROMPT_REVIEW.md](PROMPT_REVIEW.md).
- 사용자 질문 없이 새 차단을 기록하고 중단한다. 커밋·Content 재저장·PIE 실행 없음.

## 상품 수 계약에 대한 추가 마스터 결정

- 마스터가 기대값 16은 서비스 2단위의 잔존 값이며 `f15742a`의 SVC4-001에서 안마의자·평상·TV·세신대 네 상품을 추가해 현재 20개라고 확정했다. 20·unit-four 메시지 갱신과 네 ID 존재 assertion만 이번 구현 범위에 포함했다.
- 해당 커밋의 `.md/PROMPT_UNREAL.md`에서 ProductId `MassageChair`, `RestBench`, `Television`, `ScrubTable`의 append 계약을 읽어 확인했다.
- `ServiceBlueprintLoadAutomationTests.cpp`의 카탈로그 assertion만 승인대로 갱신했다. 다른 테스트·runtime·Content는 추가 변경하지 않았다. 같은 Build Policy와 `BathhouseSim.Service`, `BathhouseSim.Computer`, `BathhouseSim.Interaction` headless 검증을 재실행한다.

## 최종 해결과 검증 결과

- 기대값을 20, 메시지를 `Catalog includes the authored unit-four products`로 갱신했다. SVC4-001의 네 ProductId 존재만 추가 확인하고 기존 상품별 정의·가격 검증을 유지했다.
- 같은 UE 5.8 Build Policy 전체 빌드 성공(exit 0, DLL 링크 포함, 21.72초).
- Headless Automation Policy의 템플릿 맵·`-DDC-ForceMemoryCache`로 `BathhouseSim.Service+BathhouseSim.Computer+BathhouseSim.Interaction` 실행 성공(exit 0). **54/54 통과**, 실패·미실행 0, 이 중 5개는 warning 포함 성공이다.
- Service 34/34(Amenity 12 포함), Computer 3/3, Interaction 17/17. BlueprintLoad의 상품 수·네 ID 검증과 세신 표시·snapshot·EndPlay 테스트가 모두 성공했다.
- 최종 HEAD `3b0e2ca696e89a61a280a82227202427f680b435`, Source/Config diff SHA-256 `000f88cbfa1559b47535a2edc06aa3ec9f4a68f4a53c8c1ffb499f7f6be41f3f`.
- 빌드 로그·파일별 hash: `Saved/Automation/Reports/2026-10-01/SERVICE-U4-R1-Catalog/build.log`, `build_source_identity.json`. Automation: 같은 폴더의 `Automation/index.json`, `automation_editor.log`.
- 두 차단 모두 해결되어 `PROMPT_REVIEW.md`·`PROMPT_UNREAL.md`·이 QNA를 구현 완료로 갱신했다. 이전 차단·재개 조건은 당시 기록이며 현재 미해결 항목은 없다.
- `git diff --check` 통과, Content·Config 추가 변경 없음, 커밋 없음. PIE·실제 화면은 사용자 검증으로 남는다.
