package com.example.spinner;

import android.graphics.Color;
import android.widget.TextView;
import android.widget.Toast;
import android.content.Context;
import android.util.Log; // for debugging
import android.os.Handler;
import android.os.Looper;

import org.json.JSONObject;

import okhttp3.OkHttpClient;
import okhttp3.Request;
import okhttp3.WebSocket;
import okhttp3.WebSocketListener;
import okhttp3.Response;

import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import okio.ByteString;

public class WebSocketManager {
    private static WebSocketManager instance;
    private Context context;
    private static boolean isConnectedtoWS = false;

    private static WebSocket webSocket;
    private static OkHttpClient client;
    private static WebSocketMessageListener listener;

    private final Handler mainHandler = new Handler(Looper.getMainLooper());
    private boolean reconnectScheduled = false;

    private void scheduleReconnect() {
        if (reconnectScheduled) return;

        reconnectScheduled = true;
        mainHandler.postDelayed(() -> {
            reconnectScheduled = false;
            if (!isSocketReady()) {
                Log.d("WS_STATE", "Trying to reconnect WebSocket ...");
                connectToWebSocket();
            }
        }, 2000);
    }

    // Private constructor to prevent instantiation
    private WebSocketManager(Context context) {
        this.context = context.getApplicationContext(); // saves tge application context to avoid memory leakage
        client = new OkHttpClient.Builder()
                .pingInterval(0, java.util.concurrent.TimeUnit.SECONDS) // cancels sending quiet pings every 0 seconds (cancel identifying "ghost connection" to ESP32 ap)
                .build();
    }

    private static final int MASTER_PACKET_SIZE = 99;

    // Singleton getInstance method
    public static WebSocketManager getInstance(Context context) {
        if(instance == null) {
            synchronized (WebSocketManager.class) {// only one thread can create the WebSocketManager
                if (instance == null) {
                    instance = new WebSocketManager(context);
                }
            }
        }
        return instance;
    }

    // Set the listener that handles messages
    public void setMessageListener(WebSocketMessageListener listener) {
        WebSocketManager .listener = listener;
    }

    public static WebSocketMessageListener getListener() {
        return listener;
    }


    // Connect to WebSocket
    public void connectToWebSocket() {
        Request request = new Request.Builder()
                .url("ws://192.168.4.1/ws") // Use WebSocket URL
                .build();


        webSocket = client.newWebSocket(request, new WebSocketListener() {
            @Override
            public void onOpen(WebSocket webSocket, Response response) {
                super.onOpen(webSocket, response);
                WebSocketManager.webSocket = webSocket;
                isConnectedtoWS = true;
                reconnectScheduled = false;
                if (listener != null)  listener.onOpenWebSocket();
                Log.d("WS_STATE", "WebSocket is now OPEN and ready");
            }
            @Override
            public void onMessage(WebSocket webSocket, String text) {
                super.onMessage(webSocket, text);

                // Update global spinning state here, before forwarding the message
                // to the currently active Activity.
                try {
                    JSONObject json = new JSONObject(text);
                    String action = json.optString("action", "");

                    if (action.equals("statusUpdate")) {
                        String status = json.optString("status", "");
                        if (status.equals("stopSpinning")) {
                            MainClass.top_is_spinning = false;
                        } else if (status.equals("startSpinning")) {
                            MainClass.top_is_spinning = true;
                        }
                          Log.d("SPIN_STATE", "Global top_is_spinning = " + MainClass.top_is_spinning);
                    }
                } catch (Exception e) {
                   // Log.d("WS_PARSE", "Text message is not a status JSON: " + text);
                }
                // Keep the old behavior: the active Activity still receives the message.
                if (listener != null) {
                    listener.onMessageReceived(text);
                }
            }
            @Override
            public void onMessage(WebSocket webSocket, okio.ByteString bytes) {
                super.onMessage(webSocket, bytes);

                byte[] data = bytes.toByteArray();
                // check that received at least 99 bytes
                if (data.length >= MASTER_PACKET_SIZE) {
                    ByteBuffer buffer = ByteBuffer.wrap(data).order(ByteOrder.LITTLE_ENDIAN);
                    try {
                        // must be extracted according to order of struct masterPacket (structureManager.cpp)
                        // 1. Metadata
                        byte type = buffer.get(4);                       // Offset 4
                        float time = buffer.getFloat(5);           // Offset 5

                        // 2. LUNA (begins at offset 17 after timestamps & metadata)
                        int distL = buffer.getInt(17);             // Offset 17
                        int strL = buffer.getInt(21);              // Offset 21
                        float tempL = buffer.getFloat(25);         // Offset 25

                        // 3. MPU
                        float mAngle = buffer.getFloat(29);        // Offset 29
                        float mGyro = buffer.getFloat(33);         // Offset 33
                        float mAcc = buffer.getFloat(37);          // Offset 37

                        // 4. VL (begins in offset 65)
                        float v1 = buffer.getFloat(65);            // Offset 65
                        float v2 = buffer.getFloat(69);            // Offset 69
                        float v3 = buffer.getFloat(73);            // Offset 73

                        // 5. Rotation
                        int spins = buffer.getInt(85);          // Offset 85
                        byte currTopDir= buffer.get(94);             // Offset 94
                        byte currHammerDir=buffer.get(95);           // Offset 95
                        byte currSystemMode=buffer.get(96);          // Offset 96
                        byte currTopMode = buffer.get(97);           // Offset 97
                        byte currHammerMode = buffer.get(98);        // Offset 98

                        if (listener != null) {
                            listener.onBinaryDataReceived(type, time, distL, strL, tempL,
                                    mAngle, mGyro, mAcc,
                                    v1, v2, v3, spins, currTopDir, currHammerDir,currSystemMode,currTopMode, currHammerMode);
                        }
                    } catch (Exception e) {
                        Log.e("WS_ERROR", "Error parsing binary: " + e.getMessage());
                    }
                }else {
                        Log.w("WS_DEBUG", "Received incomplete packet. Size: " + data.length);
                }
            }

            @Override
            public void onClosed(WebSocket webSocket, int code, String reason) {
                super.onClosed(webSocket, code, reason);
                isConnectedtoWS = false;
                WebSocketManager.webSocket = null;   /* zero the variable in case the connection fails and the object still exists in memory (isSocketReady() returns true)*/
                if (listener != null) {
                    listener.onClosedWebSocket(reason);
                }
                //Log.d("WS_STATE", "WebSocket is now CLOSED");
                scheduleReconnect();
            }

            @Override
            public void onFailure(WebSocket webSocket, Throwable t, Response response) {
                super.onFailure(webSocket, t, response);
                isConnectedtoWS = false;
                WebSocketManager.webSocket = null;   /* zero the variable in case the connection fails and the object still exists in memory (isSocketReady() returns true)*/
                // sends to the current activity
                if (listener != null) listener.onFailureWebSocket(t.getMessage());
                scheduleReconnect();

                if (context != null) {
                    new android.os.Handler(android.os.Looper.getMainLooper()).post(() -> {
                        android.widget.Toast.makeText(context,
                                "WS Connection Failed: " + t.getMessage(),
                                android.widget.Toast.LENGTH_LONG).show();
                    });
                }
            }
        });
    }

    public static void sendMessage(int action, String param ) {
        if (instance.isSocketReady())
        {
            try {
                //action: 1moveTop,2battery,3modeTop, 4changeActivity,5modeHammer,6moveHammer

                // Create a new JSON object
                JSONObject jsonMessage = new JSONObject();
                if (action == 1) { // moveTop
                    jsonMessage.put("action", "moveTop");
                    jsonMessage.put("direction", param);
                } else if (action == 2) {  // battery
                    jsonMessage.put("action", "battery");
                } else if (action == 3) {  // mode on/off
                    jsonMessage.put("action", "modeTop");
                    jsonMessage.put("mode", param);
                } else if (action == 4) {  // changeActivity
                    jsonMessage.put("action", "changeActivity");
                    jsonMessage.put("activity", param);
                } else if (action == 5) {  // mode on/off
                    jsonMessage.put("action", "modeHammer");
                    jsonMessage.put("mode", param);
                } else if (action == 6) { // moveHammer
                    jsonMessage.put("action", "moveHammer");
                    jsonMessage.put("direction", param);
                }
                // Send the JSON message over WebSocket
                if (webSocket != null && webSocket.send(jsonMessage.toString())) {
                    // Don't update the UI yet, only send the message, update the UI on message received
                } else {
                    jsonMessage.put("action", "failToConnect");
                }
            } catch (Exception e) {
                e.printStackTrace();
            }
        }
    }

    public static void sendBinaryCommand(byte commandId, byte direction, byte mode, byte value) {
        if (webSocket != null && isConnectedtoWS) {
            byte[] command = new byte[4];

            command[0] = commandId; //action: 1 = moveTop, 2 = battery, 3 = modeTop, 4 = changeActivity, 5 = modeHammer, 6 = moveHammer
            command[1] = direction;
            command[2] = mode;
            command[3] = value;

            // binary send via OkHttp
            webSocket.send(okio.ByteString.of(command));
            android.util.Log.d("WS_BINARY", "Sent: ID=" + commandId + " Dir=" + direction);
        } else {
            android.util.Log.e("WS_ERROR", "Cannot send binary: WebSocket not connected");
        }
    }

    public boolean isSocketReady() {
        // only check if the object exists (instead of sending messages that load the websocket)
        return (webSocket != null) && isConnectedtoWS;
    }

    public void closeWebSocket() {
        if (webSocket != null) {
            webSocket.close(1000, "Goodbye!");
        }
        if (client != null) {
            client.dispatcher().executorService().shutdown();
        }
    }

    // function to send binary command (4 bytes)

    // Wrappers
    public static void setTopMode(boolean isOn) {
        sendBinaryCommand((byte)3, (byte)0, (byte)(isOn ? 1 : 0), (byte)0);
    }
    public static void setHammerMode(boolean isOn) {
        sendBinaryCommand((byte)5, (byte)0, (byte)(isOn ? 1 : 0), (byte)0);
    }
    // battery request (ID=2)
    public static void requestBattery() {
        sendBinaryCommand((byte)2, (byte)0, (byte)0, (byte)0);
    }
    // battery check (ID=2) (on binary command)
    public void checkBattery() {
        WebSocketManager.sendBinaryCommand((byte)2, (byte)0, (byte)0, (byte)0);
    }
    // change activity to OPERATE (ID=3, value=5)
    public void startOperateMode() {
        WebSocketManager.sendBinaryCommand((byte)4, (byte)0, (byte)0, (byte)6/*operate(start moveHammer)*/);
    }
}
