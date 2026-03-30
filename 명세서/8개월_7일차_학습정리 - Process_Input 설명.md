 CInput_Device::Process_Input 상세 설명

  전체 흐름

  OS가 키보드/마우스 하드웨어 이벤트 감지
      ↓
  WndProc에 WM_INPUT 메시지 전달
      ↓
  CGameInstance::Process_RawInput(lParam)
      ↓
  CInput_Device::Process_Input(lParam)  ← 여기
      ↓
  static 누적 버퍼에 저장

  코드를 단계별로 분해

  void CInput_Device::Process_Input(LPARAM lParam)
  {

  1단계: 데이터 크기 확인

      UINT dwSize = 0;
      GetRawInputData(reinterpret_cast<HRAWINPUT>(lParam), RID_INPUT,
          nullptr, &dwSize, sizeof(RAWINPUTHEADER));

  - lParam은 WndProc에서 넘어온 Raw Input 핸들
  - HRAWINPUT으로 캐스팅하여 GetRawInputData에 전달
  - 세 번째 인자 nullptr → "데이터를 주지 말고 크기만 알려줘"
  - dwSize에 필요한 바이트 수가 들어옴

  왜 2번 호출하나?
  → RAWINPUT 구조체 크기가 디바이스마다 다를 수 있어서
  → 먼저 크기를 물어보고, 그 크기만큼 메모리를 할당한 뒤 실제 데이터를 받음

  2단계: 메모리 할당 + 실제 데이터 수신

      BYTE* lpb = new BYTE[dwSize];
      if (GetRawInputData(reinterpret_cast<HRAWINPUT>(lParam), RID_INPUT,
          lpb, &dwSize, sizeof(RAWINPUTHEADER)) != dwSize)
      {
          Safe_Delete_Array(lpb);
          return;
      }

  - dwSize만큼 메모리 할당
  - 이번엔 lpb 전달 → 실제 데이터를 lpb에 채워줌
  - 반환값 ≠ dwSize면 실패 → 정리하고 리턴

  3단계: RAWINPUT 구조체로 해석

      RAWINPUT* raw = reinterpret_cast<RAWINPUT*>(lpb);

  RAWINPUT 구조체 메모리 레이아웃:
  RAWINPUT
  ├── header
  │   ├── dwType        ← RIM_TYPEKEYBOARD 또는 RIM_TYPEMOUSE
  │   ├── dwSize
  │   └── hDevice       ← 어떤 디바이스에서 왔는지 (마우스 2개 구분 가능)
  └── data (union)
      ├── keyboard      ← dwType이 RIM_TYPEKEYBOARD일 때
      │   ├── MakeCode  ← 하드웨어 스캔코드
      │   ├── Flags     ← RI_KEY_BREAK = 키 떼짐
      │   └── VKey      ← Virtual Key Code (VK_W 등) ★ 우리가 사용
      └── mouse         ← dwType이 RIM_TYPEMOUSE일 때
          ├── lLastX    ← 마우스 X 이동량 (delta)
          ├── lLastY    ← 마우스 Y 이동량 (delta)
          ├── usButtonFlags ← 버튼 누름/뗌 플래그
          └── usButtonData  ← 휠 스크롤량

  4단계: 키보드 처리

      if (raw->header.dwType == RIM_TYPEKEYBOARD)
      {
          USHORT vKey = raw->data.keyboard.VKey;

          if (vKey < 256)
          {
              if (raw->data.keyboard.Flags & RI_KEY_BREAK)
                  s_byRawKeyState[vKey] = 0x00;   // 키 떼짐
              else
                  s_byRawKeyState[vKey] = 0x80;   // 키 눌림
          }
      }

  - VKey → 예: W키 누르면 VK_W(0x57)
  - RI_KEY_BREAK 비트가 있으면 키를 뗀 것, 없으면 누른 것
  - 0x80을 저장하는 이유: 나중에 & 0x80으로 눌림 판별하는 패턴 유지

  W키 누름 → s_byRawKeyState[0x57] = 0x80
  W키 뗌   → s_byRawKeyState[0x57] = 0x00

  게임 로직: Get_KeyState(VK_W) & 0x80 → true/false

  5단계: 마우스 처리

      else if (raw->header.dwType == RIM_TYPEMOUSE)
      {
          // 이동량 누적
          s_lRawMouseAccum[ETOUI(MOUSEAXIS::X)] += raw->data.mouse.lLastX;
          s_lRawMouseAccum[ETOUI(MOUSEAXIS::Y)] += raw->data.mouse.lLastY;

  - lLastX/lLastY는 이전 이벤트 대비 이동 픽셀
  - 한 프레임에 WM_INPUT이 여러 번 올 수 있으므로 +=로 누적

  프레임 내 WM_INPUT 3번 발생:
    1차: lLastX = +3   → 누적: 3
    2차: lLastX = +5   → 누적: 8
    3차: lLastX = -1   → 누적: 7
  Update()에서 m_lMouseDelta[X] = 7 로 복사 → 이 프레임 총 이동량

          // 버튼 상태
          USHORT flags = raw->data.mouse.usButtonFlags;

          if (flags & RI_MOUSE_LEFT_BUTTON_DOWN)
              s_byRawMouseBtn[ETOUI(MOUSEBTN::LBUTTON)] = 0x80;
          if (flags & RI_MOUSE_LEFT_BUTTON_UP)
              s_byRawMouseBtn[ETOUI(MOUSEBTN::LBUTTON)] = 0x00;
          // ... 우클릭, 중클릭 동일 패턴

  - usButtonFlags는 비트 플래그 → 여러 이벤트가 동시에 담길 수 있음
  - DOWN/UP이 별도 플래그이므로 if를 else if 없이 각각 체크

          // 휠 스크롤
          if (flags & RI_MOUSE_WHEEL)
              s_lRawMouseAccum[ETOUI(MOUSEAXIS::WHEEL)] +=
                  static_cast<SHORT>(raw->data.mouse.usButtonData);
      }

  - usButtonData는 USHORT이지만 휠 값은 부호 있는 값(위=양수, 아래=음수)
  - SHORT로 캐스팅해야 음수가 정상 처리됨 (보통 ±120 단위)

  6단계: 정리

      Safe_Delete_Array(lpb);
  }

  왜 static 함수인가

  WndProc은 Win32 콜백 → C 스타일 함수 포인터
      ↓
  멤버 함수는 this 포인터가 필요해서 콜백으로 등록 불가
      ↓
  static 함수로 만들어서 호출
      ↓
  인스턴스 멤버 접근 불가 → static 버퍼에 저장
      ↓
  Update()에서 static 버퍼 → 인스턴스 멤버로 복사

  데이터 생명주기 요약

  [WM_INPUT 이벤트]        [매 프레임 Update()]       [게임 로직]
  s_byRawKeyState[256] → m_byKeyState[256]      → Get_KeyState(VK_W)
  s_byRawMouseBtn[3]   → m_byMouseBtnState[3]   → Get_MouseBtnState(MOUSEBTN::LBUTTON)
  s_lRawMouseAccum[3]  → m_lMouseDelta[3]       → Get_MouseDelta(MOUSEAXIS::X)
                          (마우스 누적 초기화)