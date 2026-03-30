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
	virtual HRESULT			Initialize_Prototype(const _char* pModelFilePath);
	virtual HRESULT			Initialize(void* pArg);

public:
	HRESULT					Render();				// 모든 메시를 순회하며 렌더링

private:
	// 파일로부터 읽어낸 모든 정보를 담고 있음
	const aiScene*			m_pAIScene = { nullptr };
	Importer				m_Importer = {};

	size_t					m_iNumMeshes = {};
	vector<class CMesh*>	m_Meshes;

private:
	HRESULT					Ready_Meshes();

public:
	static CModel*			Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext, const _char* pModelFilePath);
	virtual CComponent*		Clone(void* pArg) override;
	virtual void			Free() override;

};

NS_END