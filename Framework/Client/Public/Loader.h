#pragma once

#include "Client_Defines.h"
#include "Base.h"

NS_BEGIN(Client)

class CLoader final : public CBase
{
private:
	CLoader(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
	virtual ~CLoader() = default;

public:
	HRESULT					Initialize(LEVEL eNextLevelID);
	HRESULT					Loading();

private:
	ID3D11Device*			m_pDevice = { nullptr };
	ID3D11DeviceContext*	m_pContext = { nullptr };
	LEVEL					m_eNextLevelID = { LEVEL::END };

	HANDLE					m_hThread = { };
	CRITICAL_SECTION		m_CriticalSection = { };

public:
	static CLoader* Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext, LEVEL eNextLevelID);
	virtual void Free() override;
};

NS_END
