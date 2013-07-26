// AcquireTransport.h : header file
//
#pragma once

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#pragma comment(lib, "wsock32.lib")	// search for wsock32 lib at compile time

// "CTRL" packet control codes
enum { GeneralControlCode=1, ServerControlCode, ClientControlCode };
// "CTRL" packet requests
enum { RequestForVersion=1, ClosingUp, ServerDisconnected };  
enum { StartAcquisition=1, StopAcquisition, StartImpedance, ChangeSetup, DCCorrection };
enum { RequestEdfHeader=1, RequestAstFile, RequestStartData, RequestStopData, RequestBasicInfo };
// "DATA" packet codes and requests
enum { DataType_InfoBlock=1, DataType_EegData };
enum { InfoType_Version=1, InfoType_EdfHeader, InfoType_BasicInfo };
enum { DataTypeRaw16bit=1, DataTypeRaw32bit };

class CAcqMessage : public CObject
{
public:
    char	m_chId[4];		// ID string, no trailing '\0'
    WORD	m_wCode;		// Code
    WORD	m_wRequest;		// Request
    DWORD	m_dwSize;		// Body size (in bytes)
	LPVOID	m_pBody;		// Ptr to body data
	// Implementation
	int		GetHeaderSize() { return 12; }	// Header portion includes m_chId,m_wCode,m_wRequest,m_dwSize
	BOOL	IsCtrlPacket () { return (!memcmp(m_chId,"CTRL",4));}
	BOOL	IsDataPacket () { return (!memcmp(m_chId,"DATA",4));}

// Construction
public:
	CAcqMessage()  { memset(m_chId,0,16);}
	CAcqMessage(char* id,WORD code,WORD request,DWORD size)
		{ memcpy(m_chId,id,4);m_wCode=code;m_wRequest=request;m_dwSize=size;m_pBody=NULL;}
	~CAcqMessage() { if (m_pBody) delete m_pBody; m_pBody = NULL; }
	void	Convert(BOOL bSending)
	{
		if (bSending) {
			// Convert from host byte order to network  byte order (Little-Endian to Big-Endian)
			m_wCode=htons(m_wCode);m_wRequest=htons(m_wRequest);m_dwSize=htonl(m_dwSize); 
		} else {
			// Convert from network byte order to host byte order (Big-Endian to Little-Endian)
			m_wCode=ntohs(m_wCode);m_wRequest=ntohs(m_wRequest);m_dwSize=ntohl(m_dwSize);
		}
	}
};

/////////////////////////////////////////////////////////////////////////////
// CAcquireTransport window

class CAcquireTransport : public CWnd
{
	class CAcqSocket : public CSocket
	{
	public:
		// The only customized procedure
		void SendEx(CAcqMessage *message,LPVOID lpBuf);
	};

// Construction
public:
	CAcquireTransport(LPCSTR szServer,int nPort);
	virtual ~CAcquireTransport();
// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CAcquireTransport)
	public:
	//}}AFX_VIRTUAL

// Implementation
public:
	// External interface
	BOOL	Start();
	BOOL	Stop ();
	BOOL	GetData	(void *msg,long size);
	BOOL	SendCommand	(WORD type,WORD subtype=0); 
	void	ReceiveData();
	BOOL	GetStatMessage(CString& str);

	// Generated message map functions
protected:
	//{{AFX_MSG(CAcquireTransport)
		// NOTE - the ClassWizard will add and remove member functions here.
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
//	afx_msg LRESULT OnNetAcquireMsg(WPARAM wParam,LPARAM lParam);
private:
	int					m_nPort;							// TCP port
	CString				m_strServer;						// Acquire server IP-address
	CString				m_strStatMsg;						// String with statistic information
	BOOL				m_bStatMsgChanged;					// TRUE, if statistic msg was changed
	CAcqSocket*			m_pSocket;							// Communication socket that sends/receives messages
	CObList*			m_pQueue;							// Queue for data packets
	CCriticalSection	m_cs;								// Locker for queue access
	BOOL				m_bClientMode;						// TRUE in client mode
	BOOL				m_bOtherSideDown;					// TRUE, if other side shutdown first
	DWORD				m_dwRecvTotal;						// Total received data
	DWORD				m_dwRecvLastSec;					// Received during last second
	DWORD				m_dwSecondStart;					// Start of second
	DWORD				m_dwTotalStart;						// Time of transmission start
	volatile BOOL		m_bThreadActive;					// TRUE while acquisition thread is running
	volatile BOOL		m_bTransportActive;					// TRUE while acquisition thread is running
	HANDLE				m_hThread;							// Handler to receiving thread
	// Implementation
	void				InitStatVars();						// 
	void				Disconnect ();						//				
	void				AddMessage(CAcqMessage *pMsg);		//
	CAcqMessage*		RemoveMessage();					//
	BOOL				ProcessCtrlMsg(CAcqMessage *pMsg);	//
	BOOL				ProcessDataMsg(CAcqMessage *pMsg);	//
	void				UpdateStatVars(DWORD dwS,DWORD dwR);//
};
