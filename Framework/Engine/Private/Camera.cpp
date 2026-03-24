#include "Camera.h"
#include "Transform_3D.h"

CCamera::CCamera(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
	: CGameObject{ pDevice, pContext }
{

}

CCamera::CCamera(const CCamera& Prototype)
	: CGameObject { Prototype }
{

}

HRESULT CCamera::Initialize_Prototype()
{
	return S_OK;
}

HRESULT CCamera::Initialize(void* pArg)
{
	auto	pDesc		= static_cast<CAMERA_DESC*>(pArg);

	m_fNear				= pDesc->fNear;
	m_fFar				= pDesc->fFar;
	m_fFovy				= pDesc->fFovy;

	if (FAILED(__super::Initialize(pArg)))
		return E_FAIL;

	static_cast<CTransform_3D*>(m_pTransformCom)->Set_State(STATE::POSITION, XMVectorSetW(XMLoadFloat3(&pDesc->vEye), 1.f));
	static_cast<CTransform_3D*>(m_pTransformCom)->LookAt(XMVectorSetW(XMLoadFloat3(&pDesc->vAt), 1.f));

	return S_OK;
}

void CCamera::Priority_Update(_float fTimeDelta)
{

}

void CCamera::Update(_float fTimeDelta)
{

}

void CCamera::Late_Update(_float fTimeDelta)
{

}

HRESULT CCamera::Render()
{
	return S_OK;
}

void CCamera::Free()
{
	__super::Free();
}
