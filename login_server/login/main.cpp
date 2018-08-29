#include <stdio.h>
#include <signal.h>
#include <string.h>
#include <stdlib.h>
#include <sys/time.h>     //for setrlimit
#include <sys/resource.h> //for setrlimit
#include <unistd.h>       //for setrlimit
#include "Server.h"
#include "Log.h"

const int MAX_NOFILE       = 1024*1024;
const char* CMDSETMAXFILENR  = "echo 1048576 > /proc/sys/fs/file-max";

void sig_int( int nSignal )
{
	LOG_DETAIL(LOG_MAIN, "signal: %d received\n", nSignal);
	CServer::DeleteInstance();
}

void sig_int2( int nSignal )
{
	LOG_DETAIL(LOG_MAIN, "signal: %d received 2\n", nSignal);
}

#define _VER_LGN_		"1.0.0.2"
bool ProcessCommandLine(int argc, char* argv[])
{  
	if (1 >= argc) return false;
	if (!strcmp(argv[1], "-v"))
	{
		printf("Version: %s\n", _VER_LGN_);
	}
	return true;
}

int main( int argc, char* argv[] )
{
	printf("BuildTime: %s %s; Version: %s;\n\n", __DATE__, __TIME__, _VER_LGN_);

	if (ProcessCommandLine(argc, argv)) return 0;
	signal( SIGINT,  sig_int );
	signal( SIGHUP,  sig_int );
	signal( SIGALRM, sig_int );
	signal( SIGQUIT, sig_int );
	signal( SIGKILL, sig_int );
	signal( SIGTERM, sig_int );
	struct sigaction sa;
	sa.sa_handler = SIG_IGN;
	sigaction( SIGPIPE, &sa, 0 );

	//set the resource limit on open file number
	struct rlimit rl;
	system( CMDSETMAXFILENR );
	rl.rlim_cur = rl.rlim_max = MAX_NOFILE;//RLIM_INFINITY;
	setrlimit(RLIMIT_NOFILE, &rl);

	CServer::Instance()->Run();
	LOG_DETAIL(LOG_MAIN, "The dvip server exit normally\n");
	CServer::DeleteInstance();
	return 0;
}
