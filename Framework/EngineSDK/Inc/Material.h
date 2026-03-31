#pragma once

#include "Base.h"

NS_BEGIN(Engine)

class CMaterial final : public CBase
{
private:
	CMaterial(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
	virtual ~CMaterial() = default;

public:
	// aiMaterial에서 모든 타입의 텍스처를 로딩
	HRESULT									Initialize(aiMaterial* pAIMaterial, const _char* pModelFilePath);

	// 특정 타입/인덱스의 SRV를 셰이더에 바인딩
	HRESULT									Bind_ShaderResource(class CShader* pShader, const _char* pConstantName, aiTextureType  eType, _uint iIndex);

private:
	ID3D11Device*							m_pDevice = { nullptr };
	ID3D11DeviceContext*					m_pContext = { nullptr };

	// 2차원 배열 [TextureType][Texture Index]
	vector<ID3D11ShaderResourceView*>		m_Materials[AI_TEXTURE_TYPE_MAX];


public:
	static CMaterial*						Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext, aiMaterial* pAIMaterial, const _char* pModelFilePath);
	virtual void							Free() override;
};

NS_END