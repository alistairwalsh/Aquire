// stdafx.h : include file for standard system include files,
//  or project specific include files that are used frequently, but
//      are changed infrequently
//

#if !defined(AFX_STDAFX_H__2C7B8051_1C89_46EF_BE8F_F88903C558B6__INCLUDED_)
#define AFX_STDAFX_H__2C7B8051_1C89_46EF_BE8F_F88903C558B6__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#define VC_EXTRALEAN		// Exclude rarely-used stuff from Windows headers

#include <afxwin.h>         // MFC core and standard components
#include <afxext.h>         // MFC extensions
#include <afxdtctl.h>		// MFC support for Internet Explorer 4 Common Controls
#include <afxcmn.h>			// MFC support for Windows Common Controls
#include <afxmt.h>
#include <afxsock.h>

#define DELETE_ARRAY(x) if (x != NULL) {delete[] x; x = NULL;}
#define DELETE_OBJECT(x) if (x != NULL) {delete   x; x = NULL;}
#define NEL(x)  (sizeof(x)/sizeof(x[0]))

#define WM_NET_ACQUIRE_CODE	WM_USER+1
#define WM_NET_ACQUIRE_MSG	WM_USER+2

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_STDAFX_H__2C7B8051_1C89_46EF_BE8F_F88903C558B6__INCLUDED_)
