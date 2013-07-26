// NetReaderDlg.h : header file
//

#if !defined(AFX_NETREADERDLG_H__8A79B5BE_E61F_454A_B108_65FEE6585888__INCLUDED_)
#define AFX_NETREADERDLG_H__8A79B5BE_E61F_454A_B108_65FEE6585888__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

class CAcquireTransport;

/////////////////////////////////////////////////////////////////////////////
// CNetReaderDlg dialog

class CNetReaderDlg : public CDialog
{
	struct AcqBasicInfo
	{
		DWORD	dwSize;			// Size of structure, used for version control
		int		nEegChan;		// Number of EEG channels
		int		nEvtChan;		// Number of event channels
		int		nBlockPnts;		// Samples in block
		int		nRate;			// Sampling rate (in Hz)
		int		nDataSize;		// 2 for "short", 4 for "int" type of data
		float	fResolution;	// Resolution for LSB
		//
		AcqBasicInfo()  { Init(); }
		void Init() 	{ memset((void*)&dwSize,0,sizeof(AcqBasicInfo));}
		BOOL Validate() { return (nBlockPnts && nRate && nEegChan && nDataSize); }
	};
	enum { DataMode,ImpedanceMode };

	CAcquireTransport	*m_pTransport;
	volatile BOOL		m_bThreadActive;		// TRUE while acquisition thread is running
	volatile BOOL		m_bAcquisitionActive;	// TRUE while acquisition thread is running
	HANDLE				m_hThread;				// Handler to acquisition thread
	AcqBasicInfo		m_BasicInfo;
	int					m_nTotalPnts;
	int					m_nBlockPnts;
	int					m_nAllChan;
	int					m_nScaleFactor;
	int					*m_pRawData;			// Array with EEG data
	CRect				m_rectSignal;			// Area to draw signals
	BOOL				m_bConnected;			// TRUE. if server acquire EEG data
	BOOL				m_bSaving;				// TRUE, while data is stored in file
	BOOL				m_bImpedance;			// TRUE, while impedance measurement
	CFont				m_ImpedFont;			// FIXED font for impedance			
	CFont				m_EventFont;			// Vertical font for events
	CFile				m_File;					//
	DWORD				m_dwSaved;				//

	void				ShowInstructions();
	void				ShowWaitingText();
	void				ShowImpedance();
	void				EnableControls  (BOOL bConnect);
	void				EnableSaveButton(BOOL bEnable);
	BOOL				AcquisitionStart(int mode);
	BOOL				AcquisitionStop();
	void				DrawEeg(int first=-1,int cnt=-1);
	void				SetClipping(CDC* pDC,BOOL state);
	void				ShowStatus(LPCSTR str);
inline int				LimitAsShort(int value);

// Construction
public:
	CNetReaderDlg(CWnd* pParent = NULL);	// standard constructor
	~CNetReaderDlg();
	void	AcquisitionLoop();

// Dialog Data
	//{{AFX_DATA(CNetReaderDlg)
	enum { IDD = IDD_NETREADER_DIALOG };
	CSpinButtonCtrl	m_DurSpin;
	CSpinButtonCtrl	m_ScaleSpin;
	CString	m_strServerName;
	int		m_nServerPort;
	int		m_nDuration;
	int		m_nScale;
	//}}AFX_DATA

	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CNetReaderDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	HICON m_hIcon;

	// Generated message map functions
	//{{AFX_MSG(CNetReaderDlg)
	virtual BOOL OnInitDialog();
	afx_msg void OnPaint();
	afx_msg void OnStart();
	afx_msg void OnSave();
	afx_msg void OnClose();
	afx_msg void OnDeltaposScaleSpin(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnDeltaposDurSpin(NMHDR* pNMHDR, LRESULT* pResult);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
	afx_msg LRESULT OnNetAcquireCode(WPARAM,LPARAM);
	afx_msg LRESULT OnNetAcquireMsg (WPARAM,LPARAM);
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_NETREADERDLG_H__8A79B5BE_E61F_454A_B108_65FEE6585888__INCLUDED_)
