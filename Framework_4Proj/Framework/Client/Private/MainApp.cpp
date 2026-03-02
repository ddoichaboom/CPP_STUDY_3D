#include "MainApp.h"

#include "GameInstance.h"
#include "Level_Loading.h"

CMainApp::CMainApp()
	: m_pGameInstance{ CGameInstance::GetInstance() }
{
	Safe_AddRef(m_pGameInstance);
}

HRESULT CMainApp::Initialize(HWND hWnd, _uint iWinSizeX, _uint iWinSizeY)
{
	ENGINE_DESC		EngineDesc{};
	EngineDesc.hWnd = hWnd;
	EngineDesc.eWinMode = WINMODE::WIN;
	EngineDesc.iViewportWidth = iWinSizeX;
	EngineDesc.iViewportHeight = iWinSizeY;
	EngineDesc.iNumLevels = ETOUI(LEVEL::END);

	if (FAILED(m_pGameInstance->Initialize_Engine(EngineDesc, &m_pDevice, &m_pContext)))
	{
		MSG_BOX("Failed to Initialize : Engine");
		return E_FAIL;
	}

	if (FAILED(Start_Level(LEVEL::LOGO)))
		return E_FAIL;

	return S_OK;
}

void CMainApp::Update(_float fTimeDelta)
{
	m_pGameInstance->Update_Engine(fTimeDelta);
}

HRESULT CMainApp::Render()
{
	FAILED_CHECK(m_pGameInstance->Begin_Draw());

	FAILED_CHECK(m_pGameInstance->Draw());

	FAILED_CHECK(m_pGameInstance->End_Draw());

	return S_OK;
}

HRESULT CMainApp::Start_Level(LEVEL eStartLevelID)
{
	CLevel* pPreLevel = CLevel_Loading::Create(m_pDevice, m_pContext, eStartLevelID);
	if (nullptr == pPreLevel)
		return E_FAIL;

	if (FAILED(m_pGameInstance->Change_Level(ETOI(LEVEL::LOADING), pPreLevel)))
		return E_FAIL;

	return S_OK;
}

CMainApp* CMainApp::Create(HWND hWnd, _uint iWinSizeX, _uint iWinSizeY)
{
	CMainApp* pInstance = new CMainApp();

	if (FAILED(pInstance->Initialize(hWnd, iWinSizeX, iWinSizeY)))
	{
		MSG_BOX("Failed to Created : CMainApp");
		Safe_Release(pInstance);
	}

	return pInstance;

}

void CMainApp::Free()
{
	__super::Free();

	Safe_Release(m_pDevice);
	Safe_Release(m_pContext);

	m_pGameInstance->Release_Engine();
	Safe_Release(m_pGameInstance);
}
