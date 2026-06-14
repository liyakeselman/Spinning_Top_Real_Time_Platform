package com.example.spinner;

import androidx.appcompat.app.AppCompatActivity;
import androidx.core.content.ContextCompat;

import android.content.Intent;
import android.os.Bundle;
import android.os.Handler;
import android.util.Log;
import android.view.View;
import android.widget.Button;
import android.widget.ImageView;
import android.widget.TextView;
import android.widget.Toast;
import android.view.animation.Animation;
import android.view.animation.AnimationUtils;

public class connect extends AppCompatActivity  implements WebSocketMessageListener{
    private Button connect_b;
    private ImageView boaz;
    private TextView t;

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
        setContentView(R.layout.activity_connect);
        connect_b=findViewById(R.id.connect_button);
        t=findViewById(R.id.textView2);

        boaz = findViewById(R.id.boaz);
        boaz.setImageResource(R.drawable.boaz);
        WebSocketManager.getInstance(this).setMessageListener(this);
        // Connect to WebSocket
        WebSocketManager.getInstance(this).connectToWebSocket();

        connect_b.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                Animation animation = AnimationUtils.loadAnimation(connect.this, R.anim.rotate);
                boaz.startAnimation(animation);
                new Handler().postDelayed(() -> {
                    boaz.setImageResource(R.drawable.s_boaz);
                }, 1000); // change after 1 second

                new Handler().postDelayed(() -> {
                    Intent intent_p = new Intent(connect.this, ChoiceActivity.class);
                    startActivity(intent_p);
                }, 2500); // delay = animation duration in ms
            }
        });

    }

    @Override
    protected void onDestroy() {
       // if (WebSocketManager.getInstance().isSocketReady()) WebSocketManager.getInstance().closeWebSocket();
        super.onDestroy();
    }

    @Override
    protected void onStart() { // start of screen (not ws)
        boaz.setImageResource(R.drawable.boaz);
        super.onStart();
        WebSocketManager wsManager = WebSocketManager.getInstance(this);
        // First, register this screen as the active listener.
        wsManager.setMessageListener(this);

        // If not connected, try to connect.
        if (!wsManager.isSocketReady()) {
            wsManager.connectToWebSocket();
        }

        boolean isWsActive = WebSocketManager.getInstance(this).isSocketReady();
        updateWsStatusUI(isWsActive);
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
        //runOnUiThread(() -> Toast.makeText(getBaseContext(), "message" + message, Toast.LENGTH_LONG).show());
    }

    @Override
    public void onBinaryDataReceived(byte type, float timestamp_ms, int distanceLuna, int strengthLuna, float temperatureLuna, float mpuAngle, float mpuTotGyro, float mpuTotAcc, float vl1, float vl2, float vl3, int spinCount,  byte currentTopDir, byte currentHammerDir, byte currentSystemMode, byte currentTopMode, byte currentHammerMode) {
        // empty since we don't display here real-time telemetry / spin counter
    // Intentional empty implementation - connect screen doesn't need telemetry
    }

    @Override
    public void onOpenWebSocket() {
        String connectOpenWSMsg = "WS Connection Success while in Connect Screen." ;
        String wsConnected = "WS Connected!";
        runOnUiThread(() -> Toast.makeText(getBaseContext(), wsConnected, Toast.LENGTH_LONG).show());
        android.util.Log.d("WS_DEBUG", connectOpenWSMsg);
    }

    @Override
    public void onClosedWebSocket(String reason) {
        String connectCloseWSMsg = "WS Connection Closed while in Connect Screen: ";
        String wsClosed = "WS Closed!";
        runOnUiThread(() -> Toast.makeText(getBaseContext(), wsClosed + reason, Toast.LENGTH_LONG).show());
        android.util.Log.d("WS_DEBUG", connectCloseWSMsg + reason);
    }

    @Override
    public void onFailureWebSocket(String message) {
        runOnUiThread(new Runnable() {
            @Override
            public void run() {
                String connectFailWSMsg = "WS Connection Failed while in Connect Screen: " + message;
                String wsFailed= "WS Failed!";
                Toast.makeText(getBaseContext(), wsFailed, Toast.LENGTH_LONG).show();
                android.util.Log.e("WS_DEBUG", connectFailWSMsg);
            }
        });
    }
}