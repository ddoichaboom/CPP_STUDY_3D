#pragma once

#include "Client_Defines.h"

class CMainApp final : public CBase
{
private:
	CMainApp();
	virtual ~CMainApp();

public:
	HRESULT Initialize();

	
public:
	static CMainApp* Create();
	virtual void Free() override;
};

