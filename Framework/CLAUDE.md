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
├── CGameInstance (singleton) — central coordinator for graphics, timers, levels, objects, renderer, input, pipeline
├── CGraphic_Device — D3D11 device, context, swap chain initialization
├── CInput_Device (final) — Raw Input 기반 키보드/마우스 입력 처리, WndProc 이벤트 → 프레임 데이터 2단계 구조
├── CPipeLine (final) — View/Proj 행렬 저장소, 역행렬 자동 계산, 카메라 월드 위치 추출
├── CComponent (abstract) — base for components, holds Device/Context, supports prototype/clone
│   └── CTransform (abstract) — world matrix, state accessors, scale, Bind_ShaderResource (base for 3D/2D)
│       └── CTransform_3D (final) — 3D movement: Go_Straight/Backward/Left/Right, Rotation/Turn, LookAt
│       └── CTransform_2D (final) — 2D movement: Move_X, Move_Y
│   └── CShader (final) — FX11 Effect wrapper, HLSL compilation, Bind_Matrix/Bind_SRV, prototype/clone
│   └── CTexture (final) — texture loading (DDS/WIC), multi-texture SRV array, Bind_ShaderResource
│   └── CVIBuffer (abstract) — vertex/index buffer base, IA stage binding, DrawIndexed
│       └── CVIBuffer_Rect (final) — rectangle mesh (4 vertices, 6 indices, VTXTEX format)
│       └── CVIBuffer_Terrain (final) — terrain mesh from heightmap
├── CLevel (abstract) — base for game levels (Logo, Loading, GamePlay)
├── CGameObject (abstract) — base for game entities, auto-creates CTransform (3D/2D by TRANSFORMTYPE), supports Clone pattern, GAMEOBJECT_DESC
│   └── CCamera (abstract) — camera base: CAMERA_DESC (vEye/vAt/fFovy/fNear/fFar), sets Transform position+LookAt, references CPipeLine
├── CUIObject (abstract) — base for UI entities, inherits CGameObject, screen-space positioning via Update_UIState, orthographic View/Proj matrices
├── CLayer — groups GameObjects at the same depth, delegates update/render
├── CLevel_Manager (singleton) — manages current level and transitions
├── CTimer_Manager (singleton) — named timers with delta time
├── CPrototype_Manager (singleton) — prototype registry, clones GameObjects by key
├── CObject_Manager (singleton) — level/layer-based GameObject lifecycle management
├── CRenderer (singleton-like) — render group management, ordered draw calls (Priority/NonBlend/Blend/UI)
└── CLoader — threaded asset loading (HANDLE + CRITICAL_SECTION), CoInitializeEx for WIC
```

**Key patterns:**
- **Singletons** via `DECLARE_SINGLETON` / `IMPLEMENT_SINGLETON` macros
- **Prototype/Clone** for GameObjects (register prototypes, clone instances)
- **Component system**: CComponent abstract base → CTransform (abstract base) → CTransform_3D/CTransform_2D, CShader, CTexture, CVIBuffer. CGameObject auto-creates CTransform_3D or CTransform_2D based on GAMEOBJECT_DESC::eTransformType in Initialize(). CVIBuffer/CShader/CTexture uses Prototype/Clone with shared GPU resources
- **Descriptor chain**: TRANSFORM_DESC → GAMEOBJECT_DESC (+ iFlag, eTransformType=TRANSFORM_3D 기본값) → CAMERA_DESC (+ vEye/vAt/fFovy/fNear/fFar) / UIOBJECT_DESC (+ fCenterX/Y, fSizeX/Y, auto TRANSFORM_2D) → BACKGROUND_DESC / TERRAIN_DESC (Client측 확장). Passed through Initialize(void* pArg)
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
├── Engine/                         — Core engine DLL project
│   ├── Public/                     — Engine header files
│   │   ├── Engine_Defines.h        — Master header: D3D11, DirectXMath, DirectXCollision, d3dcompiler, FX11(d3dx11effect.h), DirectXTK(DDSTextureLoader/WICTextureLoader), STL includes, debug memory tracking
│   │   ├── Engine_Enum.h           — Enums: WINMODE, PROTOTYPE, TRANSFORMTYPE, RENDERID, STATE, D3DTS, MOUSEBTN, MOUSEAXIS
│   │   ├── Engine_Function.h       — Template utilities: Safe_Delete, Safe_Release, Safe_AddRef
│   │   ├── Engine_Macro.h          — Macros: ETOI/ETOUI, NULL_CHECK, FAILED_CHECK, SINGLETON, DLL export, PURE (= 0)
│   │   ├── Engine_Struct.h         — ENGINE_DESC, VTXTEX (Position + Texcoord + embedded Elements[]), VTXNORTEX (Position + Normal + Texcoord + embedded Elements[])
│   │   ├── Engine_Typedef.h        — Type aliases: _float, _int, _uint, _float2/3/4, _float4x4, _vector/_fvector, _matrix/_fmatrix
│   │   ├── Base.h                  — Abstract base class: reference counting (AddRef/Release)
│   │   ├── GameInstance.h          — Singleton: engine initialization, subsystem delegation (incl. Renderer)
│   │   ├── Graphic_Device.h        — D3D11 device, swap chain, render target, depth stencil management
│   │   ├── Timer.h                 — High-resolution timer via QueryPerformanceCounter
│   │   ├── Timer_Manager.h         — Named timer map manager (create, update, get delta)
│   │   ├── Level.h                 — Abstract base level class (Initialize/Update/Render)
│   │   ├── Level_Manager.h         — Level transition and current level management
│   │   ├── GameObject.h            — Abstract game object: GAMEOBJECT_DESC (+ eTransformType), m_pTransformCom, prototype/clone lifecycle
│   │   ├── UIObject.h              — UI game object abstract base: UIOBJECT_DESC, Update_UIState, orthographic View/Proj, Bind_ShaderResource(D3DTS)
│   │   ├── Layer.h                 — Groups GameObjects at same depth, delegates update/render
│   │   ├── Object_Manager.h        — Level/layer-indexed GameObject lifecycle manager (singleton)
│   │   ├── Prototype_Manager.h     — Level-indexed prototype storage, type-aware cloning (GAMEOBJECT/COMPONENT)
│   │   ├── Renderer.h              — Render group manager: collects GameObjects per frame, draws in render order
│   │   ├── Component.h             — Abstract component base: Device/Context refs, prototype/clone lifecycle
│   │   ├── Transform.h             — Transform abstract base: world matrix, Get/Set_State, Get_Scale, Set_Scale/Scaling, Bind_ShaderResource
│   │   ├── Transform_3D.h          — 3D transform: Go_Straight/Backward/Left/Right, Rotation/Turn, LookAt
│   │   ├── Transform_2D.h          — 2D transform: Move_X, Move_Y (screen-space movement)
│   │   ├── VIBuffer.h              — Abstract vertex/index buffer component: VB/IB management, IA binding, DrawIndexed
│   │   ├── VIBuffer_Rect.h         — Rectangle mesh: 4 VTXTEX vertices, 6 indices, TRIANGLELIST topology
│   │   ├── VIBuffer_Terrain.h      — Terrain mesh from heightmap BMP: VTXNORTEX vertices, 32-bit indices, Create(pHeightMapFilePath)
│   │   ├── Shader.h                — CShader component: FX11 Effect wrapper, HLSL compilation, per-pass InputLayout, Begin(passIndex), Bind_Matrix/Bind_SRV
│   │   ├── Camera.h                — Abstract camera base: CAMERA_DESC (vEye/vAt/fFovy/fNear/fFar), CPipeLine ref, Clone = PURE
│   │   ├── Input_Device.h          — Raw Input 기반 입력 장치: 키보드(VK_*), 마우스(버튼/이동/휠), static 누적→프레임 복사 2단계 구조
│   │   ├── PipeLine.h              — View/Proj 행렬 저장소: Get/Set_Transform, 역행렬 자동 계산, 카메라 월드 위치 추출
│   │   ├── Texture.h               — CTexture component: multi-texture SRV array, DDS/WIC loading, Bind_ShaderResource
│   │   ├── fx11/                   — DirectX Effects 11 headers
│   │   │   ├── d3dx11effect.h      — ID3DX11Effect interface definitions (FX11 library)
│   │   │   └── d3dxGlobal.h        — FX11 global utility definitions
│   │   └── DirectXTK/              — DirectX Tool Kit headers
│   │       ├── DDSTextureLoader.h  — DDS texture file loading (CreateDDSTextureFromFile)
│   │       └── WICTextureLoader.h  — WIC-based image loading: JPG/PNG/BMP (CreateWICTextureFromFile)
│   ├── Private/                    — Engine implementation files
│   │   ├── Base.cpp                — Reference counting: AddRef increments, Release decrements and auto-frees
│   │   ├── GameInstance.cpp         — Initializes all subsystems, delegates calls to managers
│   │   ├── Graphic_Device.cpp      — D3D11 device creation, swap chain setup, clear/present operations
│   │   ├── Timer.cpp               — Frame delta calculation using CPU tick frequency
│   │   ├── Timer_Manager.cpp       — Named timer map CRUD operations
│   │   ├── Level.cpp               — Base level init with device/context refs and cleanup
│   │   ├── Level_Manager.cpp       — Level switching, old level cleanup, update/render delegation
│   │   ├── GameObject.cpp          — Dual constructors, Initialize creates CTransform_3D or CTransform_2D based on TRANSFORMTYPE
│   │   ├── Layer.cpp               — GameObject list management, update/render iteration
│   │   ├── Object_Manager.cpp      — Layer map per level, Add_GameObject clones and inserts into layers
│   │   ├── Prototype_Manager.cpp   — Prototype storage in level-indexed maps, type-aware Clone dispatch
│   │   ├── Renderer.cpp            — Render group add/draw/clear, per-frame object lifecycle with AddRef/Release
│   │   ├── Component.cpp           — Component base: AddRef Device/Context in ctor, release in Free()
│   │   ├── Transform.cpp           — Transform base: init (Identity matrix), Bind_ShaderResource, Set_Scale/Scaling
│   │   ├── Transform_3D.cpp        — 3D movement/rotation implementation, Create/Clone lifecycle
│   │   ├── Transform_2D.cpp        — 2D movement implementation, Create/Clone lifecycle
│   │   ├── Camera.cpp              — Camera base: Initialize extracts projection params, sets Transform position + LookAt via CTransform_3D cast
│   │   ├── Input_Device.cpp        — Raw Input 디바이스 등록, WM_INPUT 처리(Process_Input static), 프레임별 Update 복사
│   │   ├── PipeLine.cpp            — 생성자에서 항등행렬 초기화, Update()에서 역행렬 계산 + View 역행렬에서 카메라 위치 추출
│   │   ├── UIObject.cpp            — UI base: Initialize (screen→world via Update_UIState, ortho View/Proj), Bind_ShaderResource(D3DTS)
│   │   ├── VIBuffer.cpp            — VIBuffer base: copy ctor shares VB/IB with AddRef, Bind_Resources sets IA stage, Free releases buffers
│   │   ├── VIBuffer_Rect.cpp       — Rect mesh prototype: creates VB(4 VTXTEX) and IB(6 _ushort) via CreateBuffer
│   │   ├── VIBuffer_Terrain.cpp    — Terrain mesh: BMP heightmap → VTXNORTEX vertices (height from blue channel /10.f), 32-bit index grid generation
│   │   ├── Shader.cpp              — CShader: D3DX11CompileEffectFromFile, per-pass InputLayout creation, Begin, Bind_Matrix/Bind_SRV, shared Effect/InputLayouts in clone
│   │   └── Texture.cpp             — CTexture: multi-texture loading (DDS/WIC by extension), SRV array, Bind_ShaderResource, shared SRVs in clone
│   ├── ThirdPartyLib/              — Third-party static libraries
│   │   ├── Effects11.lib           — FX11 library (Release)
│   │   ├── Effects11d.lib          — FX11 library (Debug)
│   │   ├── DirectXTK.lib           — DirectX Tool Kit (Release)
│   │   └── DirectXTKd.lib          — DirectX Tool Kit (Debug)
│   └── Bin/                        — Build output: Engine.dll, Engine.lib, Engine.pdb
│
├── Client/                         — Client application EXE project
│   ├── Default/
│   │   └── Client.cpp              — WinMain entry point, window creation, 60 FPS game loop, cleanup
│   ├── Public/                     — Client header files
│   │   ├── Client_Defines.h        — Window size (1280×720), LEVEL enum (STATIC/LOADING/LOGO/GAMEPLAY/END)
│   │   ├── MainApp.h               — Main app class: engine init, update/render loop delegation
│   │   ├── Level_Logo.h            — Logo level: Enter key triggers transition, Ready_Layer_BackGround
│   │   ├── Level_Loading.h         — Loading level: runs loader thread, transitions when complete
│   │   ├── Level_GamePlay.h        — Gameplay level: Ready_Layer_Terrain, spawns Terrain objects
│   │   ├── Loader.h                — Multi-threaded asset loader with CGameInstance for prototype registration (incl. textures)
│   │   ├── BackGround.h            — Background UI object (inherits CUIObject): BACKGROUND_DESC, CShader/CTexture/CVIBuffer_Rect, UI render group
│   │   └── Terrain.h               — Terrain game object: TERRAIN_DESC, CShader/CTexture/CVIBuffer_Terrain, NONBLEND render group
│   ├── Private/                    — Client implementation files
│   │   ├── MainApp.cpp             — Engine initialization with ENGINE_DESC, Ready_Prototype_For_Static (VIBuffer_Rect + Shader_VtxTex via VTXTEX::Elements), starts logo level
│   │   ├── Level_Logo.cpp          — Logo display, Ready_Layer_BackGround adds BackGround via Add_GameObject
│   │   ├── Level_Loading.cpp       — Spawns loader thread, transitions on completion
│   │   ├── Level_GamePlay.cpp      — Gameplay level: Ready_Layer_Terrain creates Terrain via Add_GameObject
│   │   ├── Loader.cpp              — Worker thread loading (CoInitializeEx for WIC), registers Logo(Texture_BackGround + BackGround) and GamePlay(Texture_Terrain + Shader_VtxNorTex + VIBuffer_Terrain + Terrain) prototypes
│   │   ├── BackGround.cpp          — Background UI object: Ready_Components, Bind_ShaderResources (World + UI View/Proj + texture), UI render group, Move_X via CTransform_2D
│   │   └── Terrain.cpp             — Terrain object: Ready_Components (Shader_VtxNorTex/VIBuffer_Terrain/Texture_Terrain), temporary hardcoded View/Proj matrices, NONBLEND render group
│   ├── Bin/                        — Build output: Client.exe, Client.pdb (+ copied Engine.dll)
│   │   ├── ShaderFiles/
│   │   │   ├── Shader_VtxTex.hlsl  — HLSL Effect file (VTXTEX): VS_MAIN (WVP transform), PS_MAIN (texture sampling + alpha test + grayscale), technique11
│   │   │   └── Shader_VtxNorTex.hlsl — HLSL Effect file (VTXNORTEX): VS_IN with NORMAL semantic, WVP transform, texture sampling + alpha test, technique11
│   │   └── Resources/
│   │       └── Textures/            — Texture image files (Default0.jpg, Default1.JPG, Terrain/Height.bmp, Terrain/Tile0.jpg, etc.)
│
├── EngineSDK/                      — SDK distribution folder
│   ├── Inc/                        — Copied Engine headers (via UpdateLib.bat)
│   │   ├── Input_Device.h          — (copied from Engine/Public)
│   │   ├── PipeLine.h              — (copied from Engine/Public)
│   │   ├── Shader.h                — (copied from Engine/Public)
│   │   ├── VIBuffer.h              — (copied from Engine/Public)
│   │   ├── VIBuffer_Rect.h         — (copied from Engine/Public)
│   │   └── fx11/                   — (copied from Engine/Public)
│   └── Lib/                        — Copied Engine.lib (via UpdateLib.bat)
│
├── UpdateLib.bat                   — Post-build: copies Engine headers → EngineSDK/Inc, lib → EngineSDK/Lib
└── Framework.sln                   — Visual Studio 2022 solution file
```

## Important Implementation Notes

### ENGINE_DESC Initialization
When initializing the engine in `CMainApp::Initialize()`, **MUST** set all fields:
```cpp
ENGINE_DESC EngineDesc{};
EngineDesc.hWnd = g_hWnd;
EngineDesc.eWinMode = WINMODE::WIN;
EngineDesc.iViewportWidth = g_iWinSizeX;
EngineDesc.iViewportHeight = g_iWinSizeY;
EngineDesc.iNumLevels = ETOUI(LEVEL::END);  // CRITICAL: Required for Prototype_Manager
```

### Memory Management
- **Release order matters**: In `CGameObject::Free()`, release Context before Device
- `CGameObject::Free()` also releases `m_pGameInstance` (before Context and Device)
- Always use `Safe_Release()` - never raw `delete` on COM objects
- Check for `nullptr` before releasing

### Level Transitions
- Use `SUCCEEDED()` when checking level change results, not `FAILED()`
- Example:
```cpp
if (GetKeyState(VK_RETURN) & 0x8000) {
    if (SUCCEEDED(m_pGameInstance->Change_Level(...)))
        return;
}
```

### Debug-Only Code
- Wrap debug display functions with `#ifdef _DEBUG` / `#endif`
- Example: `CLoader::Show()` function

### Engine Shutdown
- Call `CGameInstance::Release_Engine()` before `Safe_Release(m_pGameInstance)` in `CMainApp::Free()`
- `Release_Engine()` releases all subsystems in reverse order and calls `DestroyInstance()`

### Render Group System
- GameObjects register themselves to a render group in `Late_Update()` via `m_pGameInstance->Add_RenderGroup(RENDERID, this)`
- CRenderer collects objects per frame, draws them in order (Priority → NonBlend → Blend → UI), then releases and clears each group
- Each `Add_RenderGroup()` call does `Safe_AddRef()` on the object; each render pass does `Safe_Release()` after drawing
- CRenderer is created in `CGameInstance::Initialize_Engine()` and released first in `Release_Engine()`

### Resource Clearing
- `CGameInstance::Clear_Resources(iLevelIndex)` calls both `CObject_Manager::Clear()` and `CPrototype_Manager::Clear()` for the given level
- Used during level transitions to free level-specific objects and prototypes

### Component System & Descriptor Chain
- `CComponent` (abstract): Device/Context 보유, Prototype/Clone 패턴 지원하는 컴포넌트 베이스
- `CTransform` (abstract): 월드 행렬, 이동/회전 속도 저장. 공통 메서드: Set_Scale(절대)/Scaling(상대), Get_State/Set_State, Get_Scale, Bind_ShaderResource
  - `CTransform_3D` (final): 3D 이동 메서드: Go_Straight/Backward/Left/Right, Rotation(절대)/Turn(누적 회전), LookAt(타겟 방향)
  - `CTransform_2D` (final): 2D 이동 메서드: Move_X, Move_Y (스크린 좌표 기준 m_fSpeedPerSec 기반 이동)
  - `Bind_ShaderResource(pShader, pConstantName)`: 자신의 WorldMatrix를 CShader::Bind_Matrix로 전달
  - Initialize_Prototype()에서 `XMMatrixIdentity()`로 WorldMatrix 항등행렬 초기화. 복사 생성자에서 WorldMatrix 복사
- `CGameObject::Initialize(void* pArg)` 에서 `GAMEOBJECT_DESC::eTransformType`에 따라 `CTransform_3D::Create()` 또는 `CTransform_2D::Create()`로 트랜스폼 자동 생성. pArg가 nullptr이면 기본값 TRANSFORM_3D
- Descriptor 상속 체인: `TRANSFORM_DESC` → `GAMEOBJECT_DESC` (+ iFlag, eTransformType=TRANSFORM_3D 기본값) → `CAMERA_DESC` (+ vEye/vAt/fFovy/fNear/fFar) / `UIOBJECT_DESC` (+ fCenterX/Y, fSizeX/Y, 생성자에서 eTransformType=TRANSFORM_2D) → `BACKGROUND_DESC` / `TERRAIN_DESC` (Client측 확장)
- `CTransform::Initialize(pArg)`: pArg가 nullptr이면 기본값으로 조기 리턴, 아니면 TRANSFORM_DESC로 캐스팅하여 속도/회전 설정

### UIObject System (CUIObject)
- `CUIObject` (abstract): CGameObject 상속. UI 전용 중간 클래스
- `UIOBJECT_DESC`: fCenterX/Y(화면 중심 좌표), fSizeX/Y(크기). 생성자에서 `eTransformType = TRANSFORMTYPE::TRANSFORM_2D` 자동 세팅
- `Initialize(pArg)`: DESC에서 UI 파라미터 파싱 → `__super::Initialize()`로 CTransform_2D 생성 → `Update_UIState()`로 초기 월드 행렬 세팅 → 직교 View(항등)/Proj 행렬 생성
- `Update_UIState()`: m_fCenterX/Y, m_fSizeX/Y를 기반으로 Set_Scale + Set_State(POSITION)으로 스크린좌표→월드좌표 변환. Initialize에서 한 번 호출, 이후 이동은 CTransform_2D::Move_X/Y로 직접 WorldMatrix 수정
- `Bind_ShaderResource(pShader, pConstantName, D3DTS)`: UI 전용 View/Proj 행렬을 셰이더에 바인딩
- `m_TransformMatrices[D3DTS::END]`: UI용 View(항등행렬) + Proj(직교투영) 저장
- UI 객체는 RENDERID::UI 그룹에 등록하여 마지막에 렌더링

### Input System (CInput_Device — Raw Input)
- 수업 원본은 DirectInput8 사용 → **Raw Input**으로 교체 (DirectInput8은 MS 레거시, 현업/상용 엔진은 Raw Input + XInput 사용)
- `CInput_Device` (final): CBase 상속. ENGINE_DLL 없음 (CGameInstance 통해서만 접근, 내부 클래스)
- **2단계 데이터 흐름**: WndProc의 WM_INPUT → static 누적 버퍼 → Update()에서 프레임 데이터로 복사
  - `static s_byRawKeyState[256]`, `static s_byRawMouseBtn[]`, `static s_lRawMouseAccum[]`: WndProc에서 누적
  - `m_byKeyState[256]`, `m_byMouseBtnState[]`, `m_lMouseDelta[]`: 게임 로직이 읽는 프레임 데이터
- `Process_Input(LPARAM)`: static 메서드, WndProc에서 호출. GetRawInputData()로 RAWINPUT 파싱 → 키보드(VK_* 코드, 0x80/0x00) + 마우스(이동 누적, 버튼 상태, 휠 delta)
- `Initialize(HWND)`: RegisterRawInputDevices()로 키보드(0x06) + 마우스(0x02) 등록
- `Update()`: static 버퍼 → 인스턴스 버퍼 memcpy, 마우스 이동 누적값 ZeroMemory 초기화 (키/버튼 상태는 유지)
- 키 식별: VK_* Virtual Key Code 사용 (`<Windows.h>` 포함, DIK_* 스캔코드 불필요)
- enum: `MOUSEBTN { LBUTTON, RBUTTON, MBUTTON, END }`, `MOUSEAXIS { X, Y, WHEEL, END }`
- **WndProc 연결**: Client WndProc → `CGameInstance::Process_RawInput(lParam)` → `CInput_Device::Process_Input(lParam)` (CGameInstance facade 패턴 유지)
- **미완료**: CGameInstance에 Input_Device 멤버/위임 메서드 통합, Camera_Free에서 입력 사용

### PipeLine System (CPipeLine)
- `CPipeLine` (final): CBase 상속. ENGINE_DLL 없음 (CGameInstance 통해서만 접근)
- 카메라가 Set_Transform()으로 View/Proj 행렬 저장 → Update()에서 역행렬 자동 계산 + 카메라 월드 위치 추출
- `m_TransformStateMatrices[D3DTS::END]`: View/Proj 원본 행렬 (`_float4x4`)
- `m_TransformStateInverseMatrices[D3DTS::END]`: 역행렬 (XMMatrixInverse)
- `m_vCamPosition`: View 역행렬 4번째 행에서 추출한 카메라 월드 좌표 (`_float4`)
- `Set_Transform(D3DTS, _fmatrix)`: SIMD → `_float4x4` 변환 저장 (XMStoreFloat4x4)
- `Get_Transform(D3DTS)` / `Get_Transform_Inverse(D3DTS)`: `const _float4x4*` 반환
- 생성자에서 모든 행렬을 항등행렬로 초기화
- **미완료**: CGameInstance에 PipeLine 멤버/위임 메서드 통합, CCamera에서 PipeLine 참조 연결

### Camera System (CCamera)
- `CCamera` (abstract): CGameObject 상속. 카메라 전용 중간 클래스
- `CAMERA_DESC`: GAMEOBJECT_DESC 상속. vEye(카메라 위치), vAt(바라볼 지점), fFovy(수직 시야각), fNear/fFar(클리핑 평면). eTransformType은 GAMEOBJECT_DESC 기본값(TRANSFORM_3D) 그대로 사용
- `Initialize(pArg)`: DESC에서 fFovy/fNear/fFar 추출 → `__super::Initialize(pArg)`로 CTransform_3D 생성 → `Set_State(POSITION, vEye)` + `LookAt(vAt)` (CTransform_3D로 다운캐스팅 필요)
- `Clone() = PURE`: abstract 클래스이므로 Client에서 구체 카메라(Camera_Free 등)가 구현
- `m_pPipeLine`: CPipeLine 참조 보유 (Update_PipeLine()에서 View 행렬을 PipeLine에 세팅하는 용도)
- 멤버 변수 `protected` 접근 제어: 파생 클래스에서 fFovy/fNear/fFar/pPipeLine 접근 필요
- **미완료**: Camera_Free(Client측 FPS 카메라), CGameInstance 통합 후 PipeLine 연결

### Vertex Format & InputLayout Embedding
- 정점 구조체에 `static constexpr D3D11_INPUT_ELEMENT_DESC Elements[]`와 `static const unsigned int iNumElements`를 내장
- `VTXTEX`: Position(XMFLOAT3) + Texcoord(XMFLOAT2), 2 Elements, 20 bytes
- `VTXNORTEX`: Position(XMFLOAT3) + Normal(XMFLOAT3) + Texcoord(XMFLOAT2), 3 Elements, 32 bytes
- 셰이더 생성 시 `CShader::Create(..., VTXNORTEX::Elements, VTXNORTEX::iNumElements)` 형태로 사용 — 정점 포맷과 InputLayout의 결합도를 높여 불일치 방지
- **중요:** Terrain은 VTXNORTEX 정점 포맷 → 반드시 `Shader_VtxNorTex` 셰이더와 매칭. VtxTex 셰이더 사용 시 InputLayout 불일치로 크래시

### VIBuffer System (Vertex/Index Buffer)
- `CVIBuffer` (abstract): CComponent 상속. ID3D11Buffer* m_pVB/m_pIB 소유. Bind_Resources()로 IA 스테이지 세팅, Render()로 DrawIndexed() 호출
- 복사 생성자에서 VB/IB를 공유(포인터 복사 + Safe_AddRef). 같은 메시를 쓰는 클론끼리 GPU 버퍼를 중복 생성하지 않음
- `CVIBuffer_Rect` (final): VTXTEX 정점 4개(좌상/우상/우하/좌하), _ushort 인덱스 6개(삼각형 2개). D3D11_USAGE_DEFAULT, TRIANGLELIST 토폴로지
- `CVIBuffer_Terrain` (final): BMP 하이트맵으로부터 지형 메시 생성. VTXNORTEX 정점(높이: blue 채널 / 10.f, UV: j/(X-1), i/(Z-1)), _uint 32비트 인덱스, (X-1)*(Z-1)*2 삼각형. Create()에 pHeightMapFilePath 전달
- 버퍼 생성 흐름: D3D11_BUFFER_DESC + D3D11_SUBRESOURCE_DATA → m_pDevice->CreateBuffer() → Safe_Delete_Array(CPU 임시 데이터)
- Free() 순서: __super::Free() (CComponent → Device/Context 해제) → Safe_Release(m_pVB) → Safe_Release(m_pIB)

### Shader System (CShader & HLSL)
- `CShader` (final): CComponent 상속. ID3DX11Effect* m_pEffect + vector<ID3D11InputLayout*> m_vInputLayouts 소유
- Initialize_Prototype(pShaderFilePath, pElements, iNumElements): HLSL 파일 경로 + D3D11_INPUT_ELEMENT_DESC 배열을 받음
  - (1) 디버그/릴리스 플래그 설정 → (2) D3DX11CompileEffectFromFile()로 Effect 생성 → (3) 첫 번째 Technique 획득 → (4) Pass 수 확인(m_iNumPasses) → (5) 각 Pass별 VS 시그니처로 CreateInputLayout() 검증/생성
- Begin(iPassIndex): IASetInputLayout(해당 Pass의 InputLayout) + GetTechniqueByIndex(0)->GetPassByIndex(iPassIndex)->Apply(0, m_pContext) — VS/PS 등 모든 셰이더 스테이지를 파이프라인에 바인딩
- Bind_Matrix(pConstantName, pMatrix): Effect의 GetVariableByName → AsMatrix → SetMatrix. HLSL 전역 행렬 변수에 CPU 행렬 데이터 전달
- Bind_SRV(pConstantName, pSRV): Effect의 **GetVariableByName**(GetConstantBufferByName이 아님!) → AsShaderResource → SetResource. HLSL의 texture2D 변수에 SRV 바인딩
- 복사 생성자에서 m_pEffect + m_vInputLayouts 모두 공유(포인터 복사 + Safe_AddRef). CVIBuffer와 동일한 리소스 공유 패턴
- Free() 순서: __super::Free() → InputLayouts 순회 Safe_Release + clear → Safe_Release(m_pEffect)
- HLSL 파일 위치: `Client/Bin/ShaderFiles/Shader_VtxTex.hlsl` (VTXTEX용), `Shader_VtxNorTex.hlsl` (VTXNORTEX용)
- HLSL 공통 구조: 전역 행렬(g_WorldMatrix, g_ViewMatrix, g_ProjMatrix) + texture2D g_Texture + sampler DefaultSampler, VS_IN/VS_OUT (정점 구조체와 1:1 대응), VS_MAIN (WVP 변환), PS_MAIN (g_Texture.Sample() + alpha test discard), technique11 DefaultTechnique / pass DefaultPass
- Shader_VtxNorTex: VS_IN에 `float3 vNormal : NORMAL` 추가 (현재 셰이더 로직에서 미사용, 향후 라이팅용)
- vcxproj에서 hlsl 파일의 ShaderType은 반드시 `Effect (/fx)`로 설정 (Vertex로 설정 시 FXC가 `main` 진입점을 찾아 빌드 실패)

### Texture System (CTexture)
- `CTexture` (final): CComponent 상속. vector<ID3D11ShaderResourceView*> m_Textures (SRV 배열) + _uint m_iNumTextures 소유
- Initialize_Prototype(pTextureFilePath, iNumTextures): 포맷 문자열(%d)로 여러 텍스처를 순차 로드
  - _wsplitpath_s로 확장자 추출 → `.dds`는 CreateDDSTextureFromFile, `.tga`는 미지원(E_FAIL), 나머지(jpg/png/bmp)는 CreateWICTextureFromFile
- Bind_ShaderResource(pShader, pConstantName, iTextureIndex): 특정 인덱스의 SRV를 CShader::Bind_SRV로 전달
- 복사 생성자에서 SRV 포인터 배열을 얕은 복사 + 각 SRV에 Safe_AddRef (CVIBuffer/CShader와 동일한 리소스 공유 패턴)
- Free() 순서: __super::Free() → m_Textures 순회 Safe_Release + clear
- WIC 기반 로딩은 COM 초기화 필수 → CLoader에서 CoInitializeEx/CoUninitialize 호출

### Prototype Registration & Object Creation
- Register prototypes in `CLoader::Ready_Resources_For_XXX()` via `Add_Prototype(levelIndex, tag, Object::Create(...))`
- Create instances in Level's `Ready_Layer_XXX()` via `Add_GameObject(protoLevel, protoTag, layerLevel, layerTag)`
- `Clone_Prototype()` uses `PROTOTYPE` enum to dispatch: `GAMEOBJECT` calls `CGameObject::Clone()`, `COMPONENT` calls `CComponent::Clone()`
- CLoader의 별도 스레드에서 WIC 텍스처 로딩 시 **CoInitializeEx(nullptr, 0) / CoUninitialize()** 필수 (COM 기반)

### GameObject Render Order
- 렌더링 시 바인딩 순서가 중요: `Bind_ShaderResources()` → `Shader::Begin()` → `VIBuffer::Bind_Resources()` → `VIBuffer::Render()`
- Bind_ShaderResources에서 WVP 행렬 + 텍스처를 셰이더에 전달한 후, Begin으로 Pass 적용, 그다음 IA 바인딩 + DrawIndexed

### Transform WorldMatrix & SIMD 타입
- WorldMatrix(`_float4x4`)의 각 행은 `STATE` enum으로 인덱싱: RIGHT(0), UP(1), LOOK(2), POSITION(3)
- `Get_State(STATE)`: `XMLoadFloat4`로 행을 `_vector`(XMVECTOR)로 로드. `Set_State(STATE, _fvector)`: `XMStoreFloat4`로 저장
- `Get_Scale()`: 각 축 벡터(RIGHT/UP/LOOK)의 길이를 `_float3`로 반환
- `Set_Scale` vs `Scaling`: Set_Scale은 정규화 후 절대 스케일 적용, Scaling은 현재 스케일에 상대 배율 적용
- `Rotation` vs `Turn`: Rotation은 항등 행렬 기준 절대 회전(라디안), Turn은 현재 방향에서 `m_fRotationPerSec * fTimeDelta` 만큼 누적 회전
- `LookAt(_fvector vAt)`: 타겟 위치에서 월드 업(0,1,0) 기준 외적으로 Right/Up/Look 재계산, 스케일 보존
- SIMD 호출 규약 타입: `_fvector`(FXMVECTOR, 첫 3개 XMVECTOR 파라미터), `_gvector`(4번째), `_hvector`(5~6번째), `_cvector`(나머지). `_fmatrix`(첫 XMMATRIX), `_cmatrix`(나머지)
- `TRANSFORMTYPE` enum: `TRANSFORM_2D`, `TRANSFORM_3D`, `END`. CGameObject::Initialize()에서 분기용
- CTransform_3D의 이동 메서드는 Look/Right 벡터 기반 (3D 공간), CTransform_2D의 이동 메서드는 고정 축 방향 (X축/Y축) 기반 (2D 스크린)

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
