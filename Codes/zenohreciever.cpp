#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <cmath>
#include <iostream>
#include <string>
#include <sstream>
#include <iomanip>
#include "zenoh.h"
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
using namespace std;

double prevX = 0.0;
double prevY = 0.0;
double prevspeed = 0.0;
double calculated_altitude = 5000.0; 
bool firstFrame = true;

void telemetry_callback(z_loaned_sample_t* sample, void* arg) {
	z_owned_string_t payload_str;
	z_bytes_to_string(z_sample_payload(sample), &payload_str);

	string dataStr = z_string_data(z_loan(payload_str));
	double currX = 0.0, currY = 0.0, dt = 0.1;
	int frame = 0;

	sscanf_s(dataStr.c_str(), "{\"frame\":%d,\"dt\":%lf,\"x_nm\":%lf,\"y_nm\":%lf}", &frame, &dt, &currX, &currY);

	if (firstFrame) {
		prevX = currX;
		prevY = currY;
		firstFrame = false;
		z_drop(z_move(payload_str));
		return;
	}

	double deltaX = currX - prevX;
	double deltaY = currY - prevY;
	double distNM = sqrt(deltaX * deltaX + deltaY * deltaY);

	double calculated_speed = (distNM / dt) * 3600.0;

	double calculated_acc = (calculated_speed - prevspeed) / dt;

	calculated_altitude += (distNM * 1000.0);

	// Print calculated kinematics
	cout << fixed << setprecision(2);
	cout << " [RECEIVER DERIVED DATA] Frame #" << frame << endl;
	cout << "    Received Position : (" << currX << " NM, " << currY << " NM)" << endl;
	cout << "    CALCULATED Speed  : " << calculated_speed << " kts" << endl;
	cout << "    CALCULATED Accel  : " << calculated_acc << " kts/s²" << endl;
	cout << "    CALCULATED Alt    : " << calculated_altitude << " ft\n" << endl;

	prevX = currX;
	prevY = currY;
	prevspeed = calculated_speed;

	z_drop(z_move(payload_str));
}

int main(int argc, char* argv[]) {
	cout << "======================================================" << endl;
	cout << "   RECEIVER: DERIVING SPEED, ACCEL & ALT FROM (X,Y)   " << endl;
	cout << "======================================================" << endl;

	z_owned_config_t config;
	z_config_default(&config);
	z_owned_session_t session;

	if (z_open(&session, z_move(config), NULL) < 0) return -1;

	z_view_keyexpr_t keyExpression;
	z_view_keyexpr_from_str(&keyExpression, "simulation/aircraft/position");

	z_owned_closure_sample_t callback;
	z_closure_sample(&callback, telemetry_callback, NULL, NULL);

	z_owned_subscriber_t subscriber;
	z_declare_subscriber(z_loan(session), &subscriber, z_loan(keyExpression), z_move(callback), NULL);

	while (true) { Sleep(1000); }

	z_drop(z_move(subscriber));
	z_drop(z_move(session));
	return 0;
}