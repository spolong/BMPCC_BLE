#include "BMPCC_BLE_Connect.h"

#include <HardwareSerial.h>

#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEScan.h>

#include "BMPCCAdvertisedDeviceCallbacks.h"
#include "BMPCCSecurityCallback.h"
#include "BMPCCBLEClientCallbacks.h"

#include "BlueMagicState.h"


BMPCC_BLE_Connect::BMPCC_BLE_Connect()
    : /** constants */
      DEVICE_NAME("BMPCC_Remote"), SCAN_INTERVAL(1200), SCAN_WINDOW(800)
      /** camera service */
      , m_BMPCC_cameraControlServiceUUID("291D567A-6D75-11E6-8B77-86F30CA893D3")
      /** device info service */
      , m_BMPCC_deviceInfoServiceUUID("180A")
      /** characteristics */
      , m_BMPCC_manufacturerCharacteristicUUID("2A29")
      , m_BMPCC_cameraModelCharacteristicUUID("2A24")
      , m_BMPCC_cameraOutControlCharacteristicUUID("5DD3465F-1AEE-4299-8493-D2ECA2F8E1BB")
      , m_BMPCC_cameraInControlCharacteristicUUID("B864E140-76A0-416A-BF30-5876504537D9")
      , m_BMPCC_timecodeCharacteristicUUID("6D8F2110-86F1-41BF-9AFB-451D87E976C8")
      , m_BMPCC_cameraStatusCharacteristicUUID("7FE8691D-95DC-4FC5-8ABD-CA74339B51B9")
      , m_BMPCC_deviceNameCharacteristicUUID("FFAC0C52-C9FB-41A0-B063-CC76282EB89C")
      , m_BMPCC_protocolVersionCharacteristicUUID("8F1FD018-B508-456F-8F82-3D392BEE2706")
      /** callback */
      , m_pAdvertisedDeviceCallback(NULL)
      /** bluetooth device */
      , m_deviceIndex(-1)
      , m_deviceFound(false)
      /** status */
      , m_connected(false)
      , m_camera_PIN(0)
      , m_pBMPCC_cameraOutControlCharacteristic(NULL)
      , m_pBMPCC_cameraInControlCharacteristic(NULL)
      , pBMPCC_timecodeCharacteristic(NULL)
{
}

BMPCC_BLE_Connect::~BMPCC_BLE_Connect()
{
    if (m_pAdvertisedDeviceCallback)
    {
        delete m_pAdvertisedDeviceCallback;
    }
    while (!m_BMPCC_Cameras.empty())
    {
        delete m_BMPCC_Cameras.begin()->second;
        m_BMPCC_Cameras.erase(m_BMPCC_Cameras.begin());
    }
}


void BMPCC_BLE_Connect::setup(const int scan_time)
{
    // initialize BLE 初始化BLE对象（ESP-IDF接口）
    BLEDevice::init( DEVICE_NAME );
    // create callback 创建相机控制服务的广播回调
    m_pAdvertisedDeviceCallback = new BMPCCAdvertisedDeviceCallbacks( m_BMPCC_Cameras, m_BMPCC_cameraControlServiceUUID );
    // get scan object 创建扫描对象
    BLEScan *pBLEScan = BLEDevice::getScan();
    // set callback 设置设备广播回调
    pBLEScan->setAdvertisedDeviceCallbacks( m_pAdvertisedDeviceCallback );
    pBLEScan->setInterval( SCAN_INTERVAL );
    pBLEScan->setWindow( SCAN_WINDOW );
    pBLEScan->setActiveScan( true );
    pBLEScan->start( scan_time, false );
}

void BMPCC_BLE_Connect::connect(const int scan_time)
{
    // 如果没有连接则进行以下尝试，如果已经存在连接直接略过本函数。以此保证随时处于连接状态。
    if( ! m_connected )
    {
        // if not yet connected, try again 如果没有链接将重新开始扫描
        if( ! selectCamera() )
        {
            delay(1000);
            Serial.println("  --- scanning again.");
            BLEDevice::getScan()->start(scan_time, false);
            return;
        }
        // 已经连接到相机
        if( getConnected(m_BMPCC_Cameras.begin()->first, m_BMPCC_Cameras.begin()->second) )
        {
            Serial.println("We are now connected to the BLE Server.");
            m_connected = true;
            m_deviceFound = true;
        }
        // 无法连接到相机，将开始重试
        else
        {
            Serial.println("Not connected to the server; will try again.");
            m_deviceFound = false;
            return;
        }
    }
}


/**
 * 获取服务列表
 */
void BMPCC_BLE_Connect::listServices(BLEClient *pClient)
{
    // list all services for informational purposes
    std::map<std::string, BLERemoteService *> *pServiceMap = pClient->getServices();
    Serial.print("Number of entries in service map: ");
    Serial.println(pServiceMap->size());
    std::map<std::string, BLERemoteService *>::const_iterator itr = pServiceMap->begin();
    std::map<std::string, BLERemoteService *>::const_iterator ite = pServiceMap->end();
    for (; itr != ite; ++itr)
    {
        std::string msg = "Service [" + itr->first + "]  == " + itr->second->toString();
        Serial.println(msg.c_str());
        // list characteristics of the service
        std::map<uint16_t, BLERemoteCharacteristic *> *characteristics = itr->second->getCharacteristicsByHandle();
        std::map<uint16_t, BLERemoteCharacteristic *>::const_iterator citr = characteristics->begin();
        std::map<uint16_t, BLERemoteCharacteristic *>::const_iterator cite = characteristics->end();
        for (; citr != cite; ++citr)
        {
            std::string cmsg = "    characteristic == " + citr->second->toString();
            Serial.println(cmsg.c_str());
        }
    }
}

/**
 * 连接到服务
 */
BLERemoteService *BMPCC_BLE_Connect::connectToService(BLEClient *pClient, BLEUUID &uuid)
{
    BLERemoteService *pService = pClient->getService(uuid);
    if (nullptr == pService)
    {
        std::string msg = "Failed to find our service UUID: " + uuid.toString();
        Serial.println(msg.c_str());
        pClient->disconnect();
        return NULL;
    }
    return (pService);
}

/**
 * 获取特征
 */
BLERemoteCharacteristic *BMPCC_BLE_Connect::getCharacteristic(BLERemoteService *pCameraInfoSvc, BLEUUID &uuid)
{
    BLERemoteCharacteristic *characteristic = pCameraInfoSvc->getCharacteristic(uuid);
    if (nullptr == characteristic)
    {
        std::string msg = "Failed to find characteristic using UUID: " + uuid.toString();
        Serial.println(msg.c_str());
        return NULL;
    }
    return characteristic;
}



/**
 * 相机连接状态回调
 */
static void cameraStatusNotifyCallback(BLERemoteCharacteristic *pBLERemoteCharacteristic, uint8_t *pData, size_t length, bool isNotify)
{
  BlueMagicState *blu = BlueMagicState::getInstance();
  blu->statusNotify(true, pData);
  blu->setCameraStatus(pData[0]);
}

/**
 * 相机时间码回调
 */
static void timecodeNotifyCallback(BLERemoteCharacteristic *pBLERemoteCharacteristic, uint8_t *pData, size_t length, bool isNotify)
{
  BlueMagicState *blu = BlueMagicState::getInstance();
  blu->timecodeNotify(true, pData);
  // timecode
  uint8_t H, M, S, f;
  H = (pData[11] / 16 * 10) + (pData[11] % 16);
  M = (pData[10] / 16 * 10) + (pData[10] % 16);
  S = (pData[9] / 16 * 10) + (pData[9] % 16);
  f = (pData[8] / 16 * 10) + (pData[8] % 16);
  blu->setTimecode(H, M, S, f);
}

/**
 * 相机In控制回调
 */
static void cameraInControlNotifyCallback(BLERemoteCharacteristic *pBLERemoteCharacteristic, uint8_t *pData, size_t length, bool isNotify)
{
  BlueMagicState *blu = BlueMagicState::getInstance();
  blu->settingsNotify(true, pData);
  bool changed = false;

  // recording
  if (length == 13 && pData[0] == 255 && pData[1] == 9 && pData[4] == 10 && pData[5] == 1)
  {
    changed = true;
    int8_t transportMode = pData[8];
    blu->setTransportMode(transportMode);
  }

  //codec
  if (pData[0] == 255 && pData[4] == 10 && pData[5] == 0)
  {
    changed = true;
    int8_t codec = pData[8];
    int8_t quality = pData[9];
    blu->setCodec(codec);
    blu->setQuality(quality);
  }

  //resolution + framerate
  if (pData[0] == 255 && pData[4] == 1 && pData[5] == 9)
  {

    changed = true;
    int16_t frL = pData[8];
    int16_t frH = pData[9] << 8;
    int16_t frameRate = frL + frH;

    int16_t sfrL = pData[10];
    int16_t sfrH = pData[11] << 8;
    int16_t sensorRate = sfrL + sfrH;

    int16_t wL = pData[12];
    int16_t wH = pData[13] << 8;
    int16_t width = wL + wH;

    int16_t hL = pData[14];
    int16_t hH = pData[15] << 8;
    int16_t height = hL + hH;

    int8_t flags = pData[16];

    blu->setFrameRate(frameRate);
    blu->setSensorFrameRate(sensorRate);
    blu->setFrameWidth(width);
    blu->setFrameHeight(height);
    blu->setFormatFlags(flags);
  }

  // white balance
  if (pData[0] == 255 && pData[4] == 1 && pData[5] == 2)
  {
    changed = true;
    int16_t wbL = pData[8];
    int16_t wbH = pData[9] << 8;
    int16_t whiteBalance = wbL + wbH;

    int16_t tintL = pData[10];
    int16_t tintH = pData[11] << 8;
    int16_t tint = tintL + tintH;

    blu->setWhiteBalance(whiteBalance);
    blu->setTint(tint);
  }

  // zoom
  if (pData[0] == 255 && pData[4] == 0 && pData[5] == 7)
  {
    changed = true;
    int16_t zL = pData[8];
    int16_t zH = pData[9] << 8;
    int16_t zoom = zL + zH;
    blu->setZoom(zoom);
  }

  // focus
  if (pData[0] == 255 && pData[4] == 0 && pData[5] == 0)
  {
    changed = true;
    uint16_t low = pData[8];
    uint16_t high = pData[9] << 8;
    float focus = (float(low + high) / 2048.0);
    blu->setFocus(focus);
    Serial.println("focusing");
  }


  // aperture
  if (pData[0] == 255 && pData[4] == 0 && pData[5] == 2)
  {
    changed = true;
    uint16_t low = pData[8];
    uint16_t high = pData[9] << 8;
    float aperture = sqrt(pow(2, (float(low + high) / 2048.0)));
    blu->setAperture(aperture);
  }

  // iso
  if (pData[0] == 255 && pData[4] == 1 && pData[5] == 14)
  {
    changed = true;
    uint16_t low = pData[8];
    uint16_t high = pData[9] << 8;
    int32_t iso = low + high;
    blu->setIso(iso);
  }

  // shutter
  if (pData[0] == 255 && pData[4] == 1 && pData[5] == 11)
  {
    changed = true;
    uint16_t low = pData[8];
    uint16_t high = pData[9] << 8;
    int32_t shutter = low + high;
    blu->setShutter(shutter);
  }

  blu->setChanged(changed);
}



/**
 *  确认是否为 BMPCC 的相机
 */
bool BMPCC_BLE_Connect::confirmBMPCC(BLEClient *pBLEClient)
{
    if (!pBLEClient)
    {
        return false;
    }
    BLERemoteService *pCameraInfoSvc = nullptr;
    // get the camera device information service 获取相机设备信息服务
    pCameraInfoSvc = connectToService(pBLEClient, m_BMPCC_deviceInfoServiceUUID);
    if (pCameraInfoSvc)
    {
        /** ensure we are still connected and impose a scope for the pointer. */
        if (pBLEClient->isConnected())
        {
            BLERemoteCharacteristic *pBMPCC_ManufacturerCharacteristic = getCharacteristic(pCameraInfoSvc, m_BMPCC_manufacturerCharacteristicUUID);
            if (nullptr == pBMPCC_ManufacturerCharacteristic)
            {
                pBLEClient->disconnect();
                Serial.println("Failed to get manufacturer characteristic");
            }
            else
            {
                std::string manufacturer = pBMPCC_ManufacturerCharacteristic->readValue();
                if (0 != manufacturer.compare("Blackmagic Design"))
                {
                    std::string msg = "Invalid manufacturer: " + manufacturer;
                    pBLEClient->disconnect();
                }
                else
                {
                    std::string msg = "Manufacturer: " + manufacturer;
                    Serial.println(msg.c_str());
                    // all is well, stay connected.
                }
                Serial.println("cleaning up manufacturer.");
            }
        }
        /** ensure we are still connected - there were no issues above. */
        if (pBLEClient->isConnected())
        {
            // confirm that this is a BMPCC4k or BMPCC6k
            BLERemoteCharacteristic *pBMPCC_ModelCharacteristic = getCharacteristic(pCameraInfoSvc, m_BMPCC_cameraModelCharacteristicUUID);
            if (nullptr == pBMPCC_ModelCharacteristic)
            {
                pBLEClient->disconnect();
                Serial.println("Failed to get Model Characteristic.");
            }
            else
            {
                std::string model = pBMPCC_ModelCharacteristic->readValue();
                if (0 != model.substr(0, 21).compare("Pocket Cinema Camera "))
                {
                    std::string msg = "Not a BMPCC4k or BMPCC6k.  Model: " + model;
                    Serial.println(msg.c_str());
                    pBLEClient->disconnect();
                }
                else
                {
                    std::string msg = "Model: " + model;
                    Serial.println(msg.c_str());
                    // all is well, stay connected.
                }
            }
        }
        // clean up
        Serial.println("Cleaning up camera info.");
        //delete pCameraInfoSvc;
    } // camera device information service
    else
    {
        pBLEClient->disconnect();
        Serial.println("Failed to get camera device information service.");
    }
    std::string str = "Done with camera info.";
    str += ((pBLEClient->isConnected()) ? "success" : "fail");
    Serial.println(str.c_str());
    return (pBLEClient->isConnected());
}

/**
 * 连接到相机
 */
bool BMPCC_BLE_Connect::getConnected(const std::string &addr, BLEAdvertisedDevice *pCamera)
{
    if (!pCamera)
    {
        Serial.println("Camera is a NULL pointer in getConnected.");
        return false;
    }
    std::string msg = "Camera address: " + pCamera->getAddress().toString();
    Serial.println(msg.c_str());
    // set up security 设置加密通讯
    BLEDevice::setEncryptionLevel(ESP_BLE_SEC_ENCRYPT);
    BMPCCSecurityCallback *pSecurityCallback = new BMPCCSecurityCallback();
    if (!pSecurityCallback)
    {
        Serial.println("Failed to create security callback.");
        return false;
    }
    BLEDevice::setSecurityCallbacks(pSecurityCallback);
    BLESecurity *pSecurity = new BLESecurity();
    if (!pSecurity)
    {
        Serial.println("Failed to create security object.");
        delete pSecurityCallback;
        return false;
    }
    pSecurity->setKeySize();
    pSecurity->setAuthenticationMode(ESP_LE_AUTH_REQ_SC_MITM_BOND);
    pSecurity->setCapability(ESP_IO_CAP_IN);
    pSecurity->setRespEncryptionKey(ESP_BLE_ENC_KEY_MASK | ESP_BLE_ID_KEY_MASK);
    // create client 创建客户端
    BLEClient *pBLEClient = BLEDevice::createClient();
    if (!pBLEClient)
    {
        Serial.println("Failed to create client.");
        delete pSecurity;
        delete pSecurityCallback;
        return false;
    }
    BMPCCBLEClientCallbacks *clientConnectCallbacks = new BMPCCBLEClientCallbacks(&m_connected);
    if (!clientConnectCallbacks)
    {
        Serial.println("Failed to create client connect callback object.");
        delete pBLEClient;
        delete pSecurity;
        delete pSecurityCallback;
        return false;
    }
    pBLEClient->setClientCallbacks(clientConnectCallbacks);
    pBLEClient->connect(pCamera);
    listServices(pBLEClient);

    // confirm that this is a BMD camera 确认是否为BMD相机
    if (!confirmBMPCC(pBLEClient))
    {
        Serial.println("confirmBMPCC failed.  cleaning up.");
        pBLEClient->disconnect();
    }
    else
    {
        // get the camera control service 获取相机服务
        BLERemoteService *pCameraControlService = connectToService(pBLEClient, m_BMPCC_cameraControlServiceUUID);
        if (!pCameraControlService)
        {
            Serial.println("Failed to get camera control service.");
            pBLEClient->disconnect();
        }
        else
        {
            // device name 设备名
            BLERemoteCharacteristic *pBMPCC_deviceNameCharacteristic = getCharacteristic(pCameraControlService, m_BMPCC_deviceNameCharacteristicUUID);
            if (nullptr == pBMPCC_deviceNameCharacteristic)
            {
                pBLEClient->disconnect();
                Serial.println("Failed to get Device Name Characteristic");
            }
            else
            {
                if (pBMPCC_deviceNameCharacteristic->canWrite())
                {
                    // write the name to the device.  this characteristic is no longer needed.
                    pBMPCC_deviceNameCharacteristic->writeValue(DEVICE_NAME);
                }

                // protocol version 协议版本
                BLERemoteCharacteristic *pBMPCC_protocolVersionCharacteristic = getCharacteristic(pCameraControlService, m_BMPCC_protocolVersionCharacteristicUUID);
                if (nullptr == pBMPCC_protocolVersionCharacteristic)
                {
                    pBLEClient->disconnect();
                    Serial.println("Failed to get Protocol Version Characteristic");
                }
                else
                {
                    // read the value.  this characteristic is no longer needed.
                    std::string value = pBMPCC_protocolVersionCharacteristic->readValue();
                    std::string msg = "Protocol version: " + value;
                    Serial.println(msg.c_str());
                    // camera status 相机状态
                    BLERemoteCharacteristic *pBMPCC_cameraStatusCharacteristic = getCharacteristic(pCameraControlService, m_BMPCC_cameraStatusCharacteristicUUID);
                    if (nullptr == pBMPCC_cameraStatusCharacteristic)
                    {
                        pBLEClient->disconnect();
                        Serial.println("Failed to get Camera Status Characteristic");
                    }
                    else
                    {
                        if (pBMPCC_cameraStatusCharacteristic->canNotify())
                        {
                            pBMPCC_cameraStatusCharacteristic->registerForNotify(cameraStatusNotifyCallback);
                            Serial.println("cameraStatusNotify ok");
                        }
                    }
                } // protocol version characteristic 协议版本特征
            }     // device name characteristic 设备名特征

            // timecode 时间码
            if (pBLEClient->isConnected())
            {
                pBMPCC_timecodeCharacteristic = getCharacteristic(pCameraControlService, m_BMPCC_timecodeCharacteristicUUID);
                if (nullptr == pBMPCC_timecodeCharacteristic)
                {
                    pBLEClient->disconnect();
                    Serial.println("Failed to get Camera Timecode Characteristic");
                }
                else
                {
                    if (pBMPCC_timecodeCharacteristic->canNotify())
                    {
                        pBMPCC_timecodeCharacteristic->registerForNotify(timecodeNotifyCallback);
                        Serial.println("TimeNotify ok");
                    }
                }
            }

            // camera in control
            if (pBLEClient->isConnected())
            {
                m_pBMPCC_cameraInControlCharacteristic = getCharacteristic(pCameraControlService, m_BMPCC_cameraInControlCharacteristicUUID);
                if (nullptr == m_pBMPCC_cameraInControlCharacteristic)
                {
                    pBLEClient->disconnect();
                    Serial.println("Failed to get Camera In Control Characteristic");
                }
                else
                {
                    if (m_pBMPCC_cameraInControlCharacteristic->canIndicate())
                    {
                        // 此处为 Indicate 并非 Notify ，所以需要在第二个参数后设置为 false
                        m_pBMPCC_cameraInControlCharacteristic->registerForNotify(cameraInControlNotifyCallback,false);
                        Serial.println("controlNotify ok");
                    }
                }
            }

            // camera out control
            if (pBLEClient->isConnected())
            {
                m_pBMPCC_cameraOutControlCharacteristic = getCharacteristic(pCameraControlService, m_BMPCC_cameraOutControlCharacteristicUUID);
                if (nullptr == m_pBMPCC_cameraOutControlCharacteristic)
                {
                    pBLEClient->disconnect();
                    Serial.println("Failed to get Camera Out Control Characteristic");
                }
                else
                {
                    std::string value = m_pBMPCC_cameraOutControlCharacteristic->readValue();
                    std::string msg = "out control: " + value;
                    Serial.println(msg.c_str());
                }
            }

        } // camera connect service 相机连接服务
    }
    // 创建控制器
    setController();

    // if we are still connected, this is the selected camera.
    if (pBLEClient->isConnected())
    {
        std::string str("Successfully completed getConnected: " + addr);
        Serial.println(str.c_str());
        return true;
    }
    else
    {
        std::string str("Failed to connect to camera in getConnected: " + addr);
        Serial.println(str.c_str());
        delete clientConnectCallbacks;
        delete pSecurity;
        delete pSecurityCallback;
        delete pBLEClient;
        return false;
    }
}


void BMPCC_BLE_Connect::setController()
{
  m_cameraControl = new BlueMagicCameraController(m_pBMPCC_cameraOutControlCharacteristic);
}


bool BMPCC_BLE_Connect::selectCamera()
{
    Serial.println( "BMPCC_BlueTooth::selectCamera" );
    // iterate camera list 相机列表迭代
    CameraMap_t::iterator itr = m_BMPCC_Cameras.begin();
    CameraMap_t::iterator ite = m_BMPCC_Cameras.end();
    for (; ite != itr; ++itr)
    {
        // try to connect to camera 尝试连接到相机
        if( getConnected( itr->first, itr->second ) )
        {
            // save the index of the selected camera 保存已选相机索引
            m_connected = true;
            m_deviceFound = true;
        }
    }
    std::string str( "BMPCC_BlueTooth::selectCamera returning " );
    str += (m_deviceFound ? "true" : "false");
    Serial.println( str.c_str() );
    return m_deviceFound;
}