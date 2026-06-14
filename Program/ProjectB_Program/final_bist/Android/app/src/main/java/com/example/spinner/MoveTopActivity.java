package com.example.spinner;

import androidx.appcompat.app.AppCompatActivity;
import androidx.core.content.ContextCompat;

import android.content.Intent;
import android.graphics.Color;
import android.os.Bundle;

import android.util.Log; // for debugging

import android.view.View;
import android.widget.ImageButton;
import android.widget.ImageView;
import android.widget.Switch;
import android.widget.TextView;
import android.widget.Toast;


import org.json.JSONObject;


public class MoveTopActivity extends AppCompatActivity implements WebSocketMessageListener  {
    private TextView battery;
    private Switch OnOff;
    private ImageButton returnToChoice;
    private  TextView isTopSpinning;

    private ImageView backward,forward,left,right;
    static boolean on = false;
    private TextView direction;
    String ip = "";

    private void resetHardwareState() {
        // shutdown top mode (ID=3, mode = 0)
        // ID=3 (modeTop), Dir=0 (stop), Mode=0 (Off), Value=0
        WebSocketManager.sendBinaryCommand((byte)3, (byte)0, (byte)0, (byte)0);
        // shutdown hammer mode (ID=5, mode=0)
        // ID=5 (modeHammer), Dir=0 (stop), Mode=0 (Off), Value=0
        WebSocketManager.sendBinaryCommand((byte)5, (byte)0, (byte)0, (byte)0);
    }
    private void updateSpinningStatusUI(boolean isSpinning) {
        String SpinningTxt = "Spinning ...";
        String NotSpinningTxt = "Not Spinning ...";

        if (isSpinning) {
            isTopSpinning.setText(SpinningTxt);
            isTopSpinning.setTextColor(Color.GREEN);
        } else {
            isTopSpinning.setText(NotSpinningTxt);
            isTopSpinning.setTextColor(Color.GRAY);
        }
    }
    private void updateWsStatusUI(boolean connected) {
        runOnUiThread(() -> {
            if (connected) {
                String wsActive = "WS is Active";
                Toast.makeText(getBaseContext(), wsActive, Toast.LENGTH_SHORT).show();
                android.util.Log.d("WS_DEBUG", wsActive);

            } else {
                String wsNotActive = "WS is Not Active";
                //Toast.makeText(getBaseContext(), wsNotActive, Toast.LENGTH_SHORT).show();
                //android.util.Log.d("WS_DEBUG", wsNotActive);

            }
        });
    }


    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.move_top);
        // ============ findViewById && setImageResource ============
        forward = findViewById(R.id.forward);forward.setImageResource(R.drawable.up);
        backward = findViewById(R.id.backward); backward.setImageResource(R.drawable.down);
        left = findViewById(R.id.left); left.setImageResource(R.drawable.left);
        right = findViewById(R.id.right); right.setImageResource(R.drawable.right);

        OnOff = findViewById(R.id.onOff);
        direction = findViewById(R.id.textView5);
        battery = findViewById(R.id.textView18);

        returnToChoice = findViewById(R.id.return_choice);
        isTopSpinning = findViewById(R.id.is_top_spinning);

        // case 1: status updates : handle status
        updateSpinningStatusUI(MainClass.top_is_spinning);

        returnToChoice.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                Intent intent = new Intent(MoveTopActivity.this, ChoiceActivity.class);
                startActivity(intent);
            }
        });
        ip = MainClass.ip;

        // Handle modeTop (On/Off switch)
        OnOff.setOnCheckedChangeListener((buttonView, isChecked) -> {
            on = isChecked;  // Update the global 'on' boolean based on Switch state
            int mode = isChecked ? 1 : 0; // Set mode based on switch state
            // ID=2 (modeTop), Dir=0 (stop), Mode=mode (On/Off), Value=0
            WebSocketManager.sendBinaryCommand((byte)3, (byte)0, (byte)mode, (byte)0); // Send the mode update to the server
        });

        // Handle forward button click
        forward.setOnClickListener(v -> {
            if(on) {
                direction.setText("Forward cmd was sent to ESP32");

                // ID=1 (moveTop), Dir=3 (forward), Mode=1 (On), Value=0?
                WebSocketManager.sendBinaryCommand((byte)1, (byte)3, (byte)1, (byte)0);

                Log.d("TOP_UI", "Forward clicked, on=" + on +
                        ", wsReady=" + WebSocketManager.getInstance(this).isSocketReady());
                // if you see on the log "Forward clicked, on=true, wsReady=true" so it supposed to send the commend to esp
                // if you see "on=false" so the top-mode is not really "on"
                // if you see "wsReady=false" so the connection is not on
                // if all true and still doesn't move so the problem in the hardware
            }else{
                Toast.makeText(getBaseContext(), "Mode is off, can't move", Toast.LENGTH_LONG).show();
            }
        });

        // Handle backward button click
        backward.setOnClickListener(v -> {
            if(on) {
                direction.setText("Backward cmd was sent to ESP32");

                // ID=1 (moveTop), Dir=4 (backward), Mode=1 (On), Value=0?
                WebSocketManager.sendBinaryCommand((byte)1, (byte)4, (byte)1, (byte)0);

                Log.d("TOP_UI", "backward clicked, on=" + on +
                        ", wsReady=" + WebSocketManager.getInstance(this).isSocketReady());
                // if you see on the log "Backward clicked, on=true, wsReady=true" so it supposed to send the commend to esp
                // if you see "on=false" so the top-mode is not really "on"
                // if you see "wsReady=false" so the connection is not on
                // if all true and still doesn't move so the problem in the hardware
            }else{
                Toast.makeText(getBaseContext(), "Mode is off, can't move", Toast.LENGTH_LONG).show();
            }
        });

        // Handle right button click
        right.setOnClickListener(v -> {
            if(on) {
                direction.setText("Right cmd was sent to ESP32");

                // ID=1 (moveTop), Dir=2 (right), Mode=1 (On), Value=0
                WebSocketManager.sendBinaryCommand((byte)1, (byte)2, (byte)1, (byte)0);

                Log.d("TOP_UI", "Right clicked, on=" + on +
                        ", wsReady=" + WebSocketManager.getInstance(this).isSocketReady());
                // if you see on the log "Right clicked, on=true, wsReady=true" so it supposed to send the commend to esp
                // if you see "on=false" so the top-mode is not really "on"
                // if you see "wsReady=false" so the connection is not on
                // if all true and still doesn't move so the problem in the hardware
            }else{
                Toast.makeText(getBaseContext(), "Mode is off, can't move", Toast.LENGTH_LONG).show();
            }
        });

        // Handle left button click
        left.setOnClickListener(v -> {
            if(on) {
                direction.setText("Left cmd was sent to ESP32");

                // ID=1 (moveTop), Dir=1 (left), Mode=1 (On), Value=0
                WebSocketManager.sendBinaryCommand((byte)1, (byte)1, (byte)1, (byte)0);

                Log.d("TOP_UI", "left clicked, on=" + on +
                        ", wsReady=" + WebSocketManager.getInstance(this).isSocketReady());
                // if you see on the log "Left clicked, on=true, wsReady=true" so it supposed to send the commend to esp
                // if you see "on=false" so the top-mode is not really "on"
                // if you see "wsReady=false" so the connection is not on
                // if all true and still doesn't move so the problem in the hardware
            }else{
                Toast.makeText(getBaseContext(), "Mode is off, can't move", Toast.LENGTH_LONG).show();
            }
        });

        // Handle battery click
        battery.setOnClickListener(v -> {
            // ID=2 (Battery), Dir=0 (stop), Mode=1 (On), Value=0?
            WebSocketManager.sendBinaryCommand((byte)2, (byte)0, (byte)1, (byte)0);
        });
    }

    @Override
    protected void onStart() {
        super.onStart();
        if (!WebSocketManager.getInstance(this).isSocketReady()) {
            WebSocketManager.getInstance(this).connectToWebSocket();
        }
        // Set this activity as the listener to handle WebSocket messages
        WebSocketManager.getInstance(this).setMessageListener(this);

        boolean isWsActive = WebSocketManager.getInstance(this).isSocketReady();
        updateWsStatusUI(isWsActive);

        // Refresh the spinning status when entering this screen.
        updateSpinningStatusUI(MainClass.top_is_spinning);

        WebSocketManager.sendMessage(4, "moveTop"); //send message that the activity has switched

        // on ESP: enum ACTIVITY {LUNA = 1, MPU = 2, VL = 3, CHOICE = 4, CONNECT = 5, OPERATE = 6, SYNC = 7, MOVE_TOP = 8};
        // ID=4 (changeActivity), Dir=0 (stop), Mode=0 (Off), Value=8 (MOVE_TOP)
        WebSocketManager.sendBinaryCommand((byte)4, (byte)0, (byte)0, (byte)8); //send message that the activity has switched
        // init the modes when enter to the screen

        resetHardwareState();
    }

    @Override
    protected void onStop() {
        super.onStop();
        on = false;
        // ID=3 (modeTop), Dir=0 (stop), Mode=0 (Off), Value=0
        WebSocketManager.sendBinaryCommand((byte)3, (byte)0, (byte)0, (byte)0); // Send the mode update to the server
    }

    @Override
    public void onMessageReceived(String message) {
        // note: optstring is like getstring but helps to avoid crash if action is missing
        try{
            // Parse the received message as JSON
            JSONObject jsonResponse = new JSONObject(message);
            String action = jsonResponse.optString("action", "");

            switch(action) {
                case "failToConnect":
                    runOnUiThread(() -> Toast.makeText(getBaseContext(), "WebSocket not connected or send failed", Toast.LENGTH_LONG).show());
                    break;

                case "battery": // Handle "battery" action
                    // keep battery update with JSON since it is sent only when user requires and rarely happens
                    // get battery status with default fallback value 0
                    String batteryStatus = jsonResponse.optString("status", "0");
                    runOnUiThread(() -> battery.setText("Battery: " + batteryStatus + "%"));
                    break;
                case "statusUpdate":
                    // general updates on spinning-top state (starts spinning/ stops spinning)
                    String status = jsonResponse.optString("status", "");
                    runOnUiThread(() -> {
                        if (status.equals("stopSpinning")) MainClass.top_is_spinning = false;
                        else if (status.equals("startSpinning")) MainClass.top_is_spinning = true;
                        updateSpinningStatusUI(MainClass.top_is_spinning);
                    });
                    break;
                default:
                    // if received unknown action, print it to Log in debug
                    android.util.Log.d("WS_DEBUG", "Received unknown action: " + action);
                    break;
            }
        } catch (Exception e) {
            android.util.Log.e("WS_JSON", "Error parsing JSON in MoveTopActivity: " + message);
        }
    }

    @Override
    public void onBinaryDataReceived(byte type, float timestamp_ms, int distanceLuna, int strengthLuna, float temperatureLuna, float mpuAngle, float mpuTotGyro,float mpuTotAcc,  float vl1, float vl2, float vl3, int spinCount,  byte currentTopDir, byte currentHammerDir, byte currentSystemMode, byte currentTopMode, byte currentHammerMode) {
        runOnUiThread(() -> {
            // 1. update spinCount
            MainClass.spinCount = spinCount;

            // 2. translates direction of the spinning-top to text istead of JSON
            String[] topDirs = {"stop", "left", "right", "forward", "backward"};
            // check validity of value before access to array
            if (currentTopDir >= 0 && currentTopDir < topDirs.length) {
                // updates the TextView of the Top according to binary data
                direction.setText("ESP32 returned: " + topDirs[currentHammerDir]);
            }

            // 3. sync Switch (On/Off) of the app (move top screen) with the ESP
            boolean isTopOn = (currentTopMode == 1);
            on = isTopOn;   // Always update the local variable from ESP state.
            if (OnOff.isChecked() != isTopOn) {
                OnOff.setChecked(isTopOn);
            }
            on = isTopOn; // update local param
            Log.d("TOP_UI", "ESP TopMode=" + currentTopMode +
                    ", local on=" + on +
                    ", switchChecked=" + OnOff.isChecked() +
                    ", TopDir=" + currentTopDir);
        });
    }

    @Override
    public void onOpenWebSocket() {
        runOnUiThread(new Runnable() {
            @Override
            public void run() {
                String wsConnected = "WS Connected!";
                isTopSpinning.setText(wsConnected);
                isTopSpinning.setTextColor(Color.GREEN);

                Toast.makeText(getBaseContext(), wsConnected, Toast.LENGTH_SHORT).show();
                // WebSocket is connected again, so clear the red error state.
                updateSpinningStatusUI(MainClass.top_is_spinning);
                android.util.Log.d("WS_DEBUG", wsConnected + " waiting for user to direction for spinning top ...");
            }
        });
    }

    @Override
    public void onClosedWebSocket(String reason) {
        runOnUiThread(new Runnable() {
            @Override
            public void run() {
                String wsClosed = "WS Closed! ";
                String moveTopClosedWSMsg = "WS Connection Closed while in Move Top Screen: " + reason;
                isTopSpinning.setText(wsClosed);
                isTopSpinning.setTextColor(Color.RED);
                Toast.makeText(MoveTopActivity.this, wsClosed + reason, Toast.LENGTH_LONG).show();
                android.util.Log.e("WS_DEBUG", moveTopClosedWSMsg);
            }
        });
    }

    @Override
    public void onFailureWebSocket(String message) {
        runOnUiThread(new Runnable() {
            @Override
            public void run() {
                String wsFailed = "WS Failed! ";
                String moveTopFailWSMsg = "WS Connection Failed while in Move Top Screen: " + message;
                isTopSpinning.setText("WS is Not Active");
                isTopSpinning.setTextColor(Color.RED);
                Toast.makeText(MoveTopActivity.this, wsFailed + message, Toast.LENGTH_LONG).show();
                android.util.Log.e("WS_DEBUG", moveTopFailWSMsg);
            }
        });

    }

}
