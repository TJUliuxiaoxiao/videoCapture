#ifndef FFSEEKEVENT_H
#define FFSEEKEVENT_H

#include"ffevent.h"

class FFSeekEvent : public FFEvent
{
public:
    FFSeekEvent(FFPlayerContext* playerCtx,int64_t seekSec_);

    virtual void work() override;

private:
    int64_t seekSec;
    FFPlayerContext* playerCtx;
};

#endif // FFSEEKEVENT_H
