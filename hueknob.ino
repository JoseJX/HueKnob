#include <SparkFun_Qwiic_Twist_Arduino_Library.h>
#include <WiFi.h>

// Wifi network name and password
const char *ssid = "";
const char *pwd = "";

// Hue hub IP address, must be on the same subnet
const char *hub = "";
// Hue login information
// Perform the API auth sequence to get the string for this variable
const char *hueUser = "";
// Wifi connection to the hub
WiFiClient client;
// Which group of lights are we using
// Get this information from /api/<hueUser>/groups
const uint8_t lightGroup = 2;

// Hue colors to cycle through, HSV, 4 bytes per color H/L, S, V
#define COLOR_LIST_LENGTH 8
uint8_t hue_colors[COLOR_LIST_LENGTH][4] = {
  { 0x00, 0x00, 0xFE, 0xFE },  // Red
  { 0x55, 0x55, 0xFE, 0xFE },  // Green
  { 0xAA, 0xAA, 0xFE, 0xFE },  // Blue
  { 0xD5, 0x55, 0xFE, 0xFE },  // Purple
  { 0x80, 0x00, 0xFE, 0xFE },  // Teal
  { 0x2A, 0xAB, 0x00, 0xFE },  // White
  { 0x1B, 0x9B, 0xFE, 0xFE },  // Orange
  { 0xD8, 0xE4, 0x80, 0xFE },  // Pink
};

// Knob colors in RGB
uint8_t knob_colors[COLOR_LIST_LENGTH][3] = {
  { 0xFF, 0x00, 0x00 },  // Red
  { 0x00, 0xFF, 0x00 },  // Green
  { 0x00, 0x00, 0xFF },  // Blue
  { 0xFF, 0x00, 0xFF },  // Purple
  { 0x00, 0xFF, 0xFF },  // Teal
  { 0xFF, 0xFF, 0xFF },  // White
  { 0xFF, 0xA5, 0x00 },  // Orange
  { 0xF9, 0x84, 0xEF },  // Pink
};

// Current color
uint8_t color;

// Time to wait between presses, in ms
#define MS_TO_WAIT  500

// Last press time
unsigned long lastPressTime;

// Instance for the library
TWIST twist;

// Set a color from the table on the knob
void setKnob(void) {
  twist.setColor(knob_colors[color][0], knob_colors[color][1], knob_colors[color][2]);
}

// Set the color on the hue light
void setHue(int group) {
  uint16_t hue = (((uint16_t)hue_colors[color][0]) << 8) | hue_colors[color][1];
  uint8_t  sat = hue_colors[color][2];
  uint8_t  bri = hue_colors[color][3];

  Serial.print("Connecting to the Hue Hub ... ");
  if (!client.connect(hub, 80)) {
    // We couldn't find the hub, give up and start over
    Serial.println("connection failed");
    return;
  }
  Serial.println(" connected!");
  
  String cmd = (String)"{\"on\":true, \"sat\":" + String(sat, DEC) + (String)", \"bri\":" + String(bri, DEC) + (String)", \"hue\":" + String(hue, DEC) + (String)"}\n";
  String putCmd = 
    (String)"PUT /api/" + String(hueUser) + (String)"/groups/" + String(lightGroup) + (String)"/action HTTP/1.1\r\n" +
    (String)"Host: " + String(hub) + (String)"\r\n" +
    (String)"Content-Type: text/plain\r\n" +
    (String)"Content-length: " + String(cmd.length()) + (String)"\r\n" +
    (String)"\r\n";
  client.print(putCmd + cmd);
  client.flush();
  client.stop();
}

// Set up the system
void setup() {
  Serial.begin(115200);

  // Set up the twist
  if (twist.begin() == false) {
     Serial.println("Unable to connect to the twist module, it is plugged in yeah?");
  }
  // Set the I2C speed to max
  Wire.setClock(400000);

  // Turn off the light on the knob
  twist.setColor(0x00, 0x00, 0x00);

  // Set up the wifi
  WiFi.begin(ssid, pwd);
  Serial.print("Connecting to the AP ");
  uint8_t lastKnob = 0;
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    // Cycle the light on the knob while we are connecting
    lastKnob = ~lastKnob;
    twist.setColor(lastKnob,lastKnob, lastKnob);
    delay(500);
  }
  Serial.println(" connected!");
  
  // Reset the millis last press time
  lastPressTime = 0;

  // Initialize our current color setting
  int count = twist.getCount() % 24;
  color = abs(count / (24 / COLOR_LIST_LENGTH));
  setKnob();
}

// Set up the loop
void loop() {
  uint8_t i;
  
  // Update the color on the knob, if it changed
  int diff = twist.getDiff();
  if (diff != 0) {
    // Get the twist count, it's 24 clicks in each direction,
    // but we really only care about which 1/8th it's in
    int count = twist.getCount() % 24;
    // Pick the color index
    color = abs(count / (24 / COLOR_LIST_LENGTH));
    setKnob();
  }

  // If we had a press, update the hue lights
  unsigned long pressTime = millis();
  if (twist.isPressed() && ((pressTime - lastPressTime) > MS_TO_WAIT)) {
    setHue(lightGroup);
    lastPressTime = millis();
  }

  // Wait some time
  delay(10);
}
