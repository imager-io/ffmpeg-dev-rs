// Copyright (c) 2019 Colbyn Wadman
// 
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
// 
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#include <stdlib.h>
#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libavutil/imgutils.h>
#include <libavformat/avformat.h>


///////////////////////////////////////////////////////////////////////////////
// BASICS
///////////////////////////////////////////////////////////////////////////////

typedef struct {
    int width;
    int height;
} Resolution;

///////////////////////////////////////////////////////////////////////////////
// GENERIC BUFFERS
///////////////////////////////////////////////////////////////////////////////

typedef struct {
    size_t *size;
    char *data;
    FILE *stream;
} WriteOnlyBuffer;

typedef struct {
    size_t size;
    char *data;
    FILE *stream;
} ReadOnlyBuffer;

WriteOnlyBuffer new_write_only_buffer() {
    WriteOnlyBuffer buffer;
    buffer.size = malloc(sizeof(size_t));
    buffer.data = malloc(sizeof(char));
    buffer.stream = open_memstream(&buffer.data, buffer.size);
    assert(buffer.stream);
    return buffer;
}

ReadOnlyBuffer close_write_only_buffer(WriteOnlyBuffer input_buffer) {
    // INIT BUFFER
    size_t buffer_size = *input_buffer.size;
    char *new_buffer = calloc(buffer_size, sizeof(char));
    memcpy(new_buffer, input_buffer.data, buffer_size);

    // INIT STREAM
    FILE *new_stream = fmemopen(new_buffer, buffer_size, "r");

    // CLEANUP
    assert(fclose(input_buffer.stream) == 0);
    free(input_buffer.data);
    
    // DONE
    ReadOnlyBuffer output_buffer = {
        .size = buffer_size,
        .data = new_buffer,
        .stream = new_stream
    };
    return output_buffer;
}


bool wb_append(const void *ptr, size_t size_of_unit, size_t number_of_units, WriteOnlyBuffer wb) {
    assert(fwrite(ptr, size_of_unit, number_of_units, wb.stream));
    assert(fflush (wb.stream) == 0);
    return true;
}

bool rb_close(ReadOnlyBuffer buffer) {
    free(buffer.data);
    assert(fclose(buffer.stream) == 0);
    return true;
}

bool rb_save(ReadOnlyBuffer buffer, const char *filepath) {
    FILE *file = fopen(filepath, "w");
    fwrite(buffer.data, 1, buffer.size, file);
    assert(fclose(file) == 0);
    return true;
}


ReadOnlyBuffer rb_new(char *source_data, size_t source_size) {
    WriteOnlyBuffer w_buffer = new_write_only_buffer();
    wb_append(source_data, 1, source_size, w_buffer);
    return close_write_only_buffer(w_buffer);
}

ReadOnlyBuffer rb_open(const char *filepath) {
    FILE *file = fopen(filepath, "r");
    assert(file);
    
    // GET FILE SIZE
    size_t lSize;
    fseek(file , 0 , SEEK_END);
    lSize = ftell (file);
    rewind(file);

    // INIT BUFFER
    char *buffer = (char*) malloc (sizeof(char)*lSize);
    assert(buffer);

    // COPY DATA
    size_t result = fread(buffer, 1, lSize, file);
    assert(result == lSize);

    // RESET
    rewind(file);
    
    // DONE
    ReadOnlyBuffer output_buffer = {
        .size = lSize,
        .data = buffer,
        .stream = file
    };
    return output_buffer;
}


///////////////////////////////////////////////////////////////////////////////
// 8BIT BUFFER UTILS
///////////////////////////////////////////////////////////////////////////////

ReadOnlyBuffer rb_from_u8(uint8_t *source_data, size_t source_size) {
    WriteOnlyBuffer w_buffer = new_write_only_buffer();
    assert(wb_append(source_data, 1, source_size, w_buffer));
    return close_write_only_buffer(w_buffer);
}


void rb_fill_u8(uint8_t **o_buffer, size_t *o_buffer_size, ReadOnlyBuffer input) {
    size_t output_size = input.size;
    uint8_t *output_buffer = calloc(output_size, sizeof(uint8_t));
    memcpy(output_buffer, input.data, output_size);
    *o_buffer = output_buffer;
    *o_buffer_size = output_size;
}



///////////////////////////////////////////////////////////////////////////////
// CUSTOM IO CONTEXT
///////////////////////////////////////////////////////////////////////////////
struct InternalMuxBuffer {
    char *ptr;
    size_t size_left; ///< size left in the buffer
};

static int mp4_internal_read_packet(void *opaque, uint8_t *buf, int buf_size) {
    // INIT
    struct InternalMuxBuffer *bd = (struct InternalMuxBuffer *)opaque;
    buf_size = FFMIN((size_t) buf_size, bd->size_left);
    // CHECK
    if (!buf_size) {
        return AVERROR_EOF;
    }
    // COPY & UPDATE
    memcpy(buf, bd->ptr, buf_size);
    bd->ptr  += buf_size;
    bd->size_left -= buf_size;
    // DONE
    return buf_size;
}

///////////////////////////////////////////////////////////////////////////////
// MP4 PACKAGER
///////////////////////////////////////////////////////////////////////////////

ReadOnlyBuffer mp4_packager(ReadOnlyBuffer input_buffer) {
    // INPUT BUFFER
    size_t avio_ctx_buffer_size = 4096;
    uint8_t *avio_ctx_buffer = av_malloc(avio_ctx_buffer_size);
    struct InternalMuxBuffer bd = { 0 };
    bd.ptr  = input_buffer.data;
    bd.size_left = input_buffer.size;
    memcpy(bd.ptr, input_buffer.data, input_buffer.size);
    AVIOContext *avio_ctx = avio_alloc_context(
        avio_ctx_buffer,
        avio_ctx_buffer_size,
        0,
        &bd,
        &mp4_internal_read_packet,
        NULL,
        NULL
    );


    // INPUT CONTEXT
    AVFormatContext *ifmt_ctx = avformat_alloc_context();
    ifmt_ctx->pb = avio_ctx;
    assert(avformat_open_input(&ifmt_ctx, NULL, NULL, NULL) >= 0);
    assert(avformat_find_stream_info(ifmt_ctx, 0) >= 0);
    // av_dump_format(ifmt_ctx, 0, NULL, 0);


    // OUTPUT CONTEXT
    AVFormatContext *ofmt_ctx = NULL;
    avformat_alloc_output_context2(&ofmt_ctx, NULL, "mp4", NULL);
    assert(ofmt_ctx);

    // I/O ENCODED STREAMS
    int *stream_mapping = NULL;
    int stream_mapping_size = 0;
    stream_mapping_size = ifmt_ctx->nb_streams;
    stream_mapping = av_mallocz_array(stream_mapping_size, sizeof(*stream_mapping));
    assert(stream_mapping);

    assert(ifmt_ctx->nb_streams == 1);
    {
        int stream_index = 0;
        AVStream *out_stream;
        AVStream *in_stream = ifmt_ctx->streams[stream_index];
        AVCodecParameters *in_codecpar = in_stream->codecpar;

        assert(in_codecpar->codec_type == AVMEDIA_TYPE_VIDEO);

        out_stream = avformat_new_stream(ofmt_ctx, NULL);
        assert(out_stream);

        assert(avcodec_parameters_copy(out_stream->codecpar, in_codecpar) >= 0);
        
        #pragma clang diagnostic ignored "-Wdeprecated-declarations"
        enum AVCodecID codec_id = ifmt_ctx->streams[stream_index]->codec->codec_id;
        if (codec_id == AV_CODEC_ID_HEVC) {
            ofmt_ctx->strict_std_compliance = -1;
            out_stream->codecpar->codec_tag = MKTAG('h', 'v', 'c', '1');
        } else {
            out_stream->codecpar->codec_tag = 0;
        }
    }
    // av_dump_format(ofmt_ctx, 0, NULL, 1);

    // INIT OUTPUT
    assert(!(ofmt_ctx->oformat->flags & AVFMT_NOFILE));
    assert(avio_open_dyn_buf(&ofmt_ctx->pb) == 0);    
    assert(avformat_write_header(ofmt_ctx, NULL) >= 0);
    

    // LOOP
    AVPacket pkt;
    while (av_read_frame(ifmt_ctx, &pkt) == 0) {
        AVStream *in_stream;
        AVStream *out_stream;

        in_stream  = ifmt_ctx->streams[pkt.stream_index];
        if (pkt.stream_index >= stream_mapping_size || stream_mapping[pkt.stream_index] < 0) {
            av_packet_unref(&pkt);
            continue;
        }

        pkt.stream_index = stream_mapping[pkt.stream_index];
        out_stream = ofmt_ctx->streams[pkt.stream_index];

        // COPY PACKET
        pkt.pts = av_rescale_q_rnd(pkt.pts, in_stream->time_base, out_stream->time_base, AV_ROUND_NEAR_INF|AV_ROUND_PASS_MINMAX);
        pkt.dts = av_rescale_q_rnd(pkt.dts, in_stream->time_base, out_stream->time_base, AV_ROUND_NEAR_INF|AV_ROUND_PASS_MINMAX);
        pkt.duration = av_rescale_q(pkt.duration, in_stream->time_base, out_stream->time_base);
        pkt.pos = -1;

        assert(av_write_frame(ofmt_ctx, &pkt) >= 0);
        av_packet_unref(&pkt);
    }

    // FINALIZE OUTPUT
    av_write_trailer(ofmt_ctx);

    // COPY OUTPUT
    uint8_t *output_buffer;
    int output_buffer_size = avio_close_dyn_buf(ofmt_ctx->pb, &output_buffer);
    assert(output_buffer_size >= 0);
    ReadOnlyBuffer result_buffer = rb_from_u8(output_buffer, output_buffer_size);
    free(output_buffer);

    // CLEANUP
    av_freep(&avio_ctx->buffer);
    avio_context_free(&avio_ctx);
    avformat_close_input(&ifmt_ctx);
    avformat_free_context(ofmt_ctx);
    av_freep(&stream_mapping);

    // DONE
    return result_buffer;
}


void sys_mp4_packager(
    u_int8_t *in, size_t in_size,
    uint8_t **out, size_t *out_size
) {
    ReadOnlyBuffer source_buffer = rb_from_u8(in, in_size);
    ReadOnlyBuffer output_buffer = mp4_packager(source_buffer);
    rb_fill_u8(out, out_size, output_buffer);
    assert(rb_close(source_buffer));
    assert(rb_close(output_buffer));
}


///////////////////////////////////////////////////////////////////////////////
// DECODED MEDIA TYPES
///////////////////////////////////////////////////////////////////////////////

typedef enum {RAWVIDEO, IMAGE} Format;

typedef struct {
    int width;
    int height;
    int linesize[4];
    int bufsize;
    enum AVPixelFormat pix_fmt;
    uint8_t *data[4];
} DecodedMedia;

// Internal Context
typedef struct {
    AVDictionary *demux_ops;

    AVFormatContext *fmt_ctx;
    AVCodecContext *video_dec_ctx;
    int width;
    int height;
    enum AVPixelFormat pix_fmt;
    AVStream *video_stream;
    const char *src_filename;
    const char *video_dst_filename;
    FILE *video_dst_file;

    uint8_t *video_dst_data[4];
    int video_dst_linesize[4];
    int video_dst_bufsize;

    int video_stream_idx;
    AVFrame *frame;
    AVPacket pkt;
    int video_frame_count;
    int refcount;
} Decoder;

typedef struct {
    Resolution *source_resolution;
    const char *source_pixel_format;
    Format *source_format;
} DecoderOptions;


///////////////////////////////////////////////////////////////////////////////
// MISC DECODER HELPERS
///////////////////////////////////////////////////////////////////////////////

DecoderOptions empty_decoder_options() {
    DecoderOptions ops = {
        .source_resolution = NULL,
        .source_format = NULL,
        .source_pixel_format = NULL
    };
    return ops;
}

DecoderOptions yuv_decoder_options(int width, int height) {
    Resolution source_resolution = {
        .width = width,
        .height = height
    };
    Format source_format = RAWVIDEO;
    DecoderOptions ops = {
        .source_resolution = &source_resolution,
        .source_format = &source_format
    };
    return ops;
}



ReadOnlyBuffer rb_from_decoded_media(DecodedMedia media) {
    WriteOnlyBuffer w_buffer = new_write_only_buffer();
    assert(wb_append(media.data[0], 1, media.bufsize, w_buffer));
    return close_write_only_buffer(w_buffer);
}



void decoded_media_save(DecodedMedia media, const char *filepath) {
    FILE *f = fopen(filepath, "w");
    fwrite(media.data[0], 1, media.bufsize, f);
    fclose(f);
}

void decoded_media_free(DecodedMedia *media) {
    av_free(media->data[0]);
}

void print_ffplay_help(DecodedMedia media, const char *filepath) {
    printf(
        "\nffplay -pixel_format %s -video_size %dx%d -i %s\n",
        av_get_pix_fmt_name(media.pix_fmt),
        media.width,
        media.height,
        filepath
    );
}

DecodedMedia clone_decoded_media(DecodedMedia source) {
    DecodedMedia output;
    int video_dst_bufsize = av_image_alloc(
        output.data,
        output.linesize,
        source.width,
        source.height,
        source.pix_fmt,
        1
    );
    assert(video_dst_bufsize == source.bufsize);
    av_image_copy(
        output.data,
        output.linesize,
        (const uint8_t **)(source.data),
        source.linesize,
        source.pix_fmt,
        source.width,
        source.height
    );
    output.bufsize = source.bufsize;
    output.width = source.width;
    output.height = source.height;
    output.pix_fmt = source.pix_fmt;
    return output;
}

DecodedMedia mk_random_image() {
    const int width = 1000;
    const int height = 1000;
    const enum AVPixelFormat pix_fmt = AV_PIX_FMT_YUV420P;
    DecodedMedia output;
    output.width = width;
    output.height = height;
    output.pix_fmt = pix_fmt;
    output.bufsize = av_image_alloc(
        output.data,
        output.linesize,
        width,
        height,
        pix_fmt,
        1
    );
    return output;
}

///////////////////////////////////////////////////////////////////////////////
// LOW-LEVEL DECODER
///////////////////////////////////////////////////////////////////////////////

#pragma clang diagnostic ignored "-Wunused-parameter"
static int decode_packet(
    Decoder *decoder,
    DecodedMedia *output_buffer,
    int *got_frame,
    int cached,
    bool *error
) {
    int decoded = decoder->pkt.size;
    *got_frame = 0;
    if (decoder->pkt.stream_index == decoder->video_stream_idx) {
        #pragma clang diagnostic ignored "-Wdeprecated-declarations"
        int res = avcodec_decode_video2(
            decoder->video_dec_ctx,
            decoder->frame,
            got_frame,
            &decoder->pkt
        );

        if (res < 0) {
            if (error != NULL) {
                *error = true;
            } else {
                assert(res >= 0);
            }
        }

        if (*got_frame) {
            assert(decoder->video_dst_bufsize);
            assert(decoder->frame->width == decoder->width);
            assert(decoder->frame->height == decoder->height);
            assert(decoder->frame->format == decoder->pix_fmt);

            // COPY DECODED FRAME TO DESTINATION BUFFER;
            // THIS IS REQUIRED SINCE RAWVIDEO EXPECTS NON ALIGNED DATA
            av_image_copy(
                output_buffer->data,
                output_buffer->linesize,
                (const uint8_t **)(decoder->frame->data),
                decoder->frame->linesize,
                decoder->frame->format,
                decoder->frame->width,
                decoder->frame->height
            );
            output_buffer->bufsize = decoder->video_dst_bufsize;
            output_buffer->width = decoder->frame->width;
            output_buffer->height = decoder->frame->height;
            output_buffer->pix_fmt = decoder->frame->format;
        }
    }

    // IF WE USE FRAME REFERENCE COUNTING;
    // WE OWN THE DATA AND NEED TO DE-REFERENCE
    // IT WHEN WE DON'T USE IT ANYMORE
    if (*got_frame && decoder->refcount) {
        av_frame_unref(decoder->frame);
    }

    return decoded;
}

void open_codec_context(Decoder *decoder) {
    enum AVMediaType type = AVMEDIA_TYPE_VIDEO;
    int stream_index;
    AVStream *st;
    AVCodec *dec = NULL;
    AVDictionary *opts = NULL;

    stream_index = av_find_best_stream(decoder->fmt_ctx, type, -1, -1, NULL, 0);
    assert(stream_index >= 0);
    {
        st = decoder->fmt_ctx->streams[stream_index];

        // FIND DECODER FOR THE STREAM
        dec = avcodec_find_decoder(st->codecpar->codec_id);
        assert(dec);

        // ALLOCATE A CODEC CONTEXT FOR THE 'ACTUAL' DECODER
        decoder->video_dec_ctx = avcodec_alloc_context3(dec);
        assert(decoder->video_dec_ctx);

        // COPY CODEC PARAMETERS FROM INPUT STREAM TO OUTPUT CODEC CONTEXT
        assert(avcodec_parameters_to_context(decoder->video_dec_ctx, st->codecpar) >= 0);

        // INIT THE DECODERS, WITH OR WITHOUT REFERENCE COUNTING
        av_dict_set(&opts, "refcounted_frames", decoder->refcount ? "1" : "0", 0);
        assert(avcodec_open2(decoder->video_dec_ctx, dec, &opts ) == 0);
        decoder->video_stream_idx = stream_index;
    }
}


struct InternalDecoderBuffer {
    char *ptr;
    size_t size_left; ///< size left in the buffer
};

static int read_packet(void *opaque, uint8_t *buf, int buf_size) {
    // INIT
    struct InternalDecoderBuffer *bd = (struct InternalDecoderBuffer *)opaque;
    buf_size = FFMIN((size_t) buf_size, bd->size_left);
    // CHECK
    if (!buf_size) {
        return AVERROR_EOF;
    }
    // COPY & UPDATE
    memcpy(buf, bd->ptr, buf_size);
    bd->ptr  += buf_size;
    bd->size_left -= buf_size;
    // DONE
    return buf_size;
}


DecodedMedia decode_media(ReadOnlyBuffer input_buffer, DecoderOptions decode_ops, bool *decode_error) {
    // OUTPUT
    // DecodedMedia *output_buffer = calloc(1, sizeof(DecodedMedia));
    DecodedMedia output_buffer;

    // INIT DECODER CONTEXT
    int ret;
    int got_frame;
    Decoder *decoder = calloc(1, sizeof(Decoder));
    decoder->video_stream_idx = -1;
    decoder->video_frame_count = 0;
    decoder->refcount = 0;
    decoder->src_filename = NULL;
    decoder->video_dst_filename = NULL;

    // INIT DE-MUXER
    decoder->fmt_ctx = avformat_alloc_context();

    // INIT OPTIONAL DE-MUXER OPTIONS
    if (decode_ops.source_resolution) {
        int width = decode_ops.source_resolution->width;
        int height = decode_ops.source_resolution->height;
        assert(width && height);

        char video_size[100];
        snprintf(video_size, 100, "%dx%d", width, height);

        assert(av_dict_set(&decoder->demux_ops, "video_size", video_size, 0) >= 0);
    }

    if (decode_ops.source_pixel_format) {
        assert(av_dict_set(&decoder->demux_ops, "pixel_format", decode_ops.source_pixel_format, 0) >= 0);
    }

    if (decode_ops.source_format) {
        Format source_format = *decode_ops.source_format;
        assert(
            (source_format == RAWVIDEO) ||
            (source_format == IMAGE)
        );

        if (source_format == RAWVIDEO) {
            decoder->fmt_ctx->iformat = av_find_input_format("rawvideo");
        } else if (source_format == IMAGE) {
            decoder->fmt_ctx->iformat = av_find_input_format("image2pipe");
        }
        assert(decoder->fmt_ctx->iformat);
    }

    
    // INIT CUSTOM AV-IO-CONTEXT
    size_t avio_ctx_buffer_size = 4096;
    uint8_t *avio_ctx_buffer = av_malloc(avio_ctx_buffer_size);
    struct InternalDecoderBuffer bd = { 0 };
    bd.ptr  = input_buffer.data;
    bd.size_left = input_buffer.size;
    memcpy(bd.ptr, input_buffer.data, input_buffer.size);
    AVIOContext *avio_ctx = avio_alloc_context(
        avio_ctx_buffer,
        avio_ctx_buffer_size,
        0,
        &bd,
        &read_packet,
        NULL,
        NULL
    );
    assert(avio_ctx);

    // OPEN INPUT
    decoder->fmt_ctx->pb = avio_ctx;
    assert(avformat_open_input(&decoder->fmt_ctx, NULL, NULL, &decoder->demux_ops) >= 0);


    // RETRIEVE STREAM INFORMATION
    if (avformat_find_stream_info(decoder->fmt_ctx, NULL) < 0) {
        fprintf(stderr, "Could not find stream information\n");
        exit(1);
    }

    // INIT CODEC-DECODER
    open_codec_context(decoder);

    // SET CODEC-DECODER STREAM
    decoder->video_stream = decoder->fmt_ctx->streams[decoder->video_stream_idx];

    // ALLOCATE IMAGE WHERE THE DECODED IMAGE WILL BE PUT
    decoder->width = decoder->video_dec_ctx->width;
    decoder->height = decoder->video_dec_ctx->height;
    decoder->pix_fmt = decoder->video_dec_ctx->pix_fmt;
    decoder->video_dst_bufsize = av_image_alloc(
        output_buffer.data,
        output_buffer.linesize,
        decoder->width,
        decoder->height,
        decoder->pix_fmt,
        1
    );
    assert(decoder->video_dst_bufsize);

    // DUMP INPUT INFORMATION TO STDERR
    // av_dump_format(decoder->fmt_ctx, 0, decoder->src_filename, 0);
    assert(decoder->video_stream);

    decoder->frame = av_frame_alloc();
    assert(decoder->frame);

    // INITIALIZE PACKET, SET DATA TO NULL, LET THE DEMUXER FILL IT
    av_init_packet(&decoder->pkt);
    decoder->pkt.data = NULL;
    decoder->pkt.size = 0;

    // READ FRAMES FROM THE FILE
    if (decode_error != NULL) {
        *decode_error = false;
    }
    
    while (av_read_frame(decoder->fmt_ctx, &decoder->pkt) >= 0) {
        AVPacket orig_pkt = decoder->pkt;
        do {
            ret = decode_packet(
                decoder,
                &output_buffer,
                &got_frame,
                0,
                decode_error
            );
            if (ret < 0)
                break;
            decoder->pkt.data += ret;
            decoder->pkt.size -= ret;
        } while (decoder->pkt.size > 0);
        av_packet_unref(&orig_pkt);
    }


    // FLUSH CACHED FRAMES
    decoder->pkt.data = NULL;
    decoder->pkt.size = 0;
    do {
        decode_packet(
            decoder,
            &output_buffer,
            &got_frame,
            1,
            decode_error
        );
    } while (got_frame);

    // END


    // CLEANUP
    av_dict_free(&decoder->demux_ops);
    avcodec_free_context(&decoder->video_dec_ctx);
    avformat_close_input(&decoder->fmt_ctx);
    if (decoder->video_dst_file) {
        fclose(decoder->video_dst_file);
    }
    av_frame_free(&decoder->frame);
    av_free(decoder->video_dst_data[0]);
    // NOTE: THE INTERNAL BUFFER COULD HAVE CHANGED, AND BE != AVIO_CTX_BUFFER
    if (avio_ctx) {
        av_freep(&avio_ctx->buffer);
    }
    avio_context_free(&avio_ctx);

    // DONE
    return output_buffer;
}



///////////////////////////////////////////////////////////////////////////////
// HIGHER-LEVEL AD-HOCK DECODER VARIANTS
///////////////////////////////////////////////////////////////////////////////
DecodedMedia decode_h264_media(ReadOnlyBuffer input_buffer) {
    DecodedMedia result = decode_media(input_buffer, empty_decoder_options(), NULL);
    return result;
}

DecodedMedia decode_hevc_media(ReadOnlyBuffer input_buffer) {
    DecodedMedia result = decode_media(input_buffer, empty_decoder_options(), NULL);
    return result;
}

DecodedMedia decode_yuv_media(ReadOnlyBuffer input_buffer, int width, int height) {
    DecodedMedia result = decode_media(input_buffer, yuv_decoder_options(width, height), NULL);
    return result;
}

DecodedMedia decode_image_media(ReadOnlyBuffer input_buffer) {
    DecoderOptions options = empty_decoder_options();
    Format source_format = IMAGE;
    options.source_format = &source_format;
    DecodedMedia result = decode_media(input_buffer, options, NULL);
    return result;
}

DecodedMedia decode_jpeg_media_yuvj420p(ReadOnlyBuffer input_buffer, int width, int height, bool *decode_error) {
    // INIT
    DecoderOptions options = empty_decoder_options();
    // CONFIG
    Format source_format = IMAGE;
    Resolution source_resolution = {
        .width = width,
        .height = height
    };
    // APPLY
    options.source_format = &source_format;
    options.source_resolution = &source_resolution;
    options.source_pixel_format = "yuvj420p";
    // GO!
    return decode_media(input_buffer, options, decode_error);
}


///////////////////////////////////////////////////////////////////////////////
// TRANSFORM
///////////////////////////////////////////////////////////////////////////////

typedef struct {
    int new_width;
    int new_height;
    enum AVPixelFormat new_pix_fmt;
} TransformArguments;


DecodedMedia transform_media(DecodedMedia old_image, TransformArguments param) {
    // NEW IMAGE
    DecodedMedia new_image;
    new_image.width = param.new_width;
    new_image.height = param.new_height;
    new_image.pix_fmt = param.new_pix_fmt;

    // GO
    struct SwsContext *sws_ctx = sws_getContext(
        old_image.width,
        old_image.height,
        old_image.pix_fmt,
        new_image.width,
        new_image.height,
        new_image.pix_fmt,
        0,
        NULL,
        NULL,
        NULL
    );
    assert(sws_ctx);
    int buffer_size = av_image_alloc(
        new_image.data,
        new_image.linesize,
        new_image.width,
        new_image.height,
        new_image.pix_fmt,
        1
    );
    assert(buffer_size >= 0);
    new_image.bufsize = buffer_size;
    int out_slice_height = sws_scale(
        sws_ctx,
        (const uint8_t * const*) old_image.data,
        old_image.linesize,
        0,
        old_image.height,
        new_image.data,
        new_image.linesize
    );
    assert(out_slice_height >= 0);

    // CLEANUP
    sws_freeContext(sws_ctx);

    // DONE
    return new_image;
}


DecodedMedia transform_media_to_yuv(DecodedMedia old_image) {
    // CURRENT INVARIANTS
    assert(old_image.width % 2 == 0);
    assert(old_image.height % 2 == 0);
    
    // if (old_image.width >= )

    TransformArguments params = {
        .new_width = old_image.width,
        .new_height = old_image.height,
        .new_pix_fmt = AV_PIX_FMT_YUV420P
    };
    DecodedMedia yuv_image = transform_media(old_image, params);
    assert(yuv_image.pix_fmt == AV_PIX_FMT_YUV420P);
    return yuv_image;
}



///////////////////////////////////////////////////////////////////////////////
// RUST FFI
///////////////////////////////////////////////////////////////////////////////

