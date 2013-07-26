// AcquireTransport.cpp : implementation file
//

#include "stdafx.h"
#include "AcquireTransport.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

//////////////////////////////////////////////////////////////////////
// Start threads for receiving or sending (for file transfer)
//////////////////////////////////////////////////////////////////////
static DWORD WINAPI RecvThreadEntry(LPVOID arg)
{
	((CAcquireTransport *)arg)->ReceiveData();
    return 0;
}

//////////////////////////////////////////////////////////////////////
// CAcquireTransport
//////////////////////////////////////////////////////////////////////
CAcquireTransport::CAcquireTransport(LPCSTR szServer,int nPort)
{
	m_strServer			= szServer;
	m_nPort				= nPort;
	m_bClientMode		= false;
	m_hThread			= NULL;
	m_pSocket			= NULL;
	m_pQueue			= NULL;
	m_bThreadActive		= false;
	m_bTransportActive	= false;
}

CAcquireTransport::~CAcquireTransport()
{
	Disconnect();
}

BEGIN_MESSAGE_MAP(CAcquireTransport, CWnd)
	//{{AFX_MSG_MAP(CAcquireTransport)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

//////////////////////////////////////////////////////////////////////
// Start ACQUIRE client 
//////////////////////////////////////////////////////////////////////
BOOL CAcquireTransport::Start()
{
	ASSERT (!m_pSocket);

	// Create queue for messages
	m_pQueue  = new CObList;

	// Create communication socket
	m_pSocket = new CAcqSocket;
	if (!(m_pSocket->Create() && m_pSocket->Connect(m_strServer, m_nPort)))
	{
		DELETE_OBJECT(m_pQueue );
		DELETE_OBJECT(m_pSocket);
		CString str;
		str.Format("Failure to connect to server %s. Verify that\n"
				   "ACQUIRE program on that computer is running in\n"
				   "server mode and is listening port %d.\n",m_strServer,m_nPort);
		AfxMessageBox(str,MB_ICONEXCLAMATION);
		return false;
	}

	// Put socket into synchronous mode because the data will received by working thread
	DWORD arg=0;
	m_pSocket->AsyncSelect(0 ); 
	m_pSocket->IOCtl( FIONBIO, &arg );

	// Start receiving thread
    DWORD iID;
	m_hThread =::CreateThread(NULL,0,RecvThreadEntry,(LPVOID)this,0,&iID);

	// Set all varibles
	m_bClientMode	= true;
	m_bOtherSideDown= false;
	InitStatVars();

	// Request for basic info
	SendCommand(ClientControlCode,RequestBasicInfo);

	return true;
}

//////////////////////////////////////////////////////////////////////
// Stop ACQUIRE client 
//////////////////////////////////////////////////////////////////////
BOOL CAcquireTransport::Stop()
{
	if (AfxMessageBox("Do you want to stop Acquire Client?\nPress YES to confirm.\n",MB_YESNO)==IDYES)
	{
		Disconnect();
		m_bClientMode = false;
		return true;
	}
	return false;
}

//////////////////////////////////////////////////////////////////////
// Init statistics variables
//////////////////////////////////////////////////////////////////////
void CAcquireTransport::InitStatVars()
{
	m_dwRecvTotal=m_dwRecvLastSec=0;
	m_dwSecondStart=m_dwTotalStart=GetTickCount();
	m_bStatMsgChanged = false;
	m_strStatMsg.Empty();
}

//////////////////////////////////////////////////////////////////////
// Update statistic variables
//////////////////////////////////////////////////////////////////////
void CAcquireTransport::UpdateStatVars(DWORD dwSend,DWORD dwRecv)
{
	// Update amount of sent and received bytes
	m_dwRecvTotal	+= dwRecv;
	m_dwRecvLastSec += dwRecv;

	// Update status once a second.
	DWORD tick = GetTickCount();
	if (tick-m_dwSecondStart>1000)
	{
		float mcs = 0.000001f;							// micro-second
		float rev_msec = 1.f/(tick-m_dwSecondStart);	// reverse of elapsed time, in milli-seconds
		mcs /= (1.024f*1.024f);	// we will use it to convert bytes to M
		rev_msec /= (1.024f);	// we will use it to convert bytes to K

		// Format statistic message for client side
		m_strStatMsg.Format("Received %.2f M; Speed %.1f K/s",mcs*m_dwRecvTotal,rev_msec*m_dwRecvLastSec);
		m_bStatMsgChanged = true;

		// Reset "last seconds" variables
		m_dwRecvLastSec = 0;
		m_dwSecondStart = tick;
	}
}

BOOL CAcquireTransport::GetStatMessage(CString& str) 
{ 
	if (!m_bStatMsgChanged) return false;
	str = m_strStatMsg; 
	m_bStatMsgChanged = false;
	return true;
}

//////////////////////////////////////////////////////////////////////
// Process "CTRL" packet
//////////////////////////////////////////////////////////////////////
BOOL CAcquireTransport::ProcessCtrlMsg(CAcqMessage *pMsg)
{
	if (pMsg->m_wCode == GeneralControlCode)
	{
		if (pMsg->m_wRequest == ClosingUp)
		{
			m_bTransportActive	= false;
			m_bOtherSideDown	= true;
		}
	}
	else if (pMsg->m_wCode == ServerControlCode)
	{
		if (pMsg->m_wRequest == StartAcquisition) InitStatVars();
		AfxGetMainWnd()->PostMessage(WM_NET_ACQUIRE_CODE,pMsg->m_wCode,pMsg->m_wRequest);
	}
	else TRACE("Control Packet: code=%d,request=%d\n",pMsg->m_wCode,pMsg->m_wRequest);

	// Delete the packet
	DELETE_OBJECT (pMsg);	

	return true;
}

//////////////////////////////////////////////////////////////////////
// Process "DATA" packet
//////////////////////////////////////////////////////////////////////
BOOL CAcquireTransport::ProcessDataMsg(CAcqMessage *pMsg)
{
	if (pMsg->m_wCode == DataType_InfoBlock)
	{
		AfxGetMainWnd()->PostMessage(WM_NET_ACQUIRE_MSG,0,(LPARAM)pMsg);
		pMsg = NULL;
	}
	else if (pMsg->m_wCode == DataType_EegData)
	{
		if (pMsg->m_wRequest == DataTypeRaw16bit)
		{
			AddMessage(pMsg);	// Add to queue
			pMsg = NULL;		// Nullify msg pointer, because it will be deleted outside
		}
	}
	else TRACE("Data Packet: code=%d,request=%d,size %d\n",pMsg->m_wCode,pMsg->m_wRequest,pMsg->m_dwSize);

	// Delete non-processed packet
	DELETE_OBJECT (pMsg);	
	return true;
}

//////////////////////////////////////////////////////////////////////
// Stop receiving thread and delete communication socket
//////////////////////////////////////////////////////////////////////
void CAcquireTransport::Disconnect()
{
	if (m_pSocket)
	{
		SendCommand(ClientControlCode,RequestStopData);
		SendCommand(GeneralControlCode,ClosingUp);
		Sleep(100);
	}
	
	// Wait for thread (~400 mc) to die before close!
	m_bTransportActive = false;
	int cnt=0;
	while(m_bThreadActive && cnt<20) { Sleep(20); cnt++; }

	if(m_hThread)
    {
        CloseHandle(m_hThread);
        m_hThread = NULL;
	} 

	if (m_pQueue)
	{
		while (!m_pQueue->IsEmpty()) m_pQueue->RemoveHead();
    }

	DELETE_OBJECT (m_pSocket);
	DELETE_OBJECT (m_pQueue );
}

//////////////////////////////////////////////////////////////////////
//  Receive data (through working thread)
//////////////////////////////////////////////////////////////////////
void CAcquireTransport::ReceiveData()
{
	#define CLOSED_CONNECTION (res==0 || error==WSAECONNRESET || error==WSAECONNABORTED || error==WSAENOTSOCK)

	int error=0;
	int res=0;
	CAcqMessage* pMsg = 0;

	m_bThreadActive    = true;
    m_bTransportActive = true;

	// While both sockets are open
    while(m_bTransportActive)
	{
		// Create new message
		pMsg = new CAcqMessage;

		// Read the 12-byte header
		if ((res=m_pSocket->Receive( (void*)&pMsg->m_chId,pMsg->GetHeaderSize()))==SOCKET_ERROR)
			error=::GetLastError();

		// If closing down, exit thread
		if (CLOSED_CONNECTION) break;

		// Network byte order to host byte order (Big-Endian -> Little-Endian)
		pMsg->Convert(false);

		// Read the body
		int bodyLen = pMsg->m_dwSize;
		if (!error && bodyLen>0)
		{
			pMsg->m_pBody = new char[bodyLen];
			int total = 0;
			while(total<bodyLen)
			{
				if ((res=m_pSocket->Receive( (char*)pMsg->m_pBody+total, bodyLen-total ))==SOCKET_ERROR)
					error=::GetLastError();

				// If closing down, exit the loop
				if (CLOSED_CONNECTION) break;

				total += res;
			}
			// If closing down, exit thread
			if (CLOSED_CONNECTION) break;
		}

		// Process message
		if (pMsg->IsCtrlPacket()) {	ProcessCtrlMsg(pMsg); pMsg=NULL; } else
		if (pMsg->IsDataPacket()) { ProcessDataMsg(pMsg); pMsg=NULL; } 

		DELETE_OBJECT (pMsg);	// Delete un-processed message		
	}
	DELETE_OBJECT (pMsg);		// Delete un-processed message
	m_bThreadActive=false;		// 
    m_hThread = NULL;			// Nullify handler, because the thread will be destroyed at that point

	if (m_bOtherSideDown)
	{
		AfxGetMainWnd()->PostMessage(WM_NET_ACQUIRE_CODE,(WPARAM)GeneralControlCode,(LPARAM)ServerDisconnected);
	}
}

//////////////////////////////////////////////////////////////////////
// Return data packet
//////////////////////////////////////////////////////////////////////
BOOL CAcquireTransport::GetData(void *msg,long size)
{
	if (!m_pQueue || m_pQueue->GetCount()==0) return false;	// Return immediately, if no data
	
	CAcqMessage* pMsg = RemoveMessage();
	BOOL bResult = false;

	if (pMsg->IsDataPacket())
	{
		if (pMsg->m_dwSize!=(DWORD)size)
		{
			TRACE("CAcquireTransport error, wrong size %d, should be %d\n",pMsg->m_dwSize,size);
		}
		else
		{
			memcpy(msg,pMsg->m_pBody,size);
			bResult = true;
			UpdateStatVars(0,size);
		}
	} 
	DELETE_OBJECT (pMsg);
	return bResult;
}

//////////////////////////////////////////////////////////////////////
// Send Command (control code)
//////////////////////////////////////////////////////////////////////
BOOL CAcquireTransport::SendCommand(WORD command,WORD subtype)
{
	if (!m_pSocket) return false;	// No connection yet
	CAcqMessage message("CTRL",command,subtype,0);
	m_pSocket->SendEx(&message,NULL);
	return true;
}

//////////////////////////////////////////////////////////////////////
// Add Message to message fifo
//////////////////////////////////////////////////////////////////////
void CAcquireTransport::AddMessage(CAcqMessage *pMsg)
{
	CSingleLock slock(&m_cs);
	if (slock.Lock(1000))			// timeout in milliseconds, default= INFINITE
	{
		m_pQueue->AddTail(pMsg);	// add to fifo
	}
}

//////////////////////////////////////////////////////////////////////
// Remove Message
//////////////////////////////////////////////////////////////////////
CAcqMessage* CAcquireTransport::RemoveMessage()
{
	CSingleLock slock(&m_cs);
	if (slock.Lock(1000))			// timeout in milliseconds, default= INFINITE
	{
		if (!m_pQueue->IsEmpty())	// extract from fifo
			return (CAcqMessage*)m_pQueue->RemoveHead();
	}
	return NULL;
}

//////////////////////////////////////////////////////////////////////
// Send message as 2 portions: 1) header; 2) optional body 
//////////////////////////////////////////////////////////////////////
void CAcquireTransport::CAcqSocket::SendEx(CAcqMessage *message,LPVOID lpBuf)
{
	// Precaution for body size
	if (!lpBuf) message->m_dwSize = 0;
	
	// Make a local copy, because function Convert() will change m_dwSize variable
	DWORD dwSize = message->m_dwSize;

	// Convert header to network order
	message->Convert(true);

	// Send 1 portion: 12-byte message header
	CAsyncSocket::Send( message->m_chId,message->GetHeaderSize());

	// Send 2 portion: body of variable length
	if (dwSize) CAsyncSocket::Send(lpBuf,dwSize);
}
