//Server Code
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include <SPIFFS.h>

#define SERVICE_UUID        "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CHARACTERISTIC_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a8"
//#define CONTROL_CHARACTERISTIC_UUID "a1b2c3d4-e5f6-4788-b7f5-ea07361b26a8"

#define SAMPLES_RATE 8000 
#define SAMPLE_SIZE 2 // 2 bytes for 16-bit audio 
#define ADC_PIN 14
BLECharacteristic *pCharacteristic;
uint i = 0;

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
  BLEDevice::init("XIAO_ESP32S3");
  BLEServer *pServer = BLEDevice::createServer();
  BLEService *pService = pServer->createService(SERVICE_UUID);
  pCharacteristic = pService->createCharacteristic(
                                         CHARACTERISTIC_UUID,
                                         BLECharacteristic::PROPERTY_READ |
                                         BLECharacteristic::PROPERTY_WRITE |
                                         BLECharacteristic::PROPERTY_NOTIFY
                                       );
  pCharacteristic->setCallbacks(new MyCallbacks());
                        
  //BLECharacteristic *pControlCharacteristic = pService->createCharacteristic(
  //                                       CONTROL_CHARACTERISTIC_UUID,
  //                                       BLECharacteristic::PROPERTY_WRITE
  //                                     );
  //pControlCharacteristic->setCallbacks(new MyCallbacks());
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
  for (int i = 0; i < SAMPLES_RATE * 3; i++) { // 10 second recording
    int audioData = analogRead(ADC_PIN);
    file.write((byte)(audioData & 0xff));
    file.write((byte)((audioData >> 8) & 0xff));
    dataSize += 2;
  }

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
}

void loop() {

}