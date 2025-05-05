#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>

#define SERVICE_UUID        "35599e64-de60-4b35-8a25-25dad1aac092"
#define CHARACTERISTIC_UUID "64837ee7-fbef-4c8c-bf95-7b5896215ff9"

#define PARALLEL_0  12
#define REPEATS 6 // higher is more reliable, but takes longer

void parallel_write(uint8_t value) {
  uint32_t output =
    (REG_READ(GPIO_OUT_REG) & ~(0xFF << PARALLEL_0)) | (((uint32_t)value) << PARALLEL_0);

  REG_WRITE(GPIO_OUT_REG, output);
}

void writeBus(bool dat, bool ext, bool clk) {
  parallel_write(
    (dat  ? 0b11111000 : 0) |
    (ext  ? 0b00000100 : 0) |
    (clk  ? 0b00000010 : 0) |
    (!clk ? 0b00000001 : 0)
  );
  
  delayMicroseconds(200);
}

void writeBit(bool b) {
  writeBus(b, true, true);
  writeBus(b, true, false);
}

void writeWord(uint8_t w) {
  writeBit(true);

  for (int i = 6; i >= 0; i--) {
    writeBit(
      ((w >> i) & 1)
    );
  }

  writeBit(false);
}

void writeSep() {
  writeBus(false, true, true);
  writeBus(false, false, true);
  writeBus(true, false, true);
  writeBus(true, false, false);
  writeBus(false, false, true);
  writeBus(false, true, true);
}

class bleCallback: public BLECharacteristicCallbacks
{
  void onWrite(BLECharacteristic *pCharacteristic)
  {
    String values = pCharacteristic->getValue();
    
    if (values.length() < 62) { return; }

    for (int i = 0; i < REPEATS; i++) {
      writeSep();

      // leading two zero bytes might be not required
      writeWord(0b0000000);
      writeWord(0b0000000);

      for (int j = 0; j < 62; j++) {
        writeWord(values[j]);
      }
    }
    
    writeSep();
  }
};

void setup()
{
  for (int i = 0; i < 8; i++) {
    pinMode(PARALLEL_0 + i, OUTPUT);
  }
  
  // BLE setup
  BLEDevice::init("ib-fallblatt");
  BLEServer *pServer = BLEDevice::createServer();
  BLEService *pService = pServer->createService(SERVICE_UUID);
  BLECharacteristic *pCharacteristic = pService->createCharacteristic(
                                         CHARACTERISTIC_UUID,
                                         BLECharacteristic::PROPERTY_READ |
                                         BLECharacteristic::PROPERTY_WRITE
                                       );
  pCharacteristic->setCallbacks(new bleCallback());
  pCharacteristic->setValue("flap_positions");
  pService->start();
  BLEAdvertising *pAdvertising = pServer->getAdvertising();
  pAdvertising->start();
  
}

void loop() {}
