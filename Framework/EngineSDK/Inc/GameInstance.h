#pragma once

#include "Base.h"

NS_BEGIN(Engine)

class ENGINE_DLL CGameInstance final : public CBase
{
	DECLARE_SINGLETON(CGameInstance)

private:
	CGameInstance();
	virtual ~CGameInstance() = default;

public:
	HRESULT Initialize_Engine(const ENGINE_DESC& EngineDesc,
								ID3D11Device** ppDevice,
								ID3D11DeviceContext** ppContext);
	HRESULT Begin_Draw();
	HRESULT Draw();
	HRESULT End_Draw();

private:
	class CGraphic_Device* m_pGraphic_Device = { nullptr };

	

public:
	virtual void Free() override;
};

NS_END