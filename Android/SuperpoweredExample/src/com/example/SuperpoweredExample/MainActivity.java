package com.example.SuperpoweredExample;

import java.io.IOException;

import android.os.Bundle;
import android.app.Activity;
import android.view.View;
import android.content.res.AssetFileDescriptor;
import android.content.Context;
import android.media.AudioManager;
import android.widget.Button;
import android.widget.RadioButton;
import android.widget.RadioGroup;
import android.widget.SeekBar;
import android.widget.SeekBar.OnSeekBarChangeListener;

public class MainActivity extends Activity {
	boolean playing = false;
	    
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.main);
        
    	// Get the device's sample rate and buffer size to enable low-latency Android audio output, if available.
    	AudioManager audioManager = (AudioManager) this.getSystemService(Context.AUDIO_SERVICE);
    	String samplerateString = audioManager.getProperty(AudioManager.PROPERTY_OUTPUT_SAMPLE_RATE);
    	String buffersizeString = audioManager.getProperty(AudioManager.PROPERTY_OUTPUT_FRAMES_PER_BUFFER);
        if (samplerateString == null) samplerateString = "44100";
        if (buffersizeString == null) buffersizeString = "512";
        
    	// Files under res/raw are not compressed, just copied into the APK. Get the offset and length to know where our files are located.
    	AssetFileDescriptor fd0 = getResources().openRawResourceFd(R.raw.lycka), fd1 = getResources().openRawResourceFd(R.raw.nuyorica);
    	long[] params = { fd0.getStartOffset(), fd0.getLength(), fd1.getStartOffset(), fd1.getLength(), Integer.parseInt(samplerateString), Integer.parseInt(buffersizeString) };
    	try {
			fd0.getParcelFileDescriptor().close();
		} catch (IOException e) {}
    	try {
			fd1.getParcelFileDescriptor().close();
		} catch (IOException e) {}
    	
    	SuperpoweredExample(getPackageResourcePath(), params); // Arguments: path to the APK file, offset and length of the two resource files, sample rate, audio buffer size.
        
        // crossfader events
    	final SeekBar crossfader = (SeekBar)findViewById(R.id.crossFader);    
    	crossfader.setOnSeekBarChangeListener(new OnSeekBarChangeListener() {

            public void onProgressChanged(SeekBar seekBar, int progress, boolean fromUser) {
            	onCrossfader(progress);
            }

            public void onStartTrackingTouch(SeekBar seekBar) {}

            public void onStopTrackingTouch(SeekBar seekBar) {}
            
        });
    	
    	// fx fader events
    	final SeekBar fxfader = (SeekBar)findViewById(R.id.fxFader);    
    	fxfader.setOnSeekBarChangeListener(new OnSeekBarChangeListener() {

            public void onProgressChanged(SeekBar seekBar, int progress, boolean fromUser) {
            	onFxValue(progress);
            }

            public void onStartTrackingTouch(SeekBar seekBar) {
            	onFxValue(seekBar.getProgress());
            }

            public void onStopTrackingTouch(SeekBar seekBar) {
            	onFxOff();
            }
            
        });
    	
    	// fx select event
    	final RadioGroup group = (RadioGroup)findViewById(R.id.radioGroup1);
    	group.setOnCheckedChangeListener(new RadioGroup.OnCheckedChangeListener() {
    	    public void onCheckedChanged(RadioGroup radioGroup, int checkedId) {
    	    	RadioButton checkedRadioButton = (RadioButton)radioGroup.findViewById(checkedId);
    	        onFxSelect(radioGroup.indexOfChild(checkedRadioButton));
    	    }
    	});
    }
	
    public void SuperpoweredExample_PlayPause(View button) {  // Play/pause.
    	playing = !playing;
    	onPlayPause(playing);
    	Button b = (Button) findViewById(R.id.playPause);
    	b.setText(playing ? "Pause" : "Play");
    }
    
    private native void SuperpoweredExample(String apkPath, long[] offsetAndLength);
	private native void onPlayPause(boolean play);
	private native void onCrossfader(int value);
	private native void onFxSelect(int value);
	private native void onFxOff();
	private native void onFxValue(int value);
    
    static {
        System.loadLibrary("SuperpoweredExample");
    }
}
