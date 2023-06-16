#!/bin/bash

cd ../example

# Get the first Android device 
device_id=$(flutter devices | awk -F'•' '/android-arm64/ {print $2}' | tr -d '[:space:]')

# If an Android device is found
if [ -n "$device_id" ]
then
  echo "Running on $device_id"
  flutter run -d $device_id
else
  echo "No Android devices found."
fi