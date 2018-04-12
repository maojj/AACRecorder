// Tutor.FDKAacWrapper.cpp : Defines the exported functions for the DLL application.
//

#include "stdafx.h"

#include <stdlib.h>
#include <stdio.h>
#include <aacenc_lib.h>

#include "aac_enc.h"

typedef struct aacenc
{
    HANDLE_AACENCODER handle;
    AACENC_InfoStruct info;
    int channels;
} aacenc_t;

aacenc_t* aacenc_init(int samplerate, int channels, int bitrate)
{
    CHANNEL_MODE mode;
    aacenc_t* enc;

    switch (channels)
    {
    case 1: mode = MODE_1;
        break;
    case 2: mode = MODE_2;
        break;
    default: return NULL;
    }

    enc = (aacenc_t *)malloc(sizeof(aacenc_t));
    if (!enc)
        return NULL;

    enc->handle = NULL;
    enc->channels = channels;

    if (aacEncOpen(&enc->handle, 0, channels) != AACENC_OK)
    {
        fprintf(stderr, "Unable to open encoder\n");
        goto fail;
    }

    // AAC-LC
    if (aacEncoder_SetParam(enc->handle, AACENC_AOT, AOT_AAC_LC) != AACENC_OK)
    {
        fprintf(stderr, "Unable to set the AOT\n");
        goto fail;
    }

    if (aacEncoder_SetParam(enc->handle, AACENC_SAMPLERATE, samplerate) != AACENC_OK)
    {
        fprintf(stderr, "Unable to set the sample rate\n");
        goto fail;
    }

    if (aacEncoder_SetParam(enc->handle, AACENC_CHANNELMODE, mode) != AACENC_OK)
    {
        fprintf(stderr, "Unable to set the channel mode\n");
        goto fail;
    }

    if (aacEncoder_SetParam(enc->handle, AACENC_BITRATE, bitrate) != AACENC_OK)
    {
        fprintf(stderr, "Unable to set the bitrate\n");
        goto fail;
    }

    if (aacEncoder_SetParam(enc->handle, AACENC_TRANSMUX, TT_MP4_ADTS) != AACENC_OK)
    {
        fprintf(stderr, "Unable to set the ADTS transmux\n");
        goto fail;
    }

    if (aacEncEncode(enc->handle, NULL, NULL, NULL, NULL) != AACENC_OK)
    {
        fprintf(stderr, "Unable to initialize the encoder\n");
        goto fail;
    }

    if (aacEncInfo(enc->handle, &enc->info) != AACENC_OK)
    {
        fprintf(stderr, "Unable to get the encoder info\n");
        goto fail;
    }

    return enc;

fail:
    free(enc);
    return NULL;
}

void aacenc_free(aacenc_t* enc)
{
    if (enc->handle)
    {
        aacEncClose(&enc->handle);
    }
    free(enc);
}

int aacenc_get_input_buffer_size(aacenc_t* enc)
{
    return enc->channels * 2 * enc->info.frameLength;
}

int aacenc_get_max_output_buffer_size(aacenc_t* enc)
{
    return enc->info.maxOutBufBytes;
}

int aacenc_encode(aacenc_t* enc, const void* inbuf, int len, void* outbuf, int outsize)
{
    AACENC_BufDesc in_buf = {0}, out_buf = {0};
    AACENC_InArgs in_args = {0};
    AACENC_OutArgs out_args = {0};
    int in_identifier = IN_AUDIO_DATA;
    int in_size, in_elem_size;
    int out_identifier = OUT_BITSTREAM_DATA;
    int out_size, out_elem_size;
    void *in_ptr, *out_ptr;
    AACENC_ERROR err;

    in_ptr = (void *)inbuf;
    in_size = len;
    in_elem_size = 2;

    in_args.numInSamples = len / 2;
    in_buf.numBufs = 1;
    in_buf.bufs = &in_ptr;
    in_buf.bufferIdentifiers = &in_identifier;
    in_buf.bufSizes = &in_size;
    in_buf.bufElSizes = &in_elem_size;

    out_ptr = outbuf;
    out_size = outsize;
    out_elem_size = 1;
    out_buf.numBufs = 1;
    out_buf.bufs = &out_ptr;
    out_buf.bufferIdentifiers = &out_identifier;
    out_buf.bufSizes = &out_size;
    out_buf.bufElSizes = &out_elem_size;

    if ((err = aacEncEncode(enc->handle, &in_buf, &out_buf, &in_args, &out_args)) != AACENC_OK)
    {
        if (err == AACENC_ENCODE_EOF)
            return 0;
        fprintf(stderr, "Encoding failed\n");
        return -1;
    }

    return out_args.numOutBytes;
}

static aacenc_t* enc;

bool encoder_init(int sampleRate, int channels, int bitRate)
{
    if (enc != NULL)
    {
        encoder_release();
    }

    enc = aacenc_init(sampleRate, channels, bitRate);
    return enc != NULL;
}

void encoder_release()
{
    if (enc == NULL)
    {
        return;
    }

    aacenc_free(enc);
    enc = NULL;
}

int get_input_buffer_size()
{
    if (enc == NULL)
    {
        return -1;
    }

    return aacenc_get_input_buffer_size(enc);
}

int get_max_output_buffer_size()
{
    if (enc == NULL)
    {
        return -1;
    }

    return aacenc_get_max_output_buffer_size(enc);
}

int encode_buffer(const void* inbuf, int len, void* outbuf, int outsize)
{
    if (enc == NULL)
    {
        return -1;
    }

    return aacenc_encode(enc, inbuf, len, outbuf, outsize);
}
