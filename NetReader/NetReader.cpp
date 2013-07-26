// NetReader.cpp : Defines the class behaviors for the application.
//

#include "stdafx.h"
#include "NetReader.h"
#include "NetReaderDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CNetReaderApp
BEGIN_MESSAGE_MAP(CNetReaderApp, CWinApp)
	//{{AFX_MSG_MAP(CNetReaderApp)
	//}}AFX_MSG
	ON_COMMAND(ID_HELP, CWinApp::OnHelp)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CNetReaderApp construction
CNetReaderApp::CNetReaderApp()
{
}

/////////////////////////////////////////////////////////////////////////////
// The one and only CNetReaderApp object
CNetReaderApp theApp;

/////////////////////////////////////////////////////////////////////////////
// CNetReaderApp initialization

BOOL CNetReaderApp::InitInstance()
{
	// Initialize socket libraries
	::AfxSocketInit();

#ifdef _AFXDLL
	Enable3dControls();			// Call this when using MFC in a shared DLL
#else
	Enable3dControlsStatic();	// Call this when linking to MFC statically
#endif

	CNetReaderDlg dlg;
	m_pMainWnd = &dlg;
	dlg.DoModal();
	return false;
}
