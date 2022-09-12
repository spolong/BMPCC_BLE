#ifndef BMPCCADVERTISEDDEVICECALLBACKS_H
#define BMPCCADVERTISEDDEVICECALLBACKS_H

// 包含 低功耗蓝牙设备广播 类
#include <BLEAdvertisedDevice.h>

typedef std::map< std::string, BLEAdvertisedDevice * > CameraMap_t;

/**
 * For each advertised device, check to see if it is a BMPCC camera.
 * 检查所有正在广播的设备，判断是否为 BMPCC 相机
 */
class BMPCCAdvertisedDeviceCallbacks: public BLEAdvertisedDeviceCallbacks
{
  public:
    BMPCCAdvertisedDeviceCallbacks( CameraMap_t & _Cameras,
                                    BLEUUID & _cameraControlServiceUUID );
    virtual ~BMPCCAdvertisedDeviceCallbacks();

  protected:
    virtual void onResult( BLEAdvertisedDevice advertisedDevice );

  private:
    CameraMap_t & m_Cameras;
    BLEUUID & m_cameraControlServiceUUID;

};


#endif
