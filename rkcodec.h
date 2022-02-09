#ifndef _ROCKCHIP_MPP_COODEC_H_
#define _ROCKCHIP_MPP_COODEC_H_

#include "rockchip/rk_mpi.h"
#include <stdbool.h>

typedef void (*MppTask_out)(RK_U8 *data, RK_S32 len, void *extra);

typedef struct {
    /* base data */
    MppCodingType   type;
    MppCodingType   type_src;       /* for file source input */

    RK_S32          width;
    RK_S32          height;
    RK_S32          hor_stride;
    RK_S32          ver_stride;

    /* 0. rc codec data */
    MppCtx          enc_ctx;
    MppCtx          dec_ctx;
    MppApi          *enc_mpi;
    MppApi          *dec_mpi;

    /* 1. decoder data */
    MppCodingType   dec_type;
    MppPacket       dec_pkt;
    RK_U8           *dec_in_buf;
    RK_U32          dec_in_buf_size;

    /* 2. encoder data */
    MppBufferGroup  pkt_grp;
    MppBuffer       enc_pkt_buf;
    MppPacket       enc_pkt;

    MppEncRcCfg     rc_cfg;

    MppTask_out     enc_out_task;
    void            *enc_out_task_param;

    RK_U32          pkt_eos;
    RK_S32          frm_eos;
    RK_U32          enc_pkt_eos;
    MppEncCfg       cfg;

    RK_S32          frm_idx;
} MpiRc2Ctx;


typedef struct rkcodec_s {
    MpiRc2Ctx *rc2ctx;

    int (*create)(struct rkcodec_s *rkcodec, RK_S32 width, RK_S32 height, RK_U32 fps_in, RK_U32 fps_out);
    int (*destroy)(struct rkcodec_s *rkcodec);
    int (*put_decdata)(struct rkcodec_s *rkcodec, RK_U8 *data, RK_U32 datal);
    int (*enc_out_register)(struct rkcodec_s *rkcodec, MppTask_out enc_out_task, void *param);
} rkcodec_t;

int rk_codec_init(rkcodec_t *rkcodec);
int rk_codec_deinit(rkcodec_t *rkcodec);

#endif
