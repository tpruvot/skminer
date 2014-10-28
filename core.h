#ifndef COINSHIELD_LLP_CORE_H
#define COINSHIELD_LLP_CORE_H


#include "types.h"
#include "bignum.h"
#include "hash/templates.h"
#include "hash/CBlock.h"
#include "hash/Miner.h"
#include "hash/MinerThread.h"


namespace LLP
{
	
	
}



namespace Core
{
	

	class ServerConnection
	{
	public:
		LLP::Miner  *CLIENT;
		int nThreads, nTimeout;
		std::vector<MinerThread*> THREADS;
		LLP::Thread_t THREAD;
		LLP::Timer    TIMER;
		std::string   IP, PORT;

		ServerConnection(std::string ip, std::string port, int nMaxThreads, int nMaxTimeout);

		void ResetThreads();
		unsigned long long Hashes();
		void ServerThread();

	};
}

#endif
