#ifndef FFEVENT_H
#define FFEVENT_H
#include "capture/ffcapturecontext.h"

using namespace FFCaptureContextType;
class FFEvent
{
public:
    FFEvent(FFCaptureContext* captureContext_);
    virtual ~FFEvent();
    virtual void work() = 0;
protected:
    //全局上下文
    FFCaptureContext* captureContext = nullptr;
};

#endif // FFEVENT_H
