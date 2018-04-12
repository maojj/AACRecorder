#pragma once

#ifndef __AACENC_H__
#define __AACENC_H__

#ifdef __cplusplus
extern "C" {
#endif

    __declspec(dllexport) bool encoder_init(int samplerate, int channels, int bitrate);

    __declspec(dllexport) void encoder_release();

    __declspec(dllexport) int get_input_buffer_size();

    __declspec(dllexport) int get_max_output_buffer_size();

    __declspec(dllexport) int encode_buffer(const void *inbuf, int len, void *outbuf, int outsize);

#ifdef __cplusplus
}
#endif

#endif  /*__AACENC_H__*/