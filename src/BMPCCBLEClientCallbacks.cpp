#include "BMPCCBLEClientCallbacks.h"
#include <HardwareSerial.h>

BMPCCBLEClientCallbacks::BMPCCBLEClientCallbacks(bool *_pconnected)
    : BLEClientCallbacks(), m_pconnected(_pconnected)
{
}

BMPCCBLEClientCallbacks::~BMPCCBLEClientCallbacks()
{
}

void BMPCCBLEClientCallbacks::onConnect(BLEClient *pClient)
{
    // 串口打印 已连接
    Serial.println("Connected.");
    (*m_pconnected) = true;
}

void BMPCCBLEClientCallbacks::onDisconnect(BLEClient *pClient)
{
    (*m_pconnected) = false;
    pClient->disconnect();
    // 串口打印 断开连接
    Serial.println("Disconnected.");
}
