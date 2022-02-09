#ifndef _ROCKCHIP_MPI_COMMON_H_
#define _ROCKCHIP_MPI_COMMON_H_

#define MPP_ALIGN(x, a)         (((x)+(a)-1)&~((a)-1))

#define SZ_1K                   (1024)
#define SZ_2K                   (SZ_1K*2)
#define SZ_4K                   (SZ_1K*4)
#define SZ_8K                   (SZ_1K*8)
#define SZ_16K                  (SZ_1K*16)
#define SZ_32K                  (SZ_1K*32)
#define SZ_64K                  (SZ_1K*64)
#define SZ_128K                 (SZ_1K*128)
#define SZ_256K                 (SZ_1K*256)
#define SZ_512K                 (SZ_1K*512)
#define SZ_1M                   (SZ_1K*SZ_1K)
#define SZ_2M                   (SZ_1M*2)
#define SZ_4M                   (SZ_1M*4)
#define SZ_8M                   (SZ_1M*8)
#define SZ_16M                  (SZ_1M*16)
#define SZ_32M                  (SZ_1M*32)
#define SZ_64M                  (SZ_1M*64)
#define SZ_80M                  (SZ_1M*80)
#define SZ_128M                 (SZ_1M*128)

#endif
