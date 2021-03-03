#include "ProxyControlAuto.h"
#include "const.h"

#define StandardMessageCoding 0x00

ProxyControlAuto::ProxyControlAuto() :FiniteStateMachine(ProxyControlAuto_TYPE_ID, ProxyControlAuto_MBX_ID, 0, 3, 2)
{
}
ProxyControlAuto::~ProxyControlAuto()
{
}
uint8 ProxyControlAuto::GetAutomate() {
	return ProxyControlAuto_TYPE_ID;
}
uint8 ProxyControlAuto::GetMbxId() {
	return ProxyControlAuto_MBX_ID;
}
uint32 ProxyControlAuto::GetObject() {
	return GetObjectId();
}
MessageInterface *ProxyControlAuto::GetMessageInterface(uint32 id) {
	return &StandardMsgCoding;
}
void ProxyControlAuto::SetDefaultHeader(uint8 infoCoding) {
	SetMsgInfoCoding(infoCoding);
	SetMessageFromData();
}
void ProxyControlAuto::SetDefaultFSMData() {
	SetDefaultHeader(StandardMessageCoding);
}
void ProxyControlAuto::NoFreeInstances() {
	printf("[%d] ChAuto::NoFreeInstances()\n", GetObjectId());
}
void ProxyControlAuto::Reset() {
	printf("[%d] ChAuto::Reset()\n", GetObjectId());
}
void ProxyControlAuto::Initialize() {
	printf("Control::Init \n");

	SetState(FSM_PROXY_CONTROL_IDLE);

	InitEventProc(FSM_PROXY_CONTROL_IDLE, MSG_Set_Lissener_And_Connect, (PROC_FUN_PTR)&ProxyControlAuto::FSMProxyControlAutoSetLissenerAndConnect);
	InitEventProc(FSM_PROXY_CONTROL_CONNECTING, MSG_Proxy_Control_Auto_Connecting, (PROC_FUN_PTR)&ProxyControlAuto::FSMProxyControlAutoConnecting);
	InitEventProc(FSM_PROXY_CONTROL_CONNECTED, MSG_Connected_To_Server, (PROC_FUN_PTR)&ProxyControlAuto::FSMProxyControlAutoConnectedToServer);
}

/*
Prva funkcija posle Start-a

Klijent se konektuje na proksi umesto na server

*/

void ProxyControlAuto::FSMProxyControlAutoSetLissenerAndConnect(){
	printf("Control::Set_Lissener_And_Connect\n");
	int iSendResult = 0;
	int iResult = 0;
	WSAStartup(MAKEWORD(2, 2), &wsaData);

	listenAdress.sin_family = AF_INET;
	listenAdress.sin_port = htons(1650); // port na kojem proksi ceka da se klijent konektuje
	listenAdress.sin_addr.S_un.S_addr = INADDR_ANY; 

	ListenSocket = socket(AF_INET, SOCK_STREAM, 0);

	bind(ListenSocket, (sockaddr*)&listenAdress, sizeof(listenAdress));

	printf("Control::Set_Lissener_And_Connect::binded\n");

	listen(ListenSocket, SOMAXCONN);

	printf("Control::Set_Lissener_And_Connect::Listened\n");

	// Waiting for connection

	int len = sizeof(clientAdress);

	ClientSocket = accept(ListenSocket, (SOCKADDR*)&clientAdress, &len);

	printf("Control::Set_Lissener_And_Connect::Accepted\n");

	SetState(FSM_PROXY_CONTROL_CONNECTING);

	PrepareNewMessage(0x00, MSG_Proxy_Control_Auto_Connecting);
	SetMsgToAutomate(ProxyControlAuto_TYPE_ID);
	SetMsgObjectNumberTo(0);
	SendMessage(ProxyControlAuto_MBX_ID);

}

/*
	Funkcija u kojoj se proksi konektuje na server

*/
void ProxyControlAuto::FSMProxyControlAutoConnecting() {
	printf("Control::FSM_ProxyControlAuto_Connecting\n");

	// Now connecting from Proxy to server

	if ((ServerSocket = socket(AF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET) {
		printf("Failed to create socket.\n %d\n", WSAGetLastError());
	}

	serverAdress.sin_addr.s_addr = inet_addr("127.0.0.1");
	serverAdress.sin_family = AF_INET;
	serverAdress.sin_port = htons(16507);

	if (connect(ServerSocket, (struct sockaddr *)&serverAdress, sizeof(serverAdress)) < 0)
	{
		printf("connect error 1");
		return;
	}

	printf("Control::FSM_ProxyControlAuto_Connecting::Connected to server\n");

	SetState(FSM_PROXY_CONTROL_CONNECTED);

	PrepareNewMessage(0x00, MSG_Connected_To_Server);
	SetMsgToAutomate(ProxyControlAuto_TYPE_ID);
	SetMsgObjectNumberTo(0);
	SendMessage(ProxyControlAuto_MBX_ID);
}



/*
	Pokrecemo 2 thread-a
	1. sluzi za komunikaciju klijent -> proksi
	2. sluzi za komunikaciju proksi  -> server
*/
void ProxyControlAuto::FSMProxyControlAutoConnectedToServer(){
	printf("Control::Connected_To_Server\n");

	DataFromClientThread = CreateThread(NULL, 0, DataFromClientThreadFunction, (LPVOID) this, 0, &DataFromClientThreadID);
	if (DataFromClientThread == NULL)
	{
		printf("Error creating DataFromClientThread.\n");
		exit(1);
	}
	DataFromServerThread = CreateThread(NULL, 0, DataFromServerThreadFunction, (LPVOID) this, 0, &DataFromServerThreadID);
	if (DataFromClientThread == NULL)
	{
		printf("Error creating DataFromServerThread.\n");
		exit(1);
	}
	
}

DWORD ProxyControlAuto::DataFromClientThreadFunction(LPVOID param) {
	printf("Control::DataFromClientThreadFunction\n");

	ProxyControlAuto* ThisAuto = (ProxyControlAuto*)param;
	
	int readSize;
	char temp[10];

	while ((readSize = recv(ThisAuto->ClientSocket, ThisAuto->recvbuf, DEFAULT_BUFLEN, 0)) > 0) {
		ThisAuto->recvbuf[readSize] = 0;

		printf("Message from CLIENT: %s\n", ThisAuto->recvbuf);
		fflush(stdout);

		ThisAuto->iResult = send(ThisAuto->ServerSocket, ThisAuto->recvbuf, (int)strlen(ThisAuto->recvbuf), 0);
		if (ThisAuto->iResult == SOCKET_ERROR)
		{
			printf("send failed 1: %d\n", WSAGetLastError());
			break;
		}

		memset(temp, 0, 4);
		memcpy(temp, ThisAuto->recvbuf, 4);

		if ((strncmp(temp, "QUIT", 4) == 0)) {

			ThisAuto->SetState(FSM_PROXY_CONTROL_IDLE);

			closesocket(ThisAuto->ClientSocket);
			closesocket(ThisAuto->ServerSocket);
			closesocket(ThisAuto->ListenSocket);
			closesocket(ThisAuto->TransferServerSocket);
			closesocket(ThisAuto->TransferClientSocket);

			ThisAuto->PrepareNewMessage(0x00, MSG_Set_Lissener_And_Connect);
			ThisAuto->SetMsgToAutomate(ProxyControlAuto_TYPE_ID);
			ThisAuto->SetMsgObjectNumberTo(0);
			ThisAuto->SendMessage(ProxyControlAuto_MBX_ID);

			break;
		}
	}

	closesocket(ThisAuto->ClientSocket);
	closesocket(ThisAuto->ServerSocket);
	closesocket(ThisAuto->ListenSocket);
	closesocket(ThisAuto->TransferServerSocket);
	closesocket(ThisAuto->TransferClientSocket);



	return 0;
}
DWORD ProxyControlAuto::DataFromServerThreadFunction(LPVOID param) {
	printf("Control::DataFromServerThreadFunction\n");

	ProxyControlAuto* ThisAuto = (ProxyControlAuto*)param;

	int readSize;
	char temp[10];
	while (((readSize = recv(ThisAuto->ServerSocket, ThisAuto->recvbuf2, DEFAULT_BUFLEN, 0)) > 0)) {
		
		if (ThisAuto->CommandQuitCame) {
			break;
		}
		else {
			ThisAuto->recvbuf2[readSize] = 0;

			printf("Message from SERVER: %s\n", ThisAuto->recvbuf2);
			fflush(stdout);

			memset(temp, 0, 4);
			memcpy(temp, ThisAuto->recvbuf2, 4);

			/*
				227 komanda kada server salje ip + port na kojem zeli transfer sesiju, lazira se port da bi ta komunikacija na kraju isla preko proksija
			*/

			if (strncmp(temp, "227 ", 4) == 0) {  
				printf("Sending fake port adress to client\n");

				ThisAuto->recvbuf2[39] = ThisAuto->recvbuf2[39] + 1;

				ThisAuto->realTransferPort = (int(ThisAuto->recvbuf2[37] - '0')) * 256;
				ThisAuto->fakeTransferPort = ThisAuto->realTransferPort + 1;

				printf("PRAVI PORT: %d, LAZNI PORT %d", ThisAuto->realTransferPort, ThisAuto->fakeTransferPort);

				
				// Thread koji ce raditi isto kao prvi, samo ce paziti o podacima
				ThisAuto->TransferDataFromClientThread = CreateThread(NULL, 0, TransferDataFromClientThreadFunction, (LPVOID) ThisAuto, 0, &(ThisAuto->TransferDataFromClientThreadID));
				if (ThisAuto->TransferDataFromClientThread == NULL)
				{
					printf("Error creating TransferDataFromClientThread.\n");
					exit(1);
				}
			}
			if (strncmp(temp, "226 ", 4) == 0){//227 transfer complete, vracamo portove na 0 da bi tred 3 i 4 cekali 
				ThisAuto->realTransferPort = 0;
				ThisAuto->fakeTransferPort = 0;

			}

			ThisAuto->iResult = send(ThisAuto->ClientSocket, ThisAuto->recvbuf2, (int)strlen(ThisAuto->recvbuf2), 0);
			if (ThisAuto->iResult == SOCKET_ERROR)
			{
				printf("send failed 2: %d\n", WSAGetLastError());
				break;
			}

			if ((strncmp(temp, "530 ", 4) == 0)) { // ukoliko se uloguje user koji nema account 

				ThisAuto->SetState(FSM_PROXY_CONTROL_IDLE);

				closesocket(ThisAuto->ClientSocket);
				closesocket(ThisAuto->ServerSocket);
				closesocket(ThisAuto->ListenSocket);
				closesocket(ThisAuto->TransferServerSocket);
				closesocket(ThisAuto->TransferClientSocket);

				ThisAuto->PrepareNewMessage(0x00, MSG_Set_Lissener_And_Connect);
				ThisAuto->SetMsgToAutomate(ProxyControlAuto_TYPE_ID);
				ThisAuto->SetMsgObjectNumberTo(0);
				ThisAuto->SendMessage(ProxyControlAuto_MBX_ID);

				break;
			}



		}
	}

	closesocket(ThisAuto->ServerSocket);

	return 0;
}



DWORD ProxyControlAuto::TransferDataFromClientThreadFunction(LPVOID param) {
	printf("Control::TransferDataFromClientThreadFunction\n");

	ProxyControlAuto* ThisAuto = (ProxyControlAuto*)param;


	while (!ThisAuto->fakeTransferPort);


	ThisAuto->listenAdress.sin_family = AF_INET;
	ThisAuto->listenAdress.sin_port = htons(ThisAuto->fakeTransferPort);
	ThisAuto->listenAdress.sin_addr.S_un.S_addr = INADDR_ANY;

	ThisAuto->ListenSocket = socket(AF_INET, SOCK_STREAM, 0);

	if (bind(ThisAuto->ListenSocket, (sockaddr*)&(ThisAuto->listenAdress), sizeof(ThisAuto->listenAdress)) != 0) {
		printf("NIJE SE BIND %d\n", WSAGetLastError());
	}


	listen(ThisAuto->ListenSocket, SOMAXCONN);

	// Waiting for connection

	int len = sizeof(ThisAuto->transferClientAdress);

	ThisAuto->TransferClientSocket = accept(ThisAuto->ListenSocket, (SOCKADDR*)&(ThisAuto->clientAdress), &len);
		

	int readSize;
	printf("Poceo recv thread 3\n");

	
	// 4. thread koji radi isto kao 2. samo za transfer
	ThisAuto->TransferDataFromServerThread = CreateThread(NULL, 0, ThisAuto->TransferDataFromServerThreadFunction, (LPVOID)ThisAuto, 0, &(ThisAuto->TransferDataFromServerThreadID));
	if (ThisAuto->DataFromClientThread == NULL)
	{
		printf("Error creating TransferDataFromServerThread.\n");
		exit(1);
	}

	printf("Zavrsio se tred 3\n");
	return 0;
}

DWORD ProxyControlAuto::TransferDataFromServerThreadFunction(LPVOID param) {
	printf("Control::TransferDataFromServerThreadFunction\n");



	ProxyControlAuto* ThisAuto = (ProxyControlAuto*)param;

	
	int readSize;

	while (!ThisAuto->realTransferPort);

	if ((ThisAuto->TransferServerSocket = socket(AF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET) {
		printf("Failed to create socket.\n %d\n", WSAGetLastError());
	}

	ThisAuto->transferServerAdress.sin_addr.s_addr = inet_addr("127.0.0.1");
	ThisAuto->transferServerAdress.sin_family = AF_INET;
	ThisAuto->transferServerAdress.sin_port = htons(ThisAuto->realTransferPort);

	if (connect(ThisAuto->TransferServerSocket, (struct sockaddr *)&(ThisAuto->transferServerAdress), sizeof(ThisAuto->transferServerAdress)) < 0)
	{
		printf("connect error 4");
		return 0;
	}
	printf("Poceo recv thread 4\n");
	
	if(ThisAuto->drugi_ulaz == 0)
		Sleep(1000);
	ThisAuto->drugi_ulaz++;
	while ((readSize = recv(ThisAuto->TransferServerSocket, ThisAuto->recvbuf4, DEFAULT_BUFLEN, 0)) > 0) {
		ThisAuto->recvbuf4[readSize] = 0;
		
		printf("Tred 4 salje %s \n", ThisAuto->recvbuf4);

		ThisAuto->iResult = send(ThisAuto->TransferClientSocket, ThisAuto->recvbuf4, (int)strlen(ThisAuto->recvbuf4), 0);
		if (ThisAuto->iResult == SOCKET_ERROR)
		{
			printf("send failed 4: %d\n", WSAGetLastError());
			break;
		}
	}
	closesocket(ThisAuto->TransferServerSocket);
	closesocket(ThisAuto->TransferClientSocket);
	closesocket(ThisAuto->ListenSocket);
	
	printf("Zavrsio se tred 4\n");

	return 0;
}
void ProxyControlAuto::Start(){

	printf("Control::Start\n");
	
	PrepareNewMessage(0x00, MSG_Set_Lissener_And_Connect);
	SetMsgToAutomate(ProxyControlAuto_TYPE_ID);
	SetMsgObjectNumberTo(0);
	SendMessage(ProxyControlAuto_MBX_ID);
}
