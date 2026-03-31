#pragma once

#include "Component.h"

NS_BEGIN(Engine)

class ENGINE_DLL CModel final : public CComponent
{
private:
	CModel(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
	CModel(const CModel& Prototype);
	virtual ~CModel() = default;

public:
	_uint						Get_NumMeshes() const { return m_iNumMeshes; }

public:
	virtual HRESULT				Initialize_Prototype(MODEL eType, const _char* pModelFilePath, _fmatrix PreTransformMatrix);
	virtual HRESULT				Initialize(void* pArg);

public:
	HRESULT						Render(_uint iMeshIndex);				
	HRESULT						Bind_Material(class CShader* pShader, const _char* pConstantName, _uint iMeshIndex, aiTextureType eType, _uint iIndex);

private:
	// 파일로부터 읽어낸 모든 정보를 담고 있음
	const aiScene*				m_pAIScene = { nullptr };
	Importer					m_Importer = {};
	MODEL						m_eType = { MODEL::END };

	size_t						m_iNumMeshes = {};
	vector<class CMesh*>		m_Meshes;

	size_t						m_iNumMaterials = {};
	vector<class CMaterial*>	m_Materials;

private:
	HRESULT						Ready_Meshes(_fmatrix PreTransformMatrix);
	HRESULT						Ready_Materials(const _char* pModelFilePath);

public:
	static CModel*				Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext, MODEL eType, const _char* pModelFilePath, _fmatrix PreTransformMatrix = XMMatrixIdentity());
	virtual CComponent*			Clone(void* pArg) override;
	virtual void				Free() override;

};

NS_END