
// template_test.h : main header file for the PROJECT_NAME application
//

#pragma once

#ifndef __AFXWIN_H__
	#error "include 'pch.h' before including this file for PCH"
#endif

#define INITGUID

#include "resource.h"		// main symbols
#include <cfgmgr32.h>
#include <strsafe.h>
#include <Devpkey.h>
#include <Guiddef.h>
DEFINE_GUID(GUID_DEVINTERFACE_WINDOWSPERF,
	0xf8047fdd, 0x7083, 0x4c2e, 0x90, 0xef, 0xc0, 0xc7, 0x3f, 0x10, 0x45, 0xfd);


// CtemplatetestApp:
// See template_test.cpp for the implementation of this class
//

class CtemplatetestApp : public CWinApp
{
public:
	CtemplatetestApp();

// Overrides
public:
	virtual BOOL InitInstance();

// Implementation

	DECLARE_MESSAGE_MAP()
};

extern CtemplatetestApp theApp;
