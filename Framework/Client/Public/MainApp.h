#pragma once

#include "Client_Defines.h"
#include "Base.h"

// 전방 선언 하는 클래스가 어디에 있는지 명시해줌
NS_BEGIN(Engine)
class CGameInstance;
NS_END

NS_BEGIN(Client)

class CMainApp final : public CBase
{
private:
	CMainApp();
	virtual ~CMainApp() = default;

public:
	HRESULT Initialize();
	void	Update(_float fTimeDelta);
	HRESULT Render();

private:
	ID3D11Device* m_pDevice = { nullptr };
	ID3D11DeviceContext* m_pContext = { nullptr };
	CGameInstance* m_pGameInstance = { nullptr };


	
public:
	static CMainApp* Create();
	virtual void Free() override;
};

NS_END
