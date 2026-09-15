#define WIN32_LEAN_AND_MEAN  // Stops Windows.h from conflicting with network protocols
#include <stdio.h>
#include <windows.h> 

// Zenoh Network Includes
#include "zenoh.h"
#include "Telemetry.h"

int main() {
	printf("=== ZENOH TELEMETRY TRANSMITTER STARTING ===\n");

	// 1. Initialize Zenoh Config
	z_owned_config_t config;
	z_config_default(&config);

	// 2. Open Zenoh Session
	z_owned_session_t session;
	if (z_open(&session, z_move(config), NULL) < 0) {
		printf("Error: Could not open Zenoh session!\n");
		return -1;
	}

	// 3. Zenoh 1.0+: Setup Key Expression string view
	z_view_keyexpr_t keyexpr;
	z_view_keyexpr_from_str(&keyexpr, "simulation/aircraft/telemetry");

	// 4. Create mock flight parameters using your Telemetry.h struct
	AircraftTelemetry packet;
	packet.aircraftId = 777;
	packet.altitude = 1524.0;    // Start at 1524 meters (~5000 ft)
	packet.speed = 150.0;       // 150 m/s airspeed
	packet.acceleration = 1.5;   // m/s^2
	packet.positionX = 0.0;
	packet.positionY = 0.0;

	printf("Network pipeline armed. Broadcasting live virtual telemetry updates...\n");

	// 5. Active Simulation Loop running at 20Hz (Every 50ms)
	while (true) {
		// Pure C++ flight math calculation loop (Zero external library dependencies)
		packet.speed += (packet.acceleration * 0.05);
		packet.altitude += 0.2;
		packet.positionX += (packet.speed * 0.05);

		// 6. Zenoh 1.0+: Map raw memory directly into a managed byte view container
		z_owned_bytes_t payload_bytes;
		z_bytes_copy_from_buf(&payload_bytes, (const uint8_t*)&packet, sizeof(packet));

		// Push the data payload bundle out over the Zenoh mesh network
		z_put_options_t options;
		z_put_options_default(&options);
		z_put(z_loan(session), z_loan(keyexpr), z_move(payload_bytes), &options);

		printf("Transmitted Telemetry: Alt=%.1fm | Speed=%.1fm/s | X=%.1f\n", packet.altitude, packet.speed, packet.positionX);

		Sleep(50); // Frame limiter keeping iterations paced at 20Hz
	}

	// Cleanup
	z_drop(z_move(session));
	return 0;
}