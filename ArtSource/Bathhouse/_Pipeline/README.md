# BathhouseSim 설비 모델링 공용 파이프라인

`MODEL-M1`(2026-10-02)에서 만든 Blender 5.2용 공용 스크립트입니다. 설비 5종(Boiler_03, Circulator_01, Shower_01, Vanity_01, ClothesLocker_01)이 이 파이프라인으로 만들어져 같은 재질 언어를 씁니다.

## 스타일 기준

- 레퍼런스: Fab "Stylized House Interior"(StylArts, https://fab.com/s/a11a111085b5). 프로젝트 `Content/StylizedKitchen` 팩과 같은 제품입니다.
- 1950년대 레트로 형태를 씁니다. 두꺼운 둥근 모서리, 모서리 위주의 마모, 위가 밝고 아래가 어두운 그라데이션이 특징입니다.
- 원본 팩의 메시·텍스처는 추출하거나 재사용하지 않았습니다.
- 레퍼런스에서는 형태와 재질 표현 방식만 참고하고 색감은 참고하지 않습니다([MODELING_STYLE_GUIDE.md](../../../.md/MODELING_STYLE_GUIDE.md) 1·3절).
- 색과 재질 종류는 구역별로 다릅니다(욕탕 `bath` 민트 도장·타일, 홀 `hall` 나무, 작업 `work` 무도장 철). 정본은 `palettes.json`입니다. 에셋별 색은 그 파일의 `assets.<에셋>.override`에서 바꾼 뒤 `run_all.sh`로 다시 굽습니다.
- 모든 재질에 노화 층(틈새 때·흘러내림·바닥 오염·먼지·녹·녹청)이 씌워집니다. 저티어는 낡고 고티어일수록 깨끗합니다. 강도는 `palettes.json` `tiers`에서 `assets.<에셋>.tier`로 고르고, `age`(0~1)로 직접 줄 수도 있습니다. 비교 렌더용으로 `TIER_OVERRIDE=<n>`·`AGE_OVERRIDE=<v>`를 build에 넘길 수 있습니다.

## 파일

| 파일 | 역할 |
|---|---|
| `palettes.json` | 색 정본: 공통 재료색(`base`), 구역 테마(`themes`: 역할별 색·재질 종류), 에셋별 테마·덮어쓰기(`assets`) |
| `bh_lib.py` | 형상 헬퍼(box/cyl/lathe/torus/tube/prism), 테마 적용(`apply_asset_palette`), 절차 재질(도장·무도장 철·금속·플라스틱·나무·타일·박스투영 타일·거울·이미지), 다이얼·라벨 텍스처 렌더러 |
| `bh_bake.py` | prep(파트 병합·UV0 아틀라스) → bake(Base/Rough/Metal/Normal/AO, Cycles GPU) → finalize(그룹별 분리·pivot 이동·UV1 라이트맵·UCX) → export(FBX) → validate(재수입 검증) → preview(턴어라운드) → save(.blend·manifest) |
| `run_bridge.py` | 열린 Blender의 MCP 애드온 브리지(127.0.0.1:9876)에 단계를 보냅니다 |
| `run_all.sh` | 에셋 하나를 build부터 save까지 실행합니다 |

각 에셋 폴더의 `scripts/build.py`는 파트를 만들고, `scripts/asset_cfg.py`에는 pivot·충돌체·미리보기 설정이 있습니다.

## 좌표 규칙

- build·cfg 값은 모두 **해당 Blueprint SceneRoot 로컬 Unreal cm**입니다. `U(x, y, z)`가 Blender m로 바꾸며 Y 부호를 뒤집습니다.
- 근거는 `.md/Work/MODEL-M1/REPORT_UNREAL_DISCOVERY.md` 1절입니다. Bath_01 glb import에서 `UE = (Bx, −By, Bz) × 100`을 실측했습니다.
  - FBX 경로도 확인했습니다(2026-10-02 사용자 import). `SM_Boiler_03`의 정면(Blender +Y)이 UE −Y로 들어왔습니다. 조건: 이 파이프라인의 export 설정 + Unreal import Force Front X Axis 끔(기본값).
- Blender Front 뷰(Numpad 1)는 UE +Y 면입니다. UE +X 정면은 Right 뷰(Numpad 3), UE −Y 정면은 Back 뷰(Ctrl+Numpad 1)에서 보입니다.
- 움직이는 부품(문·레버·바늘)은 별도 FBX이고 mesh 원점이 BP pivot입니다. BP에서 자식 mesh의 상대 위치를 0, scale을 1로 두면 맞습니다.

## 다시 만들기

```bash
sh ArtSource/Bathhouse/_Pipeline/run_all.sh ArtSource/Bathhouse/Boiler_03
```

- Blender에 Lab MCP 애드온 브리지가 켜져 있어야 합니다. 실행하면 열린 세션의 장면이 비워집니다. 작업 중인 Blender에는 실행하지 마세요.
- 재질·팔레트를 바꾸면 5종을 모두 다시 돌려야 세트가 유지됩니다.

## Unreal import(공통)

1. FBX를 import합니다. Generate Missing Collision은 끄고(UCX 사용), Normal Import Method는 Import Normals and Tangents, Uniform Scale은 1입니다.
2. 텍스처 설정:
   - BaseColor: sRGB
   - ORM: sRGB 끔, Masks 압축 (R=AO, G=Roughness, B=Metallic)
   - Normal: Normalmap (DirectX 방식이라 Flip Green 끔)
3. 재질 슬롯은 에셋마다 1개(`M_<에셋>`)입니다. 움직이는 부품 FBX도 같은 텍스처 세트를 씁니다.
4. 라이트맵 좌표 인덱스는 1(`UV1_Lightmap`)입니다.
