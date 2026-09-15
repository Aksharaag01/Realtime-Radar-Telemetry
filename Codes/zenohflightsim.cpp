#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <iostream>
#include <cmath>
#include <sstream>
#include <iomanip>
#include <string>

#include "openeaagles/base/Object.hpp"
#include "zenoh.h"

using namespace std;

#ifndef pi_value
#define pi_value 3.14159265358979323846
#endif

class AircraftEntity : public oe::base::Object
{
private:
	double x_pos;     
	double y_pos;     
	double speed;  
	double acceleration;  
	double heading;     

public:

	AircraftEntity()
	{
		x_pos = 0.0;
		y_pos = 0.0;

		speed = 250.0;
		acceleration = 2.5;
		heading = 45.0;
	}

	void updatePhysics(double deltaTime)
	{
		speed = speed + acceleration * deltaTime;
		double distanceTravelled = (speed * deltaTime) / 3600.0;

		double headrad = heading * pi_value / 180.0;
		double x_move = distanceTravelled * sin(headrad);

		double y_move = distanceTravelled * cos(headrad);
		x_pos = x_pos + x_move;
		y_pos = y_pos + y_move;
	}

	double getx_pos() const
	{
		return x_pos;
	}

	double gety_pos() const
	{
		return y_pos;
	}
};

int main()
{
	cout << "==========================================" << endl;
	cout << " Aircraft Position Transmitter Started" << endl;
	cout << "==========================================" << endl;

	z_owned_config_t config;
	z_config_default(&config);

	z_owned_session_t session;

	if (z_open(&session, z_move(config), NULL) < 0)
	{
		cout << "Unable to establish Zenoh session." << endl;
		return -1;
	}

	AircraftEntity aircraft;

	double deltaTime = 0.1;
	for (int frame = 0; frame < 200; frame++)
	{
		aircraft.updatePhysics(deltaTime);
		stringstream message;

		message << fixed << setprecision(4);

		message << "{";
		message << "\"frame\":" << frame << ",";
		message << "\"dt\":" << deltaTime << ",";
		message << "\"x_nm\":" << aircraft.getx_pos() << ",";
		message << "\"y_nm\":" << aircraft.gety_pos();
		message << "}";

		string jsonMessage = message.str();

		z_owned_bytes_t payload;
		z_bytes_from_static_str(&payload, jsonMessage.c_str());

		z_view_keyexpr_t key;
		z_view_keyexpr_from_str(&key,
			"simulation/aircraft/position");

		z_put(
			z_loan(session),
			z_loan(key),
			z_move(payload),
			NULL
		);

		cout << "Sent: " << jsonMessage << endl;
		Sleep(100);
	}

	z_drop(z_move(session));

	return 0;
}