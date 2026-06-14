package com.example.spinner;

import android.content.Intent;
import android.graphics.Color;
import android.os.Bundle;
import android.view.View;
import android.widget.ImageButton;
import android.widget.TextView;
import android.widget.Toast; // for UI (pop-up messages)
import android.util.Log; // for debugging
import android.view.WindowManager;

import androidx.activity.EdgeToEdge;
import androidx.appcompat.app.AppCompatActivity;
import androidx.cardview.widget.CardView;
import androidx.core.content.ContextCompat; // for using colors

import com.github.mikephil.charting.charts.LineChart;

import org.json.JSONObject; // for JSON
import org.json.JSONException; // for JSON

import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import okio.ByteString;

public class ChoiceActivity extends AppCompatActivity implements  WebSocketMessageListener{
    private CardView moveTop, moveHammer, lunaGraphs, mpuGraphs, vlGraphs ;
    private ImageButton returnToConnect;
    private TextView isTopSpinning ,spinCountText;

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


    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.choice);

        // configure full screen
        getWindow().setFlags(WindowManager.LayoutParams.FLAG_FULLSCREEN, WindowManager.LayoutParams.FLAG_FULLSCREEN);

        // find all UI components
        moveTop = findViewById(R.id.move_top);
        moveHammer = findViewById(R.id.move_hammer);
        lunaGraphs = findViewById(R.id.luna_graphs);
        mpuGraphs = findViewById(R.id.mpu_graphs);
        vlGraphs = findViewById(R.id.vl_graphs);
        returnToConnect = findViewById(R.id.return_choice);
        isTopSpinning = findViewById(R.id.is_top_spinning);
        spinCountText = findViewById(R.id.spin_count_text);

        // configure the Listener to WebSocket
        WebSocketManager.getInstance(this).setMessageListener(this);

        // connection check (and display toast) and spin-state update
        updateSpinningStatusUI(MainClass.top_is_spinning);

        // configure all Click Listeners
        moveTop.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                Intent intent = new Intent(ChoiceActivity.this, MoveTopActivity.class);
                startActivity(intent);
            }
        });
        moveHammer.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                Intent intent = new Intent(ChoiceActivity.this, operate.class);
                startActivity(intent);
            }
        });
        lunaGraphs.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                Intent intent = new Intent(ChoiceActivity.this, LunaActivity.class);
                startActivity(intent);
            }
        });
        mpuGraphs.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                Intent intent = new Intent(ChoiceActivity.this, MpuActivity.class);
                startActivity(intent);
            }
        });
        vlGraphs.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                Intent intent = new Intent(ChoiceActivity.this, VlActivity.class);
                startActivity(intent);
            }
        });

        // create the return button (the listener to "lets go for a spin!" button)
        returnToConnect.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                Intent intent = new Intent(ChoiceActivity.this, connect.class);
                startActivity(intent);
            }
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

        // on ESP: enum ACTIVITY {LUNA = 1, MPU = 2, VL = 3, CHOICE = 4, CONNECT = 5, OPERATE = 6, SYNC = 7, MOVE_TOP = 8};
        // ID=4 (changeActivity), Dir=0 (stop), Mode=0 (Off), Value=4(CHOICE)
        WebSocketManager.sendBinaryCommand((byte)4, (byte)0, (byte)0, (byte)4); //send message that the activity has switched
        spinCountText.setText(String.format("%d", MainClass.spinCount));
    }

    @Override
    protected void onStop() {
        super.onStop();
        // safety check: delete the listener only if I am the current listener!
        if (WebSocketManager.getListener() == this) {
            WebSocketManager.getInstance(this).setMessageListener(null);
        }
    }

    @Override
    public void onMessageReceived(String message) {
        try {
            JSONObject jsonResponse = new JSONObject(message);
            // using optString to not crash if no "action"
            String action = jsonResponse.optString("action", "");

            if (action.equals("failToConnect")) {
                runOnUiThread(() -> {
                    Toast.makeText(getBaseContext(), "WebSocket not connected or send failed", Toast.LENGTH_LONG).show();
                });
            }
            else if (action.equals("statusUpdate")) {
                String status = jsonResponse.optString("status", "");

                runOnUiThread(() -> {
                    //  status updates : handle status
                   /* if (status.equals("stopSpinning")) {
                        MainClass.top_is_spinning = false;
                    } else if (status.equals("startSpinning")) {
                        MainClass.top_is_spinning = true;
                    }*/
                    updateSpinningStatusUI(MainClass.top_is_spinning);

                    // update number of spins
                    if (status.equals("spinCountUpdate")) {
                        // pulls the data according to the key "spinCount" we configured in communicationManagement.cpp
                        int count = jsonResponse.optInt("spinCount", MainClass.spinCount);
                        MainClass.spinCount = count;
                        spinCountText.setText(String.valueOf(count));
                    }
                });
            }
        }catch (JSONException e) {
            // if receives message that is not in JSON format (like "Connected") , it simply prints it to log and not crashes
            Log.d("WS_PARSE", "Plain text message: " + message);
        } catch (Exception e) {
            // system simple messages or decoding errors
            Log.d("WS_PARSE", "Message: " + message);
        }
    }

    @Override
    public void onBinaryDataReceived(byte type, float timestamp_ms, int distanceLuna, int strengthLuna, float temperatureLuna, float mpuAngle, float mpuTotGyro, float mpuTotAcc, float vl1, float vl2, float vl3, int spinCount, byte currentTopDir, byte currentHammerDir, byte currentSystemMode, byte currentTopMode, byte currentHammerMode) {
        // we want to update spinCount no matter what is the type of activity
        runOnUiThread(() -> {
            MainClass.spinCount = spinCount; // update the global variable
            spinCountText.setText(String.valueOf(spinCount)); // update the text in screen

            // update the status Spinning .../ Not Spinning ...
          /*  boolean isSpinning = (currentTopMode == 1);
            if (isSpinning != MainClass.top_is_spinning) {
                MainClass.top_is_spinning = isSpinning;
                updateSpinningStatusUI(isSpinning);
            }*/

            // Refresh the spinning status when entering this screen.
            updateSpinningStatusUI(MainClass.top_is_spinning);

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

                Toast.makeText(ChoiceActivity.this, wsConnected, Toast.LENGTH_SHORT).show();
                // WebSocket is connected again, so clear the red error state.
                updateSpinningStatusUI(MainClass.top_is_spinning);
                android.util.Log.d("WS_DEBUG", wsConnected + " waiting user to choose action ...");
            }
        });
    }

    @Override
    public void onClosedWebSocket(String reason) {
        runOnUiThread(new Runnable() {
            @Override
            public void run() {
                String wsClosed = "WS Closed. Reason: " + (reason.isEmpty() ? "Unknown" : reason);
                Toast.makeText(ChoiceActivity.this, wsClosed, Toast.LENGTH_LONG).show();
                isTopSpinning.setText("Disconnected");
                isTopSpinning.setTextColor(ContextCompat.getColor(ChoiceActivity.this, R.color.red));
                android.util.Log.e("WS_DEBUG", wsClosed);
            }
        });
    }

    @Override
    public void onFailureWebSocket(String message) {
        runOnUiThread(new Runnable() {
            @Override
            public void run() {
                String wsFailed= "WS Failed!";
                isTopSpinning.setText("WS is Not Active");
                isTopSpinning.setTextColor(ContextCompat.getColor(ChoiceActivity.this, R.color.red));
                String choiceFailWSMsg = "WS Connection Failed while in Choice Screen: " + message;
                Toast.makeText(ChoiceActivity.this, wsFailed, Toast.LENGTH_LONG).show();

                android.util.Log.e("WS_DEBUG", choiceFailWSMsg);
            }
        });
    }

    @Override
    protected void onDestroy() {
        super.onDestroy();
        // announce the manager to stop sending messages to this screen since it had been closed
        WebSocketManager.getInstance(this).setMessageListener(null);
    }


}


