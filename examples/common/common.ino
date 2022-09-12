#include <BMPCC_BLE_Connect.h>

/**
 * This code is based on examples and data from:
 * - Sparkfun          - https://github.com/sparkfun/ESP32_Thing
 * - Arduino           - https://www.arduino.cc/en/Reference/ArduinoBLE
 * - Neil Kolban       - https://github.com/nkolban/esp32-snippets/blob/master/Documentation/BLE%20C%2B%2B%20Guide.pdf
 * - espressif         - https://github.com/espressif/arduino-esp32
 *                     - https://github.com/espressif/arduino-esp32/tree/master/libraries/BLE/src
 *                     - https://github.com/espressif/arduino-esp32/blob/master/libraries/BLE/examples/BLE_client/BLE_client.ino
 * - Blackmagic Design - https://www.blackmagicdesign.com/developer/product/camera, https://www.bhphotovideo.com/lit_files/452872.pdf
 * - BMPCC_Remote_ESP32 - https://github.com/creacominc/BMPCC_Remote_Esp32
 * - BlueMagic32       - https://github.com/schoolpost/BlueMagic32
 */

const int BAUD_RATE = 115200;
const int SCAN_TIME = 5;

/**
 * 创建 BMPCC_BLE 实例
 */
BMPCC_BLE_Connect m_bmpcc_ble_connect;

/**
 * 配置 低功耗蓝牙客户端
 */
void setup()
{
    Serial.begin(BAUD_RATE);
    // 初始化
    m_bmpcc_ble_connect.setup(SCAN_TIME);
}

void loop()
{
    // 保持连接
    m_bmpcc_ble_connect.connect(SCAN_TIME);
    // 控制程序
    for (size_t i = 0; i < 100; i++)
    {
        /* code */
        m_bmpcc_ble_connect.m_cameraControl->focus(0.01*i);
        Serial.print("Focus:");
        Serial.println(m_bmpcc_ble_connect.m_cameraControl->getFocus());
        delay(200);
    }
    for (size_t i = 100; i > 1; i--)
    {
        /* code */
        m_bmpcc_ble_connect.m_cameraControl->focus(0.01*i);
        Serial.print("Focus:");
        Serial.println(m_bmpcc_ble_connect.m_cameraControl->getFocus());
        delay(200);
    }
    delay(100);
    Serial.println(m_bmpcc_ble_connect.m_cameraControl->getIso());
    Serial.println(m_bmpcc_ble_connect.m_cameraControl->getAperture());
    Serial.println(m_bmpcc_ble_connect.m_cameraControl->getZoom());
}
