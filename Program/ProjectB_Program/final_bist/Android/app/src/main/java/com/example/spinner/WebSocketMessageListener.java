package com.example.spinner;

public interface WebSocketMessageListener {
    void onMessageReceived(String message);
    void onBinaryDataReceived(byte type, float timestamp_ms, int distanceLuna, int strengthLuna, float temperatureLuna, float mpuAngle, float mpuTotGyro,float mpuTotAcc,  float vl1, float vl2, float vl3, int spinCount, byte currentTopDir, byte currentHammerDir, byte currentSystemMode, byte currentTopMode, byte currentHammerMode);
    void onOpenWebSocket();
    void onClosedWebSocket(String reason);
    void onFailureWebSocket(String message);
}
