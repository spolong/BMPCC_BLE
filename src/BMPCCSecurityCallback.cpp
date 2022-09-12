#include "BMPCCSecurityCallback.h"
#include <HardwareSerial.h>

BMPCCSecurityCallback::BMPCCSecurityCallback()
{
}

BMPCCSecurityCallback::~BMPCCSecurityCallback()
{
}

uint32_t BMPCCSecurityCallback::onPassKeyRequest()
{
    // this probably only works on the Serial Monitor.
    // 修改为屏幕显示
    uint32_t pin = 0;
    Serial.println("Enter pass key from device: ");
    while (!Serial.available())
    {
        delay(1);
    }
    if (Serial.available() > 0)
    {
        pin = Serial.parseInt();
        Serial.print(" pin: ");
        Serial.println(pin);
    }
    Serial.print("Returning pass key: ");
    Serial.println(pin);
    return pin;
}

void BMPCCSecurityCallback::onPassKeyNotify(uint32_t pass_key)
{
    // 串口打印 没有提供验证码
    Serial.println("not providing pass key.");
}

bool BMPCCSecurityCallback::onSecurityRequest()
{
    // 串口打印 不允许连接该设备
    Serial.println("not allowing connection to this device.");
    return false;
}

void BMPCCSecurityCallback::onAuthenticationComplete(esp_ble_auth_cmpl_t status)
{
    // 串口打印 验证状态
    Serial.print("Authentication Status: ");
    Serial.println(status.success);
}

bool BMPCCSecurityCallback::onConfirmPIN(uint32_t pin)
{
    // 串口打印 PIN：
    Serial.print("PIN: ");
    Serial.println(pin);
    return (0 != pin);
}
