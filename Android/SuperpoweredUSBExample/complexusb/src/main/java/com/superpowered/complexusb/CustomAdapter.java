package com.superpowered.complexusb;

import android.content.Context;
import android.support.v4.content.ContextCompat;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.BaseAdapter;
import android.widget.CheckBox;
import android.widget.CompoundButton;
import android.widget.RelativeLayout;
import android.widget.SeekBar;
import android.widget.TextView;

import java.util.ArrayList;
import java.util.Locale;

// This provides the data for the ListView.
public class CustomAdapter extends BaseAdapter {
    private class ItemData {
        String text;
        CustomAdapterHandler handler;
        int kind;
        int index;
        boolean selected;
        float curVolume;
        float minVolume;
        float maxVolume;
        int muted;
        public ItemData(CustomAdapterHandler h, String t, int k, int i, float minv, float maxv, float curv, int m) {
            handler = h;
            text = t;
            kind = k;
            index = i;
            minVolume = minv;
            maxVolume = maxv;
            curVolume = curv;
            muted = m;
            selected = false;
        }
    }

    private class RowViewData {
        SeekBar volumeControl;
        CheckBox muteControl;
        TextView textView;
        TextView volumeView;
    }

    public static final int SECTIONHEADERITEM = 0;
    public static final int CONFIGURATIONITEM = 1;
    public static final int INPUTITEM = 2;
    public static final int OUTPUTITEM = 3;
    public static final int OUTPUTPATHITEM = 4;
    public static final int INPUTPATHITEM = 5;
    public static final int THRUPATHITEM = 6;
    public static final int VOLUMEMUTEITEM = 7;
    public static final int LATENCYITEM = 8;
    public static final int ACTIONITEM = 9;
    private ArrayList<ItemData> data = new ArrayList<>();
    private LayoutInflater inflater;
    private int sectionHeaderBackgroundColor;
    private CustomAdapterHandler handler;

    public CustomAdapter(Context context) {
        handler = (CustomAdapterHandler)context;
        inflater = (LayoutInflater)context.getSystemService(Context.LAYOUT_INFLATER_SERVICE);
        sectionHeaderBackgroundColor = ContextCompat.getColor(context, android.R.color.darker_gray);
    }

    public void removeAllItems() {
        data.clear();
    }

    public void removeItemsBelow(int kind) {
        for (int n = 0; n < data.size(); n++) if (data.get(n).kind == kind) {
            data.subList(n - 1, data.size()).clear();
            break;
        }
    }

    public void addItem(final String text, int kind, int index, float minVolume, float maxVolume, float curVolume, int mute) {
        data.add(new ItemData(handler, text, kind, index, minVolume, maxVolume, curVolume, mute));
    }

    public void addItem(final String text, int kind, int index) {
        data.add(new ItemData(handler, text, kind, index, 0, 0, 0, 2));
    }

    public void addItems(final String[] items, int kind) {
        for (int n = 0; n < items.length; n++) data.add(new ItemData(handler, items[n], kind, n, 0, 0, 0, 2));
    }

    public int addSectionHeader(final String text) {
        data.add(new ItemData(handler, text, SECTIONHEADERITEM, 0, 0, 0, 0, 2));
        return data.size();
    }

    @Override
    public int getItemViewType(int position) {
        switch (data.get(position).kind) {
            case SECTIONHEADERITEM: return 0;
            case VOLUMEMUTEITEM: return 1;
            default: return 2;
        }
    }

    @Override
    public int getViewTypeCount() {
        return 3;
    }

    @Override
    public int getCount() {
        return data.size();
    }

    @Override
    public String getItem(int position) {
        return data.get(position).text;
    }

    @Override
    public long getItemId(int position) {
        return position;
    }

    public int[] getItemKindAndIndex(int position) {
        ItemData item = data.get(position);
        return new int[]{ item.kind, item.index };
    }

    public void selectItemInSection(int kind, int index) {
        int n = 0;
        for (; n < data.size(); n++) if (data.get(n).kind == kind) break;
        for (; n < data.size(); n++) {
            ItemData item = data.get(n);
            if (item.kind != kind) break;
            item.selected = (item.index == index);
        }
    }

    public View getView(int position, View rowView, ViewGroup parent) {
        ItemData item = data.get(position);
        RowViewData viewData;

        if (rowView == null) {
            viewData = new RowViewData();
            if (item.kind == VOLUMEMUTEITEM) {
                rowView = inflater.inflate(R.layout.volume_list_item, parent, false);
                viewData.textView = (TextView)rowView.findViewById(R.id.name);
                viewData.volumeControl = (SeekBar)rowView.findViewById(R.id.volume);
                viewData.muteControl = (CheckBox)rowView.findViewById(R.id.mute);
                viewData.volumeView = (TextView)rowView.findViewById(R.id.cur);
                viewData.volumeControl.setOnSeekBarChangeListener(new SeekBar.OnSeekBarChangeListener() {
                    @Override
                    public void onProgressChanged(SeekBar seekBar, int progress, boolean fromUser) {
                        if (fromUser) {
                            ItemData item = (ItemData)seekBar.getTag();
                            item.curVolume = item.handler.onVolume(progress + item.minVolume, item.index);
                            seekBar.setProgress((int)(item.curVolume - item.minVolume));
                            RowViewData viewData = (RowViewData)((RelativeLayout)seekBar.getParent().getParent()).getTag();
                            viewData.volumeView.setText(String.format(Locale.ENGLISH, "%.1f db", item.curVolume));
                        }
                    }

                    @Override
                    public void onStartTrackingTouch(SeekBar seekBar) {}

                    @Override
                    public void onStopTrackingTouch(SeekBar seekBar) {}
                });
                viewData.muteControl.setOnCheckedChangeListener(new CompoundButton.OnCheckedChangeListener() {
                    @Override
                    public void onCheckedChanged(CompoundButton buttonView, boolean isChecked) {
                        ItemData item = (ItemData)buttonView.getTag();
                        boolean muted = item.handler.onMute(isChecked, item.index);
                        buttonView.setChecked(muted);
                        item.muted = muted ? 1 : 0;
                    }
                });
            } else {
                rowView = inflater.inflate(android.R.layout.simple_list_item_1, null);
                viewData.textView = (TextView)rowView.findViewById(android.R.id.text1);
                if (item.kind == SECTIONHEADERITEM) viewData.textView.setBackgroundColor(sectionHeaderBackgroundColor);
                viewData.volumeControl = null;
                viewData.muteControl = null;
                viewData.volumeView = null;
            }
            rowView.setTag(viewData);
        } else {
            viewData = (RowViewData)rowView.getTag();
        }

        if (viewData != null) {
            if (viewData.textView != null) viewData.textView.setText(item.selected ? "\u2713 " + item.text : item.text);
            if (viewData.volumeControl != null) {
                viewData.volumeControl.setTag(item);
                viewData.volumeControl.setMax(0);
                if (item.maxVolume > item.minVolume) {
                    viewData.volumeControl.setMax((int)(item.maxVolume - item.minVolume));
                    viewData.volumeControl.setProgress((int)(item.curVolume - item.minVolume));
                    viewData.volumeControl.setVisibility(View.VISIBLE);
                } else viewData.volumeControl.setVisibility(View.INVISIBLE);
            }
            if (viewData.volumeView != null) viewData.volumeView.setText((item.maxVolume > item.minVolume) ? item.curVolume + " db" : "");
            if (viewData.muteControl != null) {
                viewData.muteControl.setTag(item);
                if (item.muted == 2) viewData.muteControl.setVisibility(View.INVISIBLE);
                else {
                    viewData.muteControl.setChecked(item.muted == 1);
                    viewData.muteControl.setVisibility(View.VISIBLE);
                }
            }
        }
        return rowView;
    }
}
