#include <string.h>
#include <openssl/sha.h>
#include <cuda.h>
#include "cuda_runtime.h"
#include "device_launch_parameters.h"
#include <map>

#ifndef _WIN32
#include <unistd.h>
#endif

// include thrust
#include <thrust/version.h>
#include <thrust/remove.h>
#include <thrust/device_vector.h>
#include <thrust/iterator/constant_iterator.h>

#include "miner.h"

extern "C" char *device_name[8];

// nonce-array für die threads
uint32_t *d_nonceVector[8];

/* Combines top 64-bits from each hash into a single hash */
static void combine_hashes(uint32_t *out, const uint32_t *hash1, const uint32_t *hash2, const uint32_t *hash3, const uint32_t *hash4)
{
    const uint32_t *hash[4] = { hash1, hash2, hash3, hash4 };
    int bits;
    unsigned int i;
    uint32_t mask;
    unsigned int k;

    /* Transpose first 64 bits of each hash into out */
    memset(out, 0, 32);
    bits = 0;
    for (i = 7; i >= 6; i--) {
        for (mask = 0x80000000; mask; mask >>= 1) {
            for (k = 0; k < 4; k++) {
                out[(255 - bits)/32] <<= 1;
                if ((hash[k][i] & mask) != 0)
                    out[(255 - bits)/32] |= 1;
                bits++;
            }
        }
    }
}

#ifdef _MSC_VER
#include <intrin.h>
static uint32_t __inline bitsset( uint32_t x )
{
   DWORD r = 0;
   _BitScanReverse(&r, x);
   return r;
}
#else
static uint32_t bitsset( uint32_t x )
{
    return 31-__builtin_clz(x);
}
#endif

// Finde das high bit in einem Multiword-Integer.
static int findhighbit(const uint32_t *ptarget, int words)
{
    int i;
    int highbit = 0;
    for (i=words-1; i >= 0; --i)
    {
        if (ptarget[i] != 0) {
            highbit = i*32 + bitsset(ptarget[i])+1;
                break;
        }
    }
    return highbit;
}

// Generiere ein Multiword-Integer das die Zahl
// (2 << highbit) - 1 repräsentiert.
static void genmask(uint32_t *ptarget, int words, int highbit)
{
    int i;
    for (i=words-1; i >= 0; --i)
    {
        if ((i+1)*32 <= highbit)
            ptarget[i] = 0xffffffff;
        else if (i*32 > highbit)
            ptarget[i] = 0x00000000;
        else
            ptarget[i] = (1 << (highbit-i*32)) - 1;
    }
}

struct check_nonce_for_remove
{    
    check_nonce_for_remove(uint64_t target, uint32_t *hashes, uint32_t hashlen, uint32_t startNonce) :
        m_target(target),
        m_hashes(hashes),
        m_hashlen(hashlen),
        m_startNonce(startNonce) { }

    __device__
    bool operator()(const uint32_t x)
    {
        // Position im Hash Buffer
        uint32_t hashIndex = x - m_startNonce;
        // Wert des Hashes (als uint64_t) auslesen.
        // Steht im 6. und 7. Wort des Hashes (jeder dieser Hashes hat 512 Bits)
        uint64_t hashValue = *((uint64_t*)(&m_hashes[m_hashlen*hashIndex + 6]));
        // gegen das Target prüfen. Es dürfen nur Bits aus dem Target gesetzt sein.
        return (hashValue & m_target) != hashValue;
    }

    uint64_t  m_target;
    uint32_t *m_hashes;
    uint32_t  m_hashlen;
    uint32_t  m_startNonce;
};

// Zahl der CUDA Devices im System bestimmen
extern "C" int cuda_num_devices()
{
    int version;
    cudaError_t err = cudaDriverGetVersion(&version);
    if (err != cudaSuccess)
    {
     //   applog(LOG_ERR, "Unable to query CUDA driver version! Is an nVidia driver installed?");
        exit(1);
    }

    int maj = version / 1000, min = version % 100; // same as in deviceQuery sample
    if (maj < 5 || (maj == 5 && min < 5))
    {
    //    applog(LOG_ERR, "Driver does not support CUDA %d.%d API! Update your nVidia driver!", 5, 5);
        exit(1);
    }

    int GPU_N;
    err = cudaGetDeviceCount(&GPU_N);
    if (err != cudaSuccess)
    {
     //   applog(LOG_ERR, "Unable to query number of CUDA devices! Is an nVidia driver installed?");
        exit(1);
    }
    return GPU_N;
}

// Gerätenamen holen
extern char *device_name[8];
extern int device_map[8];
int device_major[8];
int device_minor[8];

extern "C" void cuda_devicenames()
{
    cudaError_t err;
    int GPU_N;
    err = cudaGetDeviceCount(&GPU_N);
    if (err != cudaSuccess)
    {
     //   applog(LOG_ERR, "Unable to query number of CUDA devices! Is an nVidia driver installed?");
        exit(1);
    }

    for (int i=0; i < GPU_N; i++)
    {
        cudaDeviceProp props;
        cudaGetDeviceProperties(&props, device_map[i]);

        device_name[i] = strdup(props.name);
		device_major[i] = props.major; 
		device_minor[i] = props.minor;
    }
}

static bool substringsearch(const char *haystack, const char *needle, int &match)
{
    int hlen = strlen(haystack);
    int nlen = strlen(needle);
    for (int i=0; i < hlen; ++i)
    {
        if (haystack[i] == ' ') continue;
        int j=0, x = 0;
        while(j < nlen)
        {
            if (haystack[i+x] == ' ') {++x; continue;}
            if (needle[j] == ' ') {++j; continue;}
            if (needle[j] == '#') return ++match == needle[j+1]-'0';
            if (tolower(haystack[i+x]) != tolower(needle[j])) break;
            ++j; ++x;
        }
        if (j == nlen) return true;
    }
    return false;
}

// CUDA Gerät nach Namen finden (gibt Geräte-Index zurück oder -1)
extern "C" int cuda_finddevice(char *name)
{
    int num = cuda_num_devices();
    int match = 0;
    for (int i=0; i < num; ++i)
    {
        cudaDeviceProp props;
        if (cudaGetDeviceProperties(&props, i) == cudaSuccess)
            if (substringsearch(props.name, name, match)) return i;
    }
    return -1;
}

// Zeitsynchronisations-Routine von cudaminer mit CPU sleep
typedef struct { double value[8]; } tsumarray;
cudaError_t MyStreamSynchronize(cudaStream_t stream, int situation, int thr_id)
{
    cudaError_t result = cudaSuccess;
    if (situation >= 0)
    {   
        static std::map<int, tsumarray> tsum;

        double a = 0.95, b = 0.05;
        if (tsum.find(situation) == tsum.end()) { a = 0.5; b = 0.5; } // faster initial convergence

        double tsync = 0.0;
        double tsleep = 0.95 * tsum[situation].value[thr_id];
        if (cudaStreamQuery(stream) == cudaErrorNotReady)
        {
            usleep((useconds_t)(1e6*tsleep));
            struct timeval tv_start, tv_end;
            gettimeofday(&tv_start, NULL);
            result = cudaStreamSynchronize(stream);
            gettimeofday(&tv_end, NULL);
            tsync = 1e-6 * (tv_end.tv_usec-tv_start.tv_usec) + (tv_end.tv_sec-tv_start.tv_sec);
        }
        if (tsync >= 0) tsum[situation].value[thr_id] = a * tsum[situation].value[thr_id] + b * (tsleep+tsync);
    }
    else
        result = cudaStreamSynchronize(stream);
    return result;
}


extern bool opt_benchmark;

