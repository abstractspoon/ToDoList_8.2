// TransText.h : main header file for the TRANSTEXT DLL
//

#if !defined(AFX_TRANSTEXT_H__2C486BBE_BE2F_49CC_9B15_5BEEFA27AB39__INCLUDED_)
#define AFX_TRANSTEXT_H__2C486BBE_BE2F_49CC_9B15_5BEEFA27AB39__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#ifndef __AFXWIN_H__
	#error include 'stdafx.h' before including this file for PCH
#endif

#include "resource.h"		// main symbols

/////////////////////////////////////////////////////////////////////////////
// CTransTextApp
// See TransText.cpp for the implementation of this class
//

class CTransTextApp : public CWinApp
{
public:
	CTransTextApp();

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CTransTextApp)
	//}}AFX_VIRTUAL

	//{{AFX_MSG(CTransTextApp)
		// NOTE - the ClassWizard will add and remove member functions here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};


/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_TRANSTEXT_H__2C486BBE_BE2F_49CC_9B15_5BEEFA27AB39__INCLUDED_)
