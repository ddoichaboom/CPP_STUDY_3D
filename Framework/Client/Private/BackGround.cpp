#include "BackGround.h"
#include "GameInstance.h"

CBackGround::CBackGround(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
	: CGameObject { pDevice, pContext }
{
}

CBackGround::CBackGround(const CBackGround& Prototype)
	: CGameObject{ Prototype }
{
}

HRESULT CBackGround::Initialize_Prototype()
{
	return S_OK;
}

HRESULT CBackGround::Initialize(void* pArg)
{
	return S_OK;
}

void CBackGround::Priority_Update(_float fTimeDelta)
{
	int a = 10;		// 중단점 용도
}

void CBackGround::Update(_float fTimeDelta)
{
	int a = 10;		// 중단점 용도
}

void CBackGround::Late_Update(_float fTimeDelta)
{

	// 업데이트 다 끝날 시점(렌더 직전)에 자기 자신을 해당되는 렌더 그룹에 등록
	m_pGameInstance->Add_RenderGroup(RENDERID::PRIORITY, this);
}

HRESULT CBackGround::Render()
{
	return S_OK;
}

CBackGround* CBackGround::Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
	CBackGround* pInstance = new CBackGround(pDevice, pContext);

	if (FAILED(pInstance->Initialize_Prototype()))
	{
		MSG_BOX("Failed to Created : CBackGround");
		Safe_Release(pInstance);
	}

	return pInstance;
}

CGameObject* CBackGround::Clone(void* pArg)
{
	CBackGround* pInstance = new CBackGround(*this);

	if (FAILED(pInstance->Initialize(pArg)))
	{
		MSG_BOX("Failed to Cloned : CBackGround");
		Safe_Release(pInstance);
	}

	return pInstance;
}

void CBackGround::Free()
{
	__super::Free();
}
