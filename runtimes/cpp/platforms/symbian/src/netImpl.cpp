/* Copyright (C) 2009 Mobile Sorcery AB

This program is free software; you can redistribute it and/or modify it under
the terms of the GNU General Public License, version 2, as published by
the Free Software Foundation.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
for more details.

You should have received a copy of the GNU General Public License
along with this program; see the file COPYING.  If not, write to the Free
Software Foundation, 59 Temple Place - Suite 330, Boston, MA
02111-1307, USA.
*/

#include <es_enum.h>
#include <CommDbConnPref.h>

#include "config_platform.h"

#include "syscall.h"
#include "socket.h"
#include "AppView.h"
#include "net/net_errors.h"
#include "net/net.h"

using namespace Base;

void Syscall::ClearNetworkingVariables() {
	gConnNextHandle = 1;
	gConnCleanupQue = NULL;
	gNetworkingState = EIdle;
}

void Syscall::ConstructNetworkingL() {
	gConnOps.SetOffset(_FOFF(ConnOp, mLink));
	gConnCleanupQue = new (ELeave) CActiveEnder(_FOFF(ConnOp, mLink));
}

#if defined(SAVE_IAP)

#ifdef __SERIES60_3X__
#define IAP_STORE_PATH "C:\\Data\\MAStore\\iap.txt"
#else  //Series 60, 2nd Ed.
#define IAP_STORE_PATH "C:\\MAStore\\iap.txt"
#endif  //__SERIES60_3X__

#ifdef GUIDO
static int getSavedIap() {
	FileStream readFile(IAP_STORE_PATH);
	if(!readFile.isOpen())
		return 0;
	TBuf8<32> buf;
	int len;
	DEBUG_ASSERT(readFile.length(len));
	buf.SetLength(len);
	DEBUG_ASSERT(readFile.read((void*)buf.Ptr(), len));
	TLex8 lex(buf);
	uint iap;
	DEBUG_ASSERT(!lex.Val(iap));
	return iap;
}
#endif	//GUIDO

static void saveIap(uint iap) {
	MyRFs myrfs;
	myrfs.Connect();
	FSS.MkDir(KMAStorePath16);

	WriteFileStream writeFile(IAP_STORE_PATH);
	DEBUG_ASSERT(writeFile.isOpen());
	TBuf8<32> buf;
	buf.Format(_L8("%i\n"), iap);
	DEBUG_ASSERT(writeFile.write(buf.Ptr(), buf.Size()));
}
#endif	//SAVE_IAP

void Syscall::StartNetworkingL(ConnOp& connOp) {
	LOGST("StartNetworkingL");
	DEBUG_ASSERT(gNetworkingState != EStarted);
	if(gNetworkingState == EStarting) {
		LOGST("SN pending");
		connOp.iStatus = KRequestPending;
		connOp.SetActive();
		gConnOpsWaitingForNetworkingStart.AppendL(&connOp);
		return;
	}

	gNetworkingState = EStarting;
	LHEL(gConnection.Open(gSocketServ));

#if 0//defined(__WINSCW__)
	{
		TUint count;
		LHEL(gConnection.EnumerateConnections(count));
		LOG("%i active connections.\n", count);
		if(count == 0) {
			gConnection.Start(connOp.iStatus);
		}
	}
#else	//hardware
	{
		//TODO: enable user to select. keep default code for Gui-Do.
		TCommDbConnPref pref;
#ifdef GUIDO
		uint iap = 0;
#ifdef SAVE_IAP
		iap = getSavedIap();
#endif
		pref.SetDialogPreference(ECommDbDialogPrefDoNotPrompt);
		pref.SetIapId(iap);	//magic number, uses the default IAP
		DEBUG_ASSERT(pref.IapId() == iap);
#else	//GUIDO
		pref.SetDialogPreference(ECommDbDialogPrefPrompt);
#endif	//GUIDO
		gConnection.Start(pref, connOp.iStatus);
	}
#endif	//__WINSCW__
	connOp.SetActive();
	LOGST("SN active");
}

void Syscall::FinishNetworkingStartL() {
	LOGST("FinishNetworkingStartL");
	TUint count;
	LHEL(gConnection.EnumerateConnections(count));
	LOG("%i active connections.\n", count);
	TConnectionInfo ci;
	TPckg<TConnectionInfo> cib(ci);
	LHEL(gConnection.GetConnectionInfo(1, cib));
	LOG("IAP %i\n", ci.iIapId);
	//TODO: dump IAP name and info
	//name seems extremely hard to get. maybe just save IAP number in an ini file?
	//what if user wants to change IAP? maybe she picked the wrong one.

#ifdef SAVE_IAP
	saveIap(ci.iIapId);
#endif

	gNetworkingState = EStarted;

	for(int i=0; i<gConnOpsWaitingForNetworkingStart.Count(); i++) {
		ConnOp* op = gConnOpsWaitingForNetworkingStart[i];
		TRequestStatus* rsp = &op->iStatus;
		User::RequestComplete(rsp, KErrNone);
	}
	gConnOpsWaitingForNetworkingStart.Close();
}

void Syscall::StopNetworking() {
	//cancel all connOps.
	TDblQueIter<ConnOp> itr(gConnOps);
	itr.SetToFirst();
	ConnOp* op;
	while((op = itr++) != NULL) {
		//delete op;	//causes crash
		op->Cancel();
	}
}

void Syscall::DestructNetworking() {
	StopNetworking();

	gConnOpsWaitingForNetworkingStart.Close();

	//destroy all humans... I mean Connections :)
	gConnections.Close();

	SAFE_DELETE(gConnCleanupQue);

#if SYNCTEST
	delete tSync;
#endif
	gConnection.Close();
}

void Syscall::CancelConnOps(MAHandle conn) {
	//cancel all operations on this connection
	TDblQueIter<ConnOp> itr(gConnOps);
	itr.SetToFirst();
	ConnOp* op;
	while((op = itr++) != NULL) {
		LOGS("CCO 0x%08X\n", op);
		LOGS("CCO check type %i handle %i == %i\n", op->mType, op->mHandle, conn);
		if(op->mHandle == conn)
			op->Cancel();
	}
}

//******************************************************************************
//CSocket
//******************************************************************************

void CSocket::Connect(TSockAddr& aAddr, CPublicActive& op) {
	LOGS("Conn...");
	mSocket.Connect(aAddr, op.iStatus);
	op.SetActive();
	LOGS("A\n");
}
bool CSocket::Write(const TDesC8& aDesc, CPublicActive& op) {
	LOGS("Writ...");
	mSocket.Write(aDesc, op.iStatus);
	op.SetActive();
	LOGS("A\n");
	return true;
}
void CSocket::RecvOneOrMoreL(TDes8& aDes, CPublicActive& op) {
	LOGS("RooM...");
	mSocket.RecvOneOrMore(aDes, 0, op.iStatus, mDummyLength);
	op.SetActive();
	LOGS("A\n");
}

//******************************************************************************
//ConnOp
//******************************************************************************

void Syscall::StartConnOpL(ConnOp* op) {
	//LOG("COP 0x%08X\n", op);
	//LOGS("cops %i\n", gConnOps.IsEmpty());
	gConnOps.AddLast(*op);
	//LOGS("cops %i 0x%08X\n", gConnOps.IsEmpty(), gConnOps.First());
	op->StartConnSubOpL();
}

void Syscall::ConnOp::SendResult(int result) {
	//should send an event to the main loop advising that an operation has been
	//completed and that the Op object should be deleted.
	//since ConnOp must not delete itself due to a design flaw/bug in Symbian.

	//no no no. There IS no main loop!

	//we still have to send a message to someone else.

	//we could have an independent active object containing a message queue
	//in the form of a linked list.
	//since we're planning to have a double-linked list of all active ConnOps
	//(to allow for proper cleanup in the event of an immediate system shutdown)
	//the message queue should also be a double-linked list. that way,
	//finished ConnOps can be easily moved from one to the other.
	//when a message is added to the queue, the active object is activated,
	//causing the message to be handled at a later time, but still before
	//the VM Core gets to run again, since that has Idle-class priority.

	//dump que
	//LOGS("cops %i 0x%08X\n", mSyscall.gConnOps.IsEmpty(), mSyscall.gConnOps.First());
	mSyscall.gConnCleanupQue->move(this);
	//LOGS("cops %i\n", mSyscall.gConnOps.IsEmpty());

	MAEvent event;
	event.type = EVENT_TYPE_CONN;
	event.conn.handle = mHandle;
	switch(mType) {
	case COC_AddrConnect:
	case COC_NameConnect:
		event.conn.opType = CONNOP_CONNECT;
		break;
	case COC_Read:
	case COC_ReadToData:
		event.conn.opType = CONNOP_READ;
		if(mType == COC_ReadToData) {
			CO_ReadToData& rto = (CO_ReadToData&)*this;
			mSyscall.resources.extract_RT_FLUX(rto.handle);
			ROOM(mSyscall.resources.add_RT_BINARY(rto.handle, rto.dst.extract()));
		}
		break;
	case COC_Write:
	case COC_WriteFromData:
		event.conn.opType = CONNOP_WRITE;
		if(mType == COC_WriteFromData) {
			CO_WriteFromData& wfo = (CO_WriteFromData&)*this;
			mSyscall.resources.extract_RT_FLUX(wfo.handle);
			ROOM(mSyscall.resources.add_RT_BINARY(wfo.handle, wfo.src.extract()));
		}
		break;
	case COC_HttpFinish:
		{
			CO_HttpFinish& hf = (CO_HttpFinish&)*this;
			event.conn.opType = hf.connect ? CONNOP_CONNECT : CONNOP_FINISH;
		}
		break;
	default:
		DEBIG_PHAT_ERROR;
	}
	event.conn.result = result;
	LOGS("Reporting o%i h%i r%i\n",
		event.conn.opType, event.conn.handle, event.conn.result);
	DEBUG_ASSERT(event.conn.result != 0);
	DEBUG_ASSERT(mSyscall.gAppView.AddEvent(event));

	DEBUG_ASSERT((mConn.state & event.conn.opType) == event.conn.opType);
	mConn.state &= ~event.conn.opType;
}

void Syscall::ConnOp::RunL() {
	LOGST("ConnOp::RunL begin");
	int result = iStatus.Int();
	if(result < 0 || mConn.connErr != 0) {
		LOG("Symbian ConnErr %i\n", result);
		ConnSubOp& sop = mSubOps.First();
		LOGS("SOC %i @ 0x%08X\n", sop.type, (uint)&sop);
		switch(sop.type) {
		case CSOC_StartNetworking:
			SendResult(CONNERR_NETWORK);
			return;
		case CSOC_Resolve:
			SendResult(CONNERR_DNS);
			return;
		case CSOC_SockConnect:
		case CSOC_Read:
		case CSOC_Write:
		case CSOC_HttpReadHeaders:
			if(mConn.connErr == 0) { switch(result) {
			case KErrDisconnected:
			case KErrEof:
			case KErrHCILinkDisconnection:
			case KErrL2PeerDisconnected:
			case KErrRfcommMuxRemoteDisconnect:
				SendResult(CONNERR_CLOSED);
				return;
			case KErrNotFound:
				SendResult(CONNERR_DNS);
				return;
			default:
				SendResult(CONNERR_GENERIC);
				return;
			} } else {
				SendResult(mConn.connErr);
				return;
			}
			break;
		default:
			LOG("Unhandled SubConnOp %i\n", sop.type);
			DEBIG_PHAT_ERROR;
		}
	}

	FifoQue<ConnSubOp>& subOps = mSubOps;
	if(!subOps.IsEmpty()) {
		ConnSubOp& sop = mSubOps.First();
		switch(sop.type) {
		case CSOC_StartNetworking:
			if(mSyscall.gNetworkingState == EStarting)
				mSyscall.FinishNetworkingStartL();
			break;
		}

		subOps.Pop();
	}
	
	if(!subOps.IsEmpty()) {
		StartConnSubOpL();
	} else {
		LOGS("OC %i\n", mType);
		switch(mType) {
		case COC_AddrConnect:
		case COC_NameConnect:
			SendResult(1);
			break;
		case COC_Read: {
			CO_Read& cr = (CO_Read&)*this;
			SendResult(cr.des.Size());
			break;
		}
		case COC_Write:
			SendResult(1);
			break;
		case COC_ReadToData: {
			CO_ReadToData& rto = (CO_ReadToData&)*this;
			SendResult(rto.des.Size());
			break;
		}
		case COC_WriteFromData: {
			SendResult(1);
			break;
		}
		case COC_HttpFinish: {
			CO_HttpFinish& hf = (CO_HttpFinish&)*this;
			SendResult(hf.http.GetResponseCode());
			break;
		}
		default:
			LOG("Unhandled ConnOp %i\n", mType);
			DEBIG_PHAT_ERROR;
		}
	}
	LOGST("ConnOp::RunL end");
}

void Syscall::ConnOp::DoCancel() {
	LOGS("Cancel %i\n", mType);
	ConnSubOp& sop = mSubOps.First();
	LOGS("SOC %i @ 0x%08X\n", sop.type, (uint)&sop);
	switch(sop.type) {
	case CSOC_StartNetworking:
		LHEL(mSyscall.gConnection.Stop());
		break;
	case CSOC_Resolve: {
		CSO_Resolve& r((CSO_Resolve&)sop);
		r.resolver.Cancel();
		break;
	}
	case CSOC_SockConnect:
	case CSOC_Read:
	case CSOC_Write:
	case CSOC_HttpReadHeaders:
		mConn.CancelAll();
		break;
	default:
		LOG("Unhandled SubConnOp %i\n", sop.type);
		DEBIG_PHAT_ERROR;
	}
	SendResult(CONNERR_CANCELED);
}

void Syscall::ConnOp::StartConnSubOpL() {
	LOGST("SCSO begin");
	ConnSubOp& sop = mSubOps.First();
	LOGS("SOC %i\n", sop.type);
	switch(sop.type) {
	case CSOC_StartNetworking: {
		mSyscall.StartNetworkingL(*this);
		break;
	}
	case CSOC_Resolve: {
		CSO_Resolve& r = (CSO_Resolve&)sop;
#ifdef __WINSCW__
		LHEL(r.resolver.Open(mSyscall.gSocketServ, KAfInet, KProtocolInetUdp));
#else
		LHEL(r.resolver.Open(mSyscall.gSocketServ, KAfInet, KProtocolInetUdp,
			mSyscall.gConnection));
#endif
		r.resolver.GetByName(r.hostname, r.result, iStatus);
		SetActive();
		break;
	}
	case CSOC_SockConnect: {
		CSO_SockConnect& c = (CSO_SockConnect&)sop;
		c.addr.SetPort(c.port);
		c.sock.Connect(c.addr, *this);
		break;
	}
	case CSOC_Write: {
		CSO_Write& w = (CSO_Write&)sop;
		if(!mConn.Write(w.desc, *this))
			SendResult(CONNERR_READONLY);
		break;
	}
	case CSOC_Read: {
		CSO_Read& r = (CSO_Read&)sop;
		mConn.RecvOneOrMoreL(r.des, *this);
		break;
	}
	case CSOC_HttpReadHeaders: {
		CSO_HttpReadHeaders& hrh = (CSO_HttpReadHeaders&)sop;
		hrh.http.ReadHeadersL(*this);
		break;
	}
	default:
		LOG("Unhandled SubConnOp %i\n", sop.type);
		DEBIG_PHAT_ERROR;
	}
	LOGST("SCSO end");
}

//******************************************************************************
//ConnOp structors
//******************************************************************************

//Prototype and Initializers
#define CO_CONSTRUCTOR_PI(name) \
	Syscall::CO_##name* Syscall::CO_##name::NewL(bool aStartNetworking,\
	Syscall& s, MAHandle h, CConnection& c,	FS_NULA2(CV_##name, DECLARE_ARG)) {\
	CO_##name* self = new (ELeave) CO_##name(s, h, c, FS_NULA2(CV_##name, REFERENCE_ARG));\
	CleanupStack::PushL(self);\
	self->ConstructL(aStartNetworking);\
	CleanupStack::Pop(self);\
	return self; }\
	void Syscall::CO_##name::ConstructL(bool aStartNetworking)

#define CO_DESTRUCTOR_P(name) Syscall::CO_##name::~CO_##name()

#define CSO_ADD(type, args...) { CSO_##type* op = new (ELeave) CSO_##type(args);\
	mSubOps.Push(op); }

CO_CONSTRUCTOR_PI(NameConnect) {
	if(aStartNetworking) {
		CSO_ADD(StartNetworking);
	}
	CSO_ADD(Resolve, *hostname, nameEntry);
	CSO_ADD(SockConnect, nameEntry().iAddr, port, sock);
}
CO_DESTRUCTOR_P(NameConnect) {
	LOGS("~NameConnect\n");
}

CO_CONSTRUCTOR_PI(AddrConnect) {
	if(aStartNetworking) {
		CSO_ADD(StartNetworking);
	}
	CSO_ADD(SockConnect, addr, port, sock);
}
CO_DESTRUCTOR_P(AddrConnect) {
	LOGS("~AddrConnect\n");
}

CO_CONSTRUCTOR_PI(Read) {
	DEBUG_ASSERT(!aStartNetworking);
	CSO_ADD(Read, des);
}
CO_DESTRUCTOR_P(Read) {
	LOGS("~Read\n");
}

CO_CONSTRUCTOR_PI(Write) {
	if(mConn.http()) if(mConn.http()->mState == CHttpConnection::SETUP) {
		if(aStartNetworking) {
			CSO_ADD(StartNetworking);
		}
		addHttpSendHeaders();
	}
	CSO_ADD(Write, desc);
}
CO_DESTRUCTOR_P(Write) {
	LOGS("~Write\n");
}

CO_CONSTRUCTOR_PI(ReadToData) {
	DEBUG_ASSERT(!aStartNetworking);
	des.Set((byte*)dst->ptr() + offset, 0, size);
	CSO_ADD(Read, des);
}
CO_DESTRUCTOR_P(ReadToData) {
	LOGS("~ReadToData\n");
}

CO_CONSTRUCTOR_PI(WriteFromData) {
	if(mConn.http()) if(mConn.http()->mState == CHttpConnection::SETUP) {
		if(aStartNetworking) {
			CSO_ADD(StartNetworking);
		}
		addHttpSendHeaders();
	}
	if(src->ptrc() == NULL) {
		buffer = new MemStream(size);
		MYASSERT(buffer != NULL, ERR_OOM);
		MYASSERT(src->readFully(*buffer), ERR_DATA_ACCESS_FAILED);
		desc.Set((byte*)buffer->ptrc() + offset, size);
	} else {
		desc.Set((byte*)src->ptrc() + offset, size);
	}
	CSO_ADD(Write, desc);
}
CO_DESTRUCTOR_P(WriteFromData) {
	LOGS("~WriteFromData\n");
}

CO_CONSTRUCTOR_PI(HttpFinish) {
	if(aStartNetworking) {
		CSO_ADD(StartNetworking);
	}
	if(http.mState == CHttpConnection::SETUP) {
		addHttpSendHeaders();
	}
	CSO_ADD(HttpReadHeaders, http);
}
CO_DESTRUCTOR_P(HttpFinish) {
	LOGS("~HttpFinish\n");
}

void Syscall::ConnOp::addHttpSendHeaders() {
	CHttpConnection* http = mConn.http();
	DEBUG_ASSERT(http->mState == CHttpConnection::SETUP);
	CSO_ADD(Resolve, *http->mHostname, http->mNameEntry);
	CSO_ADD(SockConnect, http->mNameEntry().iAddr, http->mPort, *http);

	//prepare headers
	http->FormatRequestL();
	//send headers
	http->mBufPtr.Set(CBufFlatPtr(http->mBuffer()));
	CSO_ADD(Write, http->mBufPtr);
	
	http->mState = CHttpConnection::WRITING;
}

//******************************************************************************
//CHttpConnection
//******************************************************************************

void CHttpConnection::ConstructL(const TDesC8& hostname, const TDesC8& path) {
	mRequestHeaders.OpenL();
	mResponseHeaders.OpenL();
	mPath = HBufC8::NewL(path.Length());
	*mPath = path;
	mHostname = CreateHBufC16FromDesC8L(hostname);
	mRequestHeaders.InsertL(_L8("host"), hostname);
}

CHttpConnection::~CHttpConnection() {
	LOGST("~CHttpConnection");
	mRequestHeaders.Close();
	mResponseHeaders.Close();
}

void CHttpConnection::FormatRequestL() {
	DEBUG_ASSERT(mBuffer == NULL);
	mBuffer = CBufFlat::NewL(1024);
#define APPEND(desc) mBuffer->InsertL(mBuffer->Size(), desc)

	//start with the Request Line
	switch(mMethod) {
	case HTTP_GET:
		APPEND(_L8("GET "));
		break;
	case HTTP_POST:
		APPEND(_L8("POST "));
		break;
	case HTTP_HEAD:
		APPEND(_L8("HEAD "));
		break;
	default:
		BIG_PHAT_ERROR(ERR_HTTP_METHOD_INVALID);
	}
	
	APPEND(*mPath);
	APPEND(_L8(" HTTP/1.0\r\n"));

	//add Headers
	String2Map::TIteratorC itr = mRequestHeaders.Begin();
	while(itr.HasMore()) {
		String2Map::Pair p = itr.Next();
		APPEND(p.key);
		APPEND(_L8(": "));
		APPEND(p.value);
		APPEND(_L8("\r\n"));
	}

	APPEND(_L8("\r\n"));	//and finally, the Empty Line
}

//******************************************************************************
//ReadHeadersL
//******************************************************************************

void CHttpConnection::ReadHeadersL(CPublicActive& op) {
	//mBuffer->Reset();
	LOGD("%i\n", mBuffer->Size());
	mBufPtr.Set(CBufFlatPtr(mBuffer()));
	mBufPtr.SetLength(0);
	DEBUG_ASSERT(mBufPtr.MaxLength() >= 1024);
	mSyncOp = &op;
	op.iStatus = KRequestPending;
	mLineHandler = &CHttpConnection::StatusLineHandlerL;
	mPos = 0;
	ReadMoreHeadersL();
	op.SetActive();
}

void CHttpConnection::ReadMoreHeadersL() {
	mRecvPtr.Set(mBufPtr.MyTPtr(mPos));
	CSocket::RecvOneOrMoreL(mRecvPtr, mSync);
	//continues in RunL
}

void CHttpConnection::SyncCallbackL(TAny* aPtr, TInt aResult) {
	CHttpConnection* self = (CHttpConnection*)aPtr;
	self->RunL(aResult);
}

void CHttpConnection::RunL(TInt aResult) {
	LOGST("CHttpConnection::RunL");
	DEBUG_ASSERT(mSyncOp != NULL);
	DEBUG_ASSERT(mLineHandler != NULL);
	if(IS_SYMBIAN_ERROR(aResult)) {
		LOG("CONNERR %i\n", aResult);
		CompleteReadHeaders(0, aResult);
		return;
	}
	mBufPtr.SetLength(mPos + mRecvPtr.Length());

	int startPos = mPos;
	//read a line
	//either a CR, an LF, or a CRLF pair will terminate a line.
	while(mPos < mBufPtr.Length()) {
		int oldPos = mPos;
		switch(mBufPtr[mPos]) {
		case '\r':
			if(mBufPtr[mPos+1] == '\n') {
				//we got ourselves a good line
				mPos++;
			}
		case '\n':	//fallthrough is intentional
			mPos++;
			(this->*mLineHandler)(mBufPtr.Mid(startPos, oldPos - startPos));
			if(mLineHandler == NULL)	//someone Completed the op.
				return;
			startPos = mPos;
			continue;
		default:
			mPos++;
		}
	}

	//something clever could be done here when we want to support arbitrarily large headers.
	if(mPos >= mBufPtr.MaxLength()) {
		LOG("HTTP header buffer full!\n");
		CompleteReadHeaders(CONNERR_INTERNAL, 0);
	}

	ReadMoreHeadersL();
}

void CHttpConnection::CompleteReadHeaders(int aConnErr, int aResult) {
	connErr = aConnErr;
	TRequestStatus* temp = &mSyncOp->iStatus;
	User::RequestComplete(temp, aResult);
	mSyncOp = NULL;
	mLineHandler = NULL;
}

//Shared with Base.
void CHttpConnection::StatusLineHandlerL(const TDesC8& aLine) {
	LOGS("Handling status line \"%S\"\n", &aLine);
	int responseCode = readProtocolResponseCode("HTTP/", CCP aLine.Ptr(), aLine.Length());
	if(responseCode < 0) {
		CompleteReadHeaders(responseCode, 0);
		return;
	}
	mResponseCode = responseCode;
	mLineHandler = &CHttpConnection::HeaderLineHandlerL;
}

int atoiLen(const char* str, int len) {
	TPtrC8 ptrc(CBP str, len);
	TLex8 lex(ptrc);
	int val;
	TEST_LTZ(lex.Val(val));	//on failure, logs error and returns zero.
	return val;
}

void CHttpConnection::HeaderLineHandlerL(const TDesC8& line) {
	LOGS("Handling header line \"%S\"\n", &line);
	if(line.Length() == 0) {
		CompleteReadHeaders(0, KErrNone);
		mState = FINISHED;
		return;
	}
	//format: "key: value"
	//const char* colon = strchr(baseLine, ':');
	int colonIndex = line.Locate(':');
	if(colonIndex != KErrNotFound) if(line[colonIndex + 1] != ' ')
		colonIndex = KErrNotFound;
	if(colonIndex == KErrNotFound) {
		LOG("HTTP bad header line: \"%S\"\n", &line);
		CompleteReadHeaders(CONNERR_PROTOCOL, 0);
		return;
	}
	TPtrC8 key(line.Left(colonIndex));
	TPtrC8 value(line.Mid(colonIndex + 2));
	LOGS("HTTP header %S: %S\n", &key, &value);

	//lower-case
	TCleaner<HBufC8> keyBuf(HBufC8::NewLC(key.Length()));
	keyBuf->Des().CopyLC(key);

	//if the key is already present, comma-combine the values.
	const TDesC8* oldValue = mResponseHeaders.FindL(*keyBuf);
	if(oldValue != NULL) {
		LOGS("Combined!\n");
		//value = itr->second + ", " + value;
		_LIT(KCommaSpace, ", ");
		TCleaner<HBufC8> combBuf(HBufC8::NewLC(oldValue->Length() + KCommaSpace().Length() +
			value.Length()));
		TPtr8 combPtr(combBuf->Des());
		combPtr.Append(*oldValue);
		combPtr.Append(KCommaSpace);
		combPtr.Append(value);
		mResponseHeaders.EraseL(*keyBuf);
		mResponseHeaders.InsertL(*keyBuf, combPtr);
	} else {
		mResponseHeaders.InsertL(*keyBuf, value);
	}
}

//******************************************************************************
//The rest of CHttpConnection
//******************************************************************************

void CHttpConnection::RecvOneOrMoreL(TDes8& aDes, CPublicActive& op) {
	MYASSERT(mState == FINISHED, ERR_HTTP_NOT_FINISHED);
	if(mPos < mRecvPtr.Length()) {	//there's still some data left in the buffer
		int len = MIN(mRecvPtr.Length() - mPos, aDes.MaxLength());
		aDes.Copy(mRecvPtr.Ptr() + mPos, len);
		mPos += len;
		op.SetActive();
		TRequestStatus* temp = &op.iStatus;
		User::RequestComplete(temp, len);
	} else {	//we gotta read from outside
		CSocket::RecvOneOrMoreL(aDes, op);
	}
}

//only allowed on POST sockets in WRITING state
bool CHttpConnection::Write(const TDesC8& aDesc, CPublicActive& op) {
	if(mState != WRITING && mState != FINISHING)
		return false;
	CSocket::Write(aDesc, op);
	return true;
}

void CHttpConnection::SetRequestHeaderL(const TDesC8& key, const TDesC8& value) {
	//lower-case
	TCleaner<HBufC8> buf(HBufC8::NewLC(key.Length()));
	buf->Des().CopyLC(key);

	if(mRequestHeaders.FindL(*buf))
		mRequestHeaders.EraseL(*buf);
	mRequestHeaders.InsertL(*buf, value);
}
const TDesC8* CHttpConnection::GetResponseHeaderL(const TDesC8& key) const {
	//lower-case
	TCleaner<HBufC8> buf(HBufC8::NewLC(key.Length()));
	buf->Des().CopyLC(key);

	return mResponseHeaders.FindL(*buf);
}

//******************************************************************************
//Helpers
//******************************************************************************

bool splitPurl(const TDesC8& parturl, TPtrC8& hostnamePtrC8, int& port, int portMax) {
	int port_m1_index = parturl.Locate(':');
	if(port_m1_index == KErrNotFound) {
		return false;
	}
	TLex8 portLex(parturl.Mid(port_m1_index + 1));

	//port = atoi(port_m1 + 1);
	int result = portLex.Val(port);
	if(result != KErrNone || port <= 0 || port >= portMax) {
		return false;
	}

	hostnamePtrC8.Set(parturl.Left(port_m1_index));
	return true;
}

//returns >0 or CONNERR.
int Syscall::httpCreateConnectionLC(const TDesC8& parturl, CHttpConnection*& conn,
	int method)
{
	int port_m1_index = parturl.Locate(':');
	int path_index = parturl.Locate('/');
	if(path_index == KErrNotFound) {
		return CONNERR_URL;
	}
	if(port_m1_index > path_index) {
		port_m1_index = KErrNotFound;
	}
	TPtrC8 path(parturl.Mid(path_index));
	int hostname_length;
	TUint16 port;
	if(port_m1_index == KErrNotFound) {
		port = 80;
		hostname_length = path_index;
	} else {
		TLex8 portLex(parturl.Mid(port_m1_index + 1, path_index - (port_m1_index + 1)));
		hostname_length = port_m1_index;
		if(portLex.Val(port, EDecimal) != KErrNone) {
			return CONNERR_URL;
		}
	}
	TPtrC8 hostname(parturl.Left(hostname_length));
	conn = new (ELeave) CHttpConnection(gSocketServ, method, port);
	CleanupStack::PushL(conn);
	conn->ConstructL(hostname, path);
	return 1;
}

CHttpConnection& Syscall::GetHttp(MAHandle conn) {
	CConnection* cc = gConnections.FindL(conn);
	MYASSERT(cc, ERR_CONN_HANDLE_INVALID);
	CHttpConnection* http = cc->http();
	MYASSERT(http, ERR_CONN_NOT_HTTP);
	return *http;
}

_LIT8(KHttp, "http://");
_LIT8(KSocket, "socket://");
_LIT8(KBtspp, "btspp://");

//******************************************************************************
//Proper syscalls
//******************************************************************************

SYSCALL(MAHandle, maConnect(const char* url)) {
	TPtrC8 urlP(CBP url, SYSCALL_THIS->ValidatedStrLen(url));
	LOGST("Connect %i %s", gConnNextHandle, url);
	if(gConnections.Size() >= CONN_MAX)
		return CONNERR_MAX;

	CConnection* conn;

	if(SSTREQ(urlP, KSocket)) {	//socket
		TPtrC8 parturl = urlP.Mid(KSocket().Length());
		TPtrC8 hostnamePtrC8;
		int port;
		if(!splitPurl(parturl, hostnamePtrC8, port, (1<<16))) {
			return CONNERR_URL;
		}
		Smartie<CSocket> sockp(new (ELeave) CSocket(gSocketServ, CSocket::ETcp));
		
		_LIT8(KLocalhost, "localhost");
		_LIT8(K127, "127.");
		TInetAddr addr;
		bool localhost = false;
		if(hostnamePtrC8 == KLocalhost) {
			localhost = true;
			addr.SetAddress(INET_ADDR(127,0,0,1));
		} else if(hostnamePtrC8.Length() > K127().Length()) {
			if(hostnamePtrC8.Left(K127().Length()) == K127) {
				localhost = true;
				Smartie<HBufC16> hostname(CreateHBufC16FromDesC8LC(hostnamePtrC8));
				addr.Input(*hostname());
			}
		}
		if(localhost) {
			StartConnOpL(CO_AddrConnect::NewL(false, *this, gConnNextHandle, *sockp(),
				addr, port, *sockp()));
		} else {
			Smartie<HBufC16> hostname(CreateHBufC16FromDesC8LC(hostnamePtrC8));
			CleanupStack::Pop(hostname());
			StartConnOpL(CO_NameConnect::NewL(gNetworkingState != EStarted,
				*this, gConnNextHandle, *sockp(), hostname, port, *sockp()));
		}
		conn = sockp.extract();
	} else if(SSTREQ(urlP, KHttp)) {	//http
		TPtrC8 parturl = urlP.Mid(KHttp().Length());
		CHttpConnection* http;
		TLTZ_PASS(httpCreateConnectionLC(parturl, http, HTTP_GET));
		StartConnOpL(CO_HttpFinish::NewL(gNetworkingState != EStarted,
			*this, gConnNextHandle, *http, *http, true));
		http->mState = CHttpConnection::WRITING;
		conn = http;
		CleanupStack::Pop(conn);
	} else if(SSTREQ(urlP, KBtspp)) {	//btspp
		if(!gBtAvailable) {
			return CONNERR_UNAVAILABLE;
		}
		TPtrC8 parturl = urlP.Mid(KBtspp().Length());
		TPtrC8 hostnamePtrC8;
		int port;
		if(!splitPurl(parturl, hostnamePtrC8, port, 31)) {
			return CONNERR_URL;
		}
		TRfcommSockAddr rfcsa;

		TBTDevAddr btaddr;
		for(int i=0; i<BTADDR_LEN; i++) {
			TLex8 btaLex(parturl.Mid(i*2, 2));
			int result = btaLex.Val(btaddr[i], EHex);
			if(result != KErrNone)
				return CONNERR_URL;
		}
		rfcsa.SetBTAddr(btaddr);

		Smartie<CSocket> sockp(new (ELeave) CSocket(gSocketServ, CSocket::ERfcomm));
		StartConnOpL(CO_AddrConnect::NewL(false, *this, gConnNextHandle, *sockp(),
			rfcsa, port, *sockp()));
		conn = sockp.extract();
	} else {	//error
		return CONNERR_URL;
	}
	CleanupStack::PushL(conn);
	gConnections.InsertL(gConnNextHandle, conn);
	CleanupStack::Pop(conn);
	conn->state |= CONNOP_CONNECT;
	return gConnNextHandle++;
}

SYSCALL(void, maConnClose(MAHandle conn)) {
	LOGST("ConnClose %i", conn);
	MYASSERT(gConnections.FindL(conn) != NULL, ERR_CONN_HANDLE_INVALID);
	CancelConnOps(conn);
	gConnections.EraseL(conn);
}

SYSCALL(int, maConnGetAddr(MAHandle conn, MAConnAddr* addr)) {
	if(conn == HANDLE_LOCAL) {
		if(addr->family == CONN_FAMILY_BT) {
			TBTDevAddr btaddr;
			//TPckgBuf<TBTDevAddr> pckg(btaddr);
			
			//old style, might work on Symbian 7.0 and earlier
			//update: doesn't work on 6630.
#if 0//!defined(__SERIES60_3X__)
			RSocket socket;
			socket.Open(gSocketServ, KBTAddrFamily, KSockSeqPacket, KL2CAP);
			TRequestStatus status;
			socket.Ioctl(KHCILocalAddressIoctl, status, &pckg, KSolBtHCI);
			User::WaitForRequest(status);
			LOG("KHCILocalAddressIoctl result: %i\n", status.Int());
			socket.Close();
			if(status != KErrNone) {
				return CONNERR_GENERIC;
			}
#else
#if 0	//doesn't work; bug in Symbian.
			int result = RProperty::Get(KUidSystemCategory,
				KPropertyKeyBluetoothGetLocalDeviceAddress, pckg);
			LOG("KPropertyKeyBluetoothGetLocalDeviceAddress result: %i\n", result);
			if(result != KErrNone) {
				return CONNERR_GENERIC;
			}
#else
			//open a server socket and read its address.
			RSocket socket;
			socket.Open(gSocketServ, KBTAddrFamily, KSockStream, KRFCOMM);
			int channel;
			LHEL_BASE(socket.GetOpt(KRFCOMMGetAvailableServerChannel, KSolBtRFCOMM, channel),
				return CONNERR_GENERIC);
			TBTSockAddr btsockaddr;
			btsockaddr.SetPort(channel);
			LHEL_BASE(socket.Bind(btsockaddr), return CONNERR_GENERIC);
			socket.LocalName(btsockaddr);
			socket.Close();
			btaddr = btsockaddr.BTAddr();
#endif	//0
#endif	//__SERIES60_3X__
			TBuf<20> aBTAddr;
			TBuf8<20> bt8;
			btaddr.GetReadable(aBTAddr, KNullDesC, _L(":"), KNullDesC);
			bt8.Copy(aBTAddr);
			LOG("Readable: %S\n", &bt8);
			
			for(int i=0; i<BTADDR_LEN; i++) {
				addr->bt.addr.a[i] = btaddr[i];
			}
			return 1;
		} else {
			return CONNERR_INTERNAL;
		}
	}
	//todo: implement
	return CONNERR_INTERNAL;
}

SYSCALL(void, maConnRead(MAHandle conn, void* dst, int size)) {
	LOGST("ConnRead %i 0x%08X %i\n", conn, dst, size);
	CConnection* cc = gConnections.FindL(conn);
	MYASSERT(cc, ERR_CONN_HANDLE_INVALID);
	SYSCALL_THIS->ValidateMemRange(dst, size);
	MYASSERT((cc->state & CONNOP_READ) == 0, ERR_CONN_ALREADY_READING);
	cc->state |= CONNOP_READ;
	StartConnOpL(CO_Read::NewL(false, *this, conn, *cc, TPtr8((byte*)dst, size)));
}

SYSCALL(void, maConnWrite(MAHandle conn, const void* src, int size)) {
	LOGST("ConnWrite %i 0x%08X %i\n", conn, src, size);
	CConnection* cc = gConnections.FindL(conn);
	MYASSERT(cc, ERR_CONN_HANDLE_INVALID);
	SYSCALL_THIS->ValidateMemRange(src, size);
	MYASSERT((cc->state & CONNOP_WRITE) == 0, ERR_CONN_ALREADY_WRITING);
	cc->state |= CONNOP_WRITE;
	StartConnOpL(CO_Write::NewL(gNetworkingState != EStarted, *this, conn, *cc,
		TPtrC8(CBP src, size)));
}

SYSCALL(void, maConnReadToData(MAHandle conn, MAHandle data, int offset, int size)) {
	LOGST("ConnReadToData %i %i %i %i", conn, data, offset, size);
	MYASSERT(offset >= 0, ERR_DATA_OOB);
	MYASSERT(size > 0, ERR_DATA_OOB);
	MYASSERT(offset + size > 0, ERR_DATA_OOB);

	CConnection* cc = gConnections.FindL(conn);
	MYASSERT(cc, ERR_CONN_HANDLE_INVALID);
	MYASSERT((cc->state & CONNOP_READ) == 0, ERR_CONN_ALREADY_READING);

	Smartie<Stream> stream(SYSCALL_THIS->resources.extract_RT_BINARY(data));
	MYASSERT(stream->ptr() != NULL, ERR_DATA_READ_ONLY);
	{
		int sLength;
		MYASSERT(stream->length(sLength), ERR_DATA_ACCESS_FAILED);
		MYASSERT(sLength >= offset + size, ERR_DATA_OOB);
	}
	ROOM(SYSCALL_THIS->resources.add_RT_FLUX(data, NULL));

	cc->state |= CONNOP_READ;
	StartConnOpL(CO_ReadToData::NewL(false, *this, conn, *cc,
		stream, data, offset, size));
}

SYSCALL(void, maConnWriteFromData(MAHandle conn, MAHandle data, int offset, int size)) {
	LOGST("ConnWriteFromData %i %i %i %i", conn, data, offset, size);
	MYASSERT(offset >= 0, ERR_DATA_OOB);
	MYASSERT(size > 0, ERR_DATA_OOB);
	MYASSERT(offset + size > 0, ERR_DATA_OOB);

	CConnection* cc = gConnections.FindL(conn);
	MYASSERT(cc, ERR_CONN_HANDLE_INVALID);
	MYASSERT((cc->state & CONNOP_WRITE) == 0, ERR_CONN_ALREADY_WRITING);

	Smartie<Stream> stream(SYSCALL_THIS->resources.extract_RT_BINARY(data));
	{
		int sLength;
		MYASSERT(stream->length(sLength), ERR_DATA_ACCESS_FAILED);
		MYASSERT(sLength >= offset + size, ERR_DATA_OOB);
	}
	ROOM(SYSCALL_THIS->resources.add_RT_FLUX(data, NULL));

	cc->state |= CONNOP_WRITE;
	StartConnOpL(CO_WriteFromData::NewL(gNetworkingState != EStarted, *this, conn, *cc,
		stream, data, offset, size));
}

SYSCALL(MAHandle, maHttpCreate(const char* url, int method)) {
	TPtrC8 urlP(CBP url, SYSCALL_THIS->ValidatedStrLen(url));
	LOGST("HttpCreate %i %i %s", gConnNextHandle, method, url);
	if(gConnections.Size() >= CONN_MAX)
		return CONNERR_MAX;
	CHttpConnection* conn;
	if(SSTREQ(urlP, KHttp)) {
		TPtrC8 parturl = urlP.Mid(KHttp().Length());
		TLTZ_PASS(httpCreateConnectionLC(parturl, conn, method));
		CleanupStack::Pop(conn);
	} else {
		return CONNERR_URL;
	}
	CleanupStack::PushL(conn);
	gConnections.InsertL(gConnNextHandle, conn);
	CleanupStack::Pop(conn);
	return gConnNextHandle++;
}

SYSCALL(void, maHttpSetRequestHeader(MAHandle conn, const char* key, const char* value)) {
	LOGS("HttpSetRequestHeader %i %s: %s\n", conn, key, value);
	CHttpConnection& http = GetHttp(conn);
	MYASSERT(http.mState == CHttpConnection::SETUP, ERR_HTTP_NOT_SETUP);
	http.SetRequestHeaderL(TPtrC8(CBP key), TPtrC8(CBP value));
}

SYSCALL(int, maHttpGetResponseHeader(MAHandle conn, const char* key, char* buffer,
	int bufSize))
{
	CHttpConnection& http = GetHttp(conn);
	MYASSERT(http.mState == CHttpConnection::FINISHED, ERR_HTTP_NOT_FINISHED);
	SYSCALL_THIS->ValidateMemRange(buffer, bufSize);

	const TDesC8* valueP = http.GetResponseHeaderL(TPtrC8(CBP key));
	if(valueP == NULL)
		return CONNERR_NOHEADER;

	if(bufSize > (int)valueP->Length()) {
		::memcpy(buffer, valueP->Ptr(), valueP->Length());
		buffer[valueP->Length()] = 0;
	}

	return valueP->Length();
}

SYSCALL(void, maHttpFinish(MAHandle conn)) {
	LOGS("HttpFinish %i\n", conn);
	CHttpConnection& http = GetHttp(conn);
	MYASSERT(http.mState == CHttpConnection::SETUP ||
		http.mState == CHttpConnection::WRITING, ERR_HTTP_ALREADY_FINISHED);

	http.state |= CONNOP_FINISH;
	StartConnOpL(CO_HttpFinish::NewL(gNetworkingState != EStarted, *this,
		conn, http, http, false));
	http.mState = CHttpConnection::FINISHING;
}
