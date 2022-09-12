#include "BMPCCAdvertisedDeviceCallbacks.h"

// 包含 函数对象 类模版
#include <functional>

// 包含 BLE地址 类
#include <BLEAddress.h>
// 包含 硬件串口 类
#include <HardwareSerial.h>
// 包含 BLE设备 类
#include <BLEDevice.h>


BMPCCAdvertisedDeviceCallbacks::BMPCCAdvertisedDeviceCallbacks( CameraMap_t & _Cameras,
                                                                BLEUUID & _cameraControlServiceUUID )
  : BLEAdvertisedDeviceCallbacks()
  , m_Cameras( _Cameras )
  , m_cameraControlServiceUUID( _cameraControlServiceUUID )
{
}

BMPCCAdvertisedDeviceCallbacks::~BMPCCAdvertisedDeviceCallbacks()
{
}


void BMPCCAdvertisedDeviceCallbacks::onResult( BLEAdvertisedDevice advertisedDevice )
{
  Serial.print("BLE Advertised Device found: ");
  Serial.println(advertisedDevice.toString().c_str());
  // We have found a device, let us now see if it contains the service we are looking for.
  // 发现设备并查看包含的服务是否为相机服务
  if (advertisedDevice.haveServiceUUID()
      && advertisedDevice.isAdvertisingService( m_cameraControlServiceUUID ))
  {
    // 打印到串口 发现相机
    Serial.println("found a camera.");
    // 创建 BLE地址 对象
    BLEAddress address( advertisedDevice.getAddress() );
    // 转为 BLE地址 为 字符串 
    std::string addressStr( address.toString() );
    std::string msg = "Camera address: " + addressStr;
    // 打印到串口 相机地址
    Serial.println( msg.c_str() );
    // m_Cameras 对象
    if( m_Cameras.end() == m_Cameras.find( addressStr ) )
    {
      m_Cameras[ addressStr ] = new BLEAdvertisedDevice( advertisedDevice );
    }
  }
}
