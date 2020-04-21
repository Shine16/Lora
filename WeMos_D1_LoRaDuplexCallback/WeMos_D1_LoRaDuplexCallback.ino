//https://github.com/sandeepmistry/arduino-LoRa
//https://www.instructables.com/id/Getting-LoRa-SX1278XL1278-SMT-Working-Via-SPI-on-t/

/* LoRa Duplex communication wth callback

  Sends a message every half second, and uses callback
  for new incoming messages. Implements a one-byte addressing scheme,
  with 0xFF as the broadcast address.

  Note: while sending, LoRa radio is not listening for incoming messages.
  Note2: when using the callback method, you can't use any of the Stream
  functions that rely on the timeout, such as readString, parseInt(), etc.

  created 28 April 2017 by Tom Igoe
  Modified 14 Nov 2018 by Chris Drake

======
WIRING
======
WeMos      <==> LoRa

GND        ---- GND
3V3        ---- VCC
D6* (io12) ---- MISO
D7* (io13) ---- MOSI
D5* (io14) ---- SLCK
D8  (io15) ---- NSS
D12*(io10) ---- DIO0
D4  (io2)  ---- REST (optional - NB: D4 is wired to the blue LED)

* Note that you need to solder D5, D6, D7 and D12 onto the ESP8266 chip on the motherboard, 
because they have not broken out those pins for you to use on the WeMos D1 header.
NB: There is no possible other pins to use!!  Most of the pins that are broken out for
you (A0, D3, D4, D8, D9, and D10) will (if used) prevent your board from booting 
up [D10+-, D8+, D4-, D3-], or will block you from programming it [D9], or will 
prevent your serial monitor from working [D9, D10]).
  
*/

#include <SPI.h>              // include libraries
#include <LoRa.h>

// WeMos D1 - use these pins for the WeMos D1 board . (note that you need to solder
const int csPin = 15;   // D8 = IO15;  LoRa radio chip select NSS
const int resetPin = 2; // D4 = IO2;   LoRa radio reset (optional) - NB - this is also the blue LED on the ESP8266 board
const int irqPin = 10;  // D12 = IO19; LoRa radio hardware interrupt pin DIO0
// MISO 12 (D6) These 3 pins are what the WeMos SPI library uses by default.
// MOSI 13 (D7)
// CLK 14  (D5)

/* Teensy 32 - If you're not using a WeMos - these are the pins you can use on a Teeny 3.2 board, which works as well:
const int csPin = 10;         // LoRa radio chip select
const int resetPin = 8;       // LoRa radio reset
const int irqPin = 9;         // change for your board; must be a hardware interrupt pin
 */
 
char outbuff[]="abcdefghijklmnopqrstuvwxyz0123456789\n\000";
int counter=0;

String outgoing;              // outgoing message
byte msgCount = 0;            // count of outgoing messages
byte localAddress = 0xFF;     // address of this device
byte destination = 0xBB;      // destination to send to
long lastSendTime = 0;        // last send time
long sinterval = 2000;          // interval between sends

void setup() {
  Serial.begin(115200);while((!Serial)&&(micros()<2000000));Serial.print("#\n# ");Serial.print(__FILE__);Serial.print(" ");Serial.print(__DATE__);Serial.print(" ");Serial.println(__TIME__);
  
//  Serial.begin(115200);                   // initialize serial
  //while (!Serial);

  Serial.println("LoRa Duplex with callback");

  // override the default CS, reset, and IRQ pins (optional)
  LoRa.setPins(csPin, resetPin, irqPin);// set CS, reset, IRQ pin


  if (!LoRa.begin(433E6)) {             // initialize ratio at 915 MHz
    Serial.println("LoRa init failed. Check your connections.");
    while (true);                       // if failed, do nothing (this causes the WeMos D1 to spew stack errors; probably because it's needing to reach the loop to stay alive)
  }

  LoRa.onReceive(onReceive);
  LoRa.receive();
  Serial.println("LoRa init succeeded.");
}

void loop() {
  if (millis() - lastSendTime > sinterval) {
    sprintf(outbuff,"LeLoRa World %s %d",__TIME__,counter++);
//    String message = "HeLoRa World!";   // send a message
//    sendMessage(message);
    sendMessage(outbuff);
//    Serial.print("Sending ");Serial.println(outbuff);
    Serial.print("s");
    lastSendTime = millis();            // timestamp the message
    sinterval = random(2000) + 1000;     // 2-3 seconds
    LoRa.receive();                     // go back into receive mode
  }
}

void sendMessage(String outgoing) {
  LoRa.beginPacket();                   // start packet
  LoRa.write(destination);              // add destination address
  LoRa.write(localAddress);             // add sender address
  LoRa.write(msgCount);                 // add message ID
  LoRa.write(outgoing.length());        // add payload length
  LoRa.print(outgoing);                 // add payload
  LoRa.endPacket();                     // finish packet and send it
  msgCount++;                           // increment message ID
}

void onReceive(int packetSize) {
  if (packetSize == 0) return;          // if there's no packet, return

  // read packet header bytes:
  int recipient = LoRa.read();          // recipient address
  byte sender = LoRa.read();            // sender address
  byte incomingMsgId = LoRa.read();     // incoming msg ID
  byte incomingLength = LoRa.read();    // incoming msg length

  String incoming = "";                 // payload of packet

  while (LoRa.available()) {            // can't use readString() in callback, so
    incoming += (char)LoRa.read();      // add bytes one by one
  }

  if (incomingLength != incoming.length()) {   // check length for error
    Serial.println("error: message length does not match length");
    return;                             // skip rest of function
  }

  // if the recipient isn't this device or broadcast,
  if (recipient != localAddress && recipient != 0xFF) {
    Serial.println("This message is not for me.");
    return;                             // skip rest of function
  }

  // if message is for this device, or broadcast, print details:
  Serial.print("\tRec from:0x" + String(sender, HEX));
  Serial.print(" to:0x" + String(recipient, HEX));
  Serial.print(" mID:" + String(incomingMsgId));
  Serial.print(" l:" + String(incomingLength));
  Serial.print(" Msg:" + incoming);
  Serial.print(" RSSI:" + String(LoRa.packetRssi()));
  Serial.print(" Snr:" + String(LoRa.packetSnr()));
  Serial.print(" freqErr:" + String(LoRa.packetFrequencyError()));
  Serial.print(" rnd:" + String(LoRa.random()));

  Serial.println();
}
