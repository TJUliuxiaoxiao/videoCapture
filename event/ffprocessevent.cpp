#include "ffprocessevent.h"
#include "player/ffplayercontext.h"

FFProcessEvent::FFProcessEvent(FFPlayerContext* playerCtx,int curSec_)
    :FFEvent(playerCtx),curSec(curSec_),m_playerCtx(playerCtx)
{

}

FFProcessEvent::~FFProcessEvent()
{

}

void FFProcessEvent::work()
{
     QMetaObject::invokeMethod(m_playerCtx->playerWindow,
                              "showPlayerProcessSec",
                              Qt::QueuedConnection,Q_ARG(int,curSec));
}




