# CPP_STUDY_3D

DirectX 11 기반 3D 게임 프레임워크를 직접 설계하고 구현하는 학습 프로젝트입니다.

> 수업 코드를 기반으로 구현하되, 구조적 개선 사항을 자체적으로 설계/적용하며 학습합니다.

---

## Tech Stack

| Category | Details |
|----------|---------|
| **Language** | C++ (MSVC v143 Toolset) |
| **Graphics** | DirectX 11 (D3D11) |
| **Shader** | HLSL + FX11 (Effects 11) |
| **Texture** | DirectXTK (DDSTextureLoader, WICTextureLoader) |
| **Input** | DirectInput 8 |
| **Editor UI** | ImGui (Docking branch) + ImGuizmo |
| **Build** | Visual Studio 2022 / MSBuild |
| **Platform** | Windows 10/11 (Win32, x64) |

---

## Repository Overview

```
CPP_STUDY_3D/
  |
  |-- Framework/            <-- Main (Engine.dll + Client.exe)
  |-- Framework_4Proj/      <-- Experimental (Engine.dll + Client.dll + GameApp.exe + Editor.exe)
  +-- 명세서/                <-- 수업 분석 및 학습 정리 문서
```

---

## Framework (Main Project)

**1 Solution - 2 Projects** 구조의 메인 프레임워크입니다.

### Directory Structure

```
Framework/
  |
  |-- Engine/                           [DLL] 재사용 가능한 3D 엔진
  |   |
  |   |-- Public/                       엔진 헤더 파일
  |   |   |
  |   |   |  [ Core ]
  |   |   |-- Engine_Defines.h          D3D11, FX11, DirectXTK, STL 통합 헤더
  |   |   |-- Engine_Enum.h             WINMODE, RENDERID, STATE, TRANSFORMTYPE, D3DTS
  |   |   |-- Engine_Struct.h           ENGINE_DESC, VTXTEX, VTXNORTEX (+ static InputLayout)
  |   |   |-- Engine_Macro.h            SINGLETON, NULL_CHECK, FAILED_CHECK, DLL export
  |   |   |-- Engine_Typedef.h          _float, _vector, _fvector, _matrix, _fmatrix
  |   |   |-- Engine_Function.h         Safe_Delete, Safe_Release, Safe_AddRef
  |   |   |
  |   |   |  [ Base & Singleton ]
  |   |   |-- Base.h                    COM 스타일 Reference Counting (AddRef/Release)
  |   |   |-- GameInstance.h            엔진 중앙 코디네이터 (모든 매니저 위임)
  |   |   |-- Graphic_Device.h          D3D11 장치, 스왑체인, RTV, DSV 관리
  |   |   |
  |   |   |  [ Component System ]
  |   |   |-- Component.h              컴포넌트 추상 베이스 (Prototype/Clone)
  |   |   |-- Transform.h              트랜스폼 추상 베이스 (월드 행렬, 스케일)
  |   |   |-- Transform_3D.h           +-- 3D: Go_Straight/Backward/Left/Right, Rotation, LookAt
  |   |   |-- Transform_2D.h           +-- 2D: Move_X, Move_Y (스크린 좌표 기반)
  |   |   |-- Shader.h                 FX11 Effect 래퍼 (HLSL 컴파일, Bind_Matrix/SRV)
  |   |   |-- Texture.h                DDS/WIC 텍스처 로딩, 다중 SRV 관리
  |   |   |-- VIBuffer.h               정점/인덱스 버퍼 추상 베이스 (DrawIndexed)
  |   |   |-- VIBuffer_Rect.h          +-- 사각형 메시 (4 정점, 6 인덱스)
  |   |   |-- VIBuffer_Terrain.h       +-- HeightMap 기반 지형 메시 (32bit 인덱스)
  |   |   |
  |   |   |  [ Object & Level System ]
  |   |   |-- GameObject.h             게임 엔티티 추상 베이스 (GAMEOBJECT_DESC)
  |   |   |-- UIObject.h               UI 엔티티 (직교 투영, UIOBJECT_DESC)
  |   |   |-- Level.h                  레벨 추상 베이스
  |   |   |-- Layer.h                  동일 깊이 오브젝트 그룹
  |   |   |-- Renderer.h              렌더 그룹 관리 (Priority > NonBlend > Blend > UI)
  |   |   |
  |   |   |  [ Manager Singletons ]
  |   |   |-- Level_Manager.h          레벨 전환 및 현재 레벨 관리
  |   |   |-- Timer_Manager.h          이름 기반 타이머 (QueryPerformanceCounter)
  |   |   |-- Object_Manager.h         레벨/레이어별 오브젝트 수명 관리
  |   |   |-- Prototype_Manager.h      원형 객체 저장소, 타입별 Clone 분배
  |   |   |
  |   |   |-- fx11/                    DirectX Effects 11 헤더
  |   |   +-- DirectXTK/               DirectX Tool Kit 헤더 (DDS/WIC Loader)
  |   |
  |   |-- Private/                     엔진 구현 파일 (.cpp)
  |   |-- ThirdPartyLib/               FX11, DirectXTK 정적 라이브러리
  |   +-- Bin/                         빌드 출력 (Engine.dll, Engine.lib)
  |
  |-- Client/                          [EXE] 게임 클라이언트
  |   |
  |   |-- Public/
  |   |   |-- Client_Defines.h         윈도우 크기 (1280x720), LEVEL 열거체
  |   |   |-- MainApp.h                엔진 초기화 / 업데이트 / 렌더 루프
  |   |   |-- Level_Logo.h             로고 레벨 (Enter -> Loading 전환)
  |   |   |-- Level_Loading.h          로딩 레벨 (스레드 기반 리소스 로딩)
  |   |   |-- Level_GamePlay.h         게임플레이 레벨 (Terrain 스폰)
  |   |   |-- Loader.h                 멀티스레드 리소스 로더 (COM 초기화 포함)
  |   |   |-- BackGround.h             배경 UI 오브젝트 (CUIObject 상속)
  |   |   +-- Terrain.h                지형 오브젝트 (HeightMap + VtxNorTex 셰이더)
  |   |
  |   |-- Private/                     구현 파일 (.cpp)
  |   +-- Bin/
  |       |-- ShaderFiles/
  |       |   |-- Shader_VtxTex.hlsl       Position + Texcoord 셰이더
  |       |   +-- Shader_VtxNorTex.hlsl    Position + Normal + Texcoord 셰이더
  |       +-- Resources/Textures/          텍스처 리소스
  |
  |-- EngineSDK/                       엔진 SDK 배포 (헤더 + lib 복사본)
  |-- UpdateLib.bat                    엔진 빌드 후 SDK 자동 복사 스크립트
  +-- Framework.sln                    Visual Studio 2022 솔루션
```

### Class Hierarchy

```
CBase (Reference Counting: AddRef / Release)
  |
  |-- CGameInstance (singleton)        엔진 중앙 코디네이터
  |-- CGraphic_Device                  D3D11 장치, 스왑체인, RTV, DSV
  |
  |-- CComponent (abstract)            컴포넌트 베이스 (Prototype/Clone)
  |   |-- CTransform (abstract)        월드 행렬, 스케일, Bind_ShaderResource
  |   |   |-- CTransform_3D (final)    3D 이동/회전 (Go_Straight, Rotation, LookAt)
  |   |   +-- CTransform_2D (final)    2D 이동 (Move_X, Move_Y)
  |   |-- CShader (final)              FX11 Effect 래퍼 (HLSL, Bind_Matrix/SRV)
  |   |-- CTexture (final)             DDS/WIC 다중 텍스처 SRV 관리
  |   +-- CVIBuffer (abstract)         정점/인덱스 버퍼 (IA binding, DrawIndexed)
  |       |-- CVIBuffer_Rect (final)   사각형 메시 (4정점, 6인덱스)
  |       +-- CVIBuffer_Terrain (final) HeightMap 지형 메시
  |
  |-- CGameObject (abstract)           게임 엔티티 (GAMEOBJECT_DESC, Clone)
  |   +-- CUIObject (abstract)         UI 엔티티 (직교 투영, UIOBJECT_DESC)
  |
  |-- CLevel (abstract)                레벨 베이스
  |-- CLayer                           동일 깊이 오브젝트 그룹
  |-- CRenderer (singleton-like)       렌더 그룹 (Priority > NonBlend > Blend > UI)
  |
  +-- Manager Singletons
      |-- CLevel_Manager               레벨 전환
      |-- CTimer_Manager               프레임 타이머
      |-- CObject_Manager              오브젝트 수명 관리
      +-- CPrototype_Manager           원형 등록/Clone
```

### Rendering Pipeline

```
1. Prototype Registration
   MainApp::Ready_Prototype_For_Static()
     +-- VIBuffer_Rect + Shader_VtxTex (VTXTEX::Elements 활용)

2. Threaded Loading
   Loader::Ready_Resources_For_GamePlay()      <-- 별도 스레드
     |-- Texture_Terrain (Tile0.jpg)
     |-- Shader_VtxNorTex (VTXNORTEX::Elements)
     |-- VIBuffer_Terrain (Height.bmp)
     +-- GameObject_Terrain

3. Object Spawn
   Level_GamePlay::Ready_Layer_Terrain()
     +-- Add_GameObject -> Clone

4. Per-Frame Render
   GameObject::Late_Update()
     +-- Add_RenderGroup(RENDERID, this)       <-- 렌더 그룹 등록

   Renderer::Draw()                            <-- 순서대로 Draw
     |-- [Priority]  우선 렌더 오브젝트
     |-- [NonBlend]  불투명 오브젝트 (Terrain 등)
     |-- [Blend]     반투명 오브젝트
     +-- [UI]        UI 오브젝트 (BackGround 등)

   Each Object::Render()
     |-- Bind_ShaderResources()   World/View/Proj + Texture 바인딩
     |-- Shader::Begin(0)         InputLayout + Pass Apply
     |-- VIBuffer::Bind_Resources()  IA Stage 세팅
     +-- VIBuffer::Render()       DrawIndexed
```

### Key Design Patterns

| Pattern | Description |
|---------|-------------|
| **Prototype/Clone** | 원형 객체 등록 -> Clone()으로 사본 생성. GPU 리소스(VB/IB/Effect/SRV) 공유 |
| **COM-style RefCount** | CBase의 AddRef()/Release(). 항상 Safe_Release() 사용 |
| **Component** | GameObject가 Transform, Shader, Texture, VIBuffer 등을 소유 |
| **Descriptor Chain** | TRANSFORM_DESC -> GAMEOBJECT_DESC -> UIOBJECT_DESC 상속 체인 |
| **Render Group** | Late_Update에서 그룹 등록 -> Renderer가 순서대로 Draw |
| **Threaded Loading** | CLoader에서 별도 스레드로 리소스 로딩 |
| **3-Stage Update** | Priority_Update -> Update -> Late_Update 파이프라인 |

### Self-Improvements (수업 코드 대비 자체 개선)

| 개선 사항 | 수업 코드 | 자체 개선 |
|-----------|-----------|-----------|
| Transform 구조 | 단일 CTransform(final) | CTransform(abstract) + CTransform_3D + CTransform_2D 분리 |
| Transform 자동 생성 | 항상 3D Transform | GAMEOBJECT_DESC::eTransformType에 따라 자동 분기 |
| UI 기본 Transform | 수동 지정 필요 | UIOBJECT_DESC 생성자에서 TRANSFORM_2D 자동 설정 |

---

## Framework_4Proj (Experimental)

**1 Solution - 4 Projects** 구조의 확장 프로젝트입니다.
Client를 DLL로 전환하여 GameApp과 Editor가 동일한 게임 오브젝트를 공유할 수 있도록 설계했습니다.

### Directory Structure

```
Framework_4Proj/Framework/
  |
  |-- Engine/              [DLL] 엔진 코어
  |   |-- Public/          헤더 파일 (ENGINE_DLL export)
  |   |-- Private/         구현 파일
  |   |-- ThirdPartyLib/   FX11, DirectXTK 정적 라이브러리
  |   +-- Bin/             Engine.dll, Engine.lib
  |
  |-- Client/              [DLL] 게임 로직 (<-- 기존 EXE에서 전환)
  |   |-- Public/          헤더 파일 (CLIENT_DLL export: MainApp, BackGround)
  |   |-- Private/         구현 파일 (Level, Loader 등은 내부용)
  |   +-- Bin/             Client.dll, Client.lib
  |
  |-- GameApp/             [EXE] 게임 실행 파일 (얇은 진입점)
  |   +-- Default/         wWinMain, 윈도우 생성, 60FPS 게임루프
  |
  |-- Editor/              [EXE] 에디터 도구 (ImGui 기반)
  |   |-- Public/          EditorApp.h, Editor_Defines.h
  |   |-- Private/         EditorApp.cpp (ImGui DockSpace 렌더 루프)
  |   |-- ImGui/           ImGui docking 브랜치 소스 + DX11/Win32 백엔드
  |   +-- ImGuizmo/        ImGuizmo (Gizmo 조작 라이브러리)
  |
  |-- Resources/           공유 리소스 (ShaderFiles, Textures)
  |
  |-- EngineSDK/           엔진 SDK (헤더 + lib)
  |-- ClientSDK/           클라이언트 SDK (헤더 + lib)
  |
  |-- EngineSDK_Update.bat    Engine 빌드 후 SDK + DLL 자동 복사
  |-- ClientSDK_Update.bat    Client 빌드 후 SDK + DLL 자동 복사
  +-- Framework.sln           Visual Studio 2022 솔루션 (4 projects)
```

### Architecture

```
  +---------------+       +---------------+
  | GameApp.exe   |       | Editor.exe    |
  | (진입점)       |       | (ImGui UI)    |
  +-------+-------+       +-------+-------+
          |                        |
          +----------+-------------+
                     | links
              +------+------+
              | Client.dll  |
              | (게임 로직)  |
              | MainApp     |
              | BackGround  |
              | Levels      |
              | Loader      |
              +------+------+
                     | links
              +------+------+
              | Engine.dll  |
              | (엔진 코어) |
              | GameInstance |
              | Components  |
              | Renderer    |
              | Managers    |
              +-------------+
```

### Framework vs Framework_4Proj

| 항목 | Framework (2-Proj) | Framework_4Proj (4-Proj) |
|------|-------------------|-------------------------|
| 솔루션 구조 | Engine.dll + Client.exe | Engine.dll + Client.dll + GameApp.exe + Editor.exe |
| Client 역할 | 직접 실행 (EXE) | 게임 로직 라이브러리 (DLL) |
| 에디터 | 없음 | ImGui/ImGuizmo 기반 Editor.exe |
| DLL export | ENGINE_DLL만 | ENGINE_DLL + CLIENT_DLL |
| 전역변수 | g_hWnd, g_iWinSizeX/Y (Client) | CGameInstance에 저장, Get 접근자 제공 |
| 리소스 경로 | Client/Bin/ 하위 | 공유 Resources/ 폴더 |
| 윈도우 리사이즈 | 미지원 | OnResize (SwapChain/RTV/DSV 재생성) |
| 상태 | **메인 개발 중** | 실험적 |

### Editor Features

- **ImGui Docking** 기반 DockSpace UI
- **DXGI 모니터 해상도 감지** -> 최대화 창 (WS_OVERLAPPEDWINDOW + SW_MAXIMIZE)
- **CEditorApp**: CBase 상속, Engine 초기화 + ImGui DX11 백엔드 통합
- Client.dll의 GameObject를 직접 Clone하여 편집 가능한 구조
- 향후: 맵툴, 오브젝트 배치, 이펙트툴, UI 배치, 데이터 파싱 등

---

## 명세서

수업 내용을 분석하고 현재 프로젝트와 비교 정리한 학습 문서입니다.

| File | Content |
|------|---------|
| 8개월_5일차_학습정리.md | UIObject, 직교 투영, CTransform 2D/3D 분리 설계 |
| 8개월_6일차_학습정리.md | HeightMap Terrain, CCamera 뼈대, 인덱스 최적화 이론 (16bit vs 32bit) |

---

## Build

```
Prerequisites:
  - Visual Studio 2022 (v143 toolset)
  - Windows SDK (DirectX 11 포함)

Framework (2-Project):
  1. Framework/Framework.sln 열기
  2. Engine 프로젝트 빌드
  3. UpdateLib.bat 실행 (헤더/lib -> EngineSDK 복사)
  4. Client 프로젝트 빌드
  5. Client/Bin/Client.exe 실행

Framework_4Proj (4-Project):
  1. Framework_4Proj/Framework/Framework.sln 열기
  2. Engine 빌드 -> EngineSDK_Update.bat 자동 실행
  3. Client 빌드 -> ClientSDK_Update.bat 자동 실행
  4. GameApp 또는 Editor 빌드
  5. GameApp/Bin/GameApp.exe 또는 Editor/Bin/Editor.exe 실행

Command Line:
  msbuild Framework.sln /p:Configuration=Debug /p:Platform=x64
```

---

## Learning Progress

### 7개월차 - Framework 기초

| Day | Topic |
|-----|-------|
| 1-2 | 솔루션/프로젝트 구조, Engine DLL, CBase (Reference Counting) |
| 3-5 | CGraphic_Device (D3D11 초기화), Timer, D3D9 vs D3D11 비교 |
| 6 | Level 시스템, CLoader (로딩 스레드), Level 전환 |
| 7-8 | CPrototype_Manager, CGameObject, Prototype/Clone 패턴 |
| 9 | CRenderer (렌더 그룹), Draw Call 관리 |
| 10-11 | CComponent, CTransform, SIMD (XMVECTOR/XMMATRIX) |

### 8개월차 - 렌더링 파이프라인

| Day | Topic |
|-----|-------|
| 1 | CVIBuffer, CVIBuffer_Rect (정점/인덱스 버퍼) |
| 2-3 | CShader (FX11), HLSL VS/PS 작성, BackGround 렌더링 |
| 4 | CTexture (DDS/WIC 텍스처 로딩) |
| 5 | CUIObject (직교 투영), Background Texture 적용 |
| 6 | HeightMap Terrain, CCamera 뼈대 *(진행 중)* |
