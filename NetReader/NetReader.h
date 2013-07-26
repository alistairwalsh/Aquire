// NetReader.h : main header file for the NETREADER application
//

#if !defined(AFX_NETREADER_H__A8C5C499_78F2_4A4E_B5D2_E55BACFAEBBE__INCLUDED_)
#define AFX_NETREADER_H__A8C5C499_78F2_4A4E_B5D2_E55BACFAEBBE__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#ifndef __AFXWIN_H__
	#error include 'stdafx.h' before including this file for PCH
#endif

#include "resource.h"		// main symbols

/////////////////////////////////////////////////////////////////////////////
// CNetReaderApp:
// See NetReader.cpp for the implementation of this class
//

class CNetReaderApp : public CWinApp
{
public:
	CNetReaderApp();

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CNetReaderApp)
	public:
	virtual BOOL InitInstance();
	//}}AFX_VIRTUAL

// Implementation

	//{{AFX_MSG(CNetReaderApp)
		// NOTE - the ClassWizard will add and remove member functions here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};


/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_NETREADER_H__A8C5C499_78F2_4A4E_B5D2_E55BACFAEBBE__INCLUDED_)
