#include "Loader.h"

// ENGINE
#include "GameInstance.h"

// LOGO
#include "BackGround.h"

// GAMEOBJECT
#include "Terrain.h"
#include "Camera_Free.h"

CLoader::CLoader(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
	: m_pDevice { pDevice }
	, m_pContext { pContext }
	, m_pGameInstance{ CGameInstance::GetInstance() }
{
	Safe_AddRef(m_pGameInstance);
	Safe_AddRef(m_pDevice);
	Safe_AddRef(m_pContext);
}

unsigned int APIENTRY ThreadMain(void* pArg)
{
	CLoader* pLoader = static_cast<CLoader*>(pArg);

	if (FAILED(pLoader->Loading()))
		return -1;

	return 0;
}

HRESULT CLoader::Initialize(LEVEL eNextLevelID)
{
	m_eNextLevelID = eNextLevelID;

	InitializeCriticalSection(&m_CriticalSection);

	// eNextLevelID 에 필요한 자원을 로딩하는 작업을 수행한다.
	// 누가? 스레드가 수행한다.
	m_hThread = reinterpret_cast<HANDLE>(_beginthreadex(nullptr, 0, ThreadMain, this, 0, nullptr));
	if (0 == m_hThread)
		return E_FAIL;

	return S_OK;
}

HRESULT CLoader::Loading()
{
	EnterCriticalSection(&m_CriticalSection);

	CoInitializeEx(nullptr, 0);

	HRESULT			hr = {};

	switch (m_eNextLevelID)
	{
	case LEVEL::LOGO:
		hr = Ready_Resources_For_Logo();
		break;
	case LEVEL::GAMEPLAY:
		hr = Ready_Resources_For_GamePlay();
		break;
	}

	CoUninitialize();

	LeaveCriticalSection(&m_CriticalSection);

	if (FAILED(hr))
		return E_FAIL;

	return S_OK;
}

#ifdef _DEBUG
void CLoader::Show()
{
	SetWindowText(g_hWnd, m_szLoadingText);
}
#endif

HRESULT CLoader::Ready_Resources_For_Logo()
{
	lstrcpy(m_szLoadingText, TEXT("텍스쳐 로딩 중"));

	// Prototype_Component_Texture_BackGround
	if (FAILED(m_pGameInstance->Add_Prototype(ETOUI(LEVEL::LOGO), TEXT("Prototype_Component_Texture_BackGround"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../Bin/Resources/Textures/Default%d.jpg"), 2))))
		return E_FAIL;

	lstrcpy(m_szLoadingText, TEXT("셰이더 로딩 중"));


	lstrcpy(m_szLoadingText, TEXT("정점, 인덱스 버퍼 로딩 중"));


	lstrcpy(m_szLoadingText, TEXT("객체원형 로딩 중"));

	// Prototype_GameObject_BackGround
	if (FAILED(m_pGameInstance->Add_Prototype(ETOUI(LEVEL::LOGO),TEXT("Prototype_GameObject_BackGround"), CBackGround::Create(m_pDevice, m_pContext))))
		return E_FAIL;

	lstrcpy(m_szLoadingText, TEXT("로딩이 완료되었습니다."));

	m_isFinished = true;

	return S_OK;
}

HRESULT CLoader::Ready_Resources_For_GamePlay()
{
	lstrcpy(m_szLoadingText, TEXT("텍스쳐 로딩 중"));

	// Prototype_Component_Texture_Terrain
	if (FAILED(m_pGameInstance->Add_Prototype(ETOUI(LEVEL::GAMEPLAY), TEXT("Prototype_Component_Texture_Terrain"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../Bin/Resources/Textures/Terrain/Tile0.jpg"), 1))))
		return E_FAIL;

	lstrcpy(m_szLoadingText, TEXT("셰이더 로딩 중"));

	// Prototype_Component_Shader_VtxNorTex

	if (FAILED(m_pGameInstance->Add_Prototype(ETOUI(LEVEL::GAMEPLAY), TEXT("Prototype_Component_Shader_VtxNorTex"),
		CShader::Create(m_pDevice, m_pContext, TEXT("../Bin/ShaderFiles/Shader_VtxNorTex.hlsl"), VTXNORTEX::Elements, VTXNORTEX::iNumElements))))
		return E_FAIL;



	lstrcpy(m_szLoadingText, TEXT("정점, 인덱스 버퍼 로딩 중"));

	// Prototype_Component_VIBuffer_Terrain
	if (FAILED(m_pGameInstance->Add_Prototype(ETOUI(LEVEL::GAMEPLAY), TEXT("Prototype_Component_VIBuffer_Terrain"),
		CVIBuffer_Terrain::Create(m_pDevice, m_pContext, TEXT("../Bin/Resources/Textures/Terrain/Height.bmp")))))
		return E_FAIL;

	lstrcpy(m_szLoadingText, TEXT("객체원형 로딩 중"));

	// Prototype_GameObject_Terrain
	if (FAILED(m_pGameInstance->Add_Prototype(ETOUI(LEVEL::GAMEPLAY), TEXT("Prototype_GameObject_Terrain"),
		CTerrain::Create(m_pDevice, m_pContext))))
		return E_FAIL;

	// Prototype_GameObject_Camera_Free
	if (FAILED(m_pGameInstance->Add_Prototype(ETOUI(LEVEL::GAMEPLAY), TEXT("Prototype_GameObject_Camera_Free"),
		CCamera_Free::Create(m_pDevice, m_pContext))))
		return E_FAIL;

	lstrcpy(m_szLoadingText, TEXT("로딩이 완료되었습니다."));

	m_isFinished = true;

	return S_OK;
}

CLoader* CLoader::Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext, LEVEL eNextLevelID)
{
	CLoader* pInstance = new CLoader(pDevice, pContext);

	if (FAILED(pInstance->Initialize(eNextLevelID)))
	{
		MSG_BOX("Failed to Created : CLoader");
		Safe_Release(pInstance);
	}

	return pInstance;
}

void CLoader::Free()
{
	__super::Free();

	WaitForSingleObject(m_hThread, INFINITE);

	DeleteCriticalSection(&m_CriticalSection);

	CloseHandle(m_hThread);

	Safe_Release(m_pGameInstance);

	Safe_Release(m_pDevice);
	Safe_Release(m_pContext);
}