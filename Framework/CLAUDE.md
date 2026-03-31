# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Build System

- **Visual Studio 2022** solution: `Framework.sln` (MSBuild, v143 toolset)
- Two projects: **Engine** (DLL) and **Client** (EXE). Client depends on Engine.
- Configurations: Debug/Release × Win32/x64
- Build from VS or command line: `msbuild Framework.sln /p:Configuration=Debug /p:Platform=x64`
- After building Engine, run `UpdateLib.bat` to copy headers to `EngineSDK/Inc/` and libs to `EngineSDK/Lib/`
- Output: `Engine/Bin/Engine.dll`, `Engine/Bin/Engine.lib`, `Client/Bin/Client.exe`

## Architecture

**Engine/Client separation:** Engine is a reusable DLL providing core 3D framework services. Client is a WinAPI application that links against Engine and implements concrete game logic.

**Class hierarchy:** All classes inherit from `CBase`, which provides COM-style reference counting (AddRef/Release). Never use raw `delete`; always use `Safe_Release`.

```
CBase (ref counting)
├── CGameInstance (singleton) — central coordinator for graphics, timers, levels, objects, renderer, input, pipeline, lights
├── CGraphic_Device — D3D11 device, context, swap chain initialization
├── CInput_Device (final) — Raw Input 기반 키보드/마우스 입력 처리, WndProc 이벤트 → 프레임 데이터 2단계 구조
├── CPipeLine (final) — View/Proj 행렬 저장소, 역행렬 자동 계산, 카메라 월드 위치 추출
├── CComponent (abstract) — base for components, holds Device/Context, supports prototype/clone
│   └── CTransform (abstract) — world matrix, state accessors, scale, Bind_ShaderResource (base for 3D/2D)
│       └── CTransform_3D (final) — 3D movement: Go_Straight/Backward/Left/Right, Rotation/Turn, LookAt
│       └── CTransform_2D (final) — 2D movement: Move_X, Move_Y
│   └── CShader (final) — FX11 Effect wrapper, HLSL compilation, Bind_Matrix/Bind_SRV, prototype/clone
│   └── CTexture (final) — texture loading (DDS/WIC), multi-texture SRV array, Bind_ShaderResource
│   └── CModel (final) — Assimp FBX 로딩, MODEL enum(NONANIM/ANIM), CMesh+CMaterial 관리, 메시별 Render(meshIndex)+Bind_Material
│   └── CVIBuffer (abstract) — vertex/index buffer base, IA stage binding, DrawIndexed
│       └── CVIBuffer_Rect (final) — rectangle mesh (4 vertices, 6 indices, VTXTEX format)
│       └── CVIBuffer_Terrain (final) — terrain mesh from heightmap
│       └── CMesh (final) — aiMesh → VTXMESH GPU 버퍼 변환, PreTransformMatrix 적용, MaterialIndex 보유, Clone()=nullptr
├── CMaterial (final) — CBase 상속, aiMaterial에서 텍스처 SRV 로딩, m_Materials[aiTextureType][index] 2차원 배열
├── CLevel (abstract) — base for game levels (Logo, Loading, GamePlay)
├── CGameObject (abstract) — base for game entities, auto-creates CTransform (3D/2D by TRANSFORMTYPE), supports Clone pattern, GAMEOBJECT_DESC
│   └── CCamera (abstract) — camera base: CAMERA_DESC (vEye/vAt/fFovy/fNear/fFar), Update_PipeLine, m_ProjMatrix
│       └── CCamera_Free (Client) — FPS 자유 카메라: WASD 이동 + 마우스 Yaw/Pitch, fMouseSensor
│   └── CMonster (Client, final) — CModel 기반 3D 오브젝트: Shader_VtxMesh + Model_Fiona, 메시별 Material 바인딩 + Phong Lighting
│   └── CForkLift (Client, final) — CModel 기반 3D 오브젝트: Shader_VtxMesh + Model_ForkLift, 랜덤 배치, 메시별 렌더링
├── CUIObject (abstract) — base for UI entities, inherits CGameObject, screen-space positioning via Update_UIState, orthographic View/Proj matrices
├── CLayer — groups GameObjects at the same depth, delegates update/render
├── CLevel_Manager (singleton) — manages current level and transitions
├── CTimer_Manager (singleton) — named timers with delta time
├── CPrototype_Manager (singleton) — prototype registry, clones GameObjects by key
├── CObject_Manager (singleton) — level/layer-based GameObject lifecycle management
├── CRenderer (singleton-like) — render group management, ordered draw calls (Priority/NonBlend/Blend/UI)
├── CLight (final) — individual light instance, stores LIGHT_DESC, CBase 상속 (CComponent 아님)
├── CLight_Manager (final) — list<CLight*> 기반 광원 관리, CGameInstance가 소유 (싱글톤 아님)
└── CLoader — threaded asset loading (HANDLE + CRITICAL_SECTION), CoInitializeEx for WIC
```

**Key patterns:**
- **Singletons** via `DECLARE_SINGLETON` / `IMPLEMENT_SINGLETON` macros
- **Prototype/Clone** for GameObjects (register prototypes, clone instances)
- **Component system**: CComponent abstract base → CTransform (abstract base) → CTransform_3D/CTransform_2D, CShader, CTexture, CVIBuffer, CModel. CGameObject auto-creates CTransform_3D or CTransform_2D based on GAMEOBJECT_DESC::eTransformType in Initialize(). CVIBuffer/CShader/CTexture/CModel uses Prototype/Clone with shared GPU resources
- **Material system**: CModel 내부에서 CMaterial을 소유. FBX의 aiMaterial에서 텍스처 SRV 추출. 메시→Material 인덱스 매핑으로 메시별 텍스처 바인딩
- **Descriptor chain**: TRANSFORM_DESC → GAMEOBJECT_DESC (+ iFlag, eTransformType=TRANSFORM_3D 기본값) → CAMERA_DESC (+ vEye/vAt/fFovy/fNear/fFar) / UIOBJECT_DESC (+ fCenterX/Y, fSizeX/Y, auto TRANSFORM_2D) → BACKGROUND_DESC / TERRAIN_DESC / MONSTER_DESC / FORKLIFT_DESC (Client측 확장). Passed through Initialize(void* pArg)
- **Threaded loading** via CLoader for level transitions
- **HRESULT error handling** with `if (FAILED(...)) return E_FAIL;` pattern and `NULL_CHECK` macro
- **Layered object management**: Level → Layer → GameObject hierarchy via CObject_Manager
- **3-stage update pipeline**: Priority_Update → Update → Late_Update (CObject_Manager orchestrates)
- **Render group system**: GameObjects register themselves to CRenderer in Late_Update, CRenderer draws in order (Priority → NonBlend → Blend → UI)

**Execution flow:** `Client.cpp` WinMain → `CMainApp::Initialize()` → `CGameInstance::Initialize_Engine(ENGINE_DESC)` → main loop calls Update/Render → `CObject_Manager` runs 3-stage update (Priority/Update/Late, GameObjects register to CRenderer in Late_Update) → `CGameInstance::Draw()` calls `CRenderer::Draw()` then `CLevel_Manager::Render()`. Shutdown: `CGameInstance::Release_Engine()` → `DestroyInstance()`.

## Coding Conventions

- Class names: `C` prefix (`CBase`, `CGameInstance`)
- Member variables: `m_` prefix (`m_pDevice`, `m_iRefCnt`)
- Methods: PascalCase (`Initialize`, `Priority_Update`, `Late_Update`)
- Namespaces: `Engine`, `Client` via `NS_BEGIN` / `NS_END` macros
- Singleton classes use `final` keyword
- Unicode throughout (wchar_t, `TEXT()`)
- Return `HRESULT` from initialization and setup functions
- Use `Safe_Delete`, `Safe_Release`, `Safe_AddRef` templates from `Engine_Function.h`
- Enum casting via `ETOI` (to int) / `ETOUI` (to unsigned int) macros

## Project File Tree

```
Framework/
├── Engine/
│   ├── Public/                     — 엔진 헤더
│   │   ├── Engine_Defines.h        — 마스터 인클루드 (D3D11, FX11, DirectXTK, Assimp, STL)
│   │   ├── Engine_Enum.h           — 전역 enum 정의
│   │   ├── Engine_Function.h       — Safe_Delete/Release/AddRef 템플릿
│   │   ├── Engine_Macro.h          — ETOI, NULL_CHECK, SINGLETON 등 매크로
│   │   ├── Engine_Struct.h         — ENGINE_DESC, LIGHT_DESC, 정점 구조체 (VTXTEX/VTXNORTEX/VTXMESH + embedded Elements[])
│   │   ├── Engine_Typedef.h        — 타입 별칭 (_float, _vector, _matrix 등)
│   │   ├── Base.h                  — 레퍼런스 카운팅 베이스
│   │   ├── GameInstance.h          — 싱글톤 퍼사드: 전 서브시스템 위임
│   │   ├── Graphic_Device.h        — D3D11 디바이스/스왑체인
│   │   ├── Timer.h / Timer_Manager.h — 타이머
│   │   ├── Level.h                 — 레벨 베이스
│   │   ├── Level_Manager.h         — 레벨 전환 관리
│   │   ├── GameObject.h            — 게임오브젝트 베이스 (eTransformType으로 3D/2D 자동 생성)
│   │   ├── UIObject.h              — UI 오브젝트 베이스 (직교 투영, CTransform_2D)
│   │   ├── Layer.h                 — 오브젝트 그룹
│   │   ├── Object_Manager.h        — 레벨/레이어별 오브젝트 관리 (싱글톤)
│   │   ├── Prototype_Manager.h     — 프로토타입 저장/클론 (GAMEOBJECT/COMPONENT 분기)
│   │   ├── Renderer.h              — 렌더 그룹 관리 (Priority→NonBlend→Blend→UI)
│   │   ├── Component.h             — 컴포넌트 베이스 (Prototype/Clone)
│   │   ├── Transform.h             — 트랜스폼 추상 베이스 (월드 행렬, STATE enum)
│   │   ├── Transform_3D.h          — 3D 이동/회전 (Go_Straight, Turn, LookAt 등)
│   │   ├── Transform_2D.h          — 2D 이동 (Move_X/Y)
│   │   ├── VIBuffer.h              — VB/IB 추상 베이스
│   │   ├── VIBuffer_Rect.h         — 사각형 메시 (VTXTEX)
│   │   ├── VIBuffer_Terrain.h      — BMP 하이트맵 지형 (VTXNORTEX)
│   │   ├── Shader.h                — FX11 Effect 래퍼 (Bind_Matrix/Bind_SRV/Begin)
│   │   ├── Material.h               — FBX 재질 텍스처 추출 (CBase, aiMaterial→SRV)
│   │   ├── Mesh.h                  — aiMesh→GPU 버퍼 (PreTransformMatrix, MaterialIndex, Clone=nullptr)
│   │   ├── Model.h                 — Assimp FBX 로딩, CMesh+CMaterial 관리, MODEL enum, 메시별 렌더링
│   │   ├── Camera.h                — 카메라 추상 베이스 (CAMERA_DESC, PipeLine에 View/Proj 전달)
│   │   ├── Input_Device.h          — Raw Input 입력 (VK 코드, 2단계 static→프레임 구조)
│   │   ├── PipeLine.h              — View/Proj 행렬 저장 + 역행렬/카메라 위치 계산
│   │   ├── Texture.h               — 멀티 텍스처 SRV 배열 (DDS/WIC)
│   │   ├── Light.h                 — 광원 인스턴스 (CBase, LIGHT_DESC)
│   │   ├── Light_Manager.h         — 광원 리스트 관리 (CGameInstance 소유, 비싱글톤)
│   │   ├── fx11/                   — FX11 헤더
│   │   ├── DirectXTK/              — DirectXTK 헤더 (DDS/WIC 로더)
│   │   └── assimp/                 — Assimp 헤더
│   ├── Private/                    — 엔진 구현 (.cpp, 헤더와 1:1 대응)
│   ├── ThirdPartyLib/              — 정적 라이브러리 (Effects11, DirectXTK, Assimp — Debug/Release)
│   └── Bin/                        — 빌드 출력 (Engine.dll/lib)
│
├── Client/
│   ├── Default/Client.cpp          — WinMain, 60FPS 루프, WndProc (Process_RawInput 연결)
│   ├── Public/
│   │   ├── Client_Defines.h        — 창 크기(1280×720), LEVEL enum
│   │   ├── MainApp.h               — 엔진 초기화 + 메인 루프
│   │   ├── Level_Logo.h            — 로고 레벨 (Enter→전환)
│   │   ├── Level_Loading.h         — 로딩 레벨 (CLoader 스레드)
│   │   ├── Level_GamePlay.h        — 게임플레이 (Light+Camera+BG+Monster 레이어)
│   │   ├── Loader.h                — 멀티스레드 리소스 로더 (CoInitializeEx 필요)
│   │   ├── BackGround.h            — UI 배경 (CUIObject, RENDERID::UI)
│   │   ├── Terrain.h               — 지형 (NONBLEND, Phong Lighting 바인딩)
│   │   ├── Camera_Free.h           — FPS 카메라 (WASD+마우스)
│   │   ├── Monster.h               — 3D 모델 오브젝트 (CModel Fiona, NONBLEND)
│   │   └── ForkLift.h              — 3D 모델 오브젝트 (CModel ForkLift, 랜덤 배치, NONBLEND)
│   ├── Private/                    — Client 구현 (.cpp, 헤더와 1:1 대응)
│   ├── Bin/ShaderFiles/
│   │   ├── Shader_VtxTex.hlsl      — VTXTEX용: WVP + 텍스처 샘플링
│   │   ├── Shader_VtxNorTex.hlsl   — VTXNORTEX용: Phong Lighting
│   │   └── Shader_VtxMesh.hlsl     — VTXMESH용: Phong Lighting + g_DiffuseTexture
│   └── Bin/Resources/              — Textures/, Models/ (FBX)
│
├── EngineSDK/                      — UpdateLib.bat이 복사하는 SDK (Inc+Lib)
├── UpdateLib.bat                   — Engine 헤더/lib → EngineSDK 복사
└── Framework.sln
```

## Important Implementation Notes (Gotchas)

### ENGINE_DESC — iNumLevels 누락 시 크래시
```cpp
EngineDesc.iNumLevels = ETOUI(LEVEL::END);  // 반드시 설정. Prototype_Manager 내부 배열 크기 결정
```

### Release 순서
- `CGameObject::Free()`: m_pGameInstance → Context → Device 순서
- `CMainApp::Free()`: `Release_Engine()` 먼저 → `Safe_Release(m_pGameInstance)` 다음
- `Release_Engine()` 내부: Light_Manager(가장 먼저) → ... → Renderer(가장 나중에 생성된 것부터)

### Level Transitions — SUCCEEDED 사용
```cpp
if (SUCCEEDED(m_pGameInstance->Change_Level(...)))  // FAILED()가 아님!
    return;
```

### Render Group — AddRef/Release 사이클
- `Late_Update()`에서 `Add_RenderGroup(RENDERID, this)` → 내부에서 `Safe_AddRef()`
- `CRenderer::Draw()`에서 그린 후 `Safe_Release()` + clear. 매 프레임 반복

### Update 파이프라인 순서
`Input_Device::Update` → `Priority_Update` → `Update` → `PipeLine::Update` → `Late_Update`
- 카메라는 Priority_Update에서 입력 처리 (다른 오브젝트가 갱신된 View/Proj 사용하도록)

### 정점 포맷 ↔ 셰이더 매칭 (불일치 시 크래시)
| 오브젝트 | 정점 포맷 | 셰이더 | Elements 전달 |
|----------|-----------|--------|---------------|
| BackGround (UI) | VTXTEX | Shader_VtxTex | VTXTEX::Elements |
| Terrain | VTXNORTEX | Shader_VtxNorTex | VTXNORTEX::Elements |
| Monster/ForkLift (Model) | VTXMESH | Shader_VtxMesh | VTXMESH::Elements |

### CModel 복사 생성자 — m_Meshes + m_Materials 복사 필수
복사 생성자에서 `m_Meshes`/`m_Materials` 벡터 복사 + 각각 `Safe_AddRef` 누락 시 Clone의 `Render()`가 빈 벡터를 순회하여 아무것도 안 그려짐. CMesh::Clone()은 nullptr 반환이므로 개별 클론 불가.

### CShader::Bind_SRV — GetVariableByName 사용
`GetConstantBufferByName`이 아닌 `GetVariableByName`→`AsShaderResource`→`SetResource`. 혼동 시 바인딩 실패.

### HLSL ShaderType 설정
vcxproj에서 .hlsl 파일의 ShaderType은 반드시 `Effect (/fx)`. Vertex로 설정하면 FXC가 `main` 진입점을 찾아 빌드 실패.

### WndProc → Raw Input 연결 체인
`Client WndProc` → `CGameInstance::Process_RawInput(lParam)` (static) → `CInput_Device::Process_Input(lParam)` (static). 키 식별은 Virtual Key Code ('W', VK_SPACE 등).

### CLoader 스레드 — CoInitializeEx 필수
별도 스레드에서 WIC 텍스처 로딩 시 `CoInitializeEx(nullptr, 0)` / `CoUninitialize()` 호출 필수 (COM 기반).

### Prototype 등록/생성 패턴
- 등록: `CLoader::Ready_Resources_For_XXX()` → `Add_Prototype(levelIndex, tag, Object::Create(...))`
- 생성: Level의 `Ready_Layer_XXX()` → `Add_GameObject(protoLevel, protoTag, layerLevel, layerTag)`
- `Clone_Prototype()`은 `PROTOTYPE` enum으로 분기: GAMEOBJECT→`Clone()`, COMPONENT→`Clone()`

### GameObject Render() 호출 순서
- **VIBuffer 기반**: `Bind_ShaderResources()` → `Shader::Begin()` → `VIBuffer::Bind_Resources()` → `VIBuffer::Render()`
- **CModel 기반 (메시별 렌더링)**: `Bind_ShaderResources()` → for(각 메시) { `Bind_Material(i, DIFFUSE)` → `Shader::Begin()` → `Model::Render(i)` }

### Descriptor 상속 체인
`TRANSFORM_DESC` → `GAMEOBJECT_DESC`(+eTransformType, 기본=3D) → `CAMERA_DESC`(+vEye/vAt/fFovy) / `UIOBJECT_DESC`(+fCenterX/Y, 자동 2D) → Client DESC 확장. pArg nullptr이면 기본값 TRANSFORM_3D.

## 명세서 작성 원칙

명세서 작성 시 [`SPEC_GUIDE.md`](SPEC_GUIDE.md) 참고.

## CLAUDE.md Update Routine

**Trigger conditions (둘 다 적용):**
- **Auto**: When project files are added, removed, or significantly modified during a session, Claude Code proactively proposes CLAUDE.md updates after the main task is complete
- **Manual**: User can request updates at any time (e.g., "CLAUDE.md 갱신해")

**Update steps:**
1. **Detect changes**: Compare current file structure against the Project File Tree above
2. **Identify updates needed**: Check for:
   - New files added (update tree + add one-line description)
   - Files removed (remove from tree)
   - File role changed significantly (update description)
   - New implementation notes discovered (add to Important Implementation Notes)
   - Architecture changes (update Architecture section)
3. **Propose changes to user**: Present the CLAUDE.md diff in a code block and ask for confirmation
4. **Apply after approval**: Update CLAUDE.md only after user confirms

## Working with Claude Code in This Repository

**Communication:** Korean and English are both acceptable.

**File Modification Policy:**
- **CRITICAL:** Claude Code must NEVER directly modify files using Edit/Write tools (encoding issues occur)
- **EXCEPTION:** CLAUDE.md file can be directly modified by Claude Code
- Always present code changes in markdown code blocks
- User will review and manually apply changes after understanding them
- This prevents encoding corruption in Visual Studio projects

**SubAgent Usage:**
- Use SubAgents (Explore, Task, etc.) to maintain context and handle complex work
- Always review SubAgent outputs before presenting to user
- If output quality is insufficient, re-run the SubAgent task
- Request permission before using SubAgents beyond Explore and Task types
