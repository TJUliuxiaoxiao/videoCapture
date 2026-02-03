#include "ffafilter.h"
#include "queue/ffaframequeue.h"
#include "decoder/ffadecoder.h"
#include "clock/ffglobalclock.h"
#include "capture/ffcapturecontext.h"
using namespace FFCaptureContextType;

#define FF_AUDIO_TIME_BASE {1,48000}
FFAFilter::FFAFilter() {}
