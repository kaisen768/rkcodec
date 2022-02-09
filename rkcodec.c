#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>

#include "rkcodec.h"
#include "mpi_common.h"

static RK_S32 mpi_enc_width_default_stride(RK_S32 width, MppFrameFormat fmt)
{
    RK_S32 stride = 0;

    switch (fmt & MPP_FRAME_FMT_MASK) {
    case MPP_FMT_YUV420SP :
    case MPP_FMT_YUV420SP_VU : {
        stride = MPP_ALIGN(width, 8);
    } break;
    case MPP_FMT_YUV420P : {
        /* NOTE: 420P need to align to 16 so chroma can align to 8 */
        stride = MPP_ALIGN(width, 16);
    } break;
    case MPP_FMT_YUV422P:
    case MPP_FMT_YUV422SP:
    case MPP_FMT_YUV422SP_VU: {
        /* NOTE: 422 need to align to 8 so chroma can align to 16 */
        stride = MPP_ALIGN(width, 8);
    } break;
    case MPP_FMT_YUV444SP :
    case MPP_FMT_YUV444P : {
        stride = MPP_ALIGN(width, 8);
    } break;
    case MPP_FMT_RGB565:
    case MPP_FMT_BGR565:
    case MPP_FMT_RGB555:
    case MPP_FMT_BGR555:
    case MPP_FMT_RGB444:
    case MPP_FMT_BGR444:
    case MPP_FMT_YUV422_YUYV :
    case MPP_FMT_YUV422_YVYU :
    case MPP_FMT_YUV422_UYVY :
    case MPP_FMT_YUV422_VYUY : {
        /* NOTE: for vepu limitation */
        stride = MPP_ALIGN(width, 8) * 2;
    } break;
    case MPP_FMT_RGB888 :
    case MPP_FMT_BGR888 : {
        /* NOTE: for vepu limitation */
        stride = MPP_ALIGN(width, 8) * 3;
    } break;
    case MPP_FMT_RGB101010 :
    case MPP_FMT_BGR101010 :
    case MPP_FMT_ARGB8888 :
    case MPP_FMT_ABGR8888 :
    case MPP_FMT_BGRA8888 :
    case MPP_FMT_RGBA8888 : {
        /* NOTE: for vepu limitation */
        stride = MPP_ALIGN(width, 8) * 4;
    } break;
    default : {
        fprintf(stderr, "do not support type %d\n", fmt);
    } break;
    }

    return stride;
}

static MPP_RET rc2ctx_set_value(MpiRc2Ctx *ctx, RK_S32 width, RK_S32 height, RK_U32 fps_in, RK_U32 fps_out)
{
    MppEncRcCfg *rc_cfg;

    if (!ctx)
        return MPP_ERR_NULL_PTR;

    if (width <= 0 || height <= 0)
        return MPP_ERR_VALUE;

    memset(ctx, 0, sizeof(MpiRc2Ctx));
    rc_cfg = &ctx->rc_cfg;

    /* 0. base data */
    ctx->type               = MPP_VIDEO_CodingHEVC;
    ctx->type_src           = MPP_VIDEO_CodingAVC;

    ctx->width              = width;
    ctx->height             = height;
    ctx->hor_stride         = mpi_enc_width_default_stride(width, MPP_FMT_YUV420SP);
    ctx->ver_stride         = height;

    /* -bps */
    rc_cfg->bps_target      = 0;
    rc_cfg->bps_max         = 0;
    rc_cfg->bps_min         = 0;
    rc_cfg->rc_mode         = MPP_ENC_RC_MODE_CBR;

    /* -fps */
    rc_cfg->fps_in_flex     = 0;
    rc_cfg->fps_in_num      = fps_in ? fps_in : 30;
    rc_cfg->fps_in_denorm   = 1;
    rc_cfg->fps_out_flex    = 0;
    rc_cfg->fps_out_num     = fps_out ? fps_out : 30;;
    rc_cfg->fps_out_denorm  = 1;

    /* -g gop mode */
    rc_cfg->gop             = rc_cfg->fps_out_num;

    /* 1. dec decoder data */
    ctx->dec_type           = ctx->type_src;

    return MPP_OK;
}

static MPP_RET rk_mpi_rc_buffer_init(MpiRc2Ctx *ctx)
{
    /* NOTE: packet buffer may overflow */
    size_t packet_size  = SZ_512K;
    MPP_RET ret = MPP_OK;

    ret = mpp_buffer_group_get_internal(&ctx->pkt_grp, MPP_BUFFER_TYPE_ION);
    if (ret) {
        fprintf(stderr, "failed to get buffer group for output packet ret %d\n", ret);
        goto err0;
    }

    ret = mpp_buffer_get(ctx->pkt_grp, &ctx->enc_pkt_buf, packet_size);
    if (ret) {
        fprintf(stderr, "failed to get buffer for input frame ret %d\n", ret);
        goto err1;
    }

    ctx->frm_idx = 0;

    return MPP_OK;

    mpp_buffer_put(ctx->enc_pkt_buf);
    ctx->enc_pkt_buf = NULL;
err1:
    mpp_buffer_group_put(ctx->pkt_grp);
    ctx->pkt_grp = NULL;
err0:
    return ret;
}

static void rk_mpi_rc_buffer_deinit(MpiRc2Ctx *ctx)
{
    if (ctx->enc_pkt_buf) {
        mpp_buffer_put(ctx->enc_pkt_buf);
        ctx->enc_pkt_buf = NULL;
    }

    if (ctx->pkt_grp) {
        mpp_buffer_group_put(ctx->pkt_grp);
        ctx->pkt_grp = NULL;
    }
}

static MPP_RET rk_mpi_rc_dec_init(MpiRc2Ctx *ctx)
{
    RK_U32 need_split = 1;
    MppPollType block = MPP_POLL_NON_BLOCK;
    RK_U32 fast_en = 0;
    MppApi *dec_mpi = NULL;
    MppCtx dec_ctx = NULL;
    MppFrameFormat format = MPP_FMT_YUV420SP;
    MPP_RET ret = MPP_OK;

    // decoder init
    ret = mpp_create(&ctx->dec_ctx, &ctx->dec_mpi);
    if (ret) {
        fprintf(stderr, "mpp_create decoder failed\n");
        goto err0;
    }

    dec_mpi = ctx->dec_mpi;
    dec_ctx = ctx->dec_ctx;

    ret = mpp_packet_init(&ctx->dec_pkt, ctx->dec_in_buf, ctx->dec_in_buf_size);
    if (ret) {
        fprintf(stderr, "mpp_packet_init failed\n");
        goto err1;
    }

    ret = dec_mpi->control(dec_ctx, MPP_DEC_SET_PARSER_SPLIT_MODE, &need_split);
    if (ret) {
        fprintf(stderr, "dec_mpi->control failed\n");
        goto err2;
    }

    ret = dec_mpi->control(dec_ctx, MPP_DEC_SET_PARSER_FAST_MODE, &fast_en);
    if (ret) {
        fprintf(stderr, "dec_mpi->control failed\n");
        goto err2;
    }

    ret = dec_mpi->control(dec_ctx, MPP_SET_INPUT_TIMEOUT, (MppParam)&block);
    if (ret) {
        fprintf(stderr, "dec_mpi->control dec MPP_SET_INPUT_TIMEOUT failed\n");
        goto err2;
    }

    block = MPP_POLL_NON_BLOCK;
    ret = dec_mpi->control(dec_ctx, MPP_SET_OUTPUT_TIMEOUT, (MppParam)&block);
    if (ret) {
        fprintf(stderr, "dec_mpi->control MPP_SET_OUTPUT_TIMEOUT failed\n");
        goto err2;
    }

    ret = mpp_init(dec_ctx, MPP_CTX_DEC, ctx->dec_type);
    if (ret) {
        fprintf(stderr, "mpp_init dec failed\n");
        goto err2;
    }

    ret = dec_mpi->control(dec_ctx, MPP_DEC_SET_OUTPUT_FORMAT, &format);
    if (ret) {
        fprintf(stderr, "dec_mpi->control failed\n");
        goto err2;
    }

    return MPP_OK;

err2:
    mpp_packet_deinit(&ctx->dec_pkt);
    ctx->dec_pkt = NULL;
err1:
    fprintf(stderr, "@@@@@@@@@@ free ctx->dec_ctx\n");
    mpp_destroy(ctx->dec_ctx);
    ctx->dec_ctx = NULL;
err0:
    return ret;
}

static void rk_mpi_rc_dec_deinit(MpiRc2Ctx *ctx)
{
    if (ctx->dec_pkt) {
        mpp_packet_deinit(&ctx->dec_pkt);
        ctx->dec_pkt = NULL;
    }

    if (ctx->dec_ctx) {
        ctx->dec_mpi->reset(ctx->dec_ctx);

        mpp_destroy(ctx->dec_ctx);
        ctx->dec_ctx = NULL;
    }
}

static MPP_RET rk_mpi_rc_enc_init(MpiRc2Ctx *ctx)
{
    MppApi *enc_mpi = NULL;
    MppCtx enc_ctx = NULL;
    MppPollType block = MPP_POLL_NON_BLOCK;
    MppEncRcCfg *rc_cfg = &ctx->rc_cfg;
    MppEncSeiMode sei_mode = MPP_ENC_SEI_MODE_ONE_SEQ;
    MppEncCfg cfg = ctx->cfg;
    RK_U32 debreath_en = 0;
    RK_U32 debreath_s = 0;
    MPP_RET ret = MPP_OK;

    // encoder init
    ret = mpp_create(&ctx->enc_ctx, &ctx->enc_mpi);
    if (ret) {
        fprintf(stderr, "mpp_create encoder failed\n");
        goto err0;
    }

    enc_mpi = ctx->enc_mpi;
    enc_ctx = ctx->enc_ctx;

    block = MPP_POLL_BLOCK;
    ret = enc_mpi->control(enc_ctx, MPP_SET_INPUT_TIMEOUT, (MppParam)&block);
    if (ret) {
        fprintf(stderr, "enc_mpi->control MPP_SET_INPUT_TIMEOUT failed\n");
        goto err1;
    }
    block = MPP_POLL_BLOCK;
    ret = enc_mpi->control(enc_ctx, MPP_SET_OUTPUT_TIMEOUT, (MppParam)&block);
    if (ret) {
        fprintf(stderr, "enc_mpi->control MPP_SET_OUTPUT_TIMEOUT failed\n");
        goto err1;
    }

    ret = mpp_init(enc_ctx, MPP_CTX_ENC, ctx->type);
    if (ret) {
        fprintf(stderr, "mpp_init enc failed\n");
        goto err1;
    }

    if (ctx->width)
        mpp_enc_cfg_set_s32(cfg, "prep:width", ctx->width);
    if (ctx->height)
        mpp_enc_cfg_set_s32(cfg, "prep:height", ctx->height);
    if (ctx->hor_stride)
        mpp_enc_cfg_set_s32(cfg, "prep:hor_stride", ctx->hor_stride);
    if (ctx->ver_stride)
        mpp_enc_cfg_set_s32(cfg, "prep:ver_stride", ctx->ver_stride);
    mpp_enc_cfg_set_s32(cfg, "prep:format", MPP_FMT_YUV420SP);

    mpp_enc_cfg_set_s32(cfg, "rc:mode",  rc_cfg->rc_mode);

    switch (rc_cfg->rc_mode) {
    case MPP_ENC_RC_MODE_FIXQP : {
        /* do not set bps on fix qp mode */
    } break;
    case MPP_ENC_RC_MODE_CBR : {
        /* CBR mode has narrow bound */
        mpp_enc_cfg_set_s32(cfg, "rc:bps_target", rc_cfg->bps_target);
        mpp_enc_cfg_set_s32(cfg, "rc:bps_max", rc_cfg->bps_max ? rc_cfg->bps_max : rc_cfg->bps_target * 3 / 2);
        mpp_enc_cfg_set_s32(cfg, "rc:bps_min", rc_cfg->bps_min ? rc_cfg->bps_max : rc_cfg->bps_target / 2);
    } break;
    case MPP_ENC_RC_MODE_VBR : {
        /* CBR mode has wide bound */
        mpp_enc_cfg_set_s32(cfg, "rc:bps_target", rc_cfg->bps_target);
        mpp_enc_cfg_set_s32(cfg, "rc:bps_max", rc_cfg->bps_max ? rc_cfg->bps_max : rc_cfg->bps_target * 17 / 16);
        mpp_enc_cfg_set_s32(cfg, "rc:bps_min", rc_cfg->bps_min ? rc_cfg->bps_max : rc_cfg->bps_target * 1 / 16);
    } break;
    default : {
        fprintf(stderr, "unsupport encoder rc mode %d\n", rc_cfg->rc_mode);
    } break;
    }

    /* fix input / output frame rate */
    mpp_enc_cfg_set_s32(cfg, "rc:fps_in_flex", rc_cfg->fps_in_flex);
    mpp_enc_cfg_set_s32(cfg, "rc:fps_in_num", rc_cfg->fps_in_num);
    mpp_enc_cfg_set_s32(cfg, "rc:fps_in_denorm",  rc_cfg->fps_in_denorm);
    mpp_enc_cfg_set_s32(cfg, "rc:fps_out_flex", rc_cfg->fps_out_flex);
    mpp_enc_cfg_set_s32(cfg, "rc:fps_out_num",  rc_cfg->fps_out_num);
    mpp_enc_cfg_set_s32(cfg, "rc:fps_out_denorm", rc_cfg->fps_out_denorm);
    mpp_enc_cfg_set_s32(cfg, "rc:gop", rc_cfg->gop);

    /* drop frame or not when bitrate overflow */
    mpp_enc_cfg_set_u32(cfg, "rc:drop_mode", MPP_ENC_RC_DROP_FRM_DISABLED);
    mpp_enc_cfg_set_u32(cfg, "rc:drop_thd", 20);        /* 20% of max bps */
    mpp_enc_cfg_set_u32(cfg, "rc:drop_gap", 1);         /* Do not continuous drop frame */

    mpp_enc_cfg_set_u32(cfg, "rc:debreath_en", debreath_en);
    mpp_enc_cfg_set_u32(cfg, "rc:debreath_strength", debreath_s);

    /* setup codec  */
    mpp_enc_cfg_set_s32(cfg, "codec:type",  ctx->type);
    switch (ctx->type) {
    case MPP_VIDEO_CodingAVC : {
        /*
         * H.264 profile_idc parameter
         * 66  - Baseline profile
         * 77  - Main profile
         * 100 - High profile
         */
        mpp_enc_cfg_set_s32(cfg, "h264:profile", 100);
        /*
         * H.264 level_idc parameter
         * 10 / 11 / 12 / 13    - qcif@15fps / cif@7.5fps / cif@15fps / cif@30fps
         * 20 / 21 / 22         - cif@30fps / half-D1@@25fps / D1@12.5fps
         * 30 / 31 / 32         - D1@25fps / 720p@30fps / 720p@60fps
         * 40 / 41 / 42         - 1080p@30fps / 1080p@30fps / 1080p@60fps
         * 50 / 51 / 52         - 4K@30fps
         */
        mpp_enc_cfg_set_s32(cfg, "h264:level", 40);
        mpp_enc_cfg_set_s32(cfg, "h264:cabac_en", 1);
        mpp_enc_cfg_set_s32(cfg, "h264:cabac_idc", 0);
        mpp_enc_cfg_set_s32(cfg, "h264:trans8x8", 1);

        if (rc_cfg->rc_mode == MPP_ENC_RC_MODE_FIXQP) {
            mpp_enc_cfg_set_s32(cfg, "h264:qp_init", 20);
            mpp_enc_cfg_set_s32(cfg, "h264:qp_max", 16);
            mpp_enc_cfg_set_s32(cfg, "h264:qp_min", 16);
            mpp_enc_cfg_set_s32(cfg, "h264:qp_max_i", 20);
            mpp_enc_cfg_set_s32(cfg, "h264:qp_min_i", 20);
        } else {
            mpp_enc_cfg_set_s32(cfg, "h264:qp_init", 26);
            mpp_enc_cfg_set_s32(cfg, "h264:qp_max", 51);
            mpp_enc_cfg_set_s32(cfg, "h264:qp_min", 10);
            mpp_enc_cfg_set_s32(cfg, "h264:qp_max_i", 46);
            mpp_enc_cfg_set_s32(cfg, "h264:qp_min_i", 18);
        }
    } break;
    case MPP_VIDEO_CodingMJPEG : {
        mpp_enc_cfg_set_s32(cfg, "jpeg:q_factor", 80);
        mpp_enc_cfg_set_s32(cfg, "jpeg:qf_max", 99);
        mpp_enc_cfg_set_s32(cfg, "jpeg:qf_min", 1);
    } break;
    case MPP_VIDEO_CodingVP8 : {
        mpp_enc_cfg_set_s32(cfg, "vp8:qp_init", 40);
        mpp_enc_cfg_set_s32(cfg, "vp8:qp_max",  127);
        mpp_enc_cfg_set_s32(cfg, "vp8:qp_min",  0);
        mpp_enc_cfg_set_s32(cfg, "vp8:qp_max_i", 127);
        mpp_enc_cfg_set_s32(cfg, "vp8:qp_min_i", 0);
    } break;
    case MPP_VIDEO_CodingHEVC : {
        mpp_enc_cfg_set_s32(cfg, "h265:qp_init", rc_cfg->rc_mode == MPP_ENC_RC_MODE_FIXQP ? -1 : 26);
        mpp_enc_cfg_set_s32(cfg, "h265:qp_max", 51);
        mpp_enc_cfg_set_s32(cfg, "h265:qp_min", 10);
        mpp_enc_cfg_set_s32(cfg, "h265:qp_max_i", 46);
        mpp_enc_cfg_set_s32(cfg, "h265:qp_min_i", 18);
    } break;
    default : {
        fprintf(stderr, "unsupport encoder coding type %d\n",  ctx->type);
    } break;
    }

    ret = enc_mpi->control(enc_ctx, MPP_ENC_SET_CFG, cfg);
    if (ret) {
        fprintf(stderr, "mpi control enc set cfg failed ret %d\n", ret);
        goto err1;
    }

    /* optional */
    ret = enc_mpi->control(enc_ctx, MPP_ENC_SET_SEI_CFG, &sei_mode);
    if (ret) {
        fprintf(stderr, "mpi control enc set sei cfg failed ret %d\n", ret);
        goto err1;
    }

    return MPP_OK;

err1:
    mpp_destroy(ctx->enc_ctx);
    ctx->enc_ctx = NULL;
err0:
    return ret;
}

static void rk_mpi_rc_enc_deinit(MpiRc2Ctx *ctx)
{
    if (ctx->enc_ctx) {
        ctx->enc_mpi->reset(ctx->enc_ctx);
        mpp_destroy(ctx->enc_ctx);
        ctx->enc_ctx = NULL;
    }
}

static MPP_RET rk_mpi_decode_data_put(MpiRc2Ctx *ctx, RK_U8 *data, RK_U32 datal)
{
    MppPacket packet = ctx->dec_pkt;
    MppApi *mpi = ctx->dec_mpi;
    MppCtx dec_ctx = ctx->dec_ctx;
    MPP_RET ret = MPP_OK;
    RK_S32 dec_pkt_done = 0;

    mpp_packet_set_data(packet, data);
    mpp_packet_set_size(packet, datal);
    mpp_packet_set_pos(packet, data);
    mpp_packet_set_length(packet, datal);

    do {
        ret = mpi->decode_put_packet(dec_ctx, packet);
        if (MPP_OK == ret)
            dec_pkt_done = 1;
        else
            usleep(5000);
    } while (!dec_pkt_done);

    return ret;
}

static MPP_RET rk_mpi_encode_run(MpiRc2Ctx *ctx, MppTask_out enc_out_task, void *enc_out_task_param)
{
    MPP_RET ret = MPP_OK;

    MppApi *mpi = ctx->dec_mpi;
    MppCtx dec_ctx = ctx->dec_ctx;
    MppFrame frm = NULL;

    do {
        ret = mpi->decode_get_frame(dec_ctx, &frm);
        if (ret) {
            fprintf(stderr, "decode_get_frame failed ret %d\n", ret);
            break;
        }

        if (frm) {
            ctx->frm_eos = mpp_frame_get_eos(frm);
            if (mpp_frame_get_info_change(frm)) {
                mpi->control(dec_ctx, MPP_DEC_SET_INFO_CHANGE_READY, NULL);
            } else {
                void *ptr;
                size_t len;

                ctx->enc_mpi->encode_put_frame(ctx->enc_ctx, frm);
                ctx->enc_mpi->encode_get_packet(ctx->enc_ctx, &ctx->enc_pkt);

                if (ctx->enc_pkt) {
                    len = mpp_packet_get_length(ctx->enc_pkt);
                    ptr = mpp_packet_get_pos(ctx->enc_pkt);

                    if (enc_out_task)
                        enc_out_task(ptr, len, enc_out_task_param);

                    if (mpp_packet_has_meta(ctx->enc_pkt)) {
                        MppFrame frame = NULL;
                        MppMeta meta = mpp_packet_get_meta(ctx->enc_pkt);
                        mpp_meta_get_frame(meta,  KEY_INPUT_FRAME, &frame);
                        if (frame) {
                            frm = frame;
                        }
                    }
                    ctx->enc_pkt_eos = mpp_packet_get_eos(ctx->enc_pkt);
                    ctx->frm_idx++;
                    mpp_packet_deinit(&ctx->enc_pkt);
                }
            }

            mpp_frame_deinit(&frm);
            frm = NULL;
        } else {
            // usleep(3000);
            usleep(10000);

            if (ctx->pkt_eos)
                break;
            else
                continue;
        }

        if (ctx->enc_pkt_eos)
            break;
    } while (1);

    return ret;
}

static int rk_codec_put_decdata(rkcodec_t *rkcodec, RK_U8 *data, RK_U32 datal)
{
    if (!rkcodec || !rkcodec->rc2ctx)
        return MPP_ERR_NULL_PTR;

    if (!data || !datal)
        return MPP_ERR_VALUE;

    return rk_mpi_decode_data_put(rkcodec->rc2ctx, data, datal);
}

static void *rk_codec_encode_run(void *arg)
{
    MpiRc2Ctx *ctx = arg;

    while (!ctx->pkt_eos) {
        rk_mpi_encode_run(ctx, ctx->enc_out_task, ctx->enc_out_task_param);
    }

    return NULL;
}

static int rk_codec_enc_out_register(rkcodec_t *rkcodec, MppTask_out enc_out_task, void *enc_out_param)
{
    int r;
    pthread_t enc_tid;

    if (!rkcodec || !rkcodec->rc2ctx)
        return MPP_ERR_NULL_PTR;

    rkcodec->rc2ctx->enc_out_task = enc_out_task;
    rkcodec->rc2ctx->enc_out_task_param = enc_out_param;

    r = pthread_create(&enc_tid, NULL, rk_codec_encode_run, rkcodec->rc2ctx);
    if (r)
        return MPP_ERR_INIT;
    pthread_detach(enc_tid);

    return MPP_OK;
}

static int rk_codec_create(rkcodec_t *rkcodec, RK_S32 width, RK_S32 height, RK_U32 fps_in, RK_U32 fps_out)
{
    MPP_RET ret;

    if (!rkcodec)
        return MPP_ERR_NULL_PTR;

    if (!rkcodec->rc2ctx)
        return MPP_ERR_NULL_PTR;

    ret = rc2ctx_set_value(rkcodec->rc2ctx, width, height, fps_in, fps_out);
    if (ret != MPP_OK)
        goto err0;

    ret = rk_mpi_rc_buffer_init(rkcodec->rc2ctx);
    if (ret != MPP_OK)
        goto err0;

    ret = rk_mpi_rc_dec_init(rkcodec->rc2ctx);
    if (ret != MPP_OK)
        goto err1;

    ret = mpp_enc_cfg_init(&rkcodec->rc2ctx->cfg);
    if (ret != MPP_OK)
        goto err2;

    ret = rk_mpi_rc_enc_init(rkcodec->rc2ctx);
    if (ret != MPP_OK)
        goto err3;

    return MPP_OK;

err3:
    mpp_enc_cfg_deinit(rkcodec->rc2ctx->cfg);
    rkcodec->rc2ctx->cfg = NULL;
err2:
    rk_mpi_rc_dec_deinit(rkcodec->rc2ctx);
err1:
    rk_mpi_rc_buffer_deinit(rkcodec->rc2ctx);
err0:
    return ret;
}

static int rk_codec_destroy(rkcodec_t *rkcodec)
{
    if (!rkcodec)
        return MPP_ERR_NULL_PTR;

    if (!rkcodec->rc2ctx)
        return MPP_ERR_NULL_PTR;

    rkcodec->rc2ctx->pkt_eos = 1;

    rk_mpi_rc_enc_deinit(rkcodec->rc2ctx);

    if (rkcodec->rc2ctx->cfg) {
        mpp_enc_cfg_deinit(rkcodec->rc2ctx->cfg);
        rkcodec->rc2ctx->cfg = NULL;
    }

    rk_mpi_rc_buffer_deinit(rkcodec->rc2ctx);

    rk_mpi_rc_dec_deinit(rkcodec->rc2ctx);

    return MPP_OK;
}

int rk_codec_init(rkcodec_t *rkcodec)
{
    if (!rkcodec)
        return MPP_ERR_NULL_PTR;

    rkcodec->rc2ctx = malloc(sizeof(MpiRc2Ctx));
    if (!rkcodec->rc2ctx)
        return MPP_ERR_MALLOC;

    rkcodec->create = rk_codec_create;
    rkcodec->destroy = rk_codec_destroy;
    rkcodec->put_decdata = rk_codec_put_decdata;
    rkcodec->enc_out_register = rk_codec_enc_out_register;

    return MPP_OK;
}

int rk_codec_deinit(rkcodec_t *rkcodec)
{
    if (!rkcodec)
        return MPP_ERR_NULL_PTR;

    if (rkcodec->rc2ctx) {
        free(rkcodec->rc2ctx);
        rkcodec->rc2ctx = NULL;
    }

    return MPP_OK;
}
