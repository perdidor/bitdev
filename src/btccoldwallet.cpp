#include <Arduino.h>
#include <ArduinoNvs.h>

#include <menu.h>
#include <menuIO/serialOut.h>
#include <menuIO/chainStream.h>
#include <menuIO/serialIn.h>

#include "Bitcoin.h"
#include "PSBT.h"
#include "Conversion.h"
#include "Hash.h"

#define INIT_SEED_SIZE 128
#define MAX_DEPTH 1

// This is pin for hardware user interaction button
#define BUTTON_PIN 0

HDPrivateKey masterkey;
uint8_t fingerprint[4];
uint8_t hdseed[INIT_SEED_SIZE];

result ShowMasterXPub(void);
result SignPSBT(void);
bool WaitForUserButtonPress(void);

MENU(mainMenu, "Press key to invoke action:", Menu::doNothing, Menu::noEvent, Menu::wrapStyle
  ,OP("Show Root Public Key",ShowMasterXPub,enterEvent)
  ,OP("Sign PSBT",SignPSBT,enterEvent)
);

serialIn serial(Serial);
MENU_INPUTS(in,&serial);

MENU_OUTPUTS(out,MAX_DEPTH
  ,SERIAL_OUT(Serial)
  ,NONE//must have 2 items at least
);

NAVROOT(nav,mainMenu,MAX_DEPTH,in,out);

/// @brief accepts partially signed bitcoin transaction in base64 format, signs it and prints result to serial output
/// in both hex and base64 formats
/// @return menu result value
result SignPSBT() {
  PSBT tx;
  Serial.println("");
  Serial.println("Input base64-encoded PSBT text, end with <LF>");
  bool cont = true;
  char txinput[2048];
  int charsRead;
  uint32_t cyclespassed = 0;
  while (cont) {
      if (Serial.available() > 0) {
        charsRead = Serial.readBytesUntil('\n', txinput, sizeof(txinput) - 1);
        txinput[charsRead] = '\0';
        tx.parseBase64(txinput);
        if (tx) {
          Serial.println("\n================Transaction details================");
          Serial.println("output address\t\t\t\tamount(BTC)");
          float totaloutput = 0.00000000;
          for (size_t i = 0; i < tx.tx.outputsNumber; i++) {
            char buff[32];
            Serial.print(tx.tx.txOuts[i].address());
            sprintf(buff, "\t%.8f", tx.tx.txOuts[i].btcAmount());
            totaloutput += tx.tx.txOuts[i].btcAmount();
            Serial.println(buff);
          }
          Serial.println("---------------");
          char buff[128];
          sprintf(buff, "Total transaction amount\t\t%.8f", totaloutput);
          Serial.println(buff);
          sprintf(buff, "Transaction fee(BTC)\t\t\t%.8f", tx.fee() * 0.00000001);
          Serial.println(buff);
          Serial.println("==============Transaction details END==============");
          bool userbutton = WaitForUserButtonPress();
          if (userbutton) {
            HDPrivateKey account = masterkey.derive("m/84'/0'/0'/");
            uint8_t signs = tx.sign(account);
            if (signs > 0) {
              Serial.println("Signed transaction HEX:");
              Serial.println(tx.tx);
              Serial.println("Signed transaction Base64 string:");
              Serial.println(tx.toBase64());
            } else {
              Serial.println("Signature failed.");
            }
          } else {
            Serial.println("Signature cancelled.");
          }
        } else {
          Serial.println("Transaction parse failed.");
        }
      cont = false;
    } else {
      delay(1);
    }
    cyclespassed++;
    if (cyclespassed >= 10000) {
      cont = false;
      Serial.println("Signature cancelled by timeout.");
    }
  }
  return proceed;
}

/// @brief prints root public key (xpub) and 4 bytes fingerprint to serial output
/// @return menu result value
result ShowMasterXPub() {
  Serial.println("");
  HDPrivateKey account = masterkey.derive("m/84'/0'/0'/");
  HDPublicKey xpub = account.xpub();
  xpub.type = P2WPKH;
  Serial.println("Root Public Key:");
  Serial.println(xpub);
  Serial.println("Key fingerprint:");
  char buff[16];
  for (size_t i = 0; i < 3; i++) {
    sprintf(buff, "%02x", fingerprint[i]);
    Serial.print(buff);
  }
  sprintf(buff, "%02x", fingerprint[3]);
  Serial.println(buff);
  return proceed;
}

/// @brief waits for user press and hold physical button on device to sign.
/// @return bool value indicating result, true if button was PRESSED
bool WaitForUserButtonPress() {
  uint32_t pressedcounter = 0;
  uint32_t cyclescounter = 0;
  bool pressed = false;
  Serial.println("\n==========TRANSACTION SIGNING IN PROGRESS==========");
  Serial.println("===================================================");
  Serial.println("!!!!!!!!!!!PRESS AND HOLD BUTTON TO SIGN!!!!!!!!!!!");
  Serial.println("!!!!!!!!!!!!!!OR JUST WAIT FOR CANCEL!!!!!!!!!!!!!!");
  Serial.println("===================================================");
  Serial.println("===================================================");
  while ((pressedcounter < 2700) && (cyclescounter < 10000)) {
    pressed = (digitalRead(BUTTON_PIN) == LOW);
    if (pressed) {
      pressedcounter++;
    } else {
      pressedcounter = 0;
    }
    cyclescounter++;
    char buff[64];
    sprintf(buff, "\rButton %s Time left: %.1f               ", pressed ? "PRESSED" : "NOT PRESSED", (10000 - cyclescounter) * 0.001);
    Serial.print(buff);
  }
  char buff[128];
  sprintf(buff, "\nUser interaction stop. Button considered as %s (%d cycles, min 2700)", (pressedcounter >= 2700) ? "PRESSED" : "NOT PRESSED", pressedcounter);
  Serial.println(buff);
  return (pressedcounter >= 2700);
}

/// @brief setup routine
void setup() {
  Serial.begin(115200);
  while(!Serial) {
    ;
  }
  NVS.begin();
  if (!NVS.getBlob("btcseed", hdseed, INIT_SEED_SIZE)) {
    Serial.print("Generating new wallet...");
    esp_fill_random(hdseed, INIT_SEED_SIZE);
    Serial.print(" ... ");
    NVS.setBlob("btcseed", hdseed, INIT_SEED_SIZE, true);
    Serial.println("done.");
  } else {
    Serial.println("Startup done.");
  }
  masterkey.fromSeed(hdseed, INIT_SEED_SIZE);
  esp_fill_random(hdseed, INIT_SEED_SIZE);
  Serial.println("Ready.");
  HDPrivateKey account = masterkey.derive("m/84'/0'/0'/");
  account.fingerprint(fingerprint);
  pinMode(BUTTON_PIN, INPUT_PULLUP);
}

/// @brief main loop
void loop() {
  nav.poll();
  delay(10);
}