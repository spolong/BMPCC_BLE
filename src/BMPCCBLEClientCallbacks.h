#ifndef BMPCCBLECLIENTCALLBACKS_H
#define BMPCCBLECLIENTCALLBACKS_H

#include <BLEClient.h>

/**
 * BMPCC 客户端回调 类
 */
class BMPCCBLEClientCallbacks : public BLEClientCallbacks
{
public:
    BMPCCBLEClientCallbacks(bool *_pconnected);
    virtual ~BMPCCBLEClientCallbacks();

protected:
    virtual void onConnect(BLEClient *pClient);

    virtual void onDisconnect(BLEClient *pClient);

private:
    bool *m_pconnected;
};

#endif
