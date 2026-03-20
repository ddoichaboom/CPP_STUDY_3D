#include "Terrain.h"
#include "GameInstance.h"

CTerrain::CTerrain(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
    : CGameObject{ pDevice, pContext }
{
}

CTerrain::CTerrain(const CTerrain& Prototype)
    : CGameObject { Prototype }
{
}

HRESULT CTerrain::Initialize_Prototype()
{
    return S_OK;
}

HRESULT CTerrain::Initialize(void* pArg)
{
    FAILED_CHECK(__super::Initialize(pArg));

    FAILED_CHECK(Ready_Components());

    return S_OK;
}

void CTerrain::Priority_Update(_float fTimeDelta)
{
}

void CTerrain::Update(_float fTimeDelta)
{
}

void CTerrain::Late_Update(_float fTimeDelta)
{
    m_pGameInstance->Add_RenderGroup(RENDERID::NONBLEND, this);
}

HRESULT CTerrain::Render()
{
    FAILED_CHECK(Bind_ShaderResources());

    FAILED_CHECK(m_pShaderCom->Begin(0));

    FAILED_CHECK(m_pVIBufferCom->Bind_Resources());

    FAILED_CHECK(m_pVIBufferCom->Render());

    return S_OK;
}

HRESULT CTerrain::Ready_Components()
{
    // Com_Shader
    FAILED_CHECK(__super::Add_Component(ETOUI(LEVEL::STATIC), TEXT("Prototype_Component_Shader_VtxTex"),
        TEXT("Com_Shader"), reinterpret_cast<CComponent**>(&m_pShaderCom)));

    // Com_VIBuffer
    FAILED_CHECK(__super::Add_Component(ETOUI(LEVEL::STATIC), TEXT("Prototype_Component_VIBuffer_Terrain"),
        TEXT("Com_VIBuffer"), reinterpret_cast<CComponent**>(&m_pVIBufferCom)));

    // Com_Texture
    FAILED_CHECK(__super::Add_Component(ETOUI(LEVEL::GAMEPLAY), TEXT("Prototype_Component_Texture_Terrain"),
        TEXT("Com_Texture"), reinterpret_cast<CComponent**>(&m_pTextureCom)));

    return S_OK;
}

HRESULT CTerrain::Bind_ShaderResources()
{
    FAILED_CHECK(m_pTransformCom->Bind_ShaderResource(m_pShaderCom, "g_WorldMatrix"));

    FAILED_CHECK(m_pTextureCom->Bind_ShaderResource(m_pShaderCom, "g_Texture", 0));

    return S_OK;
}

CTerrain* CTerrain::Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
    CTerrain* pInstance = new CTerrain(pDevice, pContext);

    if (FAILED(pInstance->Initialize_Prototype()))
    {
        MSG_BOX("Failed to Created : CTerrain");
        Safe_Release(pInstance);
    }

    return pInstance;
}

CGameObject* CTerrain::Clone(void* pArg)
{
    CTerrain* pInstance = new CTerrain(*this);

    if (FAILED(pInstance->Initialize(pArg)))
    {
        MSG_BOX("Failed to Cloned : CTerrain");
        Safe_Release(pInstance);
    }

    return pInstance;
}

void CTerrain::Free()
{
    __super::Free();

    Safe_Release(m_pVIBufferCom);
    Safe_Release(m_pShaderCom);
    Safe_Release(m_pTextureCom);
}