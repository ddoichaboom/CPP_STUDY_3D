#include "VIBuffer_Terrain.h"

CVIBuffer_Terrain::CVIBuffer_Terrain(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
    : CVIBuffer{ pDevice, pContext }
{
}

CVIBuffer_Terrain::CVIBuffer_Terrain(const CVIBuffer_Terrain& Prototype)
    : CVIBuffer { Prototype }
{
}

HRESULT CVIBuffer_Terrain::Initialize_Prototype(const _tchar* pHeightMapFilePath)
{
    m_iNumVertexBuffers     = 1;
    m_iNumVertices          = 4;
    m_iVertexStride         = sizeof(VTXTEX);

    m_iNumIndices           = 6;
    m_iIndexStride          = 2;
    m_eIndexFormat          = DXGI_FORMAT_R16_UINT;

    m_ePrimitiveType        = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;

    // (1) 사각형을 표현하기 위한 정점.
    D3D11_BUFFER_DESC               VertexBufferDesc{};
    VertexBufferDesc.ByteWidth              = m_iVertexStride * m_iNumVertices;
    VertexBufferDesc.Usage                  = D3D11_USAGE_DEFAULT;
    VertexBufferDesc.BindFlags              = D3D11_BIND_VERTEX_BUFFER;
    VertexBufferDesc.CPUAccessFlags         = 0;
    VertexBufferDesc.MiscFlags              = 0;
    VertexBufferDesc.StructureByteStride    = m_iVertexStride;

    VTXTEX* pVertices                       = new VTXTEX[m_iNumVertices];
    ZeroMemory(pVertices, sizeof(VTXTEX) * m_iNumVertices);

    pVertices[0].vPosition                  = _float3(-0.5f, 0.5f, 0.f);
    pVertices[0].vTexcoord                  = _float2(0.f, 0.f);

    pVertices[1].vPosition                  = _float3(0.5f, 0.5f, 0.f);
    pVertices[1].vTexcoord                  = _float2(1.f, 0.f);

    pVertices[2].vPosition                  = _float3(0.5f, -0.5f, 0.f);
    pVertices[2].vTexcoord                  = _float2(1.f, 1.f);
    
    pVertices[3].vPosition                  = _float3(-0.5f, -0.5f, 0.f);
    pVertices[3].vTexcoord                  = _float2(0.f, 1.f);

    D3D11_SUBRESOURCE_DATA          VertexInitialData{};
    VertexInitialData.pSysMem               = pVertices;

    FAILED_CHECK(m_pDevice->CreateBuffer(&VertexBufferDesc, &VertexInitialData, &m_pVB));

    Safe_Delete_Array(pVertices);


    // (2) 인덱스 버퍼 생성
    D3D11_BUFFER_DESC               IndexBufferDesc{};
    IndexBufferDesc.ByteWidth               = m_iIndexStride * m_iNumIndices;
    IndexBufferDesc.Usage                   = D3D11_USAGE_DEFAULT;
    IndexBufferDesc.BindFlags               = D3D11_BIND_INDEX_BUFFER;
    IndexBufferDesc.CPUAccessFlags          = 0;
    IndexBufferDesc.MiscFlags               = 0;
    IndexBufferDesc.StructureByteStride     = m_iIndexStride;

    _ushort* pIndices = new _ushort[m_iNumIndices];
    ZeroMemory(pIndices, sizeof(_ushort) * m_iNumIndices);

    // 첫번째 삼각형 (우상단 - 대각선 기준)
    pIndices[0] = 0;
    pIndices[1] = 1;
    pIndices[2] = 2;

    // 두 번째 삼각형 (좌하단 - 대각선 기준)
    pIndices[3] = 0;
    pIndices[4] = 2;
    pIndices[5] = 3;

    D3D11_SUBRESOURCE_DATA          IndexInitialData{};
    IndexInitialData.pSysMem                = pIndices;

    FAILED_CHECK(m_pDevice->CreateBuffer(&IndexBufferDesc, &IndexInitialData, &m_pIB));

    Safe_Delete_Array(pIndices);

    return S_OK;
}

HRESULT CVIBuffer_Terrain::Initialize(void* pArg)
{
    return S_OK;
}

CVIBuffer_Terrain* CVIBuffer_Terrain::Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext, const _tchar* pHeightMapFilePath)
{
    CVIBuffer_Terrain* pInstance = new CVIBuffer_Terrain(pDevice, pContext);

    if (FAILED(pInstance->Initialize_Prototype(pHeightMapFilePath)))
    {
        MSG_BOX("Failed to Created : CVIBuffer_Terrain");
        Safe_Release(pInstance);
    }

    return pInstance;
}

CComponent* CVIBuffer_Terrain::Clone(void* pArg)
{
    CVIBuffer_Terrain* pInstance = new CVIBuffer_Terrain(*this);

    if (FAILED(pInstance->Initialize(pArg)))
    {
        MSG_BOX("Failed to Cloned : CVIBuffer_Terrain");
        Safe_Release(pInstance);
    }

    return pInstance;
}

void CVIBuffer_Terrain::Free()
{
    __super::Free();
}
