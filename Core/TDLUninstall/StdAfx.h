// stdafx.h : include file for standard system include files,
//  or project specific include files that are used frequently, but
//      are changed infrequently
//

#if !defined(AFX_STDAFX_H__0D5A6B24_6591_4AD3_93E3_8C785B97D55C__INCLUDED_)
#define AFX_STDAFX_H__0D5A6B24_6591_4AD3_93E3_8C785B97D55C__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#ifndef WINVER
#define WINVER 0x0600
#endif	// WINVER

#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0600
#endif

#ifndef _WIN32_IE 
#define _WIN32_IE 0x0600
#endif

#define VC_EXTRALEAN		// Exclude rarely-used stuff from Windows headers
#define NO_WARN_MBCS_MFC_DEPRECATION // for VS2015

#pragma warning(disable:4996) // deprecated functions

#include <afxwin.h>         // MFC core and standard components
#include <afxext.h>         // MFC extensions
#include <afxdtctl.h>		// MFC support for Internet Explorer 4 Common Controls
#ifndef _AFX_NO_AFXCMN_SUPPORT
#include <afxcmn.h>			// MFC support for Windows Common Controls
#endif // _AFX_NO_AFXCMN_SUPPORT


//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_STDAFX_H__0D5A6B24_6591_4AD3_93E3_8C785B97D55C__INCLUDED_)
