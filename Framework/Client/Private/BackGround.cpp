#include "BackGround.h"
#include "GameInstance.h"
#include "Terrain.h"
#include "Transform_2D.h"

CBackGround::CBackGround(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
	: CUIObject{ pDevice, pContext }
{
}

CBackGround::CBackGround(const CBackGround& Prototype)
	: CUIObject{ Prototype }
{
}

HRESULT CBackGround::Initialize_Prototype()
{
	return S_OK;
}

HRESULT CBackGround::Initialize(void* pArg)
{
	BACKGROUND_DESC			Desc{};

	Desc.fSpeedPerSec = 10.f;
	Desc.fCenterX = g_iWinSizeX * 0.5f;
	Desc.fCenterY = g_iWinSizeY * 0.5f;
	Desc.fSizeX = g_iWinSizeX;
	Desc.fSizeY = g_iWinSizeY;

	FAILED_CHECK(__super::Initialize(&Desc));

	FAILED_CHECK(Ready_Components());


	return S_OK;
}

void CBackGround::Priority_Update(_float fTimeDelta)
{
	int a = 10;		// 중단점 용도
}

void CBackGround::Update(_float fTimeDelta)
{
	static_cast<CTransform_2D*>(m_pTransformCom)->Move_Y(fTimeDelta);
}

void CBackGround::Late_Update(_float fTimeDelta)
{
	// 업데이트 다 끝날 시점(렌더 직전)에 자기 자신을 해당되는 렌더 그룹에 등록
	m_pGameInstance->Add_RenderGroup(RENDERID::UI, this);
}

HRESULT CBackGround::Render()
{
	FAILED_CHECK(Bind_ShaderResources());

	FAILED_CHECK(m_pShaderCom->Begin(0));

	FAILED_CHECK(m_pVIBufferCom->Bind_Resources());

	FAILED_CHECK(m_pVIBufferCom->Render());

	return S_OK;
}

HRESULT CBackGround::Ready_Components()
{
	// Com_Shader
	FAILED_CHECK(__super::Add_Component(ETOUI(LEVEL::STATIC), TEXT("Prototype_Component_Shader_VtxTex"),
		TEXT("Com_Shader"), reinterpret_cast<CComponent**>(&m_pShaderCom)));

	// Com_VIBuffer
	FAILED_CHECK(__super::Add_Component(ETOUI(LEVEL::STATIC), TEXT("Prototype_Component_VIBuffer_Rect"),
		TEXT("Com_VIBuffer"), reinterpret_cast<CComponent**>(&m_pVIBufferCom)));

	// Com_Texture
	FAILED_CHECK(__super::Add_Component(ETOUI(LEVEL::LOGO), TEXT("Prototype_Component_Texture_BackGround"),
		TEXT("Com_Texture"), reinterpret_cast<CComponent**>(&m_pTextureCom)));

	return S_OK;
}

HRESULT CBackGround::Bind_ShaderResources()
{
	// (1) World 행렬 바인딩
	FAILED_CHECK(m_pTransformCom->Bind_ShaderResource(m_pShaderCom, "g_WorldMatrix"));

	// (2) View 행렬 바인딩
	FAILED_CHECK(__super::Bind_ShaderResource(m_pShaderCom, "g_ViewMatrix", D3DTS::VIEW));

	// (3) Projection 행렬 바인딩
	FAILED_CHECK(__super::Bind_ShaderResource(m_pShaderCom, "g_ProjMatrix", D3DTS::PROJ));

	// (4) Texture 바인딩
	FAILED_CHECK(m_pTextureCom->Bind_ShaderResource(m_pShaderCom, "g_Texture", 0));

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

	Safe_Release(m_pShaderCom);
	Safe_Release(m_pVIBufferCom);
	Safe_Release(m_pTextureCom);
}
