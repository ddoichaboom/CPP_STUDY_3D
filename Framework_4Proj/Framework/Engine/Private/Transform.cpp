#include "Transform.h"

CTransform::CTransform(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
	: CComponent{ pDevice, pContext }
{

}

CTransform::CTransform(const CTransform& Prototype)
	: CComponent{ Prototype }
{

}

HRESULT CTransform::Initialize_Prototype()
{
	return S_OK;
}

HRESULT CTransform::Initialize(void* pArg)
{
	// pArg에 할당된 값이 없다면 기본값으로 Transform을 생성
	if (nullptr == pArg)
		return S_OK;

	// pArg가 nullptr이 아니라면 담긴 값으로 설정
	auto pDesc = static_cast<TRANSFORM_DESC*>(pArg);

	m_fRotationPerSec	= pDesc->fRotationPerSec;
	m_fSpeedPerSec		= pDesc->fSpeedPerSec;

	return S_OK;
}

CTransform* CTransform::Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
	CTransform* pInstance = new CTransform(pDevice, pContext);

	if (FAILED(pInstance->Initialize_Prototype()))
	{
		MSG_BOX("Failed to Created : CTransform");
		Safe_Release(pInstance);
	}

	return pInstance;
}

CComponent* CTransform::Clone(void* pArg)
{
	CTransform* pInstance = new CTransform(*this);

	if (FAILED(pInstance->Initialize(pArg)))
	{
		MSG_BOX("Failed to Cloned : CTransform");
		Safe_Release(pInstance);
	}

	return pInstance;

}

void CTransform::Free()
{
	__super::Free();
}
