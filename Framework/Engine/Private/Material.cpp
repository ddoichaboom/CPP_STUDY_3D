#include "Material.h"
#include "Shader.h"

CMaterial::CMaterial(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
	: m_pDevice(pDevice)
	, m_pContext(pContext)
{
	Safe_AddRef(m_pDevice);
	Safe_AddRef(m_pContext);
}

HRESULT CMaterial::Initialize(aiMaterial* pAIMaterial, const _char* pModelFilePath)
{
	_char szDrive[MAX_PATH] = {};
	_char szDir[MAX_PATH] = {};

	// (1) 모델 파일 경로에서 드라이브 + 디렉토리 추출
	_splitpath_s(pModelFilePath, szDrive, MAX_PATH, szDir, MAX_PATH, nullptr, 0, nullptr, 0);

	// (2) 모든 텍스처 타입을 순회 (Diffuse, Normal, Specular, ...)
	for (size_t i = 0; i < AI_TEXTURE_TYPE_MAX; i++)
	{
		_uint iNumTextures = pAIMaterial->GetTextureCount(static_cast<aiTextureType>(i));

		for (size_t j = 0; j < iNumTextures; j++)
		{
			aiString	strTexturePath;
			_char		szFileName[MAX_PATH] = {};
			_char		szExt[MAX_PATH] = {};

			// (3) aiMaterial에서 텍스처 상대 경로 추출
			if (FAILED(pAIMaterial->GetTexture(static_cast<aiTextureType>(i), j, &strTexturePath)))
				return E_FAIL;

			// (4) 텍스처 파일명 + 확장자 추출
			_splitpath_s(strTexturePath.data, nullptr, 0, nullptr, 0, szFileName, MAX_PATH, szExt, MAX_PATH);

			// (5) 모델 폴더 기준 전체 경로 조합 : Drive + Dir + FileName + Ext
			_char szFullPath[MAX_PATH] = {};
			strcpy_s(szFullPath, szDrive);
			strcat_s(szFullPath, szDir);
			strcat_s(szFullPath, szFileName);
			strcat_s(szFullPath, szExt);

			// (6) char -> wchar_t 변환 (DirectXTK API는 wchar_t 기반)
			_tchar szFinalPath[MAX_PATH] = {};
			MultiByteToWideChar(CP_ACP, 0, szFullPath, strlen(szFullPath), szFinalPath, MAX_PATH);

			// (7) 확장자별 로딩 분기
			HRESULT hr = {};
			ID3D11ShaderResourceView* pSRV = { nullptr };

			if (false == strcmp(szExt, ".dds"))
				hr = CreateDDSTextureFromFile(m_pDevice, szFinalPath, nullptr, &pSRV);
			else if (false == strcmp(szExt, ".tga"))
				hr = E_FAIL;
			else
				hr = CreateWICTextureFromFile(m_pDevice, szFinalPath, nullptr, &pSRV);
			
			if (FAILED(hr))
				return E_FAIL;

			// (8) 해당 타입의 SRV 벡터에 추가
			m_Materials[i].push_back(pSRV);
		}
	}

	return S_OK;
}

HRESULT CMaterial::Bind_ShaderResource(CShader* pShader, const _char* pConstantName, aiTextureType eType, _uint iIndex)
{
	if (eType >= AI_TEXTURE_TYPE_MAX ||
		iIndex >= m_Materials[eType].size())
		return E_FAIL;

	return pShader->Bind_SRV(pConstantName, m_Materials[eType][iIndex]);
}

CMaterial* CMaterial::Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext, aiMaterial* pAIMaterial, const _char* pModelFilePath)
{
	CMaterial* pInstance = new CMaterial(pDevice, pContext);

	if (FAILED(pInstance->Initialize(pAIMaterial, pModelFilePath)))
	{
		MSG_BOX("Failed to Created : CMaterial");
		Safe_Release(pInstance);
	}

	return pInstance;
}

void CMaterial::Free()
{
	__super::Free();

	for (auto& SRVs : m_Materials)
	{
		for (auto& pSRV : SRVs)
			Safe_Release(pSRV);
		SRVs.clear();
	}

	Safe_Release(m_pContext);
	Safe_Release(m_pDevice);
}
