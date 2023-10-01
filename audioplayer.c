#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <alsa/asoundlib.h>
#include <linux/soundcard.h>

#define die(...) {\
                        fprintf(stderr, __VA_ARGS__);\
                        exit(1);\
                 }


int AVSampleFormat_SNDFormat[] = {
    SND_PCM_FORMAT_U8, SND_PCM_FORMAT_S16, SND_PCM_FORMAT_S32,
    SND_PCM_FORMAT_FLOAT, SND_PCM_FORMAT_FLOAT64, SND_PCM_FORMAT_U8,
    SND_PCM_FORMAT_S16, SND_PCM_FORMAT_S32, SND_PCM_FORMAT_FLOAT,
    SND_PCM_FORMAT_FLOAT64, SND_PCM_FORMAT_FLOAT64, SND_PCM_FORMAT_FLOAT64
    };

int main(int argc, char **argv)
{
    if (argc < 2) {
        die("Usage: %s [music_file|-]\n", argv[0]);
    }

    char *url=argv[1];

    if (!strcmp(url, "-")) {
        url = "/dev/stdin";
    }

    AVFormatContext *afctx = avformat_alloc_context();

    if(avformat_open_input(&afctx,url,NULL,NULL)<0){
        die("Could not open file");
    }

    if(avformat_find_stream_info(afctx, NULL)<0){
        die("Could not find file info");
    }

    av_dump_format(afctx,0,url, 0);

    int stream_id = -1;
    int i;

    for(i = 0; i < afctx->nb_streams; i++){
        if(afctx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO){
            stream_id = i;
            break;
        }
    }

    if(stream_id == -1){
        die("Could not find Audio Stream");
    }

    const AVCodec *codec = avcodec_find_decoder(
                                afctx->streams[stream_id]->codecpar->codec_id);

    if(codec==NULL){
        die("cannot find codec!");
    }

    AVCodecContext *ctx = avcodec_alloc_context3(codec);

    if (!ctx) {
        die("failed to allocate AVCodecContext");
    }

    if (avcodec_parameters_to_context(ctx,
            afctx->streams[stream_id]->codecpar) < 0) {
        die("failed to fill the AVCodecContext");
    }

    if(avcodec_open2(ctx, codec, NULL) < 0){
        die("Codec cannot be found");
    }

    enum AVSampleFormat sfmt = ctx->sample_fmt;

    if (sfmt == AV_SAMPLE_FMT_NONE) {
        die("no sample fmt detected\n");
    }

    unsigned int channels = ctx->ch_layout.nb_channels;
    unsigned int sample_rate = ctx->sample_rate;
    int sampleSize = av_get_bytes_per_sample(sfmt);
    int res = 0;
    static snd_pcm_hw_params_t *hwparams;
    static snd_pcm_t *pcmhandle;

    if ((res = snd_pcm_open(&pcmhandle, "default", SND_PCM_STREAM_PLAYBACK, 0))
            < 0) {
        die("can't open wave device: %s\n", snd_strerror(res));
    }

    if ((res = snd_pcm_hw_params_malloc(&hwparams)) < 0) {
        die("hwparams couldn't be queried: %s\n", snd_strerror(res));
    }

    if ((res = snd_pcm_hw_params_any(pcmhandle, hwparams)) < 0) {
        snd_pcm_hw_params_free(hwparams);
        die("sound card can't initialized: %s\n", snd_strerror(res));
    }

    snd_pcm_hw_params_set_format(pcmhandle, hwparams,
                                    AVSampleFormat_SNDFormat[sfmt]);
    snd_pcm_hw_params_set_rate_near(pcmhandle, hwparams, &sample_rate, NULL);
    snd_pcm_hw_params_set_channels_near(pcmhandle, hwparams, &channels);
    snd_pcm_hw_params(pcmhandle, hwparams);
    snd_pcm_hw_params_free(hwparams);
    snd_pcm_prepare(pcmhandle);

    AVPacket *packet = av_packet_alloc();
    AVFrame *frame = av_frame_alloc();

    int buf_siz = 3 * sample_rate * sampleSize * channels; // 3sec data
    uint8_t buffer[buf_siz];
    int count = 0;
    int resend_packet = 0;
    long frames = 0;

    while(av_read_frame(afctx, packet) >= 0)
    {
        if(packet->stream_index == stream_id) {
            RESENDPACKET:
            res = avcodec_send_packet(ctx, packet);

            if (res == AVERROR(EAGAIN)) {
                resend_packet = 1;
            }else if (res < 0) {
                fprintf(stderr, "Error in sendng packet\n");
                break;
            }

            while (res >= 0) {
                res = avcodec_receive_frame(ctx, frame);

                if (res == AVERROR(EAGAIN)) {
                    continue;
                }else if(res == AVERROR_EOF || res < 0) {
                    break;
                }

                if ((buf_siz - count) < frame->linesize[0] * 2) {
                    frames = snd_pcm_bytes_to_frames(pcmhandle, count);

                    while (snd_pcm_writei(pcmhandle, (void **)buffer, frames)
                                            < 0) {
                        snd_pcm_prepare(pcmhandle);
                    }

                    count ^= count;
                }

                for (i = 0; i < frame->nb_samples; i++) {
                    for (int ch = 0; ch < channels; ch++) {
                        memcpy(buffer + count, &frame->data[ch][i*sampleSize],
                                sampleSize);
                        count += sampleSize;
                    }
                }
            }

            if (resend_packet) {
                resend_packet ^= resend_packet;
                goto RESENDPACKET;
            }

        }

        av_frame_unref(frame);
        av_packet_unref(packet);
    }

    frames = snd_pcm_bytes_to_frames(pcmhandle, count);

    while (snd_pcm_writei(pcmhandle, (void **)buffer, frames) < 0) {
        snd_pcm_prepare(pcmhandle);
    }

    snd_pcm_drain(pcmhandle);

    avformat_close_input(&afctx);
    av_packet_free(&packet);
    av_frame_free(&frame);
    avcodec_free_context(&ctx);
    avcodec_close(ctx);
    return 0;
}
