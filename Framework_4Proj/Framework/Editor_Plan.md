  Editor 세팅 제안

  1. 폴더 구조

  Framework/
  ├── Editor/
  │   ├── Default/                    ← 기존 (수정)
  │   │   ├── Editor.cpp              ← wWinMain 재작성 (GameApp 패턴 + ImGui WndProc)
  │   │   ├── Editor_Defines.h        ← 신규 (윈도우 크기, extern HWND)
  │   │   ├── Editor.vcxproj          ← 수정 (include/lib/ImGui 소스 등록)
  │   │   ├── Editor.vcxproj.filters  ← 수정
  │   │   ├── Editor.h, framework.h, targetver.h, Resource.h  ← 기존 유지
  │   │   └── Editor.rc, Editor.ico, small.ico
  │   ├── Public/                     ← 신규
  │   │   └── EditorApp.h
  │   ├── Private/                    ← 신규
  │   │   └── EditorApp.cpp
  │   ├── ImGui/                      ← 신규 (docking 브랜치에서 복사)
  │   │   ├── imgui.h / imgui.cpp
  │   │   ├── imgui_draw.cpp
  │   │   ├── imgui_tables.cpp
  │   │   ├── imgui_widgets.cpp
  │   │   ├── imgui_demo.cpp
  │   │   ├── imgui_internal.h
  │   │   ├── imconfig.h
  │   │   ├── imstb_rectpack.h / imstb_textedit.h / imstb_truetype.h
  │   │   ├── imgui_impl_dx11.h / imgui_impl_dx11.cpp    ← backends/ 폴더에서
  │   │   └── imgui_impl_win32.h / imgui_impl_win32.cpp   ← backends/ 폴더에서
  │   ├── ImGuizmo/                   ← 신규
  │   │   ├── ImGuizmo.h
  │   │   └── ImGuizmo.cpp
  │   └── Bin/                        ← 빌드 출력

  SR 프로젝트와의 차이점:
  - SR은 Editor/ImGui/에 DX9 백엔드 → 여기선 imgui_impl_dx11 사용
  - SR은 Editor/Code/, Editor/Header/ → Framework 컨벤션에 맞춰 Public/, Private/

  ---
  2. Editor_Defines.h (신규)

  GameApp_Defines.h와 동일한 패턴:

  #pragma once

  #include <Windows.h>

  static const unsigned int     g_iWinSizeX = { 1600 };  // 에디터는 패널이 많으므로 넓게
  static const unsigned int     g_iWinSizeY = { 900 };

  extern HWND   g_hWnd;

  ---
  3. CEditorApp.h (신규)

  CMainApp 패턴을 따르되, ImGui 초기화/정리 메서드가 추가됩니다. Client.dll의 Level 시스템 대신 에디터 자체 루프를 가집니다.

  #pragma once

  #include "Engine_Defines.h"
  #include "Base.h"

  NS_BEGIN(Engine)
  class CGameInstance;
  NS_END

  class CEditorApp final : public CBase
  {
  private:
        CEditorApp();
        virtual ~CEditorApp() = default;

  public:
        HRESULT                                 Initialize(HWND hWnd, _uint iWinSizeX, _uint iWinSizeY);
        void                                    Update(_float fTimeDelta);
        HRESULT                                 Render();

  private:
        ID3D11Device*                   m_pDevice = { nullptr };
        ID3D11DeviceContext*            m_pContext = { nullptr };
        CGameInstance*                  m_pGameInstance = { nullptr };

  private:
        HRESULT                                 Ready_ImGui(HWND hWnd);
        void                                    Shutdown_ImGui();

        /* ImGui Render */
        void                                    Begin_ImGuiFrame();
        void                                    Render_DockSpace();
        void                                    Render_ImGui();

  public:
        static CEditorApp*              Create(HWND hWnd, _uint iWinSizeX, _uint iWinSizeY);
        virtual void                    Free() override;
  };

  포인트:
  - CBase 상속 (레퍼런스 카운팅)
  - USING(Engine) 없이 CGameInstance* 사용 (Engine 네임스페이스 전방선언)
  - Client.dll의 CMainApp처럼 CLIENT_DLL export는 불필요 (Editor.exe 내부 클래스)
  - Level 시스템(Start_Level) 대신 ImGui 관련 메서드

  ---
  4. CEditorApp.cpp (신규)

  #include "EditorApp.h"
  #include "GameInstance.h"

  #include "imgui.h"
  #include "imgui_impl_win32.h"
  #include "imgui_impl_dx11.h"

  CEditorApp::CEditorApp()
        : m_pGameInstance{ CGameInstance::GetInstance() }
  {
        Safe_AddRef(m_pGameInstance);
  }

  HRESULT CEditorApp::Initialize(HWND hWnd, _uint iWinSizeX, _uint iWinSizeY)
  {
        /* 1. 엔진 초기화 (CMainApp과 동일) */
        ENGINE_DESC             EngineDesc{};
        EngineDesc.hWnd = hWnd;
        EngineDesc.eWinMode = WINMODE::WIN;
        EngineDesc.iViewportWidth = iWinSizeX;
        EngineDesc.iViewportHeight = iWinSizeY;
        EngineDesc.iNumLevels = 1;  // 에디터는 단일 레벨로 운영

        if (FAILED(m_pGameInstance->Initialize_Engine(EngineDesc, &m_pDevice, &m_pContext)))
        {
                MSG_BOX("Failed to Initialize : Engine");
                return E_FAIL;
        }

        /* 2. ImGui 초기화 (엔진 초기화 이후) */
        if (FAILED(Ready_ImGui(hWnd)))
        {
                MSG_BOX("Failed to Initialize : ImGui");
                return E_FAIL;
        }

        return S_OK;
  }

  void CEditorApp::Update(_float fTimeDelta)
  {
        m_pGameInstance->Update_Engine(fTimeDelta);
  }

  HRESULT CEditorApp::Render()
  {
        /* 1. ImGui 프레임 시작 */
        Begin_ImGuiFrame();

        /* 2. DockSpace 설정 */
        Render_DockSpace();

        /* 3. 에디터 UI 패널 (이후 확장) */
        // TODO: 메뉴바, 하이어라키, 인스펙터 등
  #ifdef _DEBUG
        ImGui::ShowDemoWindow();  // 개발 중 참고용
  #endif

        /* 4. 씬 렌더링 */
        FAILED_CHECK(m_pGameInstance->Begin_Draw());
        FAILED_CHECK(m_pGameInstance->Draw());

        /* 5. ImGui 드로우 (씬 위에 오버레이) */
        Render_ImGui();

        /* 6. Present */
        FAILED_CHECK(m_pGameInstance->End_Draw());

        return S_OK;
  }

  #pragma region IMGUI

  HRESULT CEditorApp::Ready_ImGui(HWND hWnd)
  {
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();

        ImGuiIO& io = ImGui::GetIO();
        io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
        // io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;  // 멀티 뷰포트 (필요 시)

        ImGui::StyleColorsDark();

        if (!ImGui_ImplWin32_Init(hWnd))
                return E_FAIL;

        if (!ImGui_ImplDX11_Init(m_pDevice, m_pContext))
                return E_FAIL;

        return S_OK;
  }

  void CEditorApp::Shutdown_ImGui()
  {
        ImGui_ImplDX11_Shutdown();
        ImGui_ImplWin32_Shutdown();
        ImGui::DestroyContext();
  }

  void CEditorApp::Begin_ImGuiFrame()
  {
        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();
  }

  void CEditorApp::Render_DockSpace()
  {
        ImGuiViewport* viewport = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos(viewport->WorkPos);
        ImGui::SetNextWindowSize(viewport->WorkSize);
        ImGui::SetNextWindowViewport(viewport->ID);

        ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoDocking;
        window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse;
        window_flags |= ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
        window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;
        window_flags |= ImGuiWindowFlags_NoBackground;

        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));

        ImGui::Begin("DockSpaceWindow", nullptr, window_flags);
        ImGui::PopStyleVar(3);

        ImGuiID dockspace_id = ImGui::GetID("MainDockSpace");
        ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_PassthruCentralNode);

        ImGui::End();
  }

  void CEditorApp::Render_ImGui()
  {
        ImGui::Render();
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
  }

  #pragma endregion

  CEditorApp* CEditorApp::Create(HWND hWnd, _uint iWinSizeX, _uint iWinSizeY)
  {
        CEditorApp* pInstance = new CEditorApp();

        if (FAILED(pInstance->Initialize(hWnd, iWinSizeX, iWinSizeY)))
        {
                MSG_BOX("Failed to Created : CEditorApp");
                Safe_Release(pInstance);
        }

        return pInstance;
  }

  void CEditorApp::Free()
  {
        __super::Free();

        Shutdown_ImGui();

        Safe_Release(m_pContext);
        Safe_Release(m_pDevice);

        m_pGameInstance->Release_Engine();
        Safe_Release(m_pGameInstance);
  }

  CMainApp과의 핵심 차이:
  - EngineDesc.iNumLevels = 1 (에디터는 레벨 시스템을 직접 안 쓰므로 최소값)
  - Start_Level() 없음 → ImGui DockSpace가 메인 UI
  - Free()에서 Shutdown_ImGui() 호출 (Device/Context Release 전에)
  - Render 순서: ImGui Frame → Begin_Draw → Draw → ImGui Render → End_Draw

  ---
  5. Editor.cpp 재작성

  현재 VS 템플릿 stub → GameApp.cpp 패턴으로 전환 + ImGui WndProc:

  // Editor.cpp : 애플리케이션에 대한 진입점을 정의합니다.

  #include "framework.h"
  #include "Editor.h"
  #include "Editor_Defines.h"

  #include "EditorApp.h"
  #include "GameInstance.h"

  // ImGui WndProc 핸들러 전방선언
  extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(
        HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

  #define MAX_LOADSTRING 100

  // 전역 변수:
  HINSTANCE           hInst;
  HWND                g_hWnd;
  WCHAR               szTitle[MAX_LOADSTRING];
  WCHAR               szWindowClass[MAX_LOADSTRING];

  // 전방선언
  ATOM                MyRegisterClass(HINSTANCE hInstance);
  BOOL                InitInstance(HINSTANCE, int);
  LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);

  int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                       _In_opt_ HINSTANCE hPrevInstance,
                       _In_ LPWSTR    lpCmdLine,
                       _In_ int       nCmdShow)
  {
  #ifdef _DEBUG
        _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
  #endif

        UNREFERENCED_PARAMETER(hPrevInstance);
        UNREFERENCED_PARAMETER(lpCmdLine);

        CEditorApp* pEditorApp = { nullptr };

        LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
        LoadStringW(hInstance, IDC_EDITOR, szWindowClass, MAX_LOADSTRING);
        MyRegisterClass(hInstance);

        if (!InitInstance(hInstance, nCmdShow))
                return FALSE;

        HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_EDITOR));

        MSG msg;

        // EditorApp 생성
        pEditorApp = CEditorApp::Create(g_hWnd, g_iWinSizeX, g_iWinSizeY);
        if (nullptr == pEditorApp)
                return FALSE;

        CGameInstance* pGameInstance = CGameInstance::GetInstance();
        Safe_AddRef(pGameInstance);

        // Timer 등록
        if (FAILED(pGameInstance->Add_Timer(TEXT("Timer_Default"))))
                return E_FAIL;
        if (FAILED(pGameInstance->Add_Timer(TEXT("Timer_60"))))
                return E_FAIL;

        _float fTimeAcc = { };

        // 게임 메인 루프 (PeekMessage)
        while (true)
        {
                if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
                {
                        if (WM_QUIT == msg.message)
                                break;

                        if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
                        {
                                TranslateMessage(&msg);
                                DispatchMessageW(&msg);
                        }
                }

                pGameInstance->Compute_Timer(TEXT("Timer_Default"));
                fTimeAcc += pGameInstance->Get_TimeDelta(TEXT("Timer_Default"));

                if (fTimeAcc >= 1.f / 60.f)
                {
                        pGameInstance->Compute_Timer(TEXT("Timer_60"));

                        pEditorApp->Update(pGameInstance->Get_TimeDelta(TEXT("Timer_60")));
                        pEditorApp->Render();

                        fTimeAcc = 0.f;
                }
        }

        Safe_Release(pGameInstance);
        Safe_Release(pEditorApp);

        return (int)msg.wParam;
  }

  ATOM MyRegisterClass(HINSTANCE hInstance)
  {
        WNDCLASSEXW wcex;
        wcex.cbSize = sizeof(WNDCLASSEX);
        wcex.style          = CS_HREDRAW | CS_VREDRAW;
        wcex.lpfnWndProc    = WndProc;
        wcex.cbClsExtra     = 0;
        wcex.cbWndExtra     = 0;
        wcex.hInstance      = hInstance;
        wcex.hIcon          = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_EDITOR));
        wcex.hCursor        = LoadCursor(nullptr, IDC_ARROW);
        wcex.hbrBackground  = (HBRUSH)(COLOR_WINDOW + 1);
        wcex.lpszMenuName   = nullptr;  // 에디터는 Win32 메뉴 대신 ImGui 메뉴바 사용
        wcex.lpszClassName  = szWindowClass;
        wcex.hIconSm        = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));
        return RegisterClassExW(&wcex);
  }

  BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
  {
        hInst = hInstance;

        RECT rcWindow = { 0, 0, g_iWinSizeX, g_iWinSizeY };
        AdjustWindowRect(&rcWindow, WS_OVERLAPPEDWINDOW, FALSE);  // FALSE: 메뉴 없음

        HWND hWnd = CreateWindowW(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
                CW_USEDEFAULT, 0,
                rcWindow.right - rcWindow.left,
                rcWindow.bottom - rcWindow.top,
                nullptr, nullptr, hInstance, nullptr);

        if (!hWnd)
                return FALSE;

        ShowWindow(hWnd, nCmdShow);
        UpdateWindow(hWnd);

        g_hWnd = hWnd;

        return TRUE;
  }

  LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
  {
        // 시스템 메시지 먼저 처리
        switch (message)
        {
        case WM_SYSCOMMAND:
                if ((wParam & 0xfff0) == SC_KEYMENU)  // Alt 키 메뉴 비활성화
                        return 0;
                break;
        case WM_CLOSE:
                DestroyWindow(hWnd);
                return 0;
        case WM_DESTROY:
                PostQuitMessage(0);
                return 0;
        }

        // ImGui 입력 처리
        if (ImGui_ImplWin32_WndProcHandler(hWnd, message, wParam, lParam))
                return true;

        return DefWindowProc(hWnd, message, wParam, lParam);
  }

  GameApp.cpp 대비 변경점:
  - CMainApp → CEditorApp
  - ImGui WndProcHandler 추가 (extern 전방선언 + switch 후 호출)
  - wcex.lpszMenuName = nullptr (Win32 메뉴 제거, ImGui 메뉴바로 대체)
  - AdjustWindowRect 세 번째 인자 FALSE (메뉴 없음)
  - WM_SYSCOMMAND에서 Alt 키 메뉴 비활성화 (ImGui 입력 충돌 방지)
  - 해상도 1600x900 (에디터 패널 공간)

  ---
  6. Editor.vcxproj 수정사항 (Debug|x64 기준)

  현재 vcxproj는 거의 빈 상태입니다. 추가해야 할 설정:

  <!-- Debug|x64 ItemDefinitionGroup 내 -->
  <ClCompile>
    <WarningLevel>Level3</WarningLevel>
    <SDLCheck>true</SDLCheck>
    <PreprocessorDefinitions>_DEBUG;_WINDOWS;%(PreprocessorDefinitions)</PreprocessorDefinitions>
    <ConformanceMode>true</ConformanceMode>
    <MultiProcessorCompilation>true</MultiProcessorCompilation>
    <RuntimeLibrary>MultiThreadedDebugDLL</RuntimeLibrary>  <!-- /MDd 필수 -->
    <AdditionalIncludeDirectories>
      ..\Public\;
      ..\..\EngineSDK\Inc\;
      ..\..\ClientSDK\Inc\;
      ..\ImGui\;
      ..\ImGuizmo\
    </AdditionalIncludeDirectories>
  </ClCompile>
  <Link>
    <SubSystem>Windows</SubSystem>
    <GenerateDebugInformation>true</GenerateDebugInformation>
    <AdditionalLibraryDirectories>
      ..\..\EngineSDK\Lib\;
      ..\..\ClientSDK\Lib\
    </AdditionalLibraryDirectories>
    <AdditionalDependencies>
      Engine.lib;Client.lib;d3d11.lib;dxguid.lib;%(AdditionalDependencies)
    </AdditionalDependencies>
  </Link>

  ImGui/ImGuizmo 소스 파일 등록 (ItemGroup에 추가):

  <ItemGroup>
    <!-- Editor 소스 -->
    <ClCompile Include="Editor.cpp" />
    <ClCompile Include="..\Private\EditorApp.cpp" />

    <!-- ImGui 소스 (Precompiled Header 사용 안 함) -->
    <ClCompile Include="..\ImGui\imgui.cpp">
      <PrecompiledHeader>NotUsing</PrecompiledHeader>
    </ClCompile>
    <ClCompile Include="..\ImGui\imgui_draw.cpp">
      <PrecompiledHeader>NotUsing</PrecompiledHeader>
    </ClCompile>
    <ClCompile Include="..\ImGui\imgui_tables.cpp">
      <PrecompiledHeader>NotUsing</PrecompiledHeader>
    </ClCompile>
    <ClCompile Include="..\ImGui\imgui_widgets.cpp">
      <PrecompiledHeader>NotUsing</PrecompiledHeader>
    </ClCompile>
    <ClCompile Include="..\ImGui\imgui_demo.cpp">
      <PrecompiledHeader>NotUsing</PrecompiledHeader>
    </ClCompile>
    <ClCompile Include="..\ImGui\imgui_impl_dx11.cpp">
      <PrecompiledHeader>NotUsing</PrecompiledHeader>
    </ClCompile>
    <ClCompile Include="..\ImGui\imgui_impl_win32.cpp">
      <PrecompiledHeader>NotUsing</PrecompiledHeader>
    </ClCompile>

    <!-- ImGuizmo -->
    <ClCompile Include="..\ImGuizmo\ImGuizmo.cpp">
      <PrecompiledHeader>NotUsing</PrecompiledHeader>
    </ClCompile>
  </ItemGroup>

  <ItemGroup>
    <!-- Editor 헤더 -->
    <ClInclude Include="Editor.h" />
    <ClInclude Include="Editor_Defines.h" />
    <ClInclude Include="framework.h" />
    <ClInclude Include="Resource.h" />
    <ClInclude Include="targetver.h" />
    <ClInclude Include="..\Public\EditorApp.h" />
  </ItemGroup>

  ---
  7. ImGui docking 브랜치에서 가져올 파일 목록

  https://github.com/ocornut/imgui/tree/docking 에서:

  루트에서 (13개):

  ┌───────────────────┬─────────────────┐
  │       파일        │      위치       │
  ├───────────────────┼─────────────────┤
  │ imgui.h           │ → Editor/ImGui/ │
  ├───────────────────┼─────────────────┤
  │ imgui.cpp         │ → Editor/ImGui/ │
  ├───────────────────┼─────────────────┤
  │ imgui_draw.cpp    │ → Editor/ImGui/ │
  ├───────────────────┼─────────────────┤
  │ imgui_tables.cpp  │ → Editor/ImGui/ │
  ├───────────────────┼─────────────────┤
  │ imgui_widgets.cpp │ → Editor/ImGui/ │
  ├───────────────────┼─────────────────┤
  │ imgui_demo.cpp    │ → Editor/ImGui/ │
  ├───────────────────┼─────────────────┤
  │ imgui_internal.h  │ → Editor/ImGui/ │
  ├───────────────────┼─────────────────┤
  │ imconfig.h        │ → Editor/ImGui/ │
  ├───────────────────┼─────────────────┤
  │ imstb_rectpack.h  │ → Editor/ImGui/ │
  ├───────────────────┼─────────────────┤
  │ imstb_textedit.h  │ → Editor/ImGui/ │
  ├───────────────────┼─────────────────┤
  │ imstb_truetype.h  │ → Editor/ImGui/ │
  └───────────────────┴─────────────────┘

  backends/ 에서 (4개):

  ┌──────────────────────┬─────────────────┐
  │         파일         │      위치       │
  ├──────────────────────┼─────────────────┤
  │ imgui_impl_dx11.h    │ → Editor/ImGui/ │
  ├──────────────────────┼─────────────────┤
  │ imgui_impl_dx11.cpp  │ → Editor/ImGui/ │
  ├──────────────────────┼─────────────────┤
  │ imgui_impl_win32.h   │ → Editor/ImGui/ │
  ├──────────────────────┼─────────────────┤
  │ imgui_impl_win32.cpp │ → Editor/ImGui/ │
  └──────────────────────┴─────────────────┘

  주의: backends/ 폴더의 cpp 파일들은 #include "imgui.h"를 상대경로로 참조합니다. 같은 Editor/ImGui/ 폴더에 넣으면 문제 없지만, 별도 폴더에 넣으면 include path 수정이 필요합니다.

  ---
  8. 실행 흐름 요약

  Editor.cpp wWinMain()
    → 윈도우 생성 (1600x900, 메뉴 없음)
    → CEditorApp::Create(hWnd, 1600, 900)
        → CGameInstance::Initialize_Engine()  → Device/Context 획득
        → Ready_ImGui(hWnd)                   → ImGui Context + DX11/Win32 Backend
    → PeekMessage 루프 (60 FPS)
        → CEditorApp::Update()               → Engine 업데이트
        → CEditorApp::Render()
            → ImGui NewFrame
            → DockSpace 설정
            → (에디터 패널들 - 이후 추가)
            → Begin_Draw (Clear)
            → Draw (Engine 렌더)
            → ImGui::Render + RenderDrawData
            → End_Draw (Present)
    → Safe_Release (EditorApp → Shutdown_ImGui → Release_Engine)

  ---
  정리

  ┌───────────────────────┬──────────────────┬───────────────────────┐
  │         작업          │       파일       │         상태          │
  ├───────────────────────┼──────────────────┼───────────────────────┤
  │ Editor_Defines.h 생성 │ Editor/Default/  │ 신규                  │
  ├───────────────────────┼──────────────────┼───────────────────────┤
  │ CEditorApp.h 생성     │ Editor/Public/   │ 신규                  │
  ├───────────────────────┼──────────────────┼───────────────────────┤
  │ CEditorApp.cpp 생성   │ Editor/Private/  │ 신규                  │
  ├───────────────────────┼──────────────────┼───────────────────────┤
  │ Editor.cpp 재작성     │ Editor/Default/  │ 수정                  │
  ├───────────────────────┼──────────────────┼───────────────────────┤
  │ Editor.vcxproj 수정   │ Editor/Default/  │ 수정                  │
  ├───────────────────────┼──────────────────┼───────────────────────┤
  │ ImGui 파일 복사       │ Editor/ImGui/    │ docking 브랜치        │
  ├───────────────────────┼──────────────────┼───────────────────────┤
  │ ImGuizmo 파일 복사    │ Editor/ImGuizmo/ │ SR 프로젝트 or GitHub │
  └───────────────────────┴──────────────────┴───────────────────────┘

  이대로 진행하면 빌드 후 파란 배경 + ImGui Demo Window가 뜨는 상태가 됩니다. 그 뒤로 맵툴, 인스펙터 등은 ImGui 패널 단위로 붙이면 됩니다.