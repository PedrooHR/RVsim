#include "memory.h"

#include <cstdio>

#include <string>
#include <map>

extern "C" { 
  #include "d4.h"
}

#define INT_MAX (2147483647)

// Read misses
#define l1imisses_read  (caches["L1i"]->miss[D4XINSTRN])
#define l1dmisses_read  (caches["L1d"]->miss[D4XREAD])
#define l2misses_read   (caches["L2"]->miss[D4XREAD])
#define l3misses_read   (caches["L3"]->miss[D4XREAD])

// write misses
#define l1dmisses_write (caches["L1d"]->miss[D4XWRITE])
#define l2misses_write  (caches["L2"]->miss[D4XWRITE])
#define l3misses_write  (caches["L3"]->miss[D4XWRITE])

void SetupDineroCache(d4cache *cache, const char *pname, int pflags,
                      int plg2bsize, int plg2sbsize, int plg2size, int passoc,
                      d4stacknode *(*preplacementf)(d4_cache_struct *, int,
                                                    d4memref,
                                                    d4_stacknode_struct *),
                      d4pendstack *(*pprefetchf)(d4_cache_struct *, d4memref,
                                                 int, d4_stacknode_struct *),
                      int pprefetch_dist, int pprefetch_abort,
                      int (*pwallocf)(d4_cache_struct *, d4memref),
                      int (*pwbackf)(d4_cache_struct *, d4memref, int,
                                     d4stacknode *, int)) {
  cache->name = (char *)pname;
  cache->flags = pflags;
  cache->lg2blocksize = plg2bsize;
  cache->lg2subblocksize = plg2sbsize;
  cache->lg2size = plg2size;
  cache->assoc = passoc;
  cache->replacementf = preplacementf;
  cache->prefetchf = pprefetchf;
  cache->prefetch_distance = pprefetch_dist;
  cache->prefetch_abortpercent = pprefetch_abort;
  cache->wallocf = pwallocf;
  cache->wbackf = pwbackf;
  cache->name_replacement = (char*)"Replacement";
  cache->name_prefetch = (char*)"Prefetch";
  cache->name_walloc = (char*)"Walloc";
  cache->name_wback = (char*)"Wback";
}

memory_t::memory_t(uint32_t size) { // size in MiBytes
  memory.clear();
  memory.resize(size * 1024, 0);

  // Cria cache mais baixo nível (memoria);
  caches["memory"] = d4new(NULL);
  // // Cria L3
  // caches["L3"] = d4new(caches["memory"]);
  // SetupDineroCache(caches["L3"], "L3", D4F_CCC, 10, 10, 23, 4, d4rep_lru, d4prefetch_none, 1, 0,
  //                  d4walloc_always, d4wback_nofetch);
  // // Cria L2
  // caches["L2"] = d4new(caches["L3"]);
  // SetupDineroCache(caches["L2"], "L3", D4F_CCC, 10, 10, 18, 4, d4rep_lru, d4prefetch_none, 1, 0,
  //                  d4walloc_always, d4wback_nofetch);
  // // Cria L1
  // caches["L1d"] = d4new(caches["L2"]);
  // SetupDineroCache(caches["L1d"], "L1d", D4F_CCC, 7, 7, 15, 4, d4rep_lru, d4prefetch_none, 1,
  //                  0, d4walloc_always, d4wback_nofetch);
  // caches["L1i"] = d4new(caches["L2"]);
  // SetupDineroCache(caches["L1i"], "L1i", D4F_CCC | D4F_RO, 7, 7, 15, 4, d4rep_lru,
  //                  d4prefetch_none, 1, 0, d4walloc_impossible,
  //                  d4wback_impossible);

  // Cria L3
  caches["L3"] = d4new(caches["memory"]);
  SetupDineroCache(caches["L3"], "L3", D4F_CCC, 7, 7, 23, 4, d4rep_lru, d4prefetch_none, 1, 0,
                   d4walloc_always, d4wback_nofetch);
  // Cria L2
  caches["L2"] = d4new(caches["L3"]);
  SetupDineroCache(caches["L2"], "L3", D4F_CCC, 6, 6, 18, 4, d4rep_lru, d4prefetch_none, 1, 0,
                   d4walloc_always, d4wback_nofetch);
  // Cria L1
  caches["L1d"] = d4new(caches["L2"]);
  SetupDineroCache(caches["L1d"], "L1d", D4F_CCC, 5, 5, 15, 4, d4rep_lru, d4prefetch_none, 1,
                   0, d4walloc_always, d4wback_nofetch);
  caches["L1i"] = d4new(caches["L2"]);
  SetupDineroCache(caches["L1i"], "L1i", D4F_CCC | D4F_RO, 5, 5, 15, 4, d4rep_lru,
                   d4prefetch_none, 1, 0, d4walloc_impossible,
                   d4wback_impossible);

  // Setup Dinero
  if (d4setup() != 0) {
    printf("Dinero IV Setup Failed \n");
    exit(0);
  }
}

uint32_t memory_t::writeMem(uint32_t address, uint8_t value, ACCESS_TYPE type) {
  uint32_t cycles = 1;
  if (type != ACCESS_TYPE::LOAD) { 
    // Verifica os misses antes do acesso
    int pld_miss, pl2_miss, pl3_miss;
    pld_miss = l1dmisses_read;
    pl2_miss = l2misses_read;
    pl3_miss = l3misses_read;

    // Faz o acesso a memória
    d4cache * top_level = (type == ACCESS_TYPE::DATA) ? caches["L1d"] : caches["L1i"];
    d4memref mem_ref; 
    mem_ref.address = (d4addr) address;
    mem_ref.size = 1;
    mem_ref.accesstype = D4XWRITE;
    d4ref(top_level, mem_ref);

    // Pega misses da cache depois de usá-la
    int ld_miss, l2_miss, l3_miss;
    ld_miss = l1dmisses_write;
    l2_miss = l2misses_write;
    l3_miss = l3misses_write;

    // Verifica se houveram misses na cache
    if (ld_miss > pld_miss) {
      cycles += 10;
      ld_misses++;
    }
    if (l2_miss > pl2_miss) {
      cycles += 20;
      l2_misses++;
    }
    if (l3_miss > pl3_miss) {
      cycles += 100;
      l3_misses++;
    }
  }
  memory[address] = value;
  return cycles;
}

uint32_t memory_t::readMem(uint32_t address, uint8_t *value, ACCESS_TYPE type) {
  uint32_t cycles = 0;
  if (type != ACCESS_TYPE::LOAD) {
    // Verifica os misses antes do acesso
    int pli_miss, pld_miss, pl2_miss, pl3_miss;
    pli_miss = l1imisses_read;
    pld_miss = l1dmisses_read;
    pl2_miss = l2misses_read;
    pl3_miss = l3misses_read;

    // Faz o acesso a memória
    d4cache * top_level = (type == ACCESS_TYPE::DATA) ? caches["L1d"] : caches["L1i"];
    d4memref mem_ref;
    mem_ref.address = (d4addr) address;
    mem_ref.size = 1;
    mem_ref.accesstype = (type == ACCESS_TYPE::DATA) ? D4XREAD : D4XINSTRN;
    d4ref(top_level, mem_ref);

    // Pega misses da cache depois de usá-la
    int li_miss, ld_miss, l2_miss, l3_miss;
    li_miss = l1imisses_read;
    ld_miss = l1dmisses_read;
    l2_miss = l2misses_read;
    l3_miss = l3misses_read;

    // Verifica se houveram misses na cache
    if (li_miss > pli_miss) {
      cycles += 10;
      li_misses++;
    }
    if (ld_miss > pld_miss) {
      cycles += 10;
      ld_misses++;
    }
    if (l2_miss > pl2_miss) {
      cycles += 20;
      l2_misses++;
    }
    if (l3_miss > pl3_miss) {
      cycles += 100;
      l3_misses++;
    }
  }
  *value = memory[address];
  return cycles; 
}

uint32_t memory_t::getTotalSize() { return memory.size(); }

void memory_t::printCacheMisses() {
  printf("Li Misses %d\n", li_misses);
  printf("Ld Misses %d\n", ld_misses);
  printf("L2 Misses %d\n", l2_misses);
  printf("L3 Misses %d\n", l3_misses);
}

void memory_t::printd4log() {
  // L1i
  printf("L1i dineroIV log:\n");
  printf("Bytes read: %e - Bytes Written: %e\n", 
         caches["L1i"]->bytes_read,
         caches["L1i"]->bytes_written);
  printf("Read fetches %e - Read misses: %e\n", 
         caches["L1i"]->fetch[D4XINSTRN],
         caches["L1i"]->miss[D4XINSTRN]);
  printf("Write fetches %e - Write misses: %e\n\n", 
         caches["L1i"]->fetch[D4XWRITE],
         caches["L1i"]->miss[D4XWRITE]);

  // L1d
  printf("L1d dineroIV log:\n");
  printf("Bytes read: %e - Bytes Written: %e\n", 
         caches["L1d"]->bytes_read,
         caches["L1d"]->bytes_written);
  printf("Read fetches %e - Read misses: %e\n", 
         caches["L1d"]->fetch[D4XREAD] + caches["L1d"]->fetch[D4XINSTRN],
         caches["L1d"]->miss[D4XREAD] + caches["L1d"]->fetch[D4XINSTRN]);
  printf("Write fetches %e - Write misses: %e\n\n", 
         caches["L1d"]->fetch[D4XWRITE],
         caches["L1d"]->miss[D4XWRITE]);

  // L2
  printf("L2 dineroIV log:\n");
  printf("Bytes read: %e - Bytes Written: %e\n", 
         caches["L2"]->bytes_read,
         caches["L2"]->bytes_written);
  printf("Read fetches %e - Read misses: %e\n", 
         caches["L2"]->fetch[D4XREAD] + caches["L2"]->fetch[D4XINSTRN],
         caches["L2"]->miss[D4XREAD] + caches["L2"]->fetch[D4XINSTRN]);
  printf("Write fetches %e - Write misses: %e\n\n", 
         caches["L2"]->fetch[D4XWRITE],
         caches["L2"]->miss[D4XWRITE]);

  // L3
  printf("L3 dineroIV log:\n");
  printf("Bytes read: %e - Bytes Written: %e\n", 
         caches["L3"]->bytes_read,
         caches["L3"]->bytes_written);
  printf("Read fetches %e - Read misses: %e\n", 
         caches["L3"]->fetch[D4XREAD] + caches["L3"]->fetch[D4XINSTRN],
         caches["L3"]->miss[D4XREAD] + caches["L3"]->fetch[D4XINSTRN]);
  printf("Write fetches %e - Write misses: %e\n\n", 
         caches["L3"]->fetch[D4XWRITE],
         caches["L3"]->miss[D4XWRITE]);
}
