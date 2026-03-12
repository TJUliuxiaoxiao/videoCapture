#ifndef FFPLAYERCONTEXT_H
#define FFPLAYERCONTEXT_H
// #include "capture/ffcapturecontext.h"
// #include "ffplayerwindow.h"
class FFVRender;
class FFARender;
class FFADecoderThread;
class FFVDecoderThread;
class FFDemuxerThread;
class FFPlayerWindow;
class FFAPacketQueue;
class FFVPacketQueue;
class FFAFrameQueue;
class FFVFrameQueue;
class FFDemuxer;
class FFADecoder;
class FFVDecoder;
// class FFCaptureContext;
struct FFPlayerContext{
    FFPlayerContext(){
        aDecoderThread = nullptr;
        vDecoderThread = nullptr;
        demuxerThread = nullptr;
        aPktQueue = nullptr;
        vPktQueue = nullptr;
        aFrmQueue = nullptr;
        vFrmQueue = nullptr;
        demuxer = nullptr;
        vDecoder = nullptr;
        // aDecoder = nullptr;
        // vRender = nullptr;
        // aRender = nullptr;
        // playerWindow = nullptr;
    }
    FFADecoderThread* aDecoderThread;
    FFVDecoderThread* vDecoderThread;
    FFDemuxerThread* demuxerThread;
    FFAPacketQueue* aPktQueue;
    FFVPacketQueue* vPktQueue;
    FFAFrameQueue* aFrmQueue;
    FFVFrameQueue* vFrmQueue;
    FFDemuxer* demuxer;
    FFADecoder* aDecoder;
    FFVDecoder* vDecoder;
    FFVRender* vRender;
    // FFARender* aRender;
    // FFPlayerWindow* playerWindow;
};

// class FFVRender;
// class FFADecoderThread;
// class FFVDecoderThread;
// class FFDemuxerThread;
// class FFPlayerWindow;
// class FFAPacketQueue;
// class FFVPacketQueue;
// class FFAFrameQueue;
// class FFVFrameQueue;
// class FFDemuxer;
// class FFADecoder;
// class FFVDecoder;
// struct FFPlayerContext
// {
//     FFPlayerContext(){

//         aDecoderThread = nullptr;
//         vDecoderThread = nullptr;

//         demuxerThread = nullptr;

//         aPktQueue = nullptr;
//         vPktQueue = nullptr;

//         aFrmQueue = nullptr;
//         vFrmQueue = nullptr;

//         demuxer = nullptr;

//         vDecoder = nullptr;
//     }
//     FFADecoderThread* aDecoderThread;
//     FFVDecoderThread* vDecoderThread;
//     FFDemuxerThread* demuxerThread;
//     FFAPacketQueue* aPktQueue;
//     FFVPacketQueue* vPktQueue;
//     FFAFrameQueue* aFrmQueue;
//     FFVFrameQueue* vFrmQueue;
//     FFDemuxer* demuxer;
//     FFADecoder* aDecoder;
//     FFVDecoder* vDecoder;
//     FFVRender* vRender;
// };
#endif // FFPLAYERCONTEXT_H
