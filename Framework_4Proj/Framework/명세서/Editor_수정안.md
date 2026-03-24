  방법 비교

  ┌─────────────────────┬───────────────────────────────────────────────┐
  │        방법         │                     특징                      │
  ├─────────────────────┼───────────────────────────────────────────────┤
  │ GetSystemMetrics    │ Win32 API, 간단하지만 주 모니터만             │
  ├─────────────────────┼───────────────────────────────────────────────┤
  │ DXGI Adapter Output │ 그래픽 카드에서 직접, Device 생성 전에도 가능 │
  ├─────────────────────┼───────────────────────────────────────────────┤
  │ EnumDisplaySettings │ Win32, 현재 디스플레이 모드 조회              │
  └─────────────────────┴───────────────────────────────────────────────┘

  DXGI 방식이 가장 적합합니다. D3D11 프로젝트이기도 하고, 그래픽 카드가 인식하는 실제 해상도를 가져오니까요.

  적용 방법

  Editor_Defines.h

  정적 상수 대신 함수로 해상도를 가져오는 방식:

  #pragma once

  #include <Windows.h>
  #include <d3d11.h>
  #include <dxgi.h>

  #pragma comment(lib, "dxgi.lib")

  extern HWND   g_hWnd;

  inline bool Get_MonitorResolution(_Out_ unsigned int* pWidth, _Out_ unsigned int* pHeight)
  {
        IDXGIFactory* pFactory = nullptr;
        if (FAILED(CreateDXGIFactory(__uuidof(IDXGIFactory), (void**)&pFactory)))
                return false;

        IDXGIAdapter* pAdapter = nullptr;
        if (FAILED(pFactory->EnumAdapters(0, &pAdapter)))
        {
                pFactory->Release();
                return false;
        }

        IDXGIOutput* pOutput = nullptr;
        if (FAILED(pAdapter->EnumOutputs(0, &pOutput)))
        {
                pAdapter->Release();
                pFactory->Release();
                return false;
        }

        DXGI_OUTPUT_DESC outputDesc{};
        pOutput->GetDesc(&outputDesc);

        *pWidth = outputDesc.DesktopCoordinates.right - outputDesc.DesktopCoordinates.left;
        *pHeight = outputDesc.DesktopCoordinates.bottom - outputDesc.DesktopCoordinates.top;

        pOutput->Release();
        pAdapter->Release();
        pFactory->Release();

        return true;
  }

  Editor.cpp (InitInstance 부분만 변경)

  BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
  {
        hInst = hInstance;

        /* DXGI로 모니터 해상도 획득 */
        unsigned int iWinSizeX = 1600;  // 실패 시 기본값
        unsigned int iWinSizeY = 900;
        Get_MonitorResolution(&iWinSizeX, &iWinSizeY);

        HWND hWnd = CreateWindowW(szWindowClass, szTitle,
                WS_POPUP,                              // 전체화면: 테두리/타이틀바 없음
                0, 0, iWinSizeX, iWinSizeY,
                nullptr, nullptr, hInstance, nullptr);

        if (!hWnd)
                return FALSE;

        ShowWindow(hWnd, SW_SHOWMAXIMIZED);
        UpdateWindow(hWnd);

        g_hWnd = hWnd;

        return TRUE;
  }

  그리고 wWinMain에서 EditorApp 생성 시에도 정적 상수 대신 윈도우 실제 크기를 넘겨줍니다:

  // EditorApp 생성 (윈도우 실제 크기 기반)
  RECT rcClient{};
  GetClientRect(g_hWnd, &rcClient);

  pEditorApp = CEditorApp::Create(g_hWnd,
        rcClient.right - rcClient.left,
        rcClient.bottom - rcClient.top);

  Editor_Defines.h 최종 형태

  g_iWinSizeX/Y 정적 상수가 필요 없어집니다:

  #pragma once

  #include <Windows.h>
  #include <d3d11.h>
  #include <dxgi.h>

  #pragma comment(lib, "dxgi.lib")

  extern HWND   g_hWnd;

  inline bool Get_MonitorResolution(_Out_ unsigned int* pWidth, _Out_ unsigned int* pHeight)
  {
        /* 위 함수 동일 */
  }

  포인트:
  - WS_POPUP 스타일 → 타이틀바/테두리 없는 전체화면 (실제 풀스크린이 아닌 보더리스 윈도우)
  - AdjustWindowRect 불필요 (WS_POPUP은 클라이언트 영역 = 윈도우 영역)
  - DXGI Factory는 해상도 조회 후 바로 Release (Engine의 Device 생성과 무관)
  - 실제 D3D11 풀스크린(WINMODE::FULL)이 아니라 보더리스 윈도우 방식이라 Alt+Tab 전환이 자유롭고, ImGui 패널 드래그에도 문제 없음

  이 방식이면 어떤 모니터에서든 자동으로 맞춰집니다.