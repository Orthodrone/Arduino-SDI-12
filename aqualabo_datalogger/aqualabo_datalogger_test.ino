/*Data Logger for Aqualabo Ponsel Sensors
Created by Christine Avery
(c) 2023 Orthodrone GmbH
Based on the work by EnviroDIY

This program is a basic data logger for SDI-12-based environmental sensors. Although it can be used with any sensor using SDI-12,
it has been designed and tested for up to 3 Aqualabo Ponsel Sensors. More/Fewer sensors can be added by adjusting the numSensors 
function and sensor addresses. The program outputs the data to csv format. 
*/

#include <SD.h>             // Include the SD card library
#include <SDI12.h>          // Include the SDI-12 library
#include <Wire.h>           // Include the Wire library for I2C communication
#include <RTClib.h>         // Include the RTC library

const int chipSelect = 10;  // SD card chip select pin, set the pin for your SD card

File dataFile;              // File object to store data

const int numSensors = 3;   // Number of SDI-12 sensors connected
const char sensorAddresses[numSensors][3] = {
  "1!",   // Sensor 1 address (SDI-12 addressed usually start at 0)
  "2!",   // Sensor 2 address
  "3!"    // Sensor 3 address
};

RTC_DS1307 rtc;             // RTC object (the arduino realtime clock, used to timestamp the data log)

void setup() {
  // Initialize the SD card
  SD.begin(chipSelect);

  // Start serial communication for debugging
  Serial.begin(9600);

  // Initialize the SDI-12 communication
  SDI12.begin();

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
}

void loop() {
  // Get the current timestamp
  String timestamp = getDateTimeString();

  // Open the data file in append mode
  dataFile = SD.open(getDateTimeString() + ".csv", FILE_WRITE);

  // Write the timestamp to the CSV file
  dataFile.print(timestamp);

  // Read measurements from each sensor
  for (int i = 0; i < numSensors; i++) {
    // Select the sensor by sending its address
    SDI12.sendCommand(sensorAddresses[i]);

    // Wait for the response
    while (SDI12.isBusy())
      ;
      
    // Read the response from the sensor
    String response = SDI12.receive();

    // Extract the measurement value from the response
    // Replace this section with your own code to extract the measurement from the SDI-12 response
    float measurement = 0.0;  // Replace with the actual measurement value

    // Write the measurement to the CSV file
    dataFile.print(",");
    dataFile.print(measurement);

    // Print the measurement to the serial monitor for debugging
    Serial.print("Sensor ");
    Serial.print(i + 1);
    Serial.print(": ");
    Serial.println(measurement);
  }

  // End the line in the CSV file
  dataFile.println();

  // Close the file
  dataFile.close();

  // Delay between measurements
  delay(1000);
}

// Function to get the current date and time as a formatted string
String getDateTimeString() {
  DateTime now = rtc.now();
  String dateTimeString = "";
  dateTimeString += String(now.year(), DEC);
  dateTimeString += "-";
  dateTimeString += zeroPad(now.month(), 2);
  dateTimeString += "-";
  dateTimeString += zeroPad(now.day(), 2);
  dateTimeString += "-";
  dateTimeString += zeroPad(now.hour(), 2);
  dateTimeString += "-";
  dateTimeString += zeroPad(now.minute(), 2);
  dateTimeString += "-";
  dateTimeString += zeroPad(now.second(), 2);
  return dateTimeString;
}

// Function to pad a number with leading zeros
String zeroPad(int number, int width) {
  String paddedNumber = String(number);
  while (paddedNumber.length() < width) {
    paddedNumber = "0" + paddedNumber;
  }
  return paddedNumber;
}


