/*Data Logger for Aqualabo Ponsel Sensors
Created by Christine Avery for
(c) 2023 Orthodrone GmbH
Based on the work by EnviroDIY

This program is a basic data logger for SDI-12-based environmental sensors. Although it can be used with any sensor using SDI-12,
it has been designed and tested for up to 3 Aqualabo Ponsel Sensors. More/Fewer sensors can be added by adjusting the numSensors 
function and sensor addresses. The program outputs the data to csv format. 
*/

#include <SDI12.h>
#include <SD.h>
#include <RTClib.h>
#include <Wire.h>

#define SERIAL_BAUD 115200 //Set baud rate, 115200 for SDI-12
#define DATA_PIN 7 //Set SDI-12 bus data pin
#define POWER_PIN -1 //Set power pin, we are using the 5V pin for power so we set the power pin to -1
#define PWM_PIN 3 //Sets PWM in pin

file dataFile; //define the file object for storing data

//Define SDI-12 bus using data pin above
SDI12 mySDI12(DATA_PIN)

// keeps track of active addresses
bool isActive[64] = {
  0,
};

// keeps track of the wait time for each active addresses
uint8_t meas_time_ms[64] = {
  0,
};

// keeps track of the time each sensor was started
uint32_t millisStarted[64] = {
  0,
};

// keeps track of the time each sensor will be ready
uint32_t millisReady[64] = {
  0,
};

// keeps track of the number of results expected
uint8_t returnedResults[64] = {
  0,
};

uint8_t numSensors = 0;

/**
 * @brief converts allowable address characters ('0'-'9', 'a'-'z', 'A'-'Z') to a
 * decimal number between 0 and 61 (inclusive) to cover the 62 possible
 * addresses.
 */
byte charToDec(char i) {
  if ((i >= '0') && (i <= '9')) return i - '0';
  if ((i >= 'a') && (i <= 'z')) return i - 'a' + 10;
  if ((i >= 'A') && (i <= 'Z'))
    return i - 'A' + 36;
  else
    return i;
}

/**
 * @brief maps a decimal number between 0 and 61 (inclusive) to allowable
 * address characters '0'-'9', 'a'-'z', 'A'-'Z',
 *
 * THIS METHOD IS UNUSED IN THIS EXAMPLE, BUT IT MAY BE HELPFUL.
 */
char decToChar(byte i) {
  if (i < 10) return i + '0';
  if ((i >= 10) && (i < 36)) return i + 'a' - 10;
  if ((i >= 36) && (i <= 62))
    return i + 'A' - 36;
  else
    return i;
}

RTC_DS1307 rtc; /*defines RTC object, the arduino RT clock, used for timestamping the data log*/

/**
 * @brief gets identification information from a sensor, and prints it to the serial
 * port
 *
 * @param i a character between '0'-'9', 'a'-'z', or 'A'-'Z'.
 */
void printInfo(char i) {
  String command = "";
  command += (char)i;
  command += "I!";
  mySDI12.sendCommand(command);
  delay(100);

  String sdiResponse = mySDI12.readStringUntil('\n');
  sdiResponse.trim();
  // allccccccccmmmmmmvvvxxx...xx<CR><LF>
  Serial.print(sdiResponse.substring(0, 1));  // address
  Serial.print(", ");
  Serial.print(sdiResponse.substring(1, 3).toFloat() / 10);  // SDI-12 version number
  Serial.print(", ");
  Serial.print(sdiResponse.substring(3, 11));  // vendor id
  Serial.print(", ");
  Serial.print(sdiResponse.substring(11, 17));  // sensor model
  Serial.print(", ");
  Serial.print(sdiResponse.substring(17, 20));  // sensor version
  Serial.print(", ");
  Serial.print(sdiResponse.substring(20));  // sensor id
  Serial.print(", ");
}

bool getResults(char i, int resultsExpected) {
  uint8_t resultsReceived = 0;
  uint8_t cmd_number      = 0;
  while (resultsReceived < resultsExpected && cmd_number <= 9) {
    String command = "";
    // in this example we will only take the 'DO' measurement
    command = "";
    command += i;
    command += "D";
    command += cmd_number;
    command += "!";  // SDI-12 command to get data [address][D][dataOption][!]
    mySDI12.sendCommand(command);

    uint32_t start = millis();
    while (mySDI12.available() < 3 && (millis() - start) < 1500) {}
    mySDI12.read();           // ignore the repeated SDI12 address
    char c = mySDI12.peek();  // check if there's a '+' and toss if so
    if (c == '+') { mySDI12.read(); }

    while (mySDI12.available()) {
      char c = mySDI12.peek();
      if (c == '-' || (c >= '0' && c <= '9') || c == '.') {
        float result = mySDI12.parseFloat(SKIP_NONE);
        Serial.print(String(result, 10));
        if (result != -9999) { resultsReceived++; }
      } else if (c == '+') {
        mySDI12.read();
        Serial.print(", ");
      } else {
        mySDI12.read();
      }
      delay(10);  // 1 character ~ 7.5ms
    }
    if (resultsReceived < resultsExpected) { Serial.print(", "); }
    cmd_number++;
  }
  mySDI12.clearBuffer();

  return resultsReceived == resultsExpected;
}

int startConcurrentMeasurement(char i, String meas_type = "") {
  mySDI12.clearBuffer();
  String command = "";
  command += i;
  command += "C";
  command += meas_type;
  command += "!";  // SDI-12 concurrent measurement command format  [address]['C'][!]
  mySDI12.sendCommand(command);
  delay(30);

  // wait for acknowlegement with format [address][ttt (3 char, seconds)][number of
  // measurments available, 0-9]
  String sdiResponse = mySDI12.readStringUntil('\n');
  sdiResponse.trim();

  // find out how long we have to wait (in seconds).
  uint8_t wait = sdiResponse.substring(1, 4).toInt();

  // Set up the number of results to expect
  int numResults = sdiResponse.substring(4).toInt();

  uint8_t sensorNum = charToDec(i);  // e.g. convert '0' to 0, 'a' to 10, 'Z' to 61.
  meas_time_ms[sensorNum]  = wait;
  millisStarted[sensorNum] = millis();
  if (wait == 0) {
    millisReady[sensorNum] = millis();
  } else {
    millisReady[sensorNum] = millis() + wait * 1000;
  }
  returnedResults[sensorNum] = numResults;

  return numResults;
}

// this checks for activity at a particular address
// expects a char, '0'-'9', 'a'-'z', or 'A'-'Z'
boolean checkActive(char i) {
  String myCommand = "";
  myCommand        = "";
  myCommand += (char)i;  // sends basic 'acknowledge' command [address][!]
  myCommand += "!";

  for (int j = 0; j < 3; j++) {  // goes through three rapid contact attempts
    mySDI12.sendCommand(myCommand);
    delay(100);
    if (mySDI12.available()) {  // If we here anything, assume we have an active sensor
      mySDI12.clearBuffer();
      return true;
    }
  }
  mySDI12.clearBuffer();
  return false;
}

void setup() {
    Serial.begin(SERIAL_BAUD);
    while (!Serial);
    Serial.println("Opening SDI-12 bus...");
    mySDI12.begin();
    delay(500); // allow things to settle

    Serial.println("Timeout Value: ");
    Serial.println(mySDI12.TIMEOUT);

    // Initialize the RTC
    Wire.begin();
    rtc.begin();

    // Check if the RTC is running
    if (!rtc.isrunning()) {
    Serial.println("RTC is NOT running!");
    // Set the following line to the date and time the program is compiled to set the RTC initially
    rtc.adjust(DateTime(__DATE__, __TIME__));
    }
  
    // Create a filename based on the date and time
    String fileName = getDateTimeString() + ".csv";

    // Open a file for writing
    dataFile = SD.open(fileName, FILE_WRITE);

    // Write headers to the CSV file, 
    dataFile.println("Timestamp, Sensor 1, Sensor 2, Sensor 3");

    // Close the file
    dataFile.close();

  // Power the sensors;
    if (POWER_PIN > 0) {
    Serial.println("Powering up sensors...");
    pinMode(POWER_PIN, OUTPUT);
    digitalWrite(POWER_PIN, HIGH);
    delay(200);
    }

  // Quickly Scan the Address Space
  Serial.println("Scanning all addresses, please wait...");
  Serial.println("Protocol Version, Sensor Address, Sensor Vendor, Sensor Model, "
                 "Sensor Version, Sensor ID");

  for (byte i = 0; i < 62; i++) {
    char addr = decToChar(i);
    if (checkActive(addr)) {
      numSensors++;
      isActive[i] = 1;
      printInfo(addr);
      Serial.println();
    }
  }
  Serial.print("Total number of sensors found:  ");
  Serial.println(numSensors);

  if (numSensors == 0) {
    Serial.println(
      "No sensors found, please check connections and restart the Arduino.");
    while (true) { delay(10); }  // do nothing forever
  }

  Serial.println();
  Serial.println("Time Elapsed (s), Measurement 1, Measurement 2, ... etc.");
  Serial.println(
    "-------------------------------------------------------------------------------");
}

void loop() {
     // Get the current timestamp
  String timestamp = getDateTimeString();

  // Open the data file in append mode
  dataFile = SD.open(getDateTimeString() + ".csv", FILE_WRITE);

  // Write the timestamp to the CSV file
  dataFile.print(timestamp);

   // start all sensors measuring concurrently
  for (byte i = 0; i < 62; i++) {
    char addr = decToChar(i);
    if (isActive[i]) { startConcurrentMeasurement(addr); }
  }

  // get all readings
  uint8_t numReadingsRecorded = 0;
  while (numReadingsRecorded < numSensors) {
    for (byte i = 0; i < 62; i++) {
      char addr = decToChar(i);
      if (isActive[i]) {
        if (millis() > millisReady[i]) {
          if (returnedResults[i] > 0) {
            Serial.print(millis() / 1000);
            Serial.print(", ");
            Serial.print(addr);
            Serial.print(", ");
            getResults(addr, returnedResults[i]);
            Serial.println();
          }
          numReadingsRecorded++;
        }
      }
    }
  }
    // Write the measurement to the CSV file
    dataFile.print("Sensor ");
    dataFile.print(",");
    dataFile.print(measurement);

    // Print the measurement to the serial monitor for debugging
    Serial.print("Sensor ");
    Serial.print(i + 1);
    Serial.print(": ");
    Serial.println(measurement);

  // End the line in the CSV file
  dataFile.println();

  // Close the file
  dataFile.close();

  // Delay between measurements
  delay(10000);  
}
