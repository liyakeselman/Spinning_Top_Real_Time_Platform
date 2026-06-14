#ifndef COMMUNICATIONMAMAGEMENT_H
#define COMMUNICATIONMAMAGEMENT_H

#pragma once

// ================= INCLUDES ======================================================

#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <AsyncTCP.h>
#include <FS.h>
#include <SPI.h>
#include "LittleFS.h" 
#include <Wire.h>
#include <ArduinoJson.h>
#include <vector>
#include "structureManager.h" 

// ================= GLOBAL OBJECTS ================================================

// global objects : web server and file handling objects 
extern AsyncWebServer server;  // create an instance of the web server (HTTP server)
extern File file;
extern AsyncWebSocket web_s;   // for connection managment

extern std::vector<AsyncWebSocketClient *> clients;

// ================= GLOBAL CONSTANTS =============================================

constexpr const char* FILE_NAME = "/sensors_data.bin";

// ================= FUNCTIONS ====================================================

void safe_volatile_strncpy(volatile char* dest, const char* src, size_t n); // not used in this implementation
void volatile_strncpy(volatile char* dest, const char* src, size_t max_len);
void wifiDualModeInit();
void initializeWebServer();
void notFound(AsyncWebServerRequest *request);
void LittleFSinit();
void openBinaryFileToAppend();
void prepareBinaryFile();
void writeDataToFile();
void writeDataToFilePartial(size_t count);
void BinUpdateApp(masterPacket currentPacket);
void sendStartSpinningMassege();
void sendStopSpinningMassege();
void onWebSocketEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len);
void sendDataToClient(AsyncWebSocketClient *client, const char *message);
void battery(char* out);

extern void dataWriteBegin();
extern void dataWriteEnd();

#endif
