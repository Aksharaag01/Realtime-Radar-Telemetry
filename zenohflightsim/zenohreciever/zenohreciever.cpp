#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <iostream>
#include <string>

#pragma comment(lib, "Ws2_32.lib")
#pragma comment(lib, "Iphlpapi.lib")
#pragma comment(lib, "Userenv.lib")
#pragma comment(lib, "Bcrypt.lib")

#include "zenoh.h"
using namespace std;
void data_handler(z_loaned_sample_t* sample, void* arg) {
	z_view_string_t key_str;
	z_keyexpr_as_view_string(z_sample_keyexpr(sample), &key_str);
	z_owned_string_t payload_str;
	z_bytes_to_string(z_sample_payload(sample), &payload_str);

	cout << "[STATE @ 40ms] -> "
		<< string(z_string_data(z_loan(payload_str)), z_string_len(z_loan(payload_str)))
		<< endl;

	z_drop(z_move(payload_str));
}

int main(int argc, char* argv[]) {
	cout << "======================================================" << endl;
	cout << "    RECEIVER: Aircraft + Radar + SAM Telemetry        " << endl;
	cout << "======================================================" << endl;

	z_owned_session_t session;
	z_owned_config_t config;
	z_config_default(&config);

	if (z_open(&session, z_move(config), NULL) < 0) {
		cerr << "Failed to open Zenoh session!" << endl;
		return -1;
	}
	cout << "Connected. Listening on 'simulation/state/all'...\n" << endl;

	z_owned_closure_sample_t callback;
	z_closure_sample(&callback, data_handler, NULL, NULL);

	z_view_keyexpr_t keyexpr;
	z_view_keyexpr_from_str(&keyexpr, "simulation/state/all");

	z_owned_subscriber_t sub;
	if (z_declare_subscriber(z_loan(session), &sub, z_loan(keyexpr), z_move(callback), NULL) < 0) {
		cerr << "Failed to declare subscriber!" << endl;
		z_drop(z_move(session));
		return -1;
	}

	cout << "Press ENTER to stop receiver..." << endl;
	cin.get();

	z_drop(z_move(sub));
	z_drop(z_move(session));

	return 0;
}