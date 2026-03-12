#ifndef FFPROCESSEVENT_H
#define FFPROCESSEVENT_H

#include "ffevent.h"
class FFPlayerWindow;

class FFProcessEvent : public FFEvent
{
public:
    FFProcessEvent(FFPlayerContext* playerctx,int curSec_);
    virtual ~FFProcessEvent() override;
    virtual void work() override;
private:
    int curSec;
    FFPlayerContext* m_playerCtx;
};

#endif // FFPROCESSEVENT_H
