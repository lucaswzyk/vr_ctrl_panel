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
	static nd_handler* _instance;
	bool is_connected;
	mode app_mode;
	NDAPISpace::NDAPI nd;
	int* device_ids;

	nd_handler() { is_connected = connect(); }
	nd_handler(const nd_handler&);

	bool connect();

	class nd_guard
	{
	public:
		~nd_guard()
		{
			if (NULL != nd_handler::_instance)
			{
				// does not throw exception if breakpoint inserted -> wait?
				delete nd_handler::_instance;
			}
		}
	};

public:
	static nd_handler* instance()
	{
		static nd_guard guard;
		if (!_instance)
		{
			_instance = new nd_handler();
		}
		
		return _instance;
	}

	bool get_is_connected() { return is_connected; }
	int get_app_mode() { return app_mode; }
	int get_device_id(NDAPISpace::Location location);

	// hand-over methods from NDAPI
	int get_number_of_imus(int device_id) { return nd.getNumberOfImus(device_id); }
	int get_rotations(NDAPISpace::imu_sensor_t* imus, int num_imus, int device_id) { return nd.getRotations(imus, num_imus, device_id); }
	void set_index_pulse()
	{
		nd.setActuatorPulse(NDAPISpace::ACT_INDEX, .5, 50, device_ids[0]);
	}
};

