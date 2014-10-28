
if WANT_JANSSON
JANSSON_INCLUDES= -I$(top_srcdir)/compat/jansson
else
JANSSON_INCLUDES=
endif

EXTRA_DIST		= autogen.sh README.txt LICENSE.txt \
			  cudaminer.sln cudaminer.vcxproj cudaminer.vcxproj.filters \
			  compat/gettimeofday.c compat/getopt/getopt_long.c cpuminer-config.h.in

SUBDIRS		= compat

bin_PROGRAMS	= skminer

skminer_SOURCES		= elist.h miner.h compat.h \
			  compat/inttypes.h compat/stdbool.h compat/unistd.h \
			  compat/sys/time.h compat/getopt/getopt.h \
    util.cpp \
    hash/CBlock.cpp hash/Connection.cpp hash/DDOS_Filter.cpp \
    hash/DDOS_Score.cpp    hash/Keccak-compact64.c   hash/KeccakDuplex.c  hash/KeccakHash.c  hash/KeccakSponge.c \
    hash/Miner.cpp hash/MinerThread.cpp   hash/Outbound.cpp hash/Packet.cpp hash/skein.cpp hash/skein_block.cpp \
    hash/Timer.cpp \
    sk-miner.cpp core.cpp \
    heavy/heavy.cu Algo/cuda_skein1024.cu Algo/sk1024.cu Algo/sk1024_keccak.cu
			  

skminer_LDFLAGS		=  @CUDA_LDFLAGS@
skminer_LDADD		= @LIBCURL@ @JANSSON_LIBS@  @WS2_LIBS@ @CUDA_LIBS@ @OPENMP_CFLAGS@ @LIBS@ -L/usr/local/lib -lboost_filesystem -lboost_system -lboost_program_options -lboost_thread
skminer_CPPFLAGS	= -msse2 @LIBCURL_CPPFLAGS@ @OPENMP_CFLAGS@ -I/usr/local/include/boost -fno-strict-aliasing $(JANSSON_INCLUDES) -DSCRYPT_KECCAK512 -DSCRYPT_CHACHA -DSCRYPT_CHOOSE_COMPILETIME -DBOOST_THREAD_USE_LIB -DBOOST_SPIRIT_THREADSAFE

# we're now targeting all major compute architectures within one binary (or not).
NVCC_ARCH = -gencode=arch=compute_50,code=\"sm_50,compute_50\"
# or -gencode=arch=compute_30,code=\"sm_30,compute_30\" -gencode=arch=compute_35,code=\"sm_35,compute_35\"

# require cuda 6.5 or higher
.cu.o:
	$(NVCC) @CFLAGS@ -I . -Xptxas "-v" $(NVCC_ARCH) --ptxas-options=-v $(JANSSON_INCLUDES) -o $@ -c $<

