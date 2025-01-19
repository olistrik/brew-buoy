#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEClient.h>
#include <LSM6DS3.h>
#include <Wire.h>

static BLEUUID

const int WINDOW_SIZE = 100;
//Create a instance of class LSM6DS3
LSM6DS3 imu(I2C_MODE, 0x6A);    //I2C device address 0x6A

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  while (!Serial);

  pinMode(PD5,OUTPUT);
  digitalWrite(PD5,HIGH);

	imu.settings.accelRange = 2;
	imu.settings.accelBandWidth = 50;

  //Call .begin() to configure the IMUs
  if (imu.begin() != 0) {
    Serial.println("Device error");
  }
}

void loop() {
	float aX = imu.readFloatAccelX();
	float aY = imu.readFloatAccelY();
	float aZ = imu.readFloatAccelZ();

	// Calculate the magnitude of the geration vector
	float magnitude = sqrt(aX * aX + aY * aY + aZ * aZ);

	// Calculate the angle to the z-axis

	float angle = 0;
	for (int i = WINDOW_SIZE; i; i--) {
		angle += acos(aX / magnitude) * 180 / PI;
		delay(10);
	}
	angle /= WINDOW_SIZE;

	// Output the angle to the z-axis
	Serial.println(angle);
}
