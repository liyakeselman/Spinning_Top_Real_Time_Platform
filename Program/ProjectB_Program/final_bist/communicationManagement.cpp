#pragma once

// ================= INCLUDES ======================================================

#include <Arduino.h>
#include "communicationManagement.h"
#include "rotation_and_display.h"
#include "motors.h"
#include "leds.h"

// ================= GLOBAL CONSTANTS ==============================================

#define LOCAL_BUFFER_SIZE 128

// ================= GLOBAL OBJECTS ================================================

// debug fields computed in loop() (ino)
extern volatile int g_bucket10;
extern volatile uint8_t g_digit;

// arrays for display strings in serial monitor (human readable) and logs
/* NOTE: sizes of arrays determined bigger to prevent overflow when using strncpy (our safe_volatile_strncpy func) 
and for memory alignment)*/
volatile char ModeTop[8] = "Off";  // holds "Off"/"On" valus
volatile char directionTop[32] = "not picked yet";
volatile char ModeHammer[8] = "Off";
volatile char directionHammer[32] = "not picked yet";
// ModeTop[8] , directionTop[32],  ModeHammer[8], directionHammer[32]

// global corresponding (to human readable values) values to use in app
volatile uint8_t currentTopDir = 0;      // 0=stop, 1=L, 2=R, 3=F, 4=B
volatile uint8_t currentHammerDir = 0;   // 0=stop, 1=L, 2=R, 3=F, 4=B
volatile uint8_t currentSystemMode = 0;  // 0=Idle, 1=LUNA, 2=MPU, 3=VL, 4=CHOICE, 5=CONNECT, 6=OPERATE, 7=SYNC, 8=MOVETOP  // enum defined in structureManager.h
volatile uint8_t currentTopMode = 0;     // 0=Off, 1 = On
volatile uint8_t currentHammerMode = 0;  // 0=Off, 1 = On

// ================= GLOBAL OBJECTS ================================================

AsyncWebServer server(80);  // create an instance of the web server (HTTP server)
File file; // file handling object
AsyncWebSocket web_s("/ws");  // used for connection managment

std::vector<AsyncWebSocketClient*> clients;

// ================= FUNCTIONS =====================================================

// ################# HELPER FUNC ################################################

// volatile for multithreading (regular strncpy doesn't work) and safe since with sequre lock
void safe_volatile_strncpy(volatile char* dest, const char* src, size_t n) {
  dataWriteBegin();  // sign to write-begin : turns dataSeq to odd num
  size_t i;
  // copy n strings until null terminator/ until finishes
  for (i = 0; i < n && src[i] != '\0'; i++) {
    dest[i] = src[i];
  }

  // // standard strncpy behavior:fill rest of the array with NULL as regular strncpy behaves
  for (; i < n; i++) {
    dest[i] = '\0';
  }
  dataWriteEnd();// sign to write-end : turns dataSeq back to even num
} // not used in this implementation

void volatile_strncpy(volatile char* dest, const char* src, size_t n) {
  size_t i;
  for (i = 0; i < n && src[i] != '\0'; i++) {
    dest[i] = src[i];
  }
  // // standard strncpy behavior:fill rest of the array with NULL as regular strncpy behaves
  for (; i < n; i++) {
    dest[i] = '\0';
  }
} // we use this strncpy (for multi-thread)

// ################# WEB SERVER SERVE ###########################################

// initialization function : inits Wi-Fi conncection
void wifiDualModeInit() {
  // Set dual mode: connects to router and also creates its own access point
  Serial.println(F(""));
  Serial.println(F(" Initializing Wi-Fi in dual mode (STA + AP) ..."));
  WiFi.mode(WIFI_AP_STA);  // gives connection to network and creates LAN to use app in a phone)
  WiFi.setSleep(false);
  WiFi.setAutoReconnect(true);

  // Connect to existing WiFi network (STA mode)
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to STA ("); Serial.print(WIFI_SSID); Serial.println(")");

  int retry_count = 0;
  while (WiFi.status() != WL_CONNECTED && retry_count < 20) {
    delay(500);
    Serial.print(".");
    retry_count++;
  }

  // for the .bin file over wifi
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println(F("Connected as STA netework")); Serial.print("STA IP address: "); Serial.println(WiFi.localIP());
  } else {
    Serial.println("\nWARNING: failded to connect to STA network. Proceeding with AP only.");
  }

  // for sync and app
  // Open Local Wi-Fi Network (start access point (AP mode))
  const char* ap_ssid = "ESP32-Top";
  const char* ap_password = "12345678";  // must be 8 characters minimum

  WiFi.softAP(ap_ssid, ap_password);
  IPAddress IP = WiFi.softAPIP();
  Serial.println(" Access Point started"); Serial.print("AP IP address: ");Serial.println(IP);  // AP NETWORK STARTED (WEBSOCKET)
  Serial.println(" Waiting to server to begin ...");
  delay(1000);
}

// initialization functions : inits web server conncetion
void initializeWebServer() {
  // Serve the .bin file
  server.on(FILE_NAME, HTTP_GET, [](AsyncWebServerRequest* request) {
  // "aplication/octet-stream" means it is raw binary file
  request->send(LittleFS, FILE_NAME, "application/octet-stream");
  });

  // Serve a simple HTML page at root
  server.on("/", HTTP_GET, [](AsyncWebServerRequest* request) {
    String html = "<html><body>";
    html += "<h1>ESP32 Spinning Top</h1>";
    html += "<h2>Current Binary File</h2>";
    html += "<p>Status: Running</p>";
    html += "<p><a href='/sensors_data.bin'>Download Binary Data (.bin)</a></p>";
    html += "<p>WebSocket: ws://" + WiFi.localIP().toString() + "/ws</p>";
    html += "</body></html>";
    request->send(200, "text/html", html);
  });

  // Attach WebSocket event handler
  // the web server listens to http port and if a request comes from /ws port it moves it to websocket object (upgrade http to websocket)
  web_s.onEvent(onWebSocketEvent);
  server.addHandler(&web_s);

  // Start the server
  server.begin();
  Serial.println(F(" HTTP and WebSocket server started on port 80"));
}
// request to web server not found
void notFound(AsyncWebServerRequest* request) {
  request->send(404, "text/plain", "Not found");
}

// ################# FILE SERVE #################################################

// initialization function : inits LittleFS
void LittleFSinit() {
  // Initialize LittleFS
  Serial.println(F(""));
  Serial.println(F(" Initializing LittleFS ..."));
  if (!LittleFS.begin(true)) {
    Serial.println(F(" ERROR : Failed to mount file system"));
    return;
  }
  Serial.println(F(" LittleFS mounted successfully"));
}

// Open Binary file for append
void openBinaryFileToAppend() {
  file = LittleFS.open(FILE_NAME, FILE_APPEND);
  // handle failure
  if (!file) {
    Serial.println(F(" ERROR : failed to open file for appending writes"));
    delay(2000);
    return;  // NOTE: return does not exit from loop - it continues to next iteration
  }
}

// Prepare binary file (creting only)
void prepareBinaryFile() {
  // opens file with .bin suffix in FILE_APPENDmode (overrides old file)
  File file = LittleFS.open("/sensors_data.bin", FILE_WRITE);
  if (!file) {
    Serial.println("Failed to create binary file");
    delay(2000);
    return;  // NOTE: return does not exit from loop - it continues to next iteration
  }

  // NOTE: no csvDataHeader is written here 
  // the file is simply created and immidiately closed. this is a new empty file empty - ready to receive binaric data
  file.close();
  Serial.println("Binary file prepared (no header needed)");

  openBinaryFileToAppend();
}

// Write the stored buffer data to .bin file
void writeDataToFile() {
  if (bufferCount == 0) return;  // nothing to write
  openBinaryFileToAppend();
  for (int i = 0; i < bufferCount; ++i) {
    file.write((const uint8_t*)&dataBuffer[i], sizeof(masterPacket));  // binaric write of packet
  }
  bufferCount = 0;  // clear the buffer
  file.close();     // close the file to ensure data is written
}

void writeDataToFilePartial(size_t count) {
  // writes "count" binary packets from dataBuffer
  for (size_t i = 0; i < count; ++i) {
    // (const uint8_t*)& says to logger: "refer packet memory address as sequence of bytes"
    file.write((const uint8_t*)&dataBuffer[i], sizeof(masterPacket));
  }
  file.flush();  // pushes the data from LittleFS buffer to file itself
}

// ################# APP SERVE ##################################################

void BinUpdateApp(masterPacket currentPacket) {
  // send to app (fast copy to buffer)
  web_s.binaryAll((uint8_t*)&currentPacket, sizeof(masterPacket));

  /*web_s.cleanupClients();
  for (auto* client : clients) {
    if (!client || client->status() != WS_CONNECTED) continue;
    // If the client is already overloaded, skip this frame.
    // It is better to drop one telemetry packet than to crash/flood the WebSocket queue.
    if (client->queueIsFull()) continue;

    client->binary((uint8_t*)&currentPacket, sizeof(masterPacket));
  }*/
}

void sendStartSpinningMassege() {
  // create local document - safe to use in multi-threading parellely
  StaticJsonDocument<200> localDoc;
  char localBuffer[LOCAL_BUFFER_SIZE];

  //Serial.println(F("[STATE] Start spinning detected"));
  localDoc["action"] = "statusUpdate";
  localDoc["status"] = "startSpinning";

  serializeJson(localDoc, localBuffer);

  for (auto* client : clients) {
    if (client->status() == WS_CONNECTED) {
      client->text(localBuffer);
    }
  }
}

void sendStopSpinningMassege() {
  StaticJsonDocument<200> localDoc;
  char localBuffer[LOCAL_BUFFER_SIZE];
  //Serial.println(F("[STATE] Stop spinning detected"));
  localDoc["action"] = "statusUpdate";
  localDoc["status"] = "stopSpinning";

  serializeJson(localDoc, localBuffer);

  for (auto* client : clients) {
    if (client->status() == WS_CONNECTED) {
      client->text(localBuffer);
    }
  }
  // localDoc.clear(); // no need to clear since it is local doc, so deletes after func finishes automatically
}

void onWebSocketEvent(AsyncWebSocket* server, AsyncWebSocketClient* client, AwsEventType type, void* arg, uint8_t* data, size_t len) {
  if (type == WS_EVT_CONNECT) {
    Serial.printf("WebSocket client connected. Total clients: %u\n", clients.size() + 1);
    clients.push_back(client);  // Add the client to the list
  } else if (type == WS_EVT_DISCONNECT) {
    Serial.println("WebSocket client disconnected");
    // no need to call client->close() since the connection is already closed
    clients.erase(std::remove(clients.begin(), clients.end(), client), clients.end());  // Remove the client from the list
  } else if (type == WS_EVT_DATA) { // determination of ESP's response to incoming messages.
    AwsFrameInfo* info = (AwsFrameInfo*)arg;

    // check if received binary data and in the correct size
    // ---- 1. Fast: binary messages (ControlCommand) 
    if (info->opcode == WS_BINARY && len == sizeof(ControlCommand)) {
      // Serial.printf("Received Binary Message, length: %u\n", len); // debug only! otherwise it slows the running
      ControlCommand* cmd = (ControlCommand*)data;
      // Serial.printf("Binary command received: ID=%d, Dir=%d\n", cmd->commandId, cmd->direction); // debug only! otherwise it slows the running

      //for moveTop : Need to change here to support the movement of the spinning top
      switch (cmd->commandId) {  // action: 1moveTop,2battery,3modeTop, 4changeActivity,5modeHammer,6moveHammer

        case 1:  // moveTop
          {
            static const char* topDirs[] = { "stop", "left", "right", "forward", "backward" };
            if (cmd->direction <= 4) {  // ensure access insided boundaries
             
              dataWriteBegin(); // one lock to all this update

              currentTopDir = cmd->direction; 
              dataAll.currentTopDir = cmd->direction; // update in struct for sync (critical for real-time)

              // update string to display
              volatile_strncpy(directionTop, topDirs[cmd->direction], sizeof(directionTop) - 1);
              directionTop[sizeof(directionTop) - 1] = '\0';  // ensure word always closed with NULL terminator

              // update logic flags of moving top
              is_left = (cmd->direction == 1);
              is_right = (cmd->direction == 2);
              is_forward = (cmd->direction == 3);
              is_backward = (cmd->direction == 4);

              // shutdown movement flags if received instruction to stop (from the app)
              if (cmd->direction == 0 /* 0 = Stop*/) {
                is_left = is_right = is_forward = is_backward = false;
              }

              dataWriteEnd(); // release lock
             
             // **************************************************************************************************
             /* for moveTop, changing the LED acording to the state of 
              * the direction and mode - can be removed or changed when the movment is implemented.
              * NOTE: IT HARMS THE POV DISPLAY!!
              * this block exists here only for debug the response to moving commands (while not spinning)
              */
             /* if (currentState == STOPPED && current_activity == MOVE_TOP){
                digitalWrite(gpio_led[0], is_right ? LOW : HIGH);
                digitalWrite(gpio_led[1], is_left ? LOW : HIGH);
                digitalWrite(gpio_led[2], is_forward ? LOW : HIGH);
                digitalWrite(gpio_led[3], is_backward ? LOW : HIGH);
              }*/
             // **************************************************************************************************
            }
            break;
          }

        case 2:  // battery
          {
            // Serial.println("Battery status requested");// debug only! otherwise it slows the running
            char battery_buf[10];
            battery(battery_buf);  // call the battery function

            // send direct response to the client that requested (the battery percents) - it is ok since it doesnt happen frequently
            String response = "{\"action\":\"battery\",\"status\":\"" + String(battery_buf) + "\"}";
            sendDataToClient(client, response.c_str());
            break;
          }

        case 3:  // modeTop
        // NOTE: Need to change here to support the movement of the spinning top
          {
            dataWriteBegin(); // start of sequring block - one lock in beginning of case
            currentTopMode = cmd->mode;
            dataAll.currentTopMode = cmd->mode;
            if (cmd->mode == 0) {  // identify if modeTop is "Off"
              // this commend updates only the mode (on/off)
              // mode sent explicitly from Android switch (0=Off, 1=On) to the ESP
              
              // shutdown movement flags if received instruction to stop (from the app)
              is_left = is_right = is_forward = is_backward = false;  // "Off" logic of spinning-top (zero all movement flags)

              //Serial.print("Mode: ");Serial.print(cmd->mode); // debug only! otherwise it slows the running

              // Reset the top direction to "not picked yet"
              volatile_strncpy(directionTop, "not picked yet", sizeof(directionTop) - 1);  // zero to init default text
              directionTop[sizeof(directionTop) - 1] = '\0'; // ensure word always closed with NULL terminator

              // Set ModeTop to "Off"
              volatile_strncpy(ModeTop, "Off", sizeof(ModeTop) - 1);
              ModeTop[sizeof(ModeTop) - 1] = '\0';
              //Serial.println("Top Mode: OFF"); // debug only! otherwise it slows the running
            } else {  // in all other cases execute "On" logic (default mode is "On")
              // Set ModeTop to "On"
              volatile_strncpy(ModeTop, "On", sizeof(ModeTop) - 1);
              ModeTop[sizeof(ModeTop) - 1] = '\0';
              //Serial.println("Top Mode: ON"); // debug only! otherwise it slows the running
            }

            dataWriteEnd();// always releases here - prevents LiveLock
            break;
          }

        case 4:  // changeActivity
          {
            // Serial.println("changing activity"); // debug only! otherwise it slows the running
            if (cmd->value >= 1 && cmd->value <= 8) {

              dataWriteBegin(); // start of sequring block - one lock to the whole block

              current_activity = (ACTIVITY)cmd->value;
              currentSystemMode = cmd->value;
              dataAll.currentSystemMode = cmd->value; // update struct to binary sync - critical for real-time

              dataWriteEnd(); // end of sequring block

              // using binary saves memory since we do not need use doc to save states/sub states of spinning top
              Serial.printf("Activity changed to: %d\n", current_activity);
            } else Serial.println("Received invalid Activity ID");
            break;
          }

        case 5:  // modeHammer
          { // saves data to display/send updated mode and direction to client from the action modeHammer

            dataWriteBegin();// start of sequring block

            currentHammerMode = cmd->mode;
            dataAll.currentHammerMode = cmd->mode; // critical for real-time
            // mode sent explicitly from Android switch (0=Off, 1=On) to the ESP
            if (cmd->mode == 0) {  // identify if modeHammer is "Off"
              motor_u2_stop();     // "Off" logic of hammer
              // Serial.print("Mode: "); Serial.print(cmd->mode); // debug only! otherwise it slows the running
              // Reset the direction to "not picked yet"
              volatile_strncpy(directionHammer, "not picked yet", sizeof(directionHammer) - 1);  // zero to init default text
              directionHammer[sizeof(directionHammer) - 1] = '\0'; // ensure word always closed with NULL terminator

              // Set ModeHammer to "Off"
              volatile_strncpy(ModeHammer, "Off", sizeof(ModeHammer) - 1);
              ModeHammer[sizeof(ModeHammer) - 1] = '\0';  // ensure word always closed with NULL terminator

              // Serial.println("Hammer Mode: OFF"); // debug only! otherwise it slows the running
            } else {  // in all other cases execute "On" logic (default ModeHammer is "On")
              // Set ModeHammer to "On"
              volatile_strncpy(ModeHammer, "On", sizeof(ModeHammer) - 1);
              ModeHammer[sizeof(ModeHammer) - 1] = '\0'; // ensure word always closed with NULL terminator
              //Serial.println("Hammer Mode: ON"); // debug only! otherwise it slows the running
            }

            dataWriteEnd(); // end of sequring block - prevents system crash
            break;
          }

        case 6:  // moveHammer
          {
            static const char* hammerDirs[] = { "stop", "left", "right", "forward", "backward" };
            if (cmd->direction <= 4) {  // ensure access 

              dataWriteBegin();// start of sequring block

              currentHammerDir = cmd->direction;
              dataAll.currentHammerDir = cmd->direction; // critical for real-time
              volatile_strncpy(directionHammer, hammerDirs[cmd->direction], sizeof(directionHammer) - 1);
              directionHammer[sizeof(directionHammer) - 1] = '\0'; // ensure word always closed with NULL terminator
              // Serial.printf("Hammer Binary Move: %d\n", cmd->direction);  // debug only! otherwise it slows the running

              switch (cmd->direction) {  // update direction variable for logs/display
                case 1:
                  {
                    motor_u2_left();
                   // Serial.println("Left"); // debug
                    break;
                  }  // Left
                case 2:
                  {
                    motor_u2_right();
                    // Serial.println("Right"); // debug
                    break;
                  }  // Right
                case 3:
                  {
                    motor_u2_forward();
                    // Serial.println("Forward"); // debug
                    break;
                  }  // Forward
                case 4:
                  {
                    motor_u2_backward();
                    // Serial.println("Backward"); // debug
                    break;
                  }  // Backward
                default:
                  {
                    motor_u2_stop();
                    Serial.println("Stop");
                    break;
                  }  // Stop
              }

              dataWriteEnd();// end of sequring block
            }
            break;
          }

        default:  // unknown
          {
            Serial.printf("Unknown Binary Command ID: %d\n", cmd->commandId); // debug only! otherwise it slows the running
            break;
          }
      } // closing bracket of switch(cmd->commandId)
    } // closing bracket for if condition to binary opcode threatment 
    // ---- 2. Slow: text messages (JSON) (optional for future operations: for rare commands) ---
    
  } // closing bracket of else if (type == WS_EVT_DATA)
  // here ws event is not WS_EVT_CONNECT/WS_EVT_DISCONNECT/WS_EVT_DATA
} // closing bracket for onWebSocketEvent function

void sendDataToClient(AsyncWebSocketClient* client, const char* message) {
  if (client != nullptr && client->status() == WS_CONNECTED) client->text(message);
  else {
        for (auto it = clients.begin(); it != clients.end();) {
          if ((*it)->status() == WS_DISCONNECTED) it = clients.erase(it);  // remove disconnected client
          else ++it;
        }
  }
}


// displays battery percentage (TODO: for further operation)
void battery(char* out) {
  //replace here with the functional calculation and set it inside battery
  unsigned long level = random(0, 100);
  sprintf(out, "%lu", level);
}
