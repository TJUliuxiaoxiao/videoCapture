#include "ffamuxerthread.h"
#include "muxer/ffmuxer.h"
#include "encoder/ffaencoder.h"
#include "queue/ffapacketqueue.h"
#include "queue/ffpacket.h"
FFAMuxerThread::FFAMuxerThread() {}

FFAMuxerThread::~FFAMuxerThread()
{

}

void FFAMuxerThread::init(FFAPacketQueue *pktQueue_, FFMuxer *muxer_, FFAEncoder *aEncoder_)
{
    pktQueue = pktQueue_;
    muxer = muxer_;
    aEncoder = aEncoder_;
}

void FFAMuxerThread::run()
{
    //初始化状态标志
    bool write[2] = {false,false};
    int sum = 0;
    //是否写入文件头,是否已经调用过复用函数
    while(!m_stop){
        FFPacket* pkt = pktQueue->dequeue();
        if(pkt == nullptr)continue;

        AVPacket* packet = &pkt->packet;

        if(packet==nullptr)continue;

        if(packet->data == 0){
            av_packet_unref(packet);
            av_packet_free(&packet);
            continue;
        }//packet->data为0表示没有有效数据，认为该包是空包

        //写入文件头
        if(!write[0]){
            muxer->writeHeader();
            write[0] = true;
        }
        int ret = muxer->mux(packet);//复用包
        //将当前包通过复用器写入输出文件
        //无论返回值如何，都将write[1]置为true,表示已经执行过
        //复用操作(后续会根据ret结果表示是否停止)
        if(!write[1]){
            write[1] = true;
        }
        if(++sum == 5000){//统计已经处理的包的数量
            m_stop = true;
            std::cerr<<"Finish Audio"<<std::endl;
        }
        std::cout<<"finish:"<<std::endl;
        if(ret<0){
            m_stop = true;
            std::cerr<<"Finish Audio,Sum = "<<std::to_string(sum)<<std::endl;
        }
        av_packet_unref(packet);
        av_packet_free(&packet);
    }
    if(write[0] && write[1]){
        muxer->writeTrailer();//写过头并至少调用过一次mux
    }
}
