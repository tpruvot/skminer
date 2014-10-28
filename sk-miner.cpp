/*
 * Copyright 2010 Jeff Garzik
 * Copyright 2012-2014 pooler
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.  See COPYING for more details.
 */

#include "cpuminer-config.h"
#define _GNU_SOURCE

#include "core.h"

// in core.cpp

//#include "hash/templates.h"
//#include "hash/CBlock.h"
//#include "hash/Miner.h"
//#include "hash/MinerThread.h"
//
//char *device_name[8]; // CB
//namespace Core
//{
//
//	/** Class to hold the basic data a Miner will use to build a Block.
//	Used to allow one Connection for any amount of threads. **/
//
//
//	/** Class to handle all the Connections via Mining LLP.
//	Independent of Mining Threads for Higher Efficiency. **/
//	class ServerConnection
//	{
//	public:
//		LLP::Miner  *CLIENT;
//		int nThreads, nTimeout;
//		std::vector<MinerThread*> THREADS;
//		LLP::Thread_t THREAD;
//		LLP::Timer    TIMER;
//		std::string   IP, PORT;
//
//
//		ServerConnection(std::string ip, std::string port, int nMaxThreads, int nMaxTimeout) : IP(ip), PORT(port), TIMER(), nThreads(nMaxThreads), nTimeout(nMaxTimeout), THREAD(boost::bind(&ServerConnection::ServerThread, this))
//		{
//			for (int nIndex = 0; nIndex < nThreads; nIndex++)
//				THREADS.push_back(new MinerThread(nIndex));
//		}
//
//		/** Reset the block on each of the Threads. **/
//		void ResetThreads()
//		{
//
//			/** Reset each individual flag to tell threads to stop mining. **/
//			for (unsigned int nIndex = 0; nIndex < THREADS.size(); nIndex++)
//			{
//				THREADS[nIndex]->SetIsBlockFound(false);
//				THREADS[nIndex]->SetIsNewBlock(true);
//				THREADS[nIndex]->SetHashes(0);
//			}
//
//		}
//
//		/** Get the total Primes Found from Each Mining Thread.
//		Then reset their counter. **/
//		unsigned long long Hashes()
//		{
//			unsigned long long nHashes = 0;
//			for (unsigned int nIndex = 0; nIndex < THREADS.size(); nIndex++)
//			{
//				nHashes += THREADS[nIndex]->GetHashes();
//				THREADS[nIndex]->SetHashes(0);
//			}
//
//			return nHashes;
//		}
//
//
//
//		/** Main Connection Thread. Handles all the networking to allow
//		Mining threads the most performance. **/
//		void ServerThread()
//		{
//
//			/** Don't begin until all mining threads are Created. **/
//			while (THREADS.size() != nThreads)
//				Sleep(1000);
//
//
//			/** Initialize the Server Connection. **/
//			CLIENT = new LLP::Miner(IP, PORT);
//
//
//			/** Initialize a Timer for the Hash Meter. **/
//			TIMER.Start();
//			unsigned int nTimerWait = 2;
//
//			unsigned int nBestHeight = 0;
//			loop
//			{
//				try
//				{
//					/** Run this thread at 1 Cycle per Second. **/
//					Sleep(50);
//
//
//					/** Attempt with best efforts to keep the Connection Alive. **/
//					if (!CLIENT->Connected() || CLIENT->Errors())
//					{
//						ResetThreads();
//
//						if (!CLIENT->Connect())
//							continue;
//						else
//							CLIENT->SetChannel(2);
//					}
//
//
//					/** Check the Block Height. **/
//					unsigned int nHeight = CLIENT->GetHeight(5);
//					if (nHeight == 0)
//					{
//						printf("[MASTER] Failed to Update Height\n");
//						CLIENT->Disconnect();
//						continue;
//					}
//
//					/** If there is a new block, Flag the Threads to Stop Mining. **/
//					if (nHeight != nBestHeight)
//					{
//						nBestHeight = nHeight;
//						printf("[MASTER] Coinshield Network: New Block %u\n", nHeight);
//
//						ResetThreads();
//					}
//
//					/** Rudimentary Meter **/
//					if (TIMER.Elapsed() > nTimerWait)
//					{
//						double  Elapsed = (double)TIMER.ElapsedMilliseconds();
//						unsigned long long nHashes = Hashes();
//						//double khash = ((double)nHashes)/Elapsed;
//						unsigned long long khash = nHashes / TIMER.ElapsedMilliseconds();
//						unsigned int nDifficulty = THREADS[0]->GetBlock()->GetBits();
//						nDifficulty = nDifficulty & 0xffffff;
//						CBigNum target;
//						target.SetCompact(nDifficulty);
//
//						//double diff = (double)(0xFFFF * pow(2, 208)) / (double)nDifficulty;
//						printf("[METERS] %llu kHash/s | Height = %u | Diff= %.08f\n", khash, nBestHeight, 1.0 / (double)nDifficulty);
//
//						TIMER.Reset();
//						if (nTimerWait == 2)
//							nTimerWait = 20;
//					}
//
//
//					/** Check if there is work to do for each Miner Thread. **/
//					for (unsigned int nIndex = 0; nIndex < THREADS.size(); nIndex++)
//					{
//
//						/** Attempt to get a new block from the Server if Thread needs One. **/
//
//						if (THREADS[nIndex]->GetIsNewBlock())
//						{
//							/** Retrieve new block from Server. **/
//							/** Delete the Block Pointer if it Exists. **/
//							CBlock* pBlock = THREADS[nIndex]->GetBlock();
//							if (pBlock != NULL)
//							{
//								delete(pBlock);
//							}
//
//							/** Retrieve new block from Server. **/
//							pBlock = CLIENT->GetBlock(5);
//
//
//							/** If the block is good, tell the Mining Thread its okay to Mine. **/
//							if (pBlock)
//							{
//								THREADS[nIndex]->SetIsBlockFound(false);
//								THREADS[nIndex]->SetIsNewBlock(false);
//								THREADS[nIndex]->SetBlock(pBlock);
//								THREADS[nIndex]->SetHashes(0); // reset hash count
//
//							}
//
//							/** If the Block didn't come in properly, Reconnect to the Server. **/
//							else
//							{
//								CLIENT->Disconnect();
//
//								break;
//							}
//
//						}
//
//						/** Submit a block from Mining Thread if Flagged. **/
//						else if (THREADS[nIndex]->GetIsBlockFound())
//						{
//
//							//							
//							/** Attempt to Submit the Block to Network. **/
//							unsigned char RESPONSE = CLIENT->SubmitBlock(THREADS[nIndex]->GetBlock()->GetMerkleRoot(), THREADS[nIndex]->GetBlock()->GetNonce(), 10);
//							double  Elapsed = (double)TIMER.ElapsedMilliseconds();
//							double  nHashes = THREADS[nIndex]->GetHashes();
//							double khash = nHashes / Elapsed;
//							Hashes();
//							TIMER.Reset();
//							if (RESPONSE == 200)
//								printf("[MASTER] Block Found by %s on thread %d                   %.1f kHash/s (accepted) Yay !!!\n", device_name[nIndex], nIndex, khash);
//							else if (RESPONSE == 201)
//							{
//
//								printf("[MASTER] Block Found by %s on thread %d                  %.1f kHash/s (rejected) Booo !!!\n", device_name[nIndex], nIndex, khash);
//								THREADS[nIndex]->SetIsNewBlock(true);
//								THREADS[nIndex]->SetIsBlockFound(false);
//							}
//							else
//							{
//								printf("[MASTER] Failure to Submit Block. Reconnecting...\n");
//								CLIENT->Disconnect();
//							}
//
//							break;
//
//
//						}
//					}
//				}
//
//				catch (std::exception& e)
//				{
//					printf("%s\n", e.what());
//				}
//			}
//
//		};
//	};
//};
//


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <inttypes.h>
#include <unistd.h>
#include <sys/time.h>
#include <time.h>
#include <math.h>
#ifdef WIN32

#include <windows.h>
#else
#include <errno.h>
#include <signal.h>
#include <sys/resource.h>
#if HAVE_SYS_SYSCTL_H
#include <sys/types.h>
#if HAVE_SYS_PARAM_H
#include <sys/param.h>
#endif
#include <sys/sysctl.h>
#endif
#endif
#include <jansson.h>
#include <curl/curl.h>
#include <openssl/sha.h>
#include "compat.h"
#include "miner.h"

#ifdef WIN32
#include <Mmsystem.h>
#pragma comment(lib, "winmm.lib")
#endif

#define PROGRAM_NAME		"skminer"

#include "util_llh.h"

// from heavy.cu
#ifdef __cplusplus
extern "C"
{
#endif
int cuda_num_devices();
void cuda_devicenames();
int cuda_finddevice(char *name);
#ifdef __cplusplus
}
#endif


static bool submit_old = false;
bool use_syslog = false;
static bool opt_background = false;
static bool opt_quiet = false;
static int opt_retries = -1;
static int opt_fail_pause = 30;
int opt_timeout = 270;
static int opt_scantime = 5;
static json_t *opt_config;
static const bool opt_time = true;
//static sha256_algos opt_algo = ALGO_HEAVY;
static int opt_n_threads = 0;
static double opt_difficulty = 1; // CH
int device_map[8] = { 0, 1, 2, 3, 4, 5, 6, 7 }; // CB
static int num_processors;
int tp_coef[8];











#ifndef WIN32
static void signal_handler(int sig)
{
	switch (sig) {
	case SIGHUP:
		//applog(LOG_INFO, "SIGHUP received");
		break;
	case SIGINT:
		//applog(LOG_INFO, "SIGINT received, exiting");
		exit(0);
		break;
	case SIGTERM:
		//applog(LOG_INFO, "SIGTERM received, exiting");
		exit(0);
		break;
	}
}
#endif

#define PROGRAM_VERSION "v0.1"
int main(int argc, char *argv[])
{
	struct thr_info *thr;
	long flags;
//	int i;

#ifdef WIN32
	SYSTEM_INFO sysinfo;
#endif

	 printf("        ***** skMiner for nVidia GPUs by djm34  *****\n");
	 printf("\t             This is version "PROGRAM_VERSION" \n");
	 printf("	based on ccMiner by Christian Buchner and Christian H. 2014 ***\n");
	 printf("                   and on primeminer by Videlicet\n");
	 printf("\t Copyright 2014 djm34\n");
	 printf("\t  BTC donation address: 1NENYmxwZGHsKFmyjTc5WferTn5VTFb7Ze\n");
	

	 if (argc < 3)
	 {
		 printf("Too Few Arguments. The Required Arguments are Ip and Port\n");
		 printf("Default Arguments are Total Threads = CPU Cores and Connection Timeout = 10 Seconds\n");
		 printf("Format for Arguments is 'IP PORT THREADS TIMEOUT'\n");

		 Sleep(10000);

		 return 0;
	 }

	num_processors = cuda_num_devices();
	std::string IP = argv[1];
	std::string PORT = argv[2];
	int nThreads = num_processors;
	if (argc > 3)
		nThreads = boost::lexical_cast<int>(argv[3]);

	int nTimeout = 10;
	if (argc > 4)
		nTimeout = boost::lexical_cast<int>(argv[4]);

	cuda_devicenames();

#ifdef _DEBUG
	nThreads = 1;
#endif

	printf("Initializing Miner %s:%s Threads = %i Timeout = %i\n", IP.c_str(), PORT.c_str(), nThreads, nTimeout);
	Core::ServerConnection MINERS(IP, PORT, nThreads, nTimeout);
	loop{ Sleep(100); }


}
