#pragma once

// #include <Arduino.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>

const char* service_uuid = "6E400001-B5A3-F393-E0A9-E50E24DCCA9E";
const char* char_tx_uuid = "6E400003-B5A3-F393-E0A9-E50E24DCCA9E";
const char* char_rx_uuid = "6E400002-B5A3-F393-E0A9-E50E24DCCA9E";

BLEServer* pServer;
BLECharacteristic* pTxCharacteristic;
BLEService* pService;
BLECharacteristic* pRxCharacteristic;
bool phone_connected = false;

class ServerCallbacks : public BLEServerCallbacks {
  void onConnect(BLEServer* pServer) {
    phone_connected = true;
    Serial.println("Phone connected");
  }

  void onDisconnect(BLEServer* pServer) {
    phone_connected = false;
    Serial.println("Phone disconnected");
    BLEDevice::startAdvertising();
    Serial.println("Restarted advertising — ready for new connection");
  }
};

class RxCallbacks : public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic* pCharacteristic) {
    String value = pCharacteristic->getValue();
    if (value.length() > 0) {
      Serial.print("Received from phone: ");
      Serial.println(value.c_str());
    }
  }
};

void ble_setup() {
  BLEDevice::init("Ripple Glasses");

  BLEDevice::setMTU(512);

  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new ServerCallbacks());

  pService = pServer->createService(service_uuid);

  pTxCharacteristic = pService->createCharacteristic(char_tx_uuid, BLECharacteristic::PROPERTY_NOTIFY);
  pTxCharacteristic->addDescriptor(new BLE2902());

  pRxCharacteristic = pService->createCharacteristic(char_rx_uuid, BLECharacteristic::PROPERTY_WRITE);
  pRxCharacteristic->setCallbacks(new RxCallbacks());

  Serial.println("BLE Service configured!");
  pService->start();

  BLEDevice::startAdvertising();
  Serial.println("BLE advertising — waiting for connection...");
}