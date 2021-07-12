#include "remux.h"


static AVOutputFormat* ofmt;
static AVFormatContext* ifmt_ctx;
static AVFormatContext* ofmt_ctx;
static AVPacket pkt;
static const char* in_filename, * out_filename;
static int ret, i;
static int stream_index = 0;
static int* stream_mapping = NULL;
static int stream_mapping_size = 0;


// Copy pasted and edited from libavformat utils.c as couldn't find a header and doesn't seem to exist in obs-deps
AVChapter* avpriv_new_chapter(int64_t id, AVRational time_base,
    int64_t start, int64_t end, const char* title)
{
    AVChapter* chapter = NULL;
    int i, ret;

    if (end != AV_NOPTS_VALUE && start > end) {
        //av_log(s, AV_LOG_ERROR, "Chapter end time %"PRId64" before start %"PRId64"\n", end, start);
        return NULL;
    }

    if (!chapter) {
        chapter = av_mallocz(sizeof(AVChapter));
        if (!chapter)
            return NULL;
        ret = av_dynarray_add_nofree(&ofmt_ctx->chapters, &ofmt_ctx->nb_chapters, chapter); // automatically adjusts nb_chapters too
        if (ret < 0) {
            av_free(chapter);
            return NULL;
        }
    }
    av_dict_set(&chapter->metadata, "title", title, 0);
    chapter->id = id;
    chapter->time_base = time_base;
    chapter->start = start;
    chapter->end = end;

    return chapter;
}


static void log_packet(const AVFormatContext* fmt_ctx, const AVPacket* pkt, const char* tag)
{
    AVRational* time_base = &fmt_ctx->streams[pkt->stream_index]->time_base;

    printf("%s: pts:%s pts_time:%s dts:%s dts_time:%s duration:%s duration_time:%s stream_index:%d\n",
        tag,
        av_ts2str(pkt->pts), av_ts2timestr(pkt->pts, time_base),
        av_ts2str(pkt->dts), av_ts2timestr(pkt->dts, time_base),
        av_ts2str(pkt->duration), av_ts2timestr(pkt->duration, time_base),
        pkt->stream_index);
}


// Modified example of remuxing
int startRemux(const char* filename, const char* newFilename) {
    ofmt = NULL;
    ifmt_ctx = NULL;
    ofmt_ctx = NULL;
    stream_index = 0;
    stream_mapping = NULL;
    stream_mapping_size = 0;

    in_filename = filename;
    out_filename = newFilename;

    if ((ret = avformat_open_input(&ifmt_ctx, in_filename, 0, 0)) < 0) {
        errorPopup("Could not open input file");
        return end(ret);
    }

    if ((ret = avformat_find_stream_info(ifmt_ctx, 0)) < 0) {
        errorPopup("Failed to retrieve input stream information");
        return end(ret);
    }

    av_dump_format(ifmt_ctx, 0, in_filename, 0);

    avformat_alloc_output_context2(&ofmt_ctx, NULL, NULL, out_filename);
    if (!ofmt_ctx) {
        errorPopup("Could not create output context");
        ret = AVERROR_UNKNOWN;
        return end(ret);

    }

    stream_mapping_size = ifmt_ctx->nb_streams;
    stream_mapping = av_mallocz_array(stream_mapping_size, sizeof(*stream_mapping));
    if (!stream_mapping) {
        ret = AVERROR(ENOMEM);
        return end(ret);
    }

    ofmt = ofmt_ctx->oformat;

    for (i = 0; i < ifmt_ctx->nb_streams; i++) {
        AVStream* out_stream;
        AVStream* in_stream = ifmt_ctx->streams[i];
        AVCodecParameters* in_codecpar = in_stream->codecpar;

        if (in_codecpar->codec_type != AVMEDIA_TYPE_AUDIO &&
            in_codecpar->codec_type != AVMEDIA_TYPE_VIDEO &&
            in_codecpar->codec_type != AVMEDIA_TYPE_SUBTITLE) {
            stream_mapping[i] = -1;
            continue;
        }

        stream_mapping[i] = stream_index++;

        out_stream = avformat_new_stream(ofmt_ctx, NULL);
        if (!out_stream) {
            errorPopup("Failed allocating output stream");
            ret = AVERROR_UNKNOWN;
            return end(ret);
        }

        ret = avcodec_parameters_copy(out_stream->codecpar, in_codecpar);
        if (ret < 0) {
            errorPopup("Failed to copy codec parameters");
            return end(ret);
        }
        out_stream->codecpar->codec_tag = 0;
    }
    av_dump_format(ofmt_ctx, 0, out_filename, 1);

    if (!(ofmt->flags & AVFMT_NOFILE)) {
        ret = avio_open(&ofmt_ctx->pb, out_filename, AVIO_FLAG_WRITE);
        if (ret < 0) {
            errorPopup("Could not open output file");
            return end(ret);
        }
    }
}

int finishRemux() {
    // Write metadata before avformat_write_header()
    ret = avformat_write_header(ofmt_ctx, NULL);
    if (ret < 0) {
        errorPopup("Error occurred when opening output file");
        return end(ret);
    }

    while (1) {
        AVStream* in_stream, * out_stream;

        ret = av_read_frame(ifmt_ctx, &pkt);
        if (ret < 0)
            break;

        in_stream = ifmt_ctx->streams[pkt.stream_index];
        if (pkt.stream_index >= stream_mapping_size ||
            stream_mapping[pkt.stream_index] < 0) {
            av_packet_unref(&pkt);
            continue;
        }

        pkt.stream_index = stream_mapping[pkt.stream_index];
        out_stream = ofmt_ctx->streams[pkt.stream_index];
        // log_packet(ifmt_ctx, &pkt, "in"); // debugging

        /* copy packet */
        pkt.pts = av_rescale_q_rnd(pkt.pts, in_stream->time_base, out_stream->time_base, AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX);
        pkt.dts = av_rescale_q_rnd(pkt.dts, in_stream->time_base, out_stream->time_base, AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX);
        pkt.duration = av_rescale_q(pkt.duration, in_stream->time_base, out_stream->time_base);
        pkt.pos = -1;
        // log_packet(ofmt_ctx, &pkt, "out"); // debugging

        ret = av_interleaved_write_frame(ofmt_ctx, &pkt);
        if (ret < 0) {
            errorPopup("Error muxing packet");
            break;
        }
        av_packet_unref(&pkt);
    }

    av_write_trailer(ofmt_ctx);

    return end(ret);
}

int end(int ret) {

    avformat_close_input(&ifmt_ctx);

    /* close output */
    if (ofmt_ctx && !(ofmt->flags & AVFMT_NOFILE))
        avio_closep(&ofmt_ctx->pb);
    avformat_free_context(ofmt_ctx); // should also free chapters

    av_freep(&stream_mapping);

    if (ret < 0 && ret != AVERROR_EOF) {
        errorPopup("Error occurred");
        return 1;
    }

    return 0;
}