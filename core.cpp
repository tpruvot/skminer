#include "cpuminer-config.h"
#define _GNU_SOURCE

#include "core.h"
#include "hash/templates.h"
#include "hash/CBlock.h"
#include "hash/Miner.h"
#include "hash/MinerThread.h"

char *device_name[8]; // CB
namespace Core
{



	ServerConnection::ServerConnection(std::string ip, std::string port, int nMaxThreads, int nMaxTimeout) 
		: IP(ip), PORT(port), TIMER(), nThreads(nMaxThreads), nTimeout(nMaxTimeout), THREAD(boost::bind(&ServerConnection::ServerThread, this))
	{
		for (int nIndex = 0; nIndex < nThreads; nIndex++)
			THREADS.push_back(new MinerThread(nIndex));
	}

	/** Reset the block on each of the Threads. **/
	void ServerConnection::ResetThreads()
	{

		/** Reset each individual flag to tell threads to stop mining. **/
		for (unsigned int nIndex = 0; nIndex < THREADS.size(); nIndex++)
		{
			THREADS[nIndex]->SetIsBlockFound(false);
			THREADS[nIndex]->SetIsNewBlock(true);
			THREADS[nIndex]->SetHashes(0);
		}

	}

	/** Get the total Primes Found from Each Mining Thread.
	Then reset their counter. **/
	unsigned long long ServerConnection::Hashes()
	{
		unsigned long long nHashes = 0;
		for (unsigned int nIndex = 0; nIndex < THREADS.size(); nIndex++)
		{
			nHashes += THREADS[nIndex]->GetHashes();
			THREADS[nIndex]->SetHashes(0);
		}

		return nHashes;
	}



	/** Main Connection Thread. Handles all the networking to allow
	Mining threads the most performance. **/
	void ServerConnection::ServerThread()
	{

		/** Don't begin until all mining threads are Created. **/
		while (THREADS.size() != nThreads)
			Sleep(1000);


		/** Initialize the Server Connection. **/
		CLIENT = new LLP::Miner(IP, PORT);


		/** Initialize a Timer for the Hash Meter. **/
		TIMER.Start();
		unsigned int nTimerWait = 2;

		unsigned int nBestHeight = 0;
		loop
		{
			try
			{
				/** Run this thread at 1 Cycle per Second. **/
				Sleep(50);


				/** Attempt with best efforts to keep the Connection Alive. **/
				if (!CLIENT->Connected() || CLIENT->Errors())
				{
					ResetThreads();

					if (!CLIENT->Connect())
						continue;
					else
						CLIENT->SetChannel(2);
				}


				/** Check the Block Height. **/
				unsigned int nHeight = CLIENT->GetHeight(5);
				if (nHeight == 0)
				{
					printf("[MASTER] Failed to Update Height\n");
					CLIENT->Disconnect();
					continue;
				}

				/** If there is a new block, Flag the Threads to Stop Mining. **/
				if (nHeight != nBestHeight)
				{
					nBestHeight = nHeight;
					printf("[MASTER] Coinshield Network: New Block %u\n", nHeight);

					ResetThreads();
				}

				/** Rudimentary Meter **/
				if (TIMER.Elapsed() > nTimerWait)
				{
					double  Elapsed = (double)TIMER.ElapsedMilliseconds();
					unsigned long long nHashes = Hashes();
					//double khash = ((double)nHashes)/Elapsed;
					unsigned long long khash = nHashes / TIMER.ElapsedMilliseconds();
					unsigned int nDifficulty = THREADS[0]->GetBlock()->GetBits();
					nDifficulty = nDifficulty & 0xffffff;
					CBigNum target;
					target.SetCompact(nDifficulty);

					//double diff = (double)(0xFFFF * pow(2, 208)) / (double)nDifficulty;
					printf("[METERS] %llu kHash/s | Height = %u | Diff= %.08f\n", khash, nBestHeight, 1.0 / (double)nDifficulty);

					TIMER.Reset();
					if (nTimerWait == 2)
						nTimerWait = 20;
				}


				/** Check if there is work to do for each Miner Thread. **/
				for (unsigned int nIndex = 0; nIndex < THREADS.size(); nIndex++)
				{

					/** Attempt to get a new block from the Server if Thread needs One. **/

					if (THREADS[nIndex]->GetIsNewBlock())
					{
						/** Retrieve new block from Server. **/
						/** Delete the Block Pointer if it Exists. **/
						CBlock* pBlock = THREADS[nIndex]->GetBlock();
						if (pBlock != NULL)
						{
							delete(pBlock);
						}

						/** Retrieve new block from Server. **/
						pBlock = CLIENT->GetBlock(5);


						/** If the block is good, tell the Mining Thread its okay to Mine. **/
						if (pBlock)
						{
							THREADS[nIndex]->SetIsBlockFound(false);
							THREADS[nIndex]->SetIsNewBlock(false);
							THREADS[nIndex]->SetBlock(pBlock);
							THREADS[nIndex]->SetHashes(0); // reset hash count

						}

						/** If the Block didn't come in properly, Reconnect to the Server. **/
						else
						{
							CLIENT->Disconnect();

							break;
						}

					}

					/** Submit a block from Mining Thread if Flagged. **/
					else if (THREADS[nIndex]->GetIsBlockFound())
					{

						//							
						/** Attempt to Submit the Block to Network. **/
						unsigned char RESPONSE = CLIENT->SubmitBlock(THREADS[nIndex]->GetBlock()->GetMerkleRoot(), THREADS[nIndex]->GetBlock()->GetNonce(), 10);
						double  Elapsed = (double)TIMER.ElapsedMilliseconds();
						double  nHashes = THREADS[nIndex]->GetHashes();
						double khash = nHashes / Elapsed;
						Hashes();
						TIMER.Reset();
						if (RESPONSE == 200)
							printf("[MASTER] Block Found by %s on thread %d                   %.1f kHash/s (accepted) Yay !!!\n", device_name[nIndex], nIndex, khash);
						else if (RESPONSE == 201)
						{

							printf("[MASTER] Block Found by %s on thread %d                  %.1f kHash/s (rejected) Booo !!!\n", device_name[nIndex], nIndex, khash);
							THREADS[nIndex]->SetIsNewBlock(true);
							THREADS[nIndex]->SetIsBlockFound(false);
						}
						else
						{
							printf("[MASTER] Failure to Submit Block. Reconnecting...\n");
							CLIENT->Disconnect();
						}

						break;


					}
				}
			}

			catch (std::exception& e)
			{
				printf("%s\n", e.what());
			}
		}

	};
}
