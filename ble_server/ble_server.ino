//Server Code
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include <SPIFFS.h>

#define SERVICE_UUID        "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CHARACTERISTIC_NOTIFY_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a8"
#define CHARACTERISTIC_SETUP_UUID "8e632a60-ff9d-4f75-8899-ca76b3b3dfec"
// #define CHARACTERISTIC_TEST_UUID "bf44ba3a-d0b3-4341-accc-3847f2e55486"
#define SAMPLES_RATE 8000 
#define SAMPLE_SIZE 2 // 2 bytes for 16-bit audio 
#define ADC_PIN 14
#define bleServerName "ENGINEOUS_BLE"

BLECharacteristic *pCharacteristic;
BLECharacteristic *pCharacteristicSetup;
// BLECharacteristic *pCharacteristicTest1;
bool initialConnection = true;
bool deviceConnected = false;
int recordTime = 10; // in seconds

class MyServerCallbacks: public BLEServerCallbacks {
  void onConnect(BLEServer* pServer) {
    Serial.println("Connect Callback");
    deviceConnected = true;
  };
  
  void onDisconnect(BLEServer* pServer) {
    Serial.println("Disconnect Callback");
    deviceConnected = false;
  }
};

class MyCharacteristicCallbacks: public BLECharacteristicCallbacks {
  void onNotify(BLECharacteristic* pCharacteristic) {
    Serial.println("Notify Callback");
  };
  
  void onWrite(BLECharacteristic* pCharacteristic) {
    Serial.println("Write Callback: Characteristics setup completed");
    initialConnection = false;
  }
};

class MyCallbacks: public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic *pCharacteristic) {
      //std::string value = pCharacteristic->getValue();

      //if (value == "1") {
        // Start file transfer
        File file = SPIFFS.open("/audio.wav");
        if (!file) {
          Serial.println("failed to open file for reading");
          return;
        }
        uint16_t mtu = BLEDevice::getMTU();
        uint8_t buffer[mtu]; // buffer size == BLE MTU 
        int bytesRead;
        while ((bytesRead = file.read(buffer, sizeof(buffer))) > 0) {
          pCharacteristic->setValue(buffer, bytesRead);
          pCharacteristic->notify();
          Serial.println("Data sent");
          //delay(20); // 20ms delay to allow app time to process
        }
        file.close();
        pCharacteristic->setValue("\0");
        pCharacteristic->notify();
        return;
      }
    //}
};

void setup() {
  Serial.begin(115200);
  Serial.println("Starting BLE work!");

  // setup BLE server
  BLEDevice::init(bleServerName);
  BLEDevice::setMTU(100);
  BLEServer *pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());

  BLEService *pService = pServer->createService(SERVICE_UUID);
  pCharacteristicSetup = pService->createCharacteristic(
                                         CHARACTERISTIC_SETUP_UUID,
                                         BLECharacteristic::PROPERTY_WRITE
                                       );
  pCharacteristicSetup->setCallbacks(new MyCharacteristicCallbacks());
  pCharacteristic = pService->createCharacteristic(
                                         CHARACTERISTIC_NOTIFY_UUID,
                                         BLECharacteristic::PROPERTY_NOTIFY
                                       );
  pCharacteristic->setCallbacks(new MyCallbacks());
  
  pService->start();
  // BLEAdvertising *pAdvertising = pServer->getAdvertising();  // this still is working for backward compatibility
  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(true);
  pAdvertising->setMinPreferred(0x06);  // functions that help with iPhone connections issue
  pAdvertising->setMinPreferred(0x12);
  BLEDevice::startAdvertising();
  Serial.println("Characteristic defined! Now you can read it in your phone!");


  // attach and start monitoring the ADC pin 
  adcAttachPin(ADC_PIN);

  // intialize SPIFFS 
  if (!SPIFFS.begin(true)) {
    Serial.println("an error occurred while mounting SPIFFS");
    return;
  }

  File file = SPIFFS.open("/audio.wav", FILE_WRITE);
  if (!file) {
    Serial.println("error opening file for writing");
    return;
  }

  // write the WAV file header 
  file.print("RIFF");
  file.write((byte)0);  // Placeholder for file size, will fill in later
  file.write((byte)0);
  file.write((byte)0);
  file.write((byte)0);
  file.print("WAVEfmt ");
  file.write((byte)16);  // Subchunk size (16 for PCM)
  file.write((byte)0);
  file.write((byte)0);
  file.write((byte)0);
  file.write((byte)1);  // Audio format (1 for PCM)
  file.write((byte)0);
  file.write((byte)1);  // Number of channels
  file.write((byte)0);
  file.write((byte)(SAMPLES_RATE & 0xff));  // Sample rate
  file.write((byte)((SAMPLES_RATE >> 8) & 0xff));
  file.write((byte)0);
  file.write((byte)0);
  file.write((byte)((SAMPLES_RATE * SAMPLE_SIZE) & 0xff));  // Byte rate
  file.write((byte)(((SAMPLES_RATE * SAMPLE_SIZE) >> 8) & 0xff));
  file.write((byte)0);
  file.write((byte)0);
  file.write((byte)(SAMPLE_SIZE));  // Block align
  file.write((byte)0);
  file.write((byte)(SAMPLE_SIZE * 8));  // Bits per sample
  file.write((byte)0);
  file.print("data");
  file.write((byte)0);  // Placeholder for data size, will fill in later
  file.write((byte)0);
  file.write((byte)0);
  file.write((byte)0);

  int dataSize = 0;
  Serial.println("Recording started");
  for (int i = 0; i < SAMPLES_RATE * recordTime; i++) {
    int audioData = analogRead(ADC_PIN);
    file.write((byte)(audioData & 0xff));
    file.write((byte)((audioData >> 8) & 0xff));
    dataSize += 2;
  }
  Serial.println("Recording ended");

  file.seek(4);
  int fileSize = 36 + dataSize;
  file.write((byte)(fileSize & 0xff));
  file.write((byte)((fileSize >> 8) & 0xff));
  file.write((byte)((fileSize >> 16) & 0xff));
  file.write((byte)((fileSize >> 24) & 0xff));
  file.seek(40);
  file.write((byte)(dataSize & 0xff));
  file.write((byte)((dataSize >> 8) & 0xff));
  file.write((byte)((dataSize >> 16) & 0xff));
  file.write((byte)((dataSize >> 24) & 0xff));

  file.close();

  Serial.println("Setup call completed");
}

void loop() {
  // put your main code here, to run repeatedly:
  if (deviceConnected) {
    // If notify starts before characteristic setup, it throws an error
    if (initialConnection) {
      Serial.println("Initial connection begins");
      while(initialConnection) {
        delay(20); //TODO - check if decreasing to 10, increase the speed
      }
      Serial.println("Initial connection finished");
    }

    File file = SPIFFS.open("/audio.wav");
    if (!file) {
      Serial.println("failed to open file for reading");
      return;
    }
    uint16_t mtu = BLEDevice::getMTU();
    uint8_t buffer[mtu]; // buffer size == BLE MTU 
    int bytesRead;
    delay(10); // delay to allow app time to process
    while ((bytesRead = file.read(buffer, sizeof(buffer))) > 0) {    
      pCharacteristic->setValue(buffer, bytesRead);
      pCharacteristic->notify();
    }
    Serial.println("Notify completed");
    file.close();
    delay(10000);
    // TODO - Implement write message once file transfer is complete
  }
}