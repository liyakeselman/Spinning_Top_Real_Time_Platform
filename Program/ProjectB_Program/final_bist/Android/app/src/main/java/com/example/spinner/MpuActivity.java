package com.example.spinner;

import static java.lang.Math.sqrt;

import android.content.Intent;
import android.graphics.Color;
import android.os.Bundle;
import android.util.Log; // for debugging
import android.view.View;
import android.view.WindowManager;
import android.widget.ImageButton;
import android.widget.TextView;
import android.widget.Toast; // for UI (pop-up messages)
import android.view.WindowManager;
import android.graphics.Color; // to use RED GREEN BLUE


import androidx.appcompat.app.AppCompatActivity;
import androidx.core.content.ContextCompat; // for using colors


import com.github.mikephil.charting.charts.LineChart;
import com.github.mikephil.charting.components.Legend;
import com.github.mikephil.charting.components.XAxis;
import com.github.mikephil.charting.components.YAxis;
import com.github.mikephil.charting.interfaces.datasets.ILineDataSet;
import com.github.mikephil.charting.data.Entry;
import com.github.mikephil.charting.data.LineData;
import com.github.mikephil.charting.data.LineDataSet;

import java.util.ArrayList; // ensures that exists for booting the first DataSet

import org.json.JSONObject; // for JSON
import org.json.JSONException; // for JSON

import java.util.ArrayList;
import java.util.List;

import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import okio.ByteString;

public class MpuActivity extends AppCompatActivity implements WebSocketMessageListener {

    private  LineChart[] mcharts = new LineChart[3];

    private ImageButton returnToChoice;
    private  TextView isTopSpinning;

    private int[]  mcolors = new int[]{
            Color.rgb(137,230,81), Color.rgb(89,199,250), Color.rgb(250,104,119),Color.rgb(240,230,30)
    };

    private int packetCounter = 0;

    // private List<List<Entry>> entries = new ArrayList<>();
    /* variable deleted since we changed the way graph manages its data by itself (with addEntry) we changes it since if there is windowSize points in graph and also windowSize points in this list we duplicate app memory usage with no need
     * the new method is that the graph itself holds the points so it saves duplicates in memory)
     */
    private static final int UI_UPDATE_INTERVAL_MS = 50; // 20 FPS

    // dynamic part: function to move x axis (Sliding Window) and Y limits configuration - to call only from onMessageReceived
    private void updateGraph(float newX, float newY,int graph_num, float yMin, float yMax) {
        LineChart chart = mcharts[graph_num];
        LineData data = chart.getData(); // receive the existing data in graph - extracts the object that holds the whole graph data

        // 1: check if it is the first time that this graph gets data
        if (data == null) { // if still no data in graph (first operating), only creates it once
            LineDataSet set = new LineDataSet(new ArrayList<>(), "Data"); // creates new and empty data series (DataSet)

            // design settings (executes only once)
            set.setLineWidth(3f);
            set.setColor(Color.WHITE);
            set.setDrawCircles(false);
            //set.setMode(LineDataSet.Mode.CUBIC_BEZIER); // curved lines
            set.setMode(LineDataSet.Mode.LINEAR); // linear lines
            set.setDrawValues(false);
            data = new LineData(set);
            set.setDrawValues(false);
            chart.setData(data); // connects the data object to graph
            data.setDrawValues(false);
        }
        chart.getLineData().setDrawValues(false);

        // add labels of numbers in axis
        if (!chart.getXAxis().isDrawLabelsEnabled()) {
            chart.getXAxis().setDrawLabels(true);
            chart.getAxisLeft().setDrawLabels(true);
        }

        // 2: adding the new point
        ILineDataSet set = data.getDataSetByIndex(0); // access to the data series (DataSet)
        data.addEntry(new Entry(newX, newY), 0); // adding only one point in efficient way (adds the point to existing data)

        // 3: Window Size management (preventing memory "chocking" by removing the oldest point when crossed the limit)
        // getEntryCount checks how much points currently in the graph
        // memory management (1500 points / 60 seconds)
        if (set.getEntryCount() > MainClass.windowSize) {
            set.removeEntry(0); // removes the oldest point (index 0)
        }

        // 4: updates the horizontal axis to keep the graph "running" to right side
        XAxis xAxis = chart.getXAxis();
        if (newX > MainClass.time_window) {
            xAxis.setAxisMinimum(newX-MainClass.time_window);
            xAxis.setAxisMaximum(newX);

        } else {
            xAxis.setAxisMinimum(0);
            xAxis.setAxisMaximum(MainClass.time_window);
        }
        //xAxis.setAxisMinimum(Math.max(newX - MainClass.time_window, 0));
        //xAxis.setAxisMaximum(Math.max(newX, MainClass.time_window));

        // 5: Update Y axis dynamically
        chart.getAxisLeft().setAxisMinimum(yMin * 1.1f);
        chart.getAxisLeft().setAxisMaximum(yMax * 1.1f);

        // 6: update graph visually (only once!)
        // update graph visually without rebuilding all
        data.notifyDataChanged(); // announce the data object that a change occurred
        chart.notifyDataSetChanged(); // announce the graph itself
        //chart.invalidate(); // the redraw function (Redraw)
        //chart.postInvalidate();
    }

    // static part: function to initial design - to call only from onCreate
    private void setupChartDesign(LineChart chart, int bgColor) {
        // (1) sets chart properties
        chart.setBackgroundColor(bgColor);
        chart.getDescription().setEnabled(false);
        chart.setDrawGridBackground(false);
        chart.setTouchEnabled(true);
        chart.setDragEnabled(true);
        chart.setScaleEnabled(true);
        chart.setPinchZoom(false);
        chart.getAxisRight().setEnabled(false);
        chart.getLegend().setEnabled(false);


        // (2) sets design for X axis
        XAxis xAxis = chart.getXAxis();
        xAxis.setPosition(XAxis.XAxisPosition.BOTTOM);
        xAxis.setTextColor(Color.BLACK);
        xAxis.setDrawGridLines(true);
        xAxis.setGridColor(Color.GRAY);
        xAxis.setAxisLineColor(Color.BLACK);
        xAxis.setAxisLineWidth(1.5f);
        xAxis.setAxisMinimum(0f);
        xAxis.setAxisMaximum(MainClass.time_window); // gives it initial range (for example 10 seconds)
        xAxis.setDrawGridLines(true);
        xAxis.setDrawLabels(false);

        // (3) sets design for Y axis
        YAxis yAxis = chart.getAxisLeft();
        yAxis.setTextColor(Color.BLACK);
        yAxis.setDrawGridLines(true);
        yAxis.setGridColor(Color.GRAY);
        yAxis.setAxisLineColor(Color.BLACK);
        yAxis.setAxisLineWidth(1.5f);
        yAxis.setAxisMinimum(0f);
        yAxis.setAxisMaximum(120); // gives it initial range
        yAxis.setDrawGridLines(true);
        yAxis.setDrawLabels(false);


        // (4) creating empty data
        LineDataSet set = new LineDataSet(new ArrayList<>(), "Data");

        // (5) line design that appears when data starts to be transferred
        set.setLineWidth(3f);
        set.setColor(Color.WHITE);
        set.setDrawCircles(false);
        set.setMode(LineDataSet.Mode.CUBIC_BEZIER);

        // (6) pins the empty set "set" to the graph
        LineData data = new LineData(set);
        chart.setData(data);// now the graph draws the axes (since it has data (even if empty)

        // (7) creates the chart view immediately
        chart.invalidate();
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
        setContentView(R.layout.mpu);

        // configure full screen
        getWindow().setFlags(WindowManager.LayoutParams.FLAG_FULLSCREEN,WindowManager.LayoutParams.FLAG_FULLSCREEN);

        // find the graph and design it (happens only once (in creation only))
        mcharts[0] = (LineChart) findViewById(R.id.mpu_chart1);
        mcharts[1] = (LineChart) findViewById(R.id.mpu_chart2);
        mcharts[2] = (LineChart) findViewById(R.id.mpu_chart3);

        setupChartDesign(mcharts[0], mcolors[0]);
        setupChartDesign(mcharts[1],  mcolors[1]);
        setupChartDesign(mcharts[2], mcolors[2]);

        // find more UI components
        isTopSpinning = findViewById(R.id.is_top_spinning);
        returnToChoice = findViewById(R.id.return_choice);

        // configure the Listener
        WebSocketManager.getInstance(this).setMessageListener(this);

        // connection check (and display toast) and spin-state update
        updateSpinningStatusUI(MainClass.top_is_spinning);

        // create the return button (the listener to "MPU Telemetry" button)
        returnToChoice.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                Intent intent = new Intent(MpuActivity.this, ChoiceActivity.class);
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

        // Refresh the spinning status when entering this screen.
        updateSpinningStatusUI(MainClass.top_is_spinning);

        // on ESP: enum ACTIVITY {LUNA = 1, MPU = 2, VL = 3, CHOICE = 4, CONNECT = 5, OPERATE = 6, SYNC = 7, MOVE_TOP = 8};
        // ID=4 (changeActivity), Dir=0 (stop), Mode=1 (Off), Value=2 (MPU)
        WebSocketManager.sendBinaryCommand((byte)4, (byte)0, (byte)0, (byte)2); //send message that the activity has switched
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
        // 1. JSON decode happens in Background (outside runOnUiThread)
            try {
                JSONObject json = new JSONObject(message);
                String action = json.optString("action", "");

                // status updates handling with onMessageReceived
                // data updates is handled with onBinaryDataReceived
               /* if (action.equals("statusUpdate")) {
                    String status = json.optString("status", "");
                    runOnUiThread(() -> {
                        if (status.equals("stopSpinning")) {
                            MainClass.top_is_spinning = false;
                        } else if (status.equals("startSpinning")) {
                            MainClass.top_is_spinning = true;
                        }
                        updateSpinningStatusUI(MainClass.top_is_spinning);
                    });
                }*/
                if (action.equals("statusUpdate")) {
                    updateSpinningStatusUI(MainClass.top_is_spinning);
                }
            } catch (Exception e) {
                // system simple messages or decoding errors
                Log.d("WS_PARSE", "Error Parsing JSON: " + message);
            }



    }

    private float sessionStartTime = -1.f; // global variable in the activity

    @Override
    public void onBinaryDataReceived(byte type, float timestamp_ms , int distanceLuna, int strengthLuna, float temperatureLuna, float mpuAngle, float mpuTotGyro, float mpuTotAcc , float vl1, float vl2, float vl3, int spinCount,  byte currentTopDir, byte currentHammerDir, byte currentSystemMode, byte currentTopMode, byte currentHammerMode) {
        packetCounter++;
        if (true/*packetCounter % 2 == 0*/) {
            float time_sec = timestamp_ms / 1000.0f;
            // only in the first point we save the time "zero" of the screen
            if (sessionStartTime == -1.0) {
                sessionStartTime = time_sec;
            }
            // calculate relative time to the moment we opened the activity screen
            float relativeTimeSec = (time_sec - sessionStartTime);

            // type corresponds enum ACTIVITY from ESP32 (structureManager.h):
            // enum ACTIVITY {LUNA = 1, MPU = 2, VL = 3, CHOICE = 4, CONNECT = 5, OPERATE = 6, SYNC = 7, MOVE_TOP = 8};
            if (type == 2 && relativeTimeSec >= 0) {  // type = 2 for MPU
                // check that the data belongs to the current screen
                runOnUiThread(() -> {
                    // update the spinCount in real-time
                    MainClass.spinCount = spinCount;

                    // update the graphs with the relative accurate time
                    // total pure mpu angle [deg]
                    if (!Float.isNaN(mpuAngle) && !Float.isInfinite(mpuAngle)) updateGraph(relativeTimeSec, mpuAngle, 0, 0f, 360f);
                    // angular velocity (total gyro) [rad/sec]
                    if (!Float.isNaN(mpuTotGyro) && !Float.isInfinite(mpuTotGyro)) updateGraph(relativeTimeSec, mpuTotGyro, 1, -45f, 45f);
                    // angular acceleration (total acc) [m/sec^2]
                    if (!Float.isNaN(mpuTotAcc) && !Float.isInfinite(mpuTotAcc)) updateGraph(relativeTimeSec, mpuTotAcc, 2, -60f, 60f);

                    mcharts[0].invalidate();
                    mcharts[1].invalidate();
                    mcharts[2].invalidate();
                });
            }
        }
    }

    @Override
    public void onOpenWebSocket() {
        runOnUiThread(new Runnable() {
            @Override
            public void run() {
                String wsConnected = "WS Connected!";
                Toast.makeText(MpuActivity.this, wsConnected, Toast.LENGTH_SHORT).show();
                // WebSocket is connected again, so clear the red error state.
                updateSpinningStatusUI(MainClass.top_is_spinning);
                for (LineChart chart : mcharts) {
                    chart.setNoDataText("Waiting for MPU data ...");
                    chart.invalidate();
                }
                android.util.Log.d("WS_DEBUG", wsConnected + " waiting for MPU data ...");
            }
        });
    }

    @Override
    public void onClosedWebSocket(String reason) {
        runOnUiThread(new Runnable() {
            @Override
            public void run() {
                String wsClosed = "WS Closed. Reason: " + (reason.isEmpty() ? "Unknown" : reason);
                Toast.makeText(MpuActivity.this, wsClosed, Toast.LENGTH_LONG).show();
                isTopSpinning.setText("Disconnected");
                isTopSpinning.setTextColor(ContextCompat.getColor(MpuActivity.this, R.color.red));
                android.util.Log.e("WS_DEBUG", wsClosed);
            }
        });
    }

     @Override
    public void onFailureWebSocket(String message) {
        runOnUiThread(new Runnable() {
            @Override
            public void run() {
                isTopSpinning.setText("WS is Not Active");
                isTopSpinning.setTextColor(ContextCompat.getColor(MpuActivity.this, R.color.red));

                for (LineChart chart : mcharts) {
                    chart.setNoDataText("No WS Connection...");
                    chart.invalidate();
                }

                String MpuFailWSMsg = "WS Connection Failed while in MPU Screen: " + message;
                String wsFailed= "WS Failed!";
                Toast.makeText(MpuActivity.this, wsFailed, Toast.LENGTH_LONG).show();
                android.util.Log.e("WS_DEBUG", MpuFailWSMsg);
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


