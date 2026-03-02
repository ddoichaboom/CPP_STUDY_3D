#include "GameObject.h"
#include "GameInstance.h"

CGameObject::CGameObject(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
	: m_pDevice{pDevice}
	, m_pContext{pContext}
	, m_pGameInstance { CGameInstance::GetInstance() }
{
	Safe_AddRef(m_pGameInstance);
	Safe_AddRef(m_pDevice);
	Safe_AddRef(m_pContext);
}

CGameObject::CGameObject(const CGameObject& Prototype)
	: m_pDevice{Prototype.m_pDevice}
	, m_pContext{Prototype.m_pContext}
	, m_pGameInstance{ CGameInstance::GetInstance() }
{
	Safe_AddRef(m_pGameInstance);
	Safe_AddRef(m_pDevice);
	Safe_AddRef(m_pContext);
}

HRESULT CGameObject::Initialize_Prototype()
{
	return S_OK;
}

HRESULT CGameObject::Initialize(void* pArg)
{
	if (nullptr != pArg)
	{
		auto pDesc = static_cast<GAMEOBJECT_DESC*>(pArg);
		m_iFlag = pDesc->iFlag;
	}

	// Transform은 모든 GameObject가 1:1로 반드시 소유하는 Component이다.
	// Texture나 Mesh처럼 여러 객체가 같은 원형을 공유해서 복제할 필요가 없고, 객체마다 고유한 위치/회전/스케일을 가지므로 원형 객체를 공유하는 의미가 없다.
	// 따라서 간단하게 Create()로 직접 생성한 후 원형 객체를 그대로 소유하는 구조이다.
	// Prototype_Manager에 등록 X , Clone()도 X 

	// 객체당 부여되어야 할 Transform-Component를 생성한다.
	m_pTransformCom = CTransform::Create(m_pDevice, m_pContext);
	if (nullptr == m_pTransformCom)
		return E_FAIL;

	// 객체에게 부여된 초기 월드 상태를 Transform에게 동기화시킨다.
	if (FAILED(m_pTransformCom->Initialize(pArg)))								
		return E_FAIL;


	return S_OK;
}

void CGameObject::Priority_Update(_float fTimeDelta)
{
}

void CGameObject::Update(_float fTimeDelta)
{

}

void CGameObject::Late_Update(_float fTimeDelta)
{
}

HRESULT CGameObject::Render()
{
	return S_OK;
}

void CGameObject::Free()
{
	__super::Free();

	Safe_Release(m_pTransformCom);
	Safe_Release(m_pGameInstance);
	Safe_Release(m_pContext);
	Safe_Release(m_pDevice);
}