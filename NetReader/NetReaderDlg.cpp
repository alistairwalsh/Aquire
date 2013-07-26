// NetReaderDlg.cpp : implementation file
//

#include "stdafx.h"
#include "NetReader.h"
#include "NetReaderDlg.h"
#include "AcquireTransport.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

static const int cMaxDur   = 10;
static const int cSCALES[] = {1,2,5,10,20,50,100,200,500,1000,2000,5000};

//////////////////////////////////////////////////////////////////////
// Start threads for data acquision, saving, processing and display
//////////////////////////////////////////////////////////////////////
static DWORD WINAPI AcquisitionThread(LPVOID arg)
{
	((CNetReaderDlg*)arg)->AcquisitionLoop();
    return 0;
}

//////////////////////////////////////////////////////////////////////
// CNetReaderDlg dialog
//////////////////////////////////////////////////////////////////////
CNetReaderDlg::CNetReaderDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CNetReaderDlg::IDD, pParent)
{
	m_hThread			= NULL;
	m_pTransport		= NULL;
	m_pRawData			= NULL;
	m_bThreadActive		= false;
    m_bAcquisitionActive= false;
	m_bConnected		= false;
	m_bSaving			= false;
	m_bImpedance		= false;
	m_dwSaved			= 0;
	m_nScaleFactor		= 8;
	m_nAllChan= m_nTotalPnts = m_nBlockPnts = 0;

	//{{AFX_DATA_INIT(CNetReaderDlg)
	m_strServerName = _T("localhost");
	m_nServerPort	= 4000;
	m_nDuration		= 2;
	m_nScale		= 200;
	//}}AFX_DATA_INIT

	// Note that LoadIcon does not require a subsequent DestroyIcon in Win32
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);

	// create vertical font for the events
	m_EventFont.CreateFont(30,0, 900, 900, FW_BOLD, FALSE, FALSE, FALSE, 
							ANSI_CHARSET, OUT_CHARACTER_PRECIS, CLIP_DEFAULT_PRECIS,
							PROOF_QUALITY,VARIABLE_PITCH|TMPF_TRUETYPE,"Arial");
	// horizontal font
	m_ImpedFont.CreateFont(14,0,0,0,FW_DONTCARE,FALSE,FALSE,FALSE,
							ANSI_CHARSET, OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,
							DEFAULT_QUALITY,FIXED_PITCH|TMPF_TRUETYPE,NULL);

}

CNetReaderDlg::~CNetReaderDlg()
{
    m_EventFont.DeleteObject();
	m_ImpedFont.DeleteObject();
	if (m_bSaving) { OnSave(); }
}

void CNetReaderDlg::OnClose() 
{
	AcquisitionStop();
	DELETE_OBJECT(m_pTransport);
	DELETE_ARRAY (m_pRawData);
	CDialog::OnClose();
}

void CNetReaderDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CNetReaderDlg)
	DDX_Control(pDX, IDC_DUR_SPIN, m_DurSpin);
	DDX_Control(pDX, IDC_SCALE_SPIN, m_ScaleSpin);
	DDX_Text(pDX, IDC_SERVER_NAME, m_strServerName);
	DDX_Text(pDX, IDC_SERVER_PORT, m_nServerPort);
	DDV_MinMaxInt(pDX, m_nServerPort, 2000, 10000);
	DDX_Text(pDX, IDC_DURATION, m_nDuration);
	DDV_MinMaxInt(pDX, m_nDuration, 1, 10);
	DDX_Text(pDX, IDC_SCALE, m_nScale);
	DDV_MinMaxInt(pDX, m_nScale, 1, 5000);
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CNetReaderDlg, CDialog)
	//{{AFX_MSG_MAP(CNetReaderDlg)
	ON_WM_PAINT()
	ON_BN_CLICKED(IDC_START, OnStart)
	ON_BN_CLICKED(IDC_SAVE, OnSave)
	ON_WM_CLOSE()
	ON_NOTIFY(UDN_DELTAPOS, IDC_SCALE_SPIN, OnDeltaposScaleSpin)
	ON_NOTIFY(UDN_DELTAPOS, IDC_DUR_SPIN, OnDeltaposDurSpin)
	//}}AFX_MSG_MAP
	ON_MESSAGE(WM_NET_ACQUIRE_CODE,OnNetAcquireCode)
	ON_MESSAGE(WM_NET_ACQUIRE_MSG ,OnNetAcquireMsg )
END_MESSAGE_MAP()

//////////////////////////////////////////////////////////////////////
// CNetReaderDlg message handlers
//////////////////////////////////////////////////////////////////////
BOOL CNetReaderDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	// Set the icon for this dialog.
	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon

	// Get position of instruction area, which will be used to draw EEG signals
	CWnd* pWnd = GetDlgItem(IDC_INSTRUCTION);
	ASSERT (pWnd);
	pWnd->GetClientRect (m_rectSignal); 
	pWnd->ClientToScreen(m_rectSignal);
	ScreenToClient(m_rectSignal);

	// Set spins which control horizontal extent and vertical scale
	m_ScaleSpin.SetRange(1,NEL(cSCALES));
	m_ScaleSpin.SetPos  (m_nScaleFactor);
	m_ScaleSpin.SetRange(1,cMaxDur);
	m_ScaleSpin.SetPos  (m_nDuration);

	// Show Instructions
	ShowInstructions();

	return TRUE;  // return TRUE  unless you set the focus to a control
}

void CNetReaderDlg::OnPaint() 
{
	CWnd* pWnd = GetDlgItem(IDC_INSTRUCTION); 
	if (!pWnd || !pWnd->IsWindowVisible())
	{
		if (m_bConnected) 
		{
			if (!m_bImpedance) DrawEeg ();
			else ShowImpedance();
		}
		else ShowWaitingText();
	}
	CDialog::OnPaint();
}

//////////////////////////////////////////////////////////////////////
// Process requests to start/stop ACQUIRE client
//////////////////////////////////////////////////////////////////////
void CNetReaderDlg::OnStart() 
{
	if (!m_pTransport) 
	{
		// Get and validate parameters, return if any error occurs
		if (!UpdateData(true)) return;		

		// Create transport
		m_pTransport = new CAcquireTransport(m_strServerName,m_nServerPort);
		m_pTransport->m_hWnd = NULL;
		if (!m_pTransport->CreateEx(0, AfxRegisterWndClass(0),_T("Acquire Socket Notification Sink"),
			WS_OVERLAPPED, 0, 0, 0, 0,GetSafeHwnd(), NULL))
		{
			TRACE0("Warning: unable to create acquire socket notify window!\n");
			AfxThrowResourceException();
			DELETE_OBJECT (m_pTransport);
			return;
		}
		ASSERT(m_pTransport->m_hWnd != NULL);

		// Connect to ACQUIRE server
		if (!m_pTransport->Start())
		{
			DELETE_OBJECT (m_pTransport);
			return;
		}

		// Show server name and used port numbe
		CString str;
		str.Format ("Server: %s:%d",m_strServerName,m_nServerPort);
		ShowStatus(str);
	}
	else
	{
		if (m_pTransport && m_pTransport->Stop())
		{
			AcquisitionStop();
			DELETE_OBJECT (m_pTransport);
			ShowStatus ("Client stops");
		}
	}
	EnableControls(m_pTransport!=NULL);
}

//////////////////////////////////////////////////////////////////////
// Toggle storage in to file
//////////////////////////////////////////////////////////////////////
void CNetReaderDlg::OnSave() 
{
	if (!m_bSaving)
	{
		CString str;
		CFileException e;
		CString strFileName = "data.bin";
		if( !m_File.Open( strFileName, CFile::modeCreate | CFile::modeWrite, &e ) )
		{
			str.Format("File %s could not be created %s\n",strFileName,e.m_cause);
			AfxMessageBox(str,MB_ICONEXCLAMATION);
			return;
		}
		m_File.Write((void*)&m_BasicInfo,sizeof(m_BasicInfo));
		m_dwSaved = sizeof(m_BasicInfo);
		m_bSaving = true;	
	}
	else
	{
		m_File.Close();
		m_bSaving = false;	
	}
}

LRESULT CNetReaderDlg::OnNetAcquireCode(WPARAM wParam,LPARAM lParam)
{
	WORD	code	= (WORD)wParam;
	WORD	request = (WORD)lParam;

	if (code == GeneralControlCode)
	{
		switch (request)
		{
		case ServerDisconnected:
			DELETE_OBJECT (m_pTransport); 
			ShowStatus ("Server disconnects");
			EnableControls(m_pTransport!=NULL);
			break;
		}
	}
	else if (code == ServerControlCode)
	{
		LPCSTR str=NULL;
		switch (request)
		{
		case StartAcquisition: 
			if (AcquisitionStart(DataMode)) str = "Start Acquisition"; 
			break;
		case StopAcquisition : 
			if (AcquisitionStop()) str = "Stop Acquisition" ;
			break;
		case StartImpedance  : 
			if (AcquisitionStart(ImpedanceMode)) str = "Start Impedance"; 
			break; 
		case DCCorrection    : str = "DC Correction"    ; break;
		case ChangeSetup	 : str = "Setup was changed"; break;
		}
		ShowStatus(str);
	}
	return 1L;
}

LRESULT CNetReaderDlg::OnNetAcquireMsg(WPARAM wParam,LPARAM lParam)
{
	LRESULT result = false;
	CAcqMessage *pMsg = (CAcqMessage *)lParam;

	if (pMsg->m_wCode == DataType_InfoBlock)
	{
		if (pMsg->m_wRequest == InfoType_BasicInfo)
		{
			AcqBasicInfo* pInfo = (AcqBasicInfo*)pMsg->m_pBody;
			ASSERT (pInfo->dwSize == sizeof(AcqBasicInfo));
			memcpy((void*)&m_BasicInfo,pMsg->m_pBody,sizeof(m_BasicInfo));
			CString status;
			status.Format("EEG:%d+%d;Pnts:%d;%dHz;Bits:%d;Res:%.3fuV/LSB",
				m_BasicInfo.nEegChan,m_BasicInfo.nEvtChan,m_BasicInfo.nBlockPnts,
				m_BasicInfo.nRate,m_BasicInfo.nDataSize*8,m_BasicInfo.fResolution);
			ShowStatus (status);
			result =  true;
		}
	}
	DELETE_OBJECT (pMsg);	// Always delete message
	return result;
}

//////////////////////////////////////////////////////////////////////
// Show instruction, disclaimer, and copyright
//////////////////////////////////////////////////////////////////////
void CNetReaderDlg::ShowInstructions()
{
	CWnd* pWnd = GetDlgItem(IDC_INSTRUCTION); if (!pWnd) return;
	pWnd->SetWindowText
		( 
		"\r\n"
		"\tDESCRIPTION:\r\n\r\n"
		"\tThis sample application (with source code) is provided to illustrate a client\\server concept under SCAN 4.3\r\n"
		"\tincluding: 1) transfer the basic setup information, EEG data, and impedance values through TCP sockets;\r\n"
		"\t2) display of the incoming EEG data signals and events; 3) storing the raw EEG data/events in binary format.\r\n\r\n"
		"\tINSTRUCTIONS:\r\n\r\n"
		"\t1. \tStart ACQUIRE.EXE as a server on local or remote computer.\r\n"
		"\t2. \tIf you use remote computer, enter it's server name, e.g. '12.34.56.78'.\r\n"
		"\t   \tIf you use local computer, leave default 'localhost' name.\r\n"
		"\t3. \tSpecify port, duration, and initial scale.\r\n"
		"\t4. \tClick on 'Connect' button. This program will connect as a client to ACQUIRE server.\r\n"
		"\t5. \tStart acquisition on server side, the signal will appear on both screens.\r\n"
		"\t   \tStatus box will show total bytes transferred and current transfer rate.\r\n"
		"\t6. \tAdjust vertical scale, if necessary.\r\n"
		"\t7. \tEnter events and perform DC corrections on server side, observe they appear on client side.\r\n"
		"\t8. \tClick on 'Save' button to store incoming data in raw binary format.\r\n"
		"\t   \tFile name is 'DATA.BIN' and it is created in working directory of client program.\r\n"
		"\t9. \tStart impedance measurement on server side and see impedance values on client side.\r\n"
		"\t10.\tClick on 'Disconnect' button to stop client operation.\r\n"
		"\t11.\tExit ACQUIRE and this program.\r\n\r\n"
		"\tDISCLAIMER:\r\n"
		"\tThis sample application is provided for informational purposes only and Neuroscan makes no warranties,\r\n"
		"\teither express or implied, in this application. The entire risk of the use of this sample remains with the user.\r\n"
		"\tTechnical support is not available for this sample application and for the provided source code.\r\n"
		"\r\n\r\n\r\n\r\n\r\n\r\n\t©1986-2003 Neuroscan. All rights reserved."
		);
	UpdateData(false);
}

void CNetReaderDlg::ShowStatus(LPCSTR str)
{
	SetDlgItemText(IDC_STATUS,str);
}

void CNetReaderDlg::EnableControls(BOOL bConnect)
{
	CWnd* pWnd;
	pWnd = GetDlgItem(IDC_INSTRUCTION); if (pWnd) pWnd->ShowWindow(bConnect?SW_HIDE:SW_SHOW);
	pWnd = GetDlgItem(IDC_SERVER_NAME); if (pWnd) pWnd->EnableWindow(!bConnect);
	pWnd = GetDlgItem(IDC_SERVER_PORT); if (pWnd) pWnd->EnableWindow(!bConnect);
	pWnd = GetDlgItem(IDC_DURATION	 ); if (pWnd) pWnd->EnableWindow(!bConnect);
	SetDlgItemText(IDC_START,bConnect?"Disconnect":"Connect");
}

void CNetReaderDlg::EnableSaveButton(BOOL bEnable)
{
	CWnd* pWnd = GetDlgItem(IDC_SAVE); 
	if (pWnd) pWnd->EnableWindow(bEnable);
}

BOOL CNetReaderDlg::AcquisitionStart(int mode)
{
	if (!m_pTransport)		  return false;		// No client/server connection
	if (m_bAcquisitionActive)
	{
		if (AcquisitionStop())return false;		// Already running
	}

	if (mode == DataMode)
	{
		m_nAllChan	= m_BasicInfo.nEegChan + m_BasicInfo.nEvtChan;
		m_nBlockPnts= m_BasicInfo.nBlockPnts;
		m_nTotalPnts= m_BasicInfo.nRate*m_nDuration;
		m_bImpedance= false;
	}
	else
	{
		m_nAllChan	= m_BasicInfo.nEegChan;
		m_nBlockPnts= 1;
		m_nTotalPnts= 1;
		m_bImpedance= true;
	}

	// Check parameters
	if (!m_BasicInfo.Validate() || !m_nTotalPnts)
	{
		AfxMessageBox(
			"Serious error with parameters.\nLook at CNetReaderDlg::AcquisitionStart() procedure.\n",
			MB_ICONEXCLAMATION);
		DELETE_OBJECT (m_pTransport);
		EnableControls(false);
		return false;
	}
		
	// Allocate array to store incoming EEG data (for all displayed duration) or impedance values
	DELETE_ARRAY ( m_pRawData);
	m_pRawData	= new int [m_nTotalPnts*m_nAllChan];
	memset ((void*)m_pRawData,0,m_nTotalPnts*m_nAllChan*sizeof(int));

	// Create "suspended" acquisition thread
	DWORD iID;
	m_hThread =::CreateThread(NULL,0,AcquisitionThread,(LPVOID)this,CREATE_SUSPENDED,&iID);

	// Proceed, if the thread was created successfully
	if (m_hThread)
	{	
		// Notify server that client is ready for data
		m_pTransport->SendCommand(ClientControlCode,RequestStartData);

		// Ready to display EEG data and store them in file
		m_bConnected = true;
		if (!m_bImpedance) EnableSaveButton(true);

		// Run the acquisition thread
		ResumeThread(m_hThread);
	}
	else DELETE_ARRAY (m_pRawData);

	return m_hThread!=NULL;
}

BOOL CNetReaderDlg::AcquisitionStop()
{
	if (!m_pTransport)		   return false;	// No client/server connection
	if (!m_bAcquisitionActive) return false;	// Acquisition isn't running

	// Notify server that client is not ready for data
	m_pTransport->SendCommand(ClientControlCode,RequestStopData);

	// Not ready to display EEG data
	m_bConnected = false;
	EnableSaveButton(false);

	// Ask acquisition thread to stop
	m_bAcquisitionActive = false;

	// Wait for thread to exit (~400 mc)
	int cnt=0;
	while(m_bThreadActive && cnt<20) { Sleep(20); cnt++; }

	// Close thread, if it is still alive
	if (m_hThread)
	{
	    if (CloseHandle(m_hThread)) m_hThread = NULL;
	}
	
	// Delete allocated array for EEG data/impedance
	DELETE_ARRAY ( m_pRawData );

	// Display "waiting..." messages
	ShowWaitingText();

	return m_hThread!=NULL;
}

//////////////////////////////////////////////////////////////////////
//  Main procedure to receive, process, and display data
//////////////////////////////////////////////////////////////////////
void CNetReaderDlg::AcquisitionLoop()
{
	int		nSample		= 0;
	long	nBlockSize	= m_nAllChan*m_nBlockPnts*m_BasicInfo.nDataSize;
	char*   pData		= new char[nBlockSize];
	
	m_bThreadActive		= true;
    m_bAcquisitionActive= true;
    while(m_bAcquisitionActive)
	{
		// Get data block
		if (!m_pTransport->GetData(pData,nBlockSize)) 
		{
			Sleep(40);			// No data, sleep 40 msec
		}
		else if (m_pRawData)
		{
			// Store raw data
			if (m_bSaving)
			{
				m_File.Write(pData,nBlockSize);
				m_dwSaved += nBlockSize;
			}

			// Update statistics
			CString str;
			if (m_pTransport->GetStatMessage(str)) 
			{
				if (m_bSaving)
				{
					CString str1;
					str1.Format("; Saved = %.2fM",(float)m_dwSaved/1024/1024);
					str += str1;
				}
				ShowStatus(str);
			}

			// !!! Process data here. !!!
			// This example do nothing except copying in to internal array
			int		*dest = m_pRawData+nSample*m_nAllChan;
			short	*sour = (short*)pData;
			for (int i=0;i<m_nBlockPnts;i++)
			{
				for (int j=0;j<m_nAllChan;j++)
				{
					*dest++ = *sour++;
				}
			}
			// Display data
			if (!m_bImpedance) DrawEeg (nSample,m_nBlockPnts);
			else ShowImpedance();

			// Advance to next sample
			nSample = (nSample+m_nBlockPnts)%m_nTotalPnts;
		}
	}
	m_bThreadActive=false;
    m_hThread = NULL;			// Nullify handler, because the thread will be destroyed at that point

	DELETE_ARRAY ( pData );

	// Close file
	if (m_bSaving) { OnSave(); }
}

//////////////////////////////////////////////////////////////////////
// Draw EEG in specified rectangle (for both offline and online)
//////////////////////////////////////////////////////////////////////
void CNetReaderDlg::DrawEeg(int first,int cnt)
{
	CClientDC dc(this);
	CRect	  rc(m_rectSignal);

	int		i,j;
	CRect	rcUpdate(rc);				// Area to be updated
	BOOL	bOnline = (first!=-1);		// TRUE while acquisition

	int		x0 = rc.left;
	int		y0 = rc.top;
	int		w  = rc.Width ();
	int		h  = rc.Height();
	float	pps= (float)rc.Width()/m_nTotalPnts;	// pixels-per-sample

	// While acquisition this branch will set the actual updated area based on 
	// 1st sample number and number of updated samples
	if (bOnline) 
	{
		int		p1 = (int)(pps*first);			// first pixel
		int		p2 = (int)(pps*(first+cnt));	// last pixel
	
		// Set left and right sides of updated area
		rcUpdate.left  = rc.left+p1;				
		rcUpdate.right = rc.left+p2;
	
		// Expand the start of drawing by one pixel at the left to ensure continuous waveforms
		if (p1>0) {								// Can't do that at the left border
			p1--;								// Expand area by one pixel at the left
			int t = (int)((float)p1/pps);		// Find data sample for that pixel
			cnt += (first-t);					// Recalculate total number of samples to draw
			first = t;							// Replace index for 1st sample
		}
	}

	COLORREF clrFore  = RGB(  0,  0,  0);
	COLORREF clrBack  = RGB(255,255,255);
	COLORREF clrTick  = RGB(128,128,128);
	
	// Change colors while saving data
	if (m_bSaving) { clrBack = RGB(255,255,224); clrFore = RGB(0,0,255); }

	// Create brush and pens
	CBrush	pBrush	(clrBack);
	CPen	pen0	(PS_SOLID,0,clrFore );
	CPen	pen1	(PS_SOLID,0,clrTick );
	CPen	pen2	(PS_DOT  ,0,clrTick );

	CRect clear = rcUpdate;						// Area to clear
	if (bOnline) clear.right += 2;				// Add extra space at the right side while acquisition
	dc.FillRect (clear,&pBrush );				// Fill updated area by back color

	// Setup DC
	int nSavedDC = dc.SaveDC();					// Save DC
	dc.SetBkMode(TRANSPARENT);
	dc.SetMapMode(MM_ANISOTROPIC);
	CPen  *oldp = dc.SelectObject(&pen1);

	// Draw time and vertical ticks
	{
		int height,length;						// Can be any...
		length = 1280;							// ... but 1280x960 provides a nice font proportion
		height =  960;					
		int	nSubTicks	= 5;					// Draw ticks every 200 ms
		int text_y=height;

		// Setup CDC
		dc.SetWindowOrg	  (0,0);
		dc.SetWindowExt	  (length,height);
		dc.SetViewportOrg (rc.TopLeft());
		dc.SetViewportExt (w,h);
		dc.SetTextAlign   (TA_RIGHT | TA_TOP);

		// Calculate spacing between sub-ticks 
		int ticks = (int  )m_nDuration*nSubTicks;	// Overall number of ticks
		float dx  = (float)length /ticks;			// Distance between ticks (in screen units) 
		float dx0 = (float)m_nTotalPnts/ticks;		// Distance between ticks (in samples)

		// Actual drawing
		for (i=0;i<=ticks;i++) 
		{
			// While acquisition draw only time-ticks which overlap with updated area
			if (bOnline) {
				int  x = (int)(dx0*i);
				if (x<first || x>first+cnt) continue;
			}

			// Tick's X-coordinate
			int x = (int)(dx*i);

			// Show tick
			BOOL bSecTick  = !(i%nSubTicks);
			CPen* pen = bSecTick?&pen1:&pen2;
			dc.SelectObject(pen);
			dc.MoveTo (x,0); 
			dc.LineTo (x,bSecTick?height:height-10);	// Ticks for 200 ms are shorter by 10 pixels

			// Show time here ...
		}
	}

	// Restore original window/viewport
	dc.SetWindowOrg	  (0,0);
	dc.SetWindowExt	  (w,h);
	dc.SetViewportOrg (0,0);
	dc.SetViewportExt (w,h);

	// Clip drawing inside signal area
	SetClipping(&dc,true);
	
	// Determine scales
	int		nHeight	= SHRT_MAX;							// Maximum Window Extension under Windows-95/98
	float	fScale  = (float)m_nScale;					// Desirable vertical scale in uV	
	float	fMul = (float)nHeight/fScale;				// Multiplicaion factor which ensure that signal with ...
														// amplitude = fScale will be shown in the portion of ...
														// screen allocated for every channel (nHeight/m_nEegChan)
	int nLength = bOnline?cnt:m_nTotalPnts;

	CPoint*	pPnts = new CPoint[nLength];				// Allocate array of CPoint to display EEG signal
	for (j=0; j<nLength; j++) pPnts[j].x = j + first;	// Precalculate X-coordinate

	dc.SelectObject(&pen0);
	float	dy = (float)rc.Height()/(m_BasicInfo.nEegChan+2);

	float	fMicrovoltsPerLSB = m_BasicInfo.fResolution;
	float	fSamplesPerPixel  = (float) m_nTotalPnts / w;
	fMul = fMul/(m_BasicInfo.nEegChan+2);

	// Show signals from top to bottom
	for (i = 0; i < m_BasicInfo.nEegChan; i++)
	{	
		int *pBuf = m_pRawData+i;						// Pointer to channels data
		if (bOnline) pBuf += first*m_nAllChan;			// During acquisition - jump to 1st updated sample
		
		// Set coordinate system
		dc.SetWindowOrg	 (0,0);
		dc.SetWindowExt	 (m_nTotalPnts,nHeight);
		dc.SetViewportOrg(x0,y0+(int)(dy*(i+1)));
		dc.SetViewportExt(w,h);
	
		// Calculate Y-coordinate
		for (j = 0; j < nLength; j++) 
		{	
			// Get value in uV
			float val = (float)(*pBuf)*fMicrovoltsPerLSB;
			int nVal  = (int)(fMul*val+0.5f);

			//int nVal = (int) (fMul * (*pBuf) + 0.5f);
	
			// Limit as short and store
			pPnts[j].y = LimitAsShort(nVal);

			// Advance to next sample value
			pBuf += m_nAllChan;
		}
		dc.Polyline(pPnts, nLength);
	}
	delete[] pPnts;

	// Restore original window/viewport
	dc.SetWindowOrg	  (0,0);
	dc.SetWindowExt	  (w,h);
	dc.SetViewportOrg (0,0);
	dc.SetViewportExt (w,h);

	// Clip drawing inside signal area
	SetClipping(&dc,true);

	// Show events, if any
	{
		int height,length;								// Can be any...
		length = 1280;									// ... but 1280x960 provides a nice font proportion
		height =  960;					

		// Setup CDC
		dc.SetWindowOrg	  (0,0);
		dc.SetWindowExt	  (length,height);
		dc.SetViewportOrg (rc.TopLeft());
		dc.SetViewportExt (w,h);
		dc.SetTextAlign   (TA_LEFT | TA_BOTTOM);

		COLORREF clrMaroon= RGB(164,128,128);
		COLORREF clrGreen = RGB(  0,164,  0);
		CPen	pen3(PS_SOLID,0,clrGreen);
		CPen	pen4(PS_SOLID,0,clrMaroon);

		int *pEvents = m_pRawData+m_BasicInfo.nEegChan;	// Pointer to events
		if (bOnline) pEvents += first*m_nAllChan;		// During acquisition - jump to 1st updated sample
		CPen*  oldPen  = dc.SelectObject(&pen3); 
	    CFont* oldFont = dc.SelectObject(&m_EventFont);
		int y1 = 0;
		int y2 = height;
		CString str;
		for (i = 0; i < nLength; i++) 
		{
			WORD code = (WORD)(*pEvents);				// Get code
			pEvents += m_nAllChan;						// Advance to next sample value
			if (code==0xFF00 || code==0xFF00) continue;

			WORD evnt = code&0xFF;
			WORD resp = code>>8;
			TRACE ("Code %x at %d: evnt - %x, resp - %x\n",code,first+i,evnt,resp);
			if (evnt)
			{
				dc.SelectObject(&pen3);
				str.Format("Event-0x%X",evnt);
				dc.SetTextColor(clrGreen);
			}
			if (resp!=0xFF)
			{
				dc.SelectObject(&pen4);
				str.Format("Response-0x%X",resp);
				dc.SetTextColor(clrMaroon);
			}
			int x = (int)((float)(first+i)*length/m_nTotalPnts);
			dc.TextOut(x,y2-10,str);
			dc.MoveTo(x,y1);
			dc.LineTo(x,y2);
		}
		dc.SelectObject(oldPen );
		dc.SelectObject(oldFont);
	}

	// Restore CDC
	dc.SelectObject(oldp);
	dc.RestoreDC(nSavedDC);
}

void CNetReaderDlg::ShowWaitingText()
{
	CClientDC dc(this);
	CPoint	center = m_rectSignal.CenterPoint();
	CBrush	pBrush	(RGB(255,255,255));
	dc.FillRect (m_rectSignal,&pBrush );				// Fill updated area by back color
	dc.SetBkMode(TRANSPARENT);
	dc.SetTextAlign(TA_CENTER);
	dc.TextOut(center.x,center.y,"Waiting for server to start acquisition or impedance");
}

// Show impedance by using memory DC to reduce screen flickering
void CNetReaderDlg::ShowImpedance()
{
	if (!m_pRawData) return;

	CClientDC dc(this);
	CRect rc = m_rectSignal;
	
	BOOL	bDoneInMemory = FALSE;
	CDC*	pMemDC = &dc;
	CDC		dcMemory;
	CBitmap	mBitmap, *pbmpOld = NULL;

	// Create compatible DC
	if (dcMemory.CreateCompatibleDC(&dc))
	{
		int w = m_rectSignal.Width();
		int h = m_rectSignal.Height();
		int bitsPerPixel = dc.GetDeviceCaps(BITSPIXEL);
		if (bitsPerPixel > 8) w *= (bitsPerPixel/8);
		if (mBitmap.CreateCompatibleBitmap(&dc, w, h))
		{	
			pbmpOld = dcMemory.SelectObject(&mBitmap);
			pMemDC = &dcMemory;
			bDoneInMemory = TRUE;
			rc = CRect(0,0,w,h);
		}
	}
	pMemDC->IntersectClipRect(rc);

	// Actual painting code
	CBrush	pBrush	(RGB(255,255,255));
	pMemDC->FillRect (rc,&pBrush );					// Fill updated area by back color
	pMemDC->SetBkMode(TRANSPARENT);
	int x = 5;
	int y = 5;
    CFont* oldFont = pMemDC->SelectObject(&m_ImpedFont);
	CSize size = dc.GetTextExtent("Channel 100: 999 kOhms");		// Get extent for sample string
	CString str;
	for (int i=0;i<m_nAllChan;i++)
	{
		int kohms = m_pRawData[i];							// Get value in KOhms
		if (kohms>=1000)									// If impedance >= 1M...
			str.Format("Channel %3d: not-connected",i+1);	// ...show "not-connected"
		else												// Otherwise,,,
			str.Format("Channel %3d: %8d KOhm",i+1,kohms);	// ,,,use actual value
		pMemDC->TextOut(rc.left+x,rc.top+y,str);			// Print the value
		y += size.cy;										// Advance to next line
		if (y+size.cy>m_rectSignal.Height()) {				// If we reach the bottom...
			 x += (size.cx+50); y = 5;						// ...roll to next column
		}
	}
	pMemDC->SelectObject(oldFont);
	// Done painting

	// BitBlt the content from memory to screen
	if (bDoneInMemory)
	{	
		dc.BitBlt(m_rectSignal.left, m_rectSignal.top ,m_rectSignal.Width(), m_rectSignal.Height(), 
			pMemDC, rc.left,rc.top, SRCCOPY);
		dcMemory.SelectObject(pbmpOld);
	}
}

//////////////////////////////////////////////////////////////////////
// Limit 32-bit integer to range of short (-32766:32767) 
//////////////////////////////////////////////////////////////////////
inline int CNetReaderDlg::LimitAsShort(int value)
{
	if (value > SHRT_MAX) return SHRT_MAX; else
	if (value < SHRT_MIN) return SHRT_MIN;
	return value;
}

//////////////////////////////////////////////////////////////////////
// Turn on/off the clipping to signal area
void CNetReaderDlg::SetClipping(CDC* pDC,BOOL bClip)
{
	if (bClip) pDC->IntersectClipRect(&m_rectSignal);	// Set
	else ::SelectClipRgn(pDC->GetSafeHdc(),NULL);		// Remove
}

// Change vertical scale (in uV)
void CNetReaderDlg::OnDeltaposScaleSpin(NMHDR* pNMHDR, LRESULT* pResult) 
{
	NM_UPDOWN* pNMUpDown = (NM_UPDOWN*)pNMHDR;
	
	if(pNMUpDown->iDelta > 0)
	{
		if(m_nScaleFactor < NEL(cSCALES))
		{
			m_nScaleFactor++;
			m_nScale=cSCALES[m_nScaleFactor-1];
			UpdateData(false);
		}
	}
	else
	{
		if (m_nScaleFactor > 1)
		{
			m_nScaleFactor--;
			m_nScale=cSCALES[m_nScaleFactor-1];
			UpdateData(false);
		}
	}
	*pResult = 0;
}

// Change horizontal duration (in secs)
void CNetReaderDlg::OnDeltaposDurSpin(NMHDR* pNMHDR, LRESULT* pResult) 
{
	NM_UPDOWN* pNMUpDown = (NM_UPDOWN*)pNMHDR;

	if(pNMUpDown->iDelta > 0)
	{
		if (m_nDuration > 1)
		{
			m_nDuration--;
			UpdateData(false);
		}
	}
	else
	{
		if(m_nDuration < cMaxDur)
		{
			m_nDuration++;
			UpdateData(false);
		}
	}
	*pResult = 0;
}
