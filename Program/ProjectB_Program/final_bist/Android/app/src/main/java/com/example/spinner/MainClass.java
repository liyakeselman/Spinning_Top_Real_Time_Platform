package com.example.spinner;


public class MainClass {
    public static final String ip = "192.168.4.1";
    public static Boolean top_is_spinning = false;

    /*
    * ST [ms] := samplingTaskInterval
    * SR [Hz] := samplingTask rate (number of messages per second) = 1/ST[sec]
    * windowSize := winSize
    * the graph includes data of X seconds,
    * where X[sec]=(winSize[number of points])/(SR[sec]) = (winSize[number of points])*(ST[sec])
     * the graph looks great only for X[sec]
    * if time_window > X[sec] the graph looks choppy since the old points are
    * deleted before they exit the screen
    * We should ensure that the ESP32 rate is properly aligned with the window size (i.e., their ratio is consistent to what we want to see in the eyes)
    *
    * if we want to see 2 minutes of rotating (without setup time)
    * so X[sec] = 120, ST[ms] = 200 --> winSize[points] = 120/0.2 = 6000 [number of points]
    * so time_window has to be less then X[sec] so the graph will look great
    *
    * for winSize[points] = 1000 and ST[ms]=20 --> X[sec] = 20. time_window[sec]= 10.0f < X
    * */
    public static int windowSize = 250; // Number of points to show
    public static float time_window = 5.0f; // display window

    public static int spinCount = 0;
}
/* worked with windowSize = 1000, time_window = 5.0f;, DESIRED = 6000 for samplingTask 200ms no write in LoggerTask*/
/* worked with windowSize = 600, time_window = 10.0f;, DESIRED = 2400 for samplingTask 100ms no write in LoggerTask*/
/* worked with windowSize = 800, time_window = 10.0f;, DESIRED = 2400 for samplingTask 70ms no write in LoggerTask*/
/* worked with windowSize = 1500, time_window = 10.0f;, DESIRED = 1500 for samplingTask 40ms no write in LoggerTask - 60 seconds sim*/
/* worked with windowSize = 1500, time_window = 10.0f;, DESIRED = 1500 for samplingTask 40ms with write in LoggerTask - 55-60 seconds sim*/

/* might work with windowSize = 1200, time_window = 10.0f;, DESIRED = 1200 for samplingTask 50ms no write in LoggerTask*/
// check- what is the math?

/* might work with windowSize = 3000, time_window = 10.0f;, DESIRED = 3000 for samplingTask 20ms with write in LoggerTask (bin)*/
// check- what is the math?

