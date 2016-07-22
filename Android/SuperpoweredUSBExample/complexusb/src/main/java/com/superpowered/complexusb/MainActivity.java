package com.superpowered.complexusb;

import android.app.Instrumentation;
import android.content.DialogInterface;
import android.support.v7.app.AlertDialog;
import android.support.v7.app.AppCompatActivity;
import android.os.Bundle;
import android.view.KeyEvent;
import android.view.View;
import android.widget.AdapterView;
import android.widget.ArrayAdapter;
import android.widget.ListView;
import android.os.Handler;
import android.widget.Toast;

import java.util.Timer;
import java.util.TimerTask;

public class MainActivity extends AppCompatActivity implements SuperpoweredUSBAudioHandler, CustomAdapterHandler {
    private ListView list;
    private ArrayAdapter<String> waitingAdapter;
    private CustomAdapter adapter;
    private int inputPath;
    private int outputPath;
    private int thruPath;
    private float inputFeatures[];
    private float outputFeatures[];
    private float thruFeatures[];
    private int inputIndex;
    private int outputIndex;
    private int lastLatencyMs;
    private Handler handler;
    private int latencyIndex;
    private int deviceID;
    private boolean hasDevice;
    private Timer fakeTouchTimer;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        inputPath = outputPath = thruPath = -1;
        inputFeatures = outputFeatures = thruFeatures = null;
        latencyIndex = 0;
        deviceID = 0;
        hasDevice = false;
        SuperpoweredUSBAudio usbAudio = new SuperpoweredUSBAudio(getApplicationContext(), this);

        waitingAdapter = new ArrayAdapter<>(this, android.R.layout.simple_list_item_1, new String[] { "Please connect a USB audio or MIDI device."});
        list = (ListView)findViewById(R.id.list);
        if (list != null) list.setAdapter(waitingAdapter);
        adapter = new CustomAdapter(this);

        usbAudio.check();
    }

    public void onItemClick(int kind, int index, int position) {
        switch (kind) {
            case CustomAdapter.SECTIONHEADERITEM:
                if (position == 0) {
                    String deviceInfo[] = getUSBAudioDeviceInfo(deviceID);
                    AlertDialog.Builder builder = new AlertDialog.Builder(this);
                    builder.setTitle("Device Info");
                    builder.setMessage(deviceInfo[2]);
                    builder.setNeutralButton("Ok", new DialogInterface.OnClickListener() {
                        @Override
                        public void onClick(DialogInterface dialog, int which) {
                            dialog.cancel();
                        }
                    });
                    builder.create().show();
                }
                break;
            case CustomAdapter.CONFIGURATIONITEM: setConfiguration(index); break;
            case CustomAdapter.OUTPUTITEM: setOutput(index); break;
            case CustomAdapter.INPUTITEM: setInput(index); break;
            case CustomAdapter.INPUTPATHITEM:
            case CustomAdapter.OUTPUTPATHITEM:
            case CustomAdapter.THRUPATHITEM: setPath(kind, index, true); break;
            case CustomAdapter.LATENCYITEM:
                latencyIndex = index;
                adapter.selectItemInSection(CustomAdapter.LATENCYITEM, latencyIndex);
                onAction(2, true);
                break;
            case CustomAdapter.ACTIONITEM: onAction(index, true); break;
        }
    }

    public void onUSBAudioDeviceAttached(int deviceIdentifier) {
        if (hasDevice) return;
        deviceID = deviceIdentifier;
        String configurationInfo[] = getConfigurationInfo(deviceID);
        adapter.removeAllItems();
        adapter.addSectionHeader("Select Configuration");
        adapter.addItems(configurationInfo, CustomAdapter.CONFIGURATIONITEM);
        if (configurationInfo.length == 1) setConfiguration(0);
        adapter.notifyDataSetChanged();

        list.setAdapter(adapter);
        list.setOnItemClickListener(new AdapterView.OnItemClickListener() {
            @Override
            public void onItemClick(AdapterView<?> parent, View view, int position, long id) {
                int[] ki = adapter.getItemKindAndIndex(position);
                MainActivity.this.onItemClick(ki[0], ki[1], position);
            }
        });

        TimerTask fakeTouchTask = new TimerTask() {
            public void run() {
                try {
                    Instrumentation instrumentation = new Instrumentation();
                    instrumentation.sendKeyDownUpSync(KeyEvent.KEYCODE_BACKSLASH);
                } catch(java.lang.Exception e) {
                    assert true;
                }
            }
        };
        fakeTouchTimer = new Timer();
        fakeTouchTimer.schedule(fakeTouchTask, 1000, 1000);
    }

    public void onUSBMIDIDeviceAttached(int deviceIdentifier) {

    }

    public void onUSBDeviceDetached(int deviceIdentifier) {
        if (deviceIdentifier == deviceID) {
            list.setAdapter(waitingAdapter);
            hasDevice = false;
            fakeTouchTimer.cancel();
            fakeTouchTimer.purge();
        }
    }

    private void setConfiguration(int index) {
        stopIO();
        setConfiguration(deviceID, index);
        String inputs[] = getInputs(deviceID);
        String outputs[] = getOutputs(deviceID);
        inputIndex = outputIndex = -1;

        adapter.selectItemInSection(CustomAdapter.CONFIGURATIONITEM, index);
        adapter.removeItemsBelow(CustomAdapter.OUTPUTITEM);
        int scrollto = adapter.addSectionHeader("Select Output");
        adapter.addItem("No Output", CustomAdapter.OUTPUTITEM, -1);
        adapter.addItems(outputs, CustomAdapter.OUTPUTITEM);
        adapter.addSectionHeader("Select Input");
        adapter.addItem("No Input", CustomAdapter.INPUTITEM, -1);
        adapter.addItems(inputs, CustomAdapter.INPUTITEM);

        boolean oneInput = inputs.length == 1, oneOutput = outputs.length == 1;
        if (oneInput) inputIndex = 0;
        if (oneOutput) outputIndex = 0;
        if (oneInput && oneOutput) updateIOOptions(true);
        else {
            updateIOOptions(false);
            adapter.notifyDataSetChanged();
            if (scrollto != -1) list.smoothScrollToPosition(scrollto);
        }
    }

    private void setOutput(int index) {
        stopIO();
        outputIndex = index;
        updateIOOptions(true);
    }

    private void setInput(int index) {
        stopIO();
        inputIndex = index;
        updateIOOptions(true);
    }

    private void updateIOOptions(boolean updateList) {
        getIOOptions(deviceID, inputIndex, outputIndex);
        int outputPathIndexes[] = getIOOptionsInt(0);
        int inputPathIndexes[] = getIOOptionsInt(1);
        int thruPathIndexes[] = getIOOptionsInt(2);
        String outputPathNames[] = getIOOptionsString(0);
        String inputPathNames[] = getIOOptionsString(1);
        String thruPathNames[] = getIOOptionsString(2);
        getIOOptionsEnd();

        adapter.selectItemInSection(CustomAdapter.INPUTITEM, inputIndex);
        adapter.selectItemInSection(CustomAdapter.OUTPUTITEM, outputIndex);
        adapter.removeItemsBelow(CustomAdapter.THRUPATHITEM);
        adapter.removeItemsBelow(CustomAdapter.INPUTPATHITEM);
        adapter.removeItemsBelow(CustomAdapter.OUTPUTPATHITEM);

        int scrollto = -1;
        if (outputPathIndexes.length > 0) {
            scrollto = adapter.addSectionHeader("Select Output Path");
            for (int n = 0; n < outputPathIndexes.length; n++) adapter.addItem(outputPathNames[n], CustomAdapter.OUTPUTPATHITEM, outputPathIndexes[n]);
        }
        if (inputPathIndexes.length > 0) {
            int pos = adapter.addSectionHeader("Select Input Path");
            if (scrollto == -1) scrollto = pos;
            for (int n = 0; n < inputPathIndexes.length; n++) adapter.addItem(inputPathNames[n], CustomAdapter.INPUTPATHITEM, inputPathIndexes[n]);
        }
        if (thruPathIndexes.length > 0) {
            int pos = adapter.addSectionHeader("Select Thru Path");
            if (scrollto == -1) scrollto = pos;
            for (int n = 0; n < thruPathIndexes.length; n++) adapter.addItem(thruPathNames[n], CustomAdapter.THRUPATHITEM, thruPathIndexes[n]);
        }

        inputFeatures = outputFeatures = thruFeatures = null;
        if (outputPathIndexes.length == 1) setPath(CustomAdapter.OUTPUTPATHITEM, outputPathIndexes[0], false);
        if (inputPathIndexes.length == 1) setPath(CustomAdapter.INPUTPATHITEM, inputPathIndexes[0], false);
        if (thruPathIndexes.length == 1) setPath(CustomAdapter.THRUPATHITEM, thruPathIndexes[0], false);

        if (updateList) {
            adapter.notifyDataSetChanged();
            if (scrollto != -1) list.smoothScrollToPosition(scrollto);
        }
    }

    public float onVolume(float volume, int index) {
        if (index >= 2000) return setVolume(deviceID, thruPath, index - 2000, volume);
        else if (index >= 1000) return setVolume(deviceID, inputPath, index - 1000, volume);
        else return setVolume(deviceID, outputPath, index, volume);
    }

    public boolean onMute(boolean mute, int index) {
        if (index >= 2000) return setMute(deviceID, thruPath, index - 2000, mute);
        else if (index >= 1000) return setMute(deviceID, inputPath, index - 1000, mute);
        else return setMute(deviceID, outputPath, index, mute);
    }

    private void setPath(int kind, int index, boolean updateList) {
        stopIO();
        adapter.removeItemsBelow(CustomAdapter.VOLUMEMUTEITEM);
        switch (kind) {
            case CustomAdapter.INPUTPATHITEM:
                inputPath = index;
                inputFeatures = getPathInfo(deviceID, index);
                adapter.selectItemInSection(kind, index);
                break;
            case CustomAdapter.OUTPUTPATHITEM:
                outputPath = index;
                outputFeatures = getPathInfo(deviceID, index);
                adapter.selectItemInSection(kind, index);
                break;
            case CustomAdapter.THRUPATHITEM:
                thruPath = index;
                thruFeatures = getPathInfo(deviceID, index);
                adapter.selectItemInSection(kind, index);
                break;
        }
        if ((outputFeatures != null) && (outputFeatures.length > 0)) {
            adapter.addSectionHeader("Output Controls");
            for (int n = 0, ch = 0; n < outputFeatures.length; n += 4, ch++) {
                if ((outputFeatures[n] < 200.0f) || (outputFeatures[n + 3] < 1.5f))
                    adapter.addItem(ch == 0 ? "Master" : "Channel " + ch + ", " + outputFeatures[n] + "db to " + outputFeatures[n + 1] + "db", CustomAdapter.VOLUMEMUTEITEM, ch,
                            outputFeatures[n], outputFeatures[n + 1], outputFeatures[n + 2], (int) outputFeatures[n + 3]);
            }
        }
        if ((inputFeatures != null) && (inputFeatures.length > 0)) {
            adapter.addSectionHeader("Input Controls");
            for (int n = 0, ch = 0; n < inputFeatures.length; n += 4, ch++) {
                if ((inputFeatures[n] < 200.0f) || (inputFeatures[n + 3] < 1.5f))
                    adapter.addItem(ch == 0 ? "Master" : "Channel " + ch + ", " + inputFeatures[n] + "db to " + inputFeatures[n + 1] + "db", CustomAdapter.VOLUMEMUTEITEM, 1000 + ch,
                        inputFeatures[n], inputFeatures[n + 1], inputFeatures[n + 2], (int)inputFeatures[n + 3]);
            }
        }
        if ((thruFeatures != null) && (thruFeatures.length > 0)) {
            adapter.addSectionHeader("Thru Controls");
            for (int n = 0, ch = 0; n < thruFeatures.length; n += 4, ch++) {
                if ((thruFeatures[n] < 200.0f) || (thruFeatures[n + 3] < 1.5f))
                    adapter.addItem(ch == 0 ? "Master" : "Channel " + ch + ", " + thruFeatures[n] + "db to " + thruFeatures[n + 1] + "db", CustomAdapter.VOLUMEMUTEITEM, 2000 + ch,
                        thruFeatures[n], thruFeatures[n + 1], thruFeatures[n + 2], (int)thruFeatures[n + 3]);
            }
        }

        if ((outputFeatures != null) || (inputFeatures != null)) {
            adapter.addSectionHeader("Buffer Size");
            adapter.addItem("128 samples", CustomAdapter.LATENCYITEM, 0);
            adapter.addItem("256 samples", CustomAdapter.LATENCYITEM, 1);
            adapter.addItem("512 samples", CustomAdapter.LATENCYITEM, 2);
            adapter.selectItemInSection(CustomAdapter.LATENCYITEM, latencyIndex);
            onAction(2, false);
        }
        if (updateList) adapter.notifyDataSetChanged();
    }

    private void onAction(int index, boolean updateList) {
        switch (index) {
            case 0:
            case 1:
                adapter.removeItemsBelow(CustomAdapter.ACTIONITEM);
                adapter.addSectionHeader("Actions");
                adapter.addItem("Stop", CustomAdapter.ACTIONITEM, 2);
                lastLatencyMs = -1000;
                startIO(deviceID, inputIndex, outputIndex, latencyIndex, index == 1);
                if (index == 1) {
                    Runnable runnable = new Runnable() {
                        @Override
                        public void run() {
                            int latencyMs = getLatencyMs();
                            if (latencyMs != lastLatencyMs) {
                                lastLatencyMs = latencyMs;
                                Toast.makeText(getApplicationContext(), "Round-trip audio latency: " + latencyMs + " ms.", Toast.LENGTH_LONG).show();
                            }
                            handler.postDelayed(this, 100);
                        }
                    };
                    handler = new Handler();
                    handler.postDelayed(runnable, 100);
                }
                break;
            case 2:
                stopIO();
                if (handler != null) {
                    handler.removeCallbacksAndMessages(null);
                    handler = null;
                }
                adapter.removeItemsBelow(CustomAdapter.ACTIONITEM);
                adapter.addSectionHeader("Actions");
                if ((inputIndex >= 0) && (outputIndex >= 0)) {
                    adapter.addItem("Start Loopback", CustomAdapter.ACTIONITEM, 0);
                    adapter.addItem("Start Latency Measurement", CustomAdapter.ACTIONITEM, 1);
                } else adapter.addItem("Start Sine Wave", CustomAdapter.ACTIONITEM, 0);
                break;
        }
        if (updateList) adapter.notifyDataSetChanged();
    }

    public native String[] getUSBAudioDeviceInfo(int deviceID);
    public native String[] getConfigurationInfo(int deviceID);
    public native void setConfiguration(int deviceID, int index);
    public native String[] getInputs(int deviceID);
    public native String[] getOutputs(int deviceID);
    public native void getIOOptions(int deviceID, int inputIOIndex, int outputIOIndex);
    public native int[] getIOOptionsInt(int outputInputThru);
    public native String[] getIOOptionsString(int outputInputThru);
    public native void getIOOptionsEnd();
    public native float[] getPathInfo(int deviceID, int index);
    public native float setVolume(int deviceID, int pathIndex, int channel, float db);
    public native boolean setMute(int deviceID, int pathIndex, int channel, boolean mute);
    public native void startIO(int deviceID, int inputIOIndex, int outputIOIndex, int latencyMs, boolean latencyMeasurement);
    public native void stopIO();
    public native int getLatencyMs();

    static {
        System.loadLibrary("complexusb");
    }
}
