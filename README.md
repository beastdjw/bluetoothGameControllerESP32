# bluetoothGameControllerESP32
Make a bluetooth (ble  >bt4.2) connection with your own buttons, rotary encoders, and/or potentiometers. You can use this for you Simracing or Flight simulator rig.

## Features
 - [x] Button press (25 buttons)
 - [x] Button release (25 buttons)
 - [] Axes movement (0 axes (x, y, z, rZ, rX, rY) --> (Left Thumb X, Left Thumb Y, Right Thumb X, Right Thumb Y, Left Trigger, Right Trigger)))
 - [] Point of view hat (d-pad)
 - [x] Report optional battery level to host (basically works, but it doesn't show up in Android's status bar)
 - [x] Customize Bluetooth device name/manufacturer
 - [x] Compatible with Windows
 - [x] Compatible with Android
 - [x] Compatible with Linux
 - [x] Compatible with MacOS X
 - [ ] Compatible with iOS (No - not even for accessibility switch - This is not a “Made for iPhone” (MFI) compatible device)

## Credits
Credits to lemmingDev as this software is based on this repoistory. (https://github.com/lemmingDev/ESP32-BLE-Gamepad) 

Credits to T-vK as this library is based on his ESP32-BLE-Mouse library (https://github.com/T-vK/ESP32-BLE-Mouse) that he provided.

Credits to chegewara as the ESP32-BLE-Mouse library is based on this piece of code that he provided.

## Electronics
All GPIO's uses internal (in the code) and external (gpio 36, 39, 27, 35) pullup restistors(i.e. 10kOhm)
The external and internal Led is connected on GPIO2, this shows if Bluetooth is connected. This pin is always used as input for a button, this works and is as designed.
