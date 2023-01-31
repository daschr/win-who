#include <unistd.h>
#include <stdio.h>
#include <stdint.h>

#include <winsock2.h>
#include <windows.h>
#include <netioapi.h>
#include <wtsapi32.h>

#ifdef DEBUG
#define BLAME(X) fprintf(stderr, X)
#define print_last_error_message() _print_last_error_message()
#else
#define BLAME(X)
#define print_last_error_message()
#endif


void _print_last_error_message(void) {
    wchar_t *emsg=NULL;

    if(!FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER|FORMAT_MESSAGE_FROM_SYSTEM, NULL, GetLastError(), 0, (wchar_t *) &emsg, 0, NULL))
        fwprintf(stderr, L"[print_last_error_message] failed getting error message!\n");
    else
        fwprintf(stderr, L"GetLastError(): %ls\n", emsg);
    LocalFree(emsg);
}

void print_client_addr(WTS_CLIENT_ADDRESS *addr){
	switch(addr->AddressFamily){
		case AF_INET:
			for(int i=0;i<4;++i){
				printf("%hhu", addr->Address[2+i]);
				if(i<3) printf(".");
			}
			break;
		case AF_INET6:
			for(int i=0;i<10;++i){
				printf("%hhX%hhX", addr->Address[(i<<1)], addr->Address[(i<<1)|1]);
				if(i<9) printf(":");
			}
			break;
		case AF_IPX:
		case AF_NETBIOS:
		case AF_UNSPEC:
		default:
			printf("unknown");
	}
}

void print_users(HANDLE server_h){
	unsigned long num_sessions=0;
	WTS_SESSION_INFOW *sessions;

	if(!WTSEnumerateSessionsW(server_h, 0, 1, &sessions, &num_sessions)){
		BLAME("Failed at EnumerateSessionsW\n");
		return;
	}

	unsigned long buf_size;
	wchar_t *username;
	WTS_CLIENT_ADDRESS *addr;

	for(unsigned long i=0;i<num_sessions;++i){
		if(!WTSQuerySessionInformationW(server_h, sessions[i].SessionId, WTSUserName, &username, &buf_size)){
			BLAME("Error at WTSQuerySessionInformation(...,WTSUserName,...)\n");
			continue;
		}
	
		if(wcslen(username)==0){
			if(username) WTSFreeMemory(username);
			username=NULL;
			continue;
		}
		
		fwprintf(stdout, L"%.*ls\t", buf_size, username);

		WTSFreeMemory(username);

		if(!WTSQuerySessionInformationW(server_h, sessions[i].SessionId, WTSClientAddress, (LPWSTR *) &addr, &buf_size))
			BLAME("Error at WTSQuerySessionInformation(...,WTSClientAddress,...)\n");
		
		if(buf_size)
			print_client_addr(addr);	
		
		printf("\n");

		if(addr){
			WTSFreeMemory(addr);
			addr=NULL;
		}
	}

	WTSFreeMemory(sessions);
}

int main(int ac, char *as[]){
	HANDLE server_h=WTS_CURRENT_SERVER_HANDLE;

	if(ac>1)
		server_h=WTSOpenServer(as[1]);

	print_users(server_h);
	
	WTSCloseServer(server_h);

	return 0;
}
