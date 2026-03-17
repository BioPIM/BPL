
// dpu-upmem-dpurte-clang -O3 -DNR_TASKLETS=16 -DSTACK_SIZE_DEFAULT=4000 ../src/main/benchmark_sdk_pim.c -o benchmark_sdk_pim

#include <mram.h>
#include <defs.h>

#define SSIZE   10000
#define BUFSIZE  128 // must be a power of 2

__mram_noinit uint64_t refSize;
__mram_noinit uint64_t qrySize;

#define NB_SKETCHES_REF (1024+256)
#define NB_SKETCHES_QRY  256

__mram_noinit uint32_t ref    [SSIZE*NB_SKETCHES_REF];
__mram_noinit uint32_t qry    [SSIZE*NB_SKETCHES_QRY];
__mram_noinit uint32_t results[NB_SKETCHES_REF*NB_SKETCHES_QRY];

inline void force_update (__mram_ptr uint32_t const* global, uint32_t local[BUFSIZE], uint32_t offset) {
	mram_read (&global[offset], local, BUFSIZE*sizeof(uint32_t));
}

inline void update (__mram_ptr uint32_t const* global, uint32_t local[BUFSIZE], uint32_t offset) {
	if (( (offset&(BUFSIZE-1)) ==0)) {
		force_update (global,local,offset);
	}
}

int main()
{
    uint64_t nbSketchRef = refSize / SSIZE / NR_TASKLETS;
	uint64_t nbSketchQry = qrySize / SSIZE;

	uint32_t offsetRefBegin = (me() * refSize)  / NR_TASKLETS;

	uint32_t offsetResults = nbSketchRef*nbSketchQry*me();
    uint32_t idxResults = 0;

	__dma_aligned uint32_t refLocal [BUFSIZE];
	__dma_aligned uint32_t qryLocal [BUFSIZE];

    for (uint32_t idxSketchQry=0, offsetQry=0; idxSketchQry<nbSketchQry; idxSketchQry++, offsetQry+=SSIZE)
    {
	    for (uint32_t idxSketchRef=0, offsetRef=offsetRefBegin; idxSketchRef<nbSketchRef; idxSketchRef++, offsetRef+=SSIZE)
        {
            uint32_t count = 0;

            uint32_t refBegin = offsetRef;
            uint32_t refEnd   = offsetRef + SSIZE;

            uint32_t qryBegin = offsetQry;
            uint32_t qryEnd   = offsetQry + SSIZE;

            force_update (ref,refLocal,refBegin);
            force_update (qry,qryLocal,qryBegin);

            while (1)  {

            	uint32_t r = refLocal[refBegin & (BUFSIZE-1)];
            	uint32_t q = qryLocal[qryBegin & (BUFSIZE-1)];

                    if (r < q)   {   if (++refBegin == refEnd)  { break; }  update (ref,refLocal,refBegin); }
               else if (r > q)   {   if (++qryBegin == qryEnd)  { break; }  update (qry,qryLocal,qryBegin); }
               else
               {
                   count++;
                   if (++refBegin == refEnd)  { break; }  update (ref,refLocal,refBegin);
                   if (++qryBegin == qryEnd)  { break; }  update (qry,qryLocal,qryBegin);
               }
            }
            results[offsetResults++] = count;
        }
    }

    return 0;
}
