
#include <EEPROM.h>

#define _BLE_TRACE_
#include <ArduinoBLE.h>
#include <LSM6DS3.h>
#include <Wire.h>

#include "common/ble.h"

#define LIGHTHOUSE_MAC	"7c:2c:67:64:ac:8a"

#define WINDOW_SIZE 100

#define CHECKSUM 0
#define CONFIG sizeof(unsigned long)

struct Config {
	bool bonded;
	uint8_t lighthouseMAC[6];
	uint8_t lighthouseIRK[16];
	uint8_t lighthouseLTK[16];
};

Config config;

BLEService buoyService(BU_SERV_UUID); 
BLEFloatCharacteristic buoyAngleDegrees(BU_ANGLE_DEG_UUID, BLERead | BLENotify | BLEEncryption);
uint8_t format[7] = {0x14, 0x00, 0x63, 0x27, 0x01, 0x00, 0x00};
BLEDescriptor unitDegrees("2904", format, sizeof(format));
BLEDescriptor angleDescription("2901", "Tilt Angle in Degrees");

//Create a instance of class LSM6DS3
LSM6DS3 imu(I2C_MODE, 0x6A);    //I2C device address 0x6A

bool acceptOrReject = true;

int logf(const char *format, ...) {
  char buffer[128]; // Adjust size as needed
  va_list args;
  va_start(args, format);
  int len = vsnprintf(buffer, sizeof(buffer), format, args);
  va_end(args);
  Serial.print(buffer);
  return len;
}

unsigned long eeprom_crc(size_t start, size_t length) {
	const unsigned long crc_table[16] = {
    0x00000000, 0x1db71064, 0x3b6e20c8, 0x26d930ac,
    0x76dc4190, 0x6b6b51f4, 0x4db26158, 0x5005713c,
    0xedb88320, 0xf00f9344, 0xd6d6a3e8, 0xcb61b38c,
    0x9b64c2b0, 0x86d3d2d4, 0xa00ae278, 0xbdbdf21c
	};

	unsigned long crc = ~0L;

	size_t end = start + length;

	for(int idx = start; idx < end; ++idx) {
		crc = crc_table[(crc ^ EEPROM[idx]) & 0x0f] ^ (crc >> 4);
    crc = crc_table[(crc ^ (EEPROM[idx] >> 4)) & 0x0f] ^ (crc >> 4);
    crc = ~crc;
	}

	return crc;
}

void saveConfig() {
		EEPROM.put(CONFIG, config);
		EEPROM.put(CHECKSUM, eeprom_crc(CONFIG, sizeof(config)));
}

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
    logf("starting IMU module failed!\n");
  }  

	// Callback function with confirmation code when new device is pairing.
  BLE.setDisplayCode([&config](uint32_t confirmCode){
    Serial.println("New device pairing request.");
    Serial.print("Confirm code matches pairing device: ");
    char code[6];
    sprintf(code, "%06d", confirmCode);
    Serial.println(code);
  });
  
  // Callback to allow accepting or rejecting pairing
  BLE.setBinaryConfirmPairing([](){
		return true;
  });

  // IRKs are keys that identify the true owner of a random mac address.
  // Add IRKs of devices you are bonded with.
  BLE.setGetIRKs([](uint8_t* nIRKs, uint8_t** BDaddrTypes, uint8_t*** BDAddrs, uint8_t*** IRKs){
    // Set to number of devices
    *nIRKs       = 2;

    *BDAddrs     = new uint8_t*[*nIRKs];
    *IRKs        = new uint8_t*[*nIRKs];
    *BDaddrTypes = new uint8_t[*nIRKs];

    // Set these to the mac and IRK for your bonded devices as printed in the serial console after bonding.
    uint8_t device1Mac[6]    = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    uint8_t device1IRK[16]   = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

    uint8_t device2Mac[6]    = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    uint8_t device2IRK[16]   = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};


    (*BDaddrTypes)[0] = 0; // Type 0 is for pubc address, type 1 is for static random
    (*BDAddrs)[0] = new uint8_t[6]; 
    (*IRKs)[0]    = new uint8_t[16];
    memcpy((*IRKs)[0]   , device1IRK,16);
    memcpy((*BDAddrs)[0], device1Mac, 6);


    (*BDaddrTypes)[1] = 0;
    (*BDAddrs)[1] = new uint8_t[6];
    (*IRKs)[1]    = new uint8_t[16];
    memcpy((*IRKs)[1]   , device2IRK,16);
    memcpy((*BDAddrs)[1], device2Mac, 6);


    return 1;
  });
  // The LTK is the secret key which is used to encrypt bluetooth traffic
  BLE.setGetLTK([](uint8_t* address, uint8_t* LTK){
    // address is input
    Serial.print("Received request for address: ");
    btct.printBytes(address,6);

    // Set these to the MAC and LTK of your devices after bonding.
    uint8_t device1Mac[6]  = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    uint8_t device1LTK[16] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    uint8_t device2Mac[6]  = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    uint8_t device2LTK[16] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    

    if(memcmp(device1Mac, address, 6) == 0) {
      memcpy(LTK, device1LTK, 16);
      return 1;
    }else if(memcmp(device2Mac, address, 6) == 0) {
      memcpy(LTK, device2LTK, 16);
      return 1;
    }
    return 0;
  });
  BLE.setStoreIRK([](uint8_t* address, uint8_t* IRK){
    Serial.print(F("New device with MAC : "));
    btct.printBytes(address,6);
    Serial.print(F("Need to store IRK   : "));
    btct.printBytes(IRK,16);
    return 1;
  });
  BLE.setStoreLTK([](uint8_t* address, uint8_t* LTK){
    Serial.print(F("New device with MAC : "));
    btct.printBytes(address,6);
    Serial.print(F("Need to store LTK   : "));
    btct.printBytes(LTK,16);
    return 1;
  });
  // IRK - The IRK to store with this mac
  BLE.setStoreIRK([&config](uint8_t* address, uint8_t* IRK) {
		Serial.println("=== New IRK ===");
		Serial.print("MAC: ");
		btct.printBytes(address, 6);
		Serial.print("IRK: ");
		btct.printBytes(IRK, 16);

		memcpy(config.lighthouseMAC, address, 6);
		memcpy(config.lighthouseIRK, IRK, 16);
		// saveConfig();

		return 1;
	});

	/*
  // address - the address to store [6 bytes]
  // LTK - the LTK to store with this mac [16 bytes]
  BLE.setStoreLTK([&config](uint8_t* address, uint8_t* LTK) {
		Serial.println("=== New LTK ===");
		Serial.print("MAC: ");
		btct.printBytes(address, 6);
		Serial.print("LTK: ");
		btct.printBytes(LTK, 16);

		memcpy(config.lighthouseMAC, address, 6);
		memcpy(config.lighthouseLTK, LTK, 16);
		// saveConfig();

		return 1;
	});

  // nIRKs      - the number of IRKs being provided.
  // BDAddrType - an array containing the type of each address (0 public, 1 static random)
  // BDAddrs    - an array containing the list of addresses
  BLE.setGetIRKs([&config](uint8_t* nIRKs, uint8_t** BDAddrType, uint8_t*** BDAddrs, uint8_t*** IRKs) {
		Serial.println("=== IRKs Requested ===");
		*nIRKs = 1;
		
    *BDAddrType = new uint8_t[*nIRKs];
		*BDAddrs    = new uint8_t*[*nIRKs];
    *IRKs       = new uint8_t*[*nIRKs];

		(*BDAddrType)[0] = 0;
		(*BDAddrs)[0] = new uint8_t[6];
		(*IRKs)[0] = new uint8_t[16];

		memcpy((*BDAddrs)[0], config.lighthouseMAC, 6);
		memcpy((*IRKs)[0], config.lighthouseIRK, 16);

		return 1;
	});
  // address - The mac address needing its LTK
  // LTK - 16 octet LTK for the mac address
	BLE.setGetLTK([&config](uint8_t* address, uint8_t* LTK) {
		Serial.println("=== LTK Requested ===");
		btct.printBytes(address, 6);

		if (memcmp(config.lighthouseMAC, address, 6) == 0) {
			memcpy(LTK, config.lighthouseLTK, 16);
			return 1;
		}
		return 0;
	});
	*/


	// begin initialization
  if (!BLE.begin()) {
    logf("starting Bluetooth® Low Energy module failed!\n");
    while (1);
  }

	unsigned long checksum;
	EEPROM.get(CHECKSUM, checksum);
	EEPROM.get(CONFIG, config);

	unsigned long crc = eeprom_crc(CONFIG, sizeof(config));
	logf("checksum: %.8X :: %.8X\n", checksum, crc);

	if (checksum != crc) {
		logf("checksum missmatch!\n");
		// set default config;
		config.bonded = false;

		memset(config.lighthouseMAC, 0x00, 6);
		memset(config.lighthouseIRK, 0x00, 16);
		memset(config.lighthouseLTK, 0x00, 16);

		saveConfig();
	} 

	logf("=== config ===\n");
	logf("bonded: '%d'\n", config.bonded);
	logf("lighthouseMAC: '%s'\n", config.lighthouseMAC);
	logf("lighthouseIRK: '%s'\n", config.lighthouseIRK);
	logf("lighthouseLTK: '%s'\n", config.lighthouseLTK);

	BLE.setLocalName("Brew Buoy");
	BLE.setAdvertisedService(buoyService);

	buoyService.addCharacteristic(buoyAngleDegrees);
	buoyAngleDegrees.addDescriptor(unitDegrees);
	buoyAngleDegrees.addDescriptor(angleDescription);
	BLE.addService(buoyService);


	buoyAngleDegrees.writeValue(0);

	BLE.advertise();
	BLE.setPairable(true);

  Serial.println("Bluetooth® device active, waiting for connections...");
}

float measureAngle() {
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

	return angle / WINDOW_SIZE;
}


bool wasConnected = false;

void loop() {
	BLEDevice central = BLE.central();

	float angle = measureAngle();

	if(buoyAngleDegrees.value() != angle) {
		buoyAngleDegrees.writeValue(angle);
	}

	if (central && central.connected()) {
		if (!wasConnected) {
			wasConnected = true;
			Serial.print("Connected to central: ");
      // print the central's BT address:
      Serial.println(central.address());
		}
  } else if (wasConnected){
    wasConnected = false;
    Serial.print("Disconnected from central: ");
    Serial.println(central.address());
	}

	//  Serial.println("Bluetooth® Low Energy Central scan");
	//
	// BLE.scan();
// // check if a peripheral has been discovered
//   BLEDevice peripheral = BLE.available();
//
//   if (peripheral) {
//     // discovered a peripheral
//     Serial.println("Discovered a peripheral");
//     Serial.println("-----------------------");
//
//     // print address
//     Serial.print("Address: ");
//     Serial.println(peripheral.address());
//
//     // print the local name, if present
//     if (peripheral.hasLocalName()) {
//       Serial.print("Local Name: ");
//       Serial.println(peripheral.localName());
//     }
//
//     // print the advertised service UUIDs, if present
//     if (peripheral.hasAdvertisedServiceUuid()) {
//       Serial.print("Service UUIDs: ");
//       for (int i = 0; i < peripheral.advertisedServiceUuidCount(); i++) {
//         Serial.print(peripheral.advertisedServiceUuid(i));
//         Serial.print(" ");
//       }
//       Serial.println();
//     }
//
//     // print the RSSI
//     Serial.print("RSSI: ");
//     Serial.println(peripheral.rssi());
//
//     Serial.println();
//   }
}
