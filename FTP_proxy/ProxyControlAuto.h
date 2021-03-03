#pragma once

#include <fsm.h>
#include <fsmsystem.h>
#include "const.h"
#include "../kernel/stdMsgpc16pl16.h"
#include "NetFSM.h"

#define WIN32_LEAN_AND_MEAN

class ProxyControlAuto : public FiniteStateMachine
{
	// for FSM
	StandardMessage StandardMsgCoding;

	MessageInterface *GetMessageInterface(uint32 id);
	void	SetDefaultHeader(uint8 infoCoding);
	void	SetDefaultFSMData();
	void	NoFreeInstances();
	void	Reset();
	uint8	GetMbxId();
	uint8	GetAutomate();
	uint32	GetObject();
	void	ResetData();

	// FSM States
	enum	ProxyControlAutoStates {
		FSM_PROXY_CONTROL_IDLE,
		FSM_PROXY_CONTROL_CONNECTING,
		FSM_PROXY_CONTROL_CONNECTED
	};
	
	

public:
	ProxyControlAuto();
	~ProxyControlAuto();

	//bool FSMMsg_2_NetMsg();
	void NetMsg_2_FSMMsg(const char* apBuffer, uint16 anBufferLength);

	void Initialize();
	void Start();

	void FSMProxyControlAutoSetLissenerAndConnect();
	void FSMProxyControlAutoConnectedToServer();
	void FSMProxyControlAutoConnecting();
	
	// Functions and fields for threads in last state
	static DWORD WINAPI DataFromServerThreadFunction(LPVOID);
	static DWORD WINAPI DataFromClientThreadFunction(LPVOID);
	static DWORD WINAPI TransferDataFromServerThreadFunction(LPVOID);
	static DWORD WINAPI TransferDataFromClientThreadFunction(LPVOID);




	HANDLE DataFromServerThread;
	DWORD DataFromServerThreadID;

	HANDLE DataFromClientThread;
	DWORD DataFromClientThreadID;

	HANDLE TransferDataFromClientThread;
	DWORD TransferDataFromClientThreadID;

	HANDLE TransferDataFromServerThread;
	DWORD TransferDataFromServerThreadID;

	int realTransferPort = 0;
	int fakeTransferPort = 0;
	int drugi_ulaz = 0;


protected:
	static DWORD WINAPI ClientListener(LPVOID);

	
	WSADATA wsaData;
	int iResult;
	uint8 CommandQuitCame = 0;

	SOCKET ListenSocket;
	SOCKET ClientSocket;
	SOCKET ServerSocket;
	SOCKET TransferServerSocket;
	SOCKET TransferClientSocket;


	sockaddr_in listenAdress;
	sockaddr_in clientAdress;
	sockaddr_in serverAdress;
	sockaddr_in transferServerAdress;
	sockaddr_in transferClientAdress;


	char recvbuf[DEFAULT_BUFLEN] = {0};
	int recvbuflen = DEFAULT_BUFLEN;

	char recvbuf2[DEFAULT_BUFLEN] = { 0 };
	int recvbuflen2 = DEFAULT_BUFLEN;

	char recvbuf3[DEFAULT_BUFLEN] = { 0 };
	int recvbuflen3 = DEFAULT_BUFLEN;
	
	char recvbuf4[DEFAULT_BUFLEN] = { 0 };
	int recvbuflen4 = DEFAULT_BUFLEN;

};

