#ifndef OPUS_ENCODER_H
#define OPUS_ENCODER_H

#include <opus.h>

// Opus encoder wrapper
// The actual implementation uses the libopus library

// Opus application types
#define OPUS_APPLICATION_VOIP 2048
#define OPUS_APPLICATION_AUDIO 2049
#define OPUS_APPLICATION_RESTRICTED_LOWDELAY 2051

// Opus encoder CTL commands
#define OPUS_SET_BITRATE_REQUEST 4002
#define OPUS_SET_VBR_REQUEST 4006
#define OPUS_SET_COMPLEXITY_REQUEST 4010

#define OPUS_SET_BITRATE(x) OPUS_SET_BITRATE_REQUEST, (x)
#define OPUS_SET_VBR(x) OPUS_SET_VBR_REQUEST, (x)
#define OPUS_SET_COMPLEXITY(x) OPUS_SET_COMPLEXITY_REQUEST, (x)

#endif // OPUS_ENCODER_H
