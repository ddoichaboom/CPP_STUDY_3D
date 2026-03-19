#include "Transform.h"
#include "Shader.h"

CTransform::CTransform(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
	: CComponent{ pDevice, pContext }
{

}

CTransform::CTransform(const CTransform& Prototype)
	: CComponent{ Prototype }
	, m_WorldMatrix { Prototype.m_WorldMatrix }
{

}

HRESULT CTransform::Initialize_Prototype()
{
	XMStoreFloat4x4(&m_WorldMatrix, XMMatrixIdentity());		// 항등 행렬로 초기화

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

HRESULT CTransform::Bind_ShaderResource(CShader* pShader, const _char* pConstantName)
{
	// 자신의 월드 행렬을 셰이더의 지정된 상수에 바인딩
	return pShader->Bind_Matrix(pConstantName, &m_WorldMatrix);
}

void CTransform::Set_Scale(_float fScaleX, _float fScaleY, _float fScaleZ)
{
	Set_State(STATE::RIGHT, XMVector3Normalize(Get_State(STATE::RIGHT)) * fScaleX);
	Set_State(STATE::UP, XMVector3Normalize(Get_State(STATE::UP)) * fScaleY);
	Set_State(STATE::LOOK, XMVector3Normalize(Get_State(STATE::LOOK)) * fScaleZ);
}

void CTransform::Scaling(_float fScaleX, _float fScaleY, _float fScaleZ)
{
	Set_State(STATE::RIGHT, Get_State(STATE::RIGHT) * fScaleX);
	Set_State(STATE::UP, Get_State(STATE::UP) * fScaleY);
	Set_State(STATE::LOOK, Get_State(STATE::LOOK) * fScaleZ);
}

void CTransform::Go_Straight(_float fTimeDelta)
{
	_vector vPosition	=	Get_State(STATE::POSITION);
	_vector vLook		=	Get_State(STATE::LOOK);

	vPosition			+=	XMVector3Normalize(vLook)* m_fSpeedPerSec* fTimeDelta;

	Set_State(STATE::POSITION, vPosition);
}

void CTransform::Go_Backward(_float fTimeDelta)
{
	_vector vPosition	=	Get_State(STATE::POSITION);
	_vector vLook		=	Get_State(STATE::LOOK);

	vPosition			-=	XMVector3Normalize(vLook) * m_fSpeedPerSec * fTimeDelta;

	Set_State(STATE::POSITION, vPosition);
}

void CTransform::Go_Left(_float fTimeDelta)
{
	_vector vPosition	=	Get_State(STATE::POSITION);
	_vector vRight		=	Get_State(STATE::RIGHT);

	vPosition			-=	XMVector3Normalize(vRight) * m_fSpeedPerSec * fTimeDelta;

	Set_State(STATE::POSITION, vPosition);
}

void CTransform::Go_Right(_float fTimeDelta)
{
	_vector vPosition	=	Get_State(STATE::POSITION);
	_vector vRight		=	Get_State(STATE::RIGHT);

	vPosition			+=	XMVector3Normalize(vRight) * m_fSpeedPerSec * fTimeDelta;

	Set_State(STATE::POSITION, vPosition);
}

void CTransform::Rotation(_fvector vAxis, _float fRadian)
{
	_float3 vScale			= Get_Scale();

	_vector vRight			= XMVectorSet(1.f, 0.f, 0.f, 0.f) * vScale.x;
	_vector vUp				= XMVectorSet(0.f, 1.f, 0.f, 0.f) * vScale.y;
	_vector vLook			= XMVectorSet(0.f, 0.f, 1.f, 0.f) * vScale.z;

	_matrix RotationMatrix	= XMMatrixRotationAxis(vAxis, fRadian);

	Set_State(STATE::RIGHT, XMVector3TransformNormal(vRight, RotationMatrix));
	Set_State(STATE::UP, XMVector3TransformNormal(vUp, RotationMatrix));
	Set_State(STATE::LOOK, XMVector3TransformNormal(vLook, RotationMatrix));
}

void CTransform::Turn(_fvector vAxis, _float fTimeDelta)
{
	_vector vRight			= Get_State(STATE::RIGHT);
	_vector vUp				= Get_State(STATE::UP);
	_vector vLook			= Get_State(STATE::LOOK);

	_matrix RotationMatrix	= XMMatrixRotationAxis(vAxis, m_fRotationPerSec * fTimeDelta);

	Set_State(STATE::RIGHT, XMVector3TransformNormal(vRight, RotationMatrix));
	Set_State(STATE::UP, XMVector3TransformNormal(vUp, RotationMatrix));
	Set_State(STATE::LOOK, XMVector3TransformNormal(vLook, RotationMatrix));
}

void CTransform::LookAt(_fvector vAt)
{
	_vector vLook			= vAt - Get_State(STATE::POSITION);
	_vector vRight			= XMVector3Cross(XMVectorSet(0.f, 1.f, 0.f, 0.f), vLook);
	_vector vUp				= XMVector3Cross(vLook, vRight);

	_float3 vScale			= Get_Scale();

	Set_State(STATE::RIGHT, XMVector3Normalize(vRight) * vScale.x);
	Set_State(STATE::UP, XMVector3Normalize(vUp) * vScale.y);
	Set_State(STATE::LOOK, XMVector3Normalize(vLook) * vScale.z);
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
