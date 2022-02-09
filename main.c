#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <stdbool.h>
#include <unistd.h>

#include "rkcodec.h"

#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"

static FILE *outfile = NULL;

static void rk_encode_out_routine(RK_U8 *data, RK_S32 len, void *param)
{
    fprintf(stderr, "out : %d\n", len);
    if (outfile)
        fwrite(data, len, 1, outfile);
}

static void camera_status_update(bool status, uint16_t hor, uint16_t ver, uint32_t num, uint32_t den)
{
    fprintf(stderr, "Camera status      : %s\n", status ? "true" : "false");
    fprintf(stderr, "       horizontal  : %u\n", hor);
    fprintf(stderr, "       vertical    : %u\n", ver);
    fprintf(stderr, "       numerator   : %u\n", num);
    fprintf(stderr, "       denominator : %u\n", den);
}

void ipcamera_routine(void *arg)
{
    int r;
    int i;
    int videoindex;

    AVFormatContext *format_ctx = NULL;
    AVDictionary *format_opts = NULL;
    AVCodecParameters *codecpar = NULL;
    AVCodecContext *codec_ctx = NULL;
    AVPacket *packet = NULL;
    AVCodecContext *avctx = NULL;
    AVStream *st = NULL;

    rkcodec_t rkcodec;

    char url[] = "rtsp://192.168.12.63:8554";

    outfile = fopen("out.h265", "w+");

    avformat_network_init();

    format_ctx = avformat_alloc_context();
    if (!format_ctx)
        goto err0;

    avctx = avcodec_alloc_context3(NULL);
    if (!avctx)
        goto err1;

    packet = av_packet_alloc();
    if (!packet)
        goto err2;
    av_init_packet(packet);

    av_dict_set(&format_opts, "stimeout", "200000000", 0);
    av_dict_set(&format_opts, "rtsp_transport", "udp", 0);
    av_dict_set(&format_opts, "muxdelay", "1", 0);
    av_dict_set(&format_opts, "buffer_size", "1024000", 0);
    av_dict_set(&format_opts, "thread_queue_size", "512", 0);

    for (;;) {
        sleep(1);

        r = avformat_open_input(&format_ctx, url, NULL, &format_opts);
        if (r) {
            goto StatUpdate;
        }

        r = avformat_find_stream_info(format_ctx, NULL);
        if (r <  0) {
            goto forNext;
        }

        videoindex = -1;
	    for (i = 0; i< format_ctx->nb_streams; i++) {
            if (format_ctx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
	    		videoindex = i;
	    		break;
	    	}
        }

        if (videoindex == -1) {
            goto forNext;
	    }

        st = format_ctx->streams[videoindex];
        codecpar = format_ctx->streams[videoindex]->codecpar;

        r = avcodec_parameters_to_context(avctx, codecpar);
        if (r) {
            goto freeAvctx;
        }

        if (codecpar->codec_id == AV_CODEC_ID_H264) {
            r = rk_codec_init(&rkcodec);
            if (r) {
                goto forNext;
            }

            r = rkcodec.create(&rkcodec, avctx->width, avctx->height, \
                st->avg_frame_rate.num && st->avg_frame_rate.den ? 
                (st->avg_frame_rate.num/st->avg_frame_rate.den) : 25, 25);
            if (r) {
                goto freeRKCodec;
            }

            r = rkcodec.enc_out_register(&rkcodec, rk_encode_out_routine, avctx);
            if (r)
                goto destroyRKCodec;
        }

        camera_status_update(true, avctx->width, avctx->height, 
                    st->avg_frame_rate.num, st->avg_frame_rate.den);

        while (av_read_frame(format_ctx, packet) >= 0) {
            if (packet->stream_index == videoindex) {
                // fprintf(stderr, "packet->size:%d\n", packet->size);

                if (codecpar->codec_id == AV_CODEC_ID_H264) /* h.264 to h.265 */
                    rkcodec.put_decdata(&rkcodec, packet->data, packet->size);
                else
                    rk_encode_out_routine(packet->data, packet->size, avctx);
            }
            av_packet_unref(packet);
        }

destroyRKCodec:
        if (codecpar->codec_id == AV_CODEC_ID_H264) {
            rkcodec.destroy(&rkcodec);
        }
freeRKCodec:
        if (codecpar->codec_id == AV_CODEC_ID_H264) {
            rk_codec_deinit(&rkcodec);
        }
freeAvctx:
        avcodec_free_context(&avctx);
forNext:
        avformat_close_input(&format_ctx);
StatUpdate:
        camera_status_update(false, 0, 0, 0, 0);
    }

    av_packet_free(&packet);
err2:
    avcodec_free_context(&avctx);
err1:
    avformat_free_context(format_ctx);
err0:
    avformat_network_deinit();
    return;
}

int main(int argc, const char *argv[])
{
    ipcamera_routine(NULL);

    return 0;
}
