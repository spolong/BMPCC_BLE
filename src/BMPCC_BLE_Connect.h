#ifndef BMPCC_BLE_CONNECT_H
#define BMPCC_BLE_CONNECT_H

/* 
*   包含 map STL模版类 
*   用于操作 K/V 数据
*/
#include <map>

// 来自Arduino-ESP32 包含 低功耗蓝牙 类
#include <BLEClient.h>
#include <BLERemoteService.h>
#include <BLERemoteCharacteristic.h>
#include <BLEAdvertisedDevice.h>

// BMPCC 设备广播回调
#include "BMPCCAdvertisedDeviceCallbacks.h"

#include "BlueMagicState.h"
// 包含 低功耗蓝牙(BLE)控制 组件
#include "BlueMagicCameraController.h"

class BMPCC_BLE_Connect
{
public:
    BMPCC_BLE_Connect();
    virtual ~BMPCC_BLE_Connect();

    void setup(const int scan_time);
    void connect(const int scan_time);
    // 摄像机控制器对象实例
    BlueMagicCameraController *m_cameraControl;
    // 摄像机控制特征（写入）
    BLERemoteCharacteristic *m_pBMPCC_cameraOutControlCharacteristic;
    // 摄像机状态特征（订阅/Indicate）
    BLERemoteCharacteristic *m_pBMPCC_cameraInControlCharacteristic;
    // 时间码特征（订阅/Notiyf）
    BLERemoteCharacteristic *pBMPCC_timecodeCharacteristic;

    /** status 
     * 状态
    */
    bool m_connected;

protected:
    void listServices(BLEClient *pClient);
    BLERemoteService *connectToService(BLEClient *pClient, BLEUUID &uuid);
    BLERemoteCharacteristic *getCharacteristic(BLERemoteService *pCameraInfoSvc, BLEUUID &uuid);

    /** callbacks 订阅回调 */
    // static void cameraStatusNotifyCallback(BLERemoteCharacteristic *pBLERemoteCharacteristic, uint8_t *pData, size_t length, bool isNotify);
    // static void timecodeNotifyCallback(BLERemoteCharacteristic *pBLERemoteCharacteristic, uint8_t *pData, size_t length, bool isNotify);
    // static void cameraInControlNotifyCallback(BLERemoteCharacteristic *pBLERemoteCharacteristic, uint8_t *pData, size_t length, bool isNotify);

    bool confirmBMPCC(BLEClient *pBLEClient);
    bool getConnected(const std::string &addr, BLEAdvertisedDevice *pDevice);
    void setRecordState(bool state);

    bool selectCamera();

    void setController();

private:
    /** constants 常数 */
    const std::string DEVICE_NAME;
    const int SCAN_INTERVAL;
    const int SCAN_WINDOW;

    /** camera service 相机服务 */
    BLEUUID m_BMPCC_cameraControlServiceUUID;
    //static BLEUUID BMPCC_serviceUUID("00001800-0000-1000-8000-00805f9b34fb");

    /** device info service 设备信息服务 */
    BLEUUID m_BMPCC_deviceInfoServiceUUID;
    /** characteristics 特征：
     *      - 制造商
     *      - 相机型号
     *      - 相机 Out 控制
     *      - 相机 In 控制
     *      - 时间码
     *      - 相机状态
     *      - 设备名
     *      - 协议版本
     */
    BLEUUID m_BMPCC_manufacturerCharacteristicUUID;
    BLEUUID m_BMPCC_cameraModelCharacteristicUUID;
    BLEUUID m_BMPCC_cameraOutControlCharacteristicUUID;
    BLEUUID m_BMPCC_cameraInControlCharacteristicUUID;
    BLEUUID m_BMPCC_timecodeCharacteristicUUID;
    BLEUUID m_BMPCC_cameraStatusCharacteristicUUID;
    BLEUUID m_BMPCC_deviceNameCharacteristicUUID;
    BLEUUID m_BMPCC_protocolVersionCharacteristicUUID;

    /** callback 回调：
     *  BMPCC 设备广播回调
     */
    BMPCCAdvertisedDeviceCallbacks *m_pAdvertisedDeviceCallback;

    /** bluetooth device
     *  蓝牙设备
     */
    CameraMap_t m_BMPCC_Cameras;
    int m_deviceIndex;
    bool m_deviceFound;

    uint32_t m_camera_PIN;

    // 私有对象（暂时禁用）
    // BLERemoteCharacteristic *m_pBMPCC_cameraOutControlCharacteristic;
    // BLERemoteCharacteristic *m_pBMPCC_cameraInControlCharacteristic;
};


#endif