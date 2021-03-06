#pragma once

#include "NDAPI.h"
#include <iostream>

#include <cgv/math/quaternion.h>

using namespace std;
class nd_handler
{
public:
	enum mode
	{
		BOTH_HANDS, LEFT_ONLY, RIGHT_ONLY, NO_DEVICE = -1
	};

private:
	NDAPISpace::NDAPI nd;
	int* device_ids;
	bool is_connected;
	mode app_mode;

	nd_handler() { is_connected = connect(); }
	nd_handler(const nd_handler&);
	nd_handler& operator = (const nd_handler&);

	bool connect();

public:
	static nd_handler& instance()
	{
		static nd_handler _instance;
		return _instance;
	}

	~nd_handler() {
		delete[] device_ids;
		try
		{
			if (is_connected)
			{
				nd.closeConnection();
			}
			nd.~NDAPI();
		}
		catch (const std::exception& e)
		{
			cout << e.what();
		}
	}

	bool get_is_connected() { return is_connected; }
	int get_app_mode() { return app_mode; }
	int get_device_id(NDAPISpace::Location location);

	// hand-over methods from NDAPI
	int get_number_of_imus(int device_id) { return nd.getNumberOfImus(device_id); }
	int get_rotations(NDAPISpace::imu_sensor_t* imus, int num_imus, int device_id) { return nd.getRotations(imus, num_imus, device_id); }
	int get_location(int id) { return nd.getDeviceLocation(id); }
	bool are_contacts_joined(NDAPISpace::Contact c1, NDAPISpace::Contact c2, int id) { return nd.areContactsJoined(c1, c2, id); }
	void set_actuator_pulse(int device_id, NDAPISpace::Actuator act, float level = .1f, float duration_ms = 100)
	{
		nd.setActuatorPulse(act, level, duration_ms, device_id);
	}
};

