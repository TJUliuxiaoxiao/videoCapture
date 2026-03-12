#include "player/ffplayercontext.h"
#include "ffreadyevent.h"
FFReadyEvent::FFReadyEvent(FFPlayerContext *playerCtx, int totalSec_, float aspect_)
    :FFEvent(playerCtx),totalSec(totalSec_),aspect(aspect_),playerCtx(playerCtx)
{
}
/*在 FFReadyEvent 的实现文件（.cpp）或头文件中，必须包含定义了 FFPlayerContext 和 FFCaptureContext 的头文件。
确保包含顺序正确，例如：
cpp
#include "player/ffplayercontext.h"   // 其中定义了 FFPlayerContext（包含 FFCaptureContext）
#include "event/ffreadyevent.h"       // 然后才是事件类的实现*/

FFReadyEvent::~FFReadyEvent()
{

}

void FFReadyEvent::work()
{
    QMetaObject::invokeMethod(
        playerCtx->playerWindow,
        "initPlayerTotalSec",Qt::QueuedConnection,
        Q_ARG(int,totalSec),Q_ARG(float,aspect));
}
