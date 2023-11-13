# Bitcoin cold wallet for ESP32 based USB modules (like WROVER or WROOM)

The "right" cold wallet should provide:
1. Safe keys generation. Achieved by robust random number generation.
2. Safe keys storage. Private key(s) and initial seed never exposed from internal storage.
3. User informed interaction requirement to sign outcoming transactions. User should make physical action to proceed with signature.
4. The device itself should not have any outward signs that it contains sensitive financial information.
That's all. No displays with 16 bit color with bitcoin price candles, and no other moronic stuff.

from above, ESP32-S3-WROOM is good option (You can use any other ESP32 based board i believe):
1. True random number generator collects entropy from RF noise
2. 70 years flash retention time. Keys export not supported in wallet firmware. Just keep it in safe place between use.
3. Has two hardware buttons RST and BOOT, one of them (BOOT) used for user interaction.
4. The device looks like electronic junk and has nothing to do with money for most people. Cheap and robust (~6USD at AliExpress).
5. Open source. Unlike Trezor, Ledger and other crap, you know what is running inside.
<img src="https://github.com/perdidor/bitdev/blob/main/esp32-s3-devkitc-1.jpg" width="640">

# How to use
To get your own BTC cold wallet working, you need to build firmware from the sources provided and burn it to ESP32 board.
1. Install Visual Studio Code
2. Install PlatformIO plugin for VS Code.
3. Download and extract this repository to any folder. Open folder in VS Code.
5. Select "Build" from project tasks. This will create firmware binary.
6. Plug ESP32 board to PC (choose "USART" marked USB connector). Select "Upload and Monitor". This will burn firmware to your ESP32 board and then open serial console. At first time you can see it creates new wallet (ONLY at first time):
```
Generating new wallet... ... done.
Ready.
```

and then wallet menu:
```
Press key to invoke action:
[1]>Show Root Public Key
[2] Sign PSBT
```
Now device is completely ready.

7. Next, let's create watch-only wallet on your PC or Android device. Download and install Electrum wallet software (https://electrum.org).
Press File-Create/Restore, then choose wallet name. Select "standard wallet", and "Use Master key".
Now, go to your  ESP32 device serial console and press [1]. It will print your Master public key(xpub):
```
Press key to invoke action:
[1]>Show Root Public Key
[2] Sign PSBT
Root Public Key:
xpub *******INFORMATION REMOVED******* YAG57
Key fingerprint:
c1 *******INFORMATION REMOVED******* 35
```
Copy and paste xpub string to Electrum import key dialog, and press "next". It's not neccessary to encrypt watch-only wallet so you can skip next dialog page and press "next" again. It will warn you that key is public and wallet will be watch-only. Press OK and close warning dialog.
Now you can monitor balance and generate new addresses in Electrum, but there is no way to spend your BTC until transaction signed with your ESP32 cold wallet.

8. Let's send some BTC to another address.
First, we creating PSBT (partially signed bitcoin transaction) at watch-only wallet:
Go to "Send" tab. Enter receiver's address and amount. Press "Pay" button. Adjust fee according to current network load. Press "Preview" button. Check details and press "Share" - "Copy to clipboard".

Go to cold wallet serial console. Press [2]. You'll see prompt to input transaction base64 text. Paste clipboard content to serial console. You will see transaction details and have about 15sec to confirm signature by pressing and holding BOOT button on cold wallet device:
```
[2] Sign PSBT
Input base64-encoded PSBT text, end with <LF>

================Transaction details================
output address                          amount(BTC)
1GXB5DKL4Kxnxaof6JCoV7RWaj3oTm1HgR      0.00006300
18WE6gLNvwJHdTdYhJkCcM5oUHWZWitBNP      0.00010000
---------------
Total transaction amount                0.00016300
Transaction fee(BTC)                    0.00006800
==============Transaction details END==============

==========TRANSACTION SIGNING IN PROGRESS==========
===================================================
!!!!!!!!!!!PRESS AND HOLD BUTTON TO SIGN!!!!!!!!!!!
!!!!!!!!!!!!!!OR JUST WAIT FOR CANCEL!!!!!!!!!!!!!!
===================================================
===================================================
Button NOT PRESSED Time left: 6.9
```

Once you've pressed and held button for enough time, transaction will be signed and printed out in both HEX and base64 formats:
```
User interaction stop. Button considered as PRESSED (2700 cycles, min 2700)
Signed transaction HEX:
020000000130d71f4971 *******INFORMATION REMOVED******* c00
Signed transaction Base64 string:
cHNidP8BAHcC *******INFORMATION REMOVED******* EN2vniQKyOAQAAAA==
```

If button is not pressed for enough time during this interaction period, signing will be cancelled:
```
User interaction stop. Button considered as NOT PRESSED (0 cycles, min 2700)
Signature cancelled.
```

Now you have two options to broadcast your signed transaction over Bitcoin network:
- HEX representation via blockchain exporer (https://www.blockchain.com/explorer/assets/btc/broadcast-transaction, https://blockchair.com/broadcast). Just paste hex string from serial console output and press "Broadcast".
- base64 representation via Electrum watch-only wallet. Tools - Load transaction - From text. Paste base64 signed transaction from serial console, check details and press "Broadcast" button.

# In case device or device's flash corrupted/damaged/flushed/modified, you will lost all your bitcoins stored on addresses derived from Master Key. Forever. If this risk is inacceptable for you, you have to implement your own Master Private key backup in code. Any external backup significantly reduces security level.

# Unplug your cold wallet from PC USB port after use. Plug it to PC ONLY when signing your transactions. DO NOT carry it with you. DO NOT show off, don’t tell or show it to anyone. DO NOT tell anyone you have crypto assets.