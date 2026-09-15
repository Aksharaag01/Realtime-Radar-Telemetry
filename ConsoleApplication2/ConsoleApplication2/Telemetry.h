#pragma once
#include <stdint.h>

// Forces the compiler to pack data tightly so it transfers across the network flawlessly
#pragma pack(push, 1)
struct AircraftTelemetry {
	uint32_t aircraftId;
	double positionX;      // X Coordinate
	double positionY;      // Y Coordinate
	double altitude;       // Altitude
	double speed;          // Velocity
	double acceleration;   // Acceleration
};
#pragma pack(pop)