#pragma once

#include "nd_handler.h"

using namespace std;

class nd_device
{
	NDAPISpace::Location location;
	int id, num_imus;
	NDAPISpace::imu_sensor_t* imu_vals;

public:
	nd_device() {};

	nd_device(NDAPISpace::Location a_location)
	{
		nd_handler& ndh = nd_handler::instance();
		id = ndh.get_device_id(a_location);
		if (id < 0)
		{
			"Error: could not find device.";
			return;
		}

		location = a_location;
		num_imus = ndh.get_number_of_imus(id);

		imu_vals = new NDAPISpace::imu_sensor_t[num_imus];
	}

	vector<cgv::math::quaternion<float>> get_cgv_rotations()
	{
		nd_handler& ndh = nd_handler::instance();
		ndh.get_rotations(imu_vals, num_imus, id);

		vector<cgv::math::quaternion<float>> cgv_rotations;
		NDAPISpace::quaternion_t nd_quat;
		for (size_t i = 0; i < num_imus; i++)
		{
			nd_quat = imu_vals[i].rawRotation;
			cgv_rotations.push_back(nd_to_cgv_quat(nd_quat));
		}

		return cgv_rotations;
	}

	bool is_left() 
	{
		return location == NDAPISpace::LOC_LEFT_HAND;
	}

	void set_actuator_pulse(NDAPISpace::Actuator act, float level=.1, float duration_ms=100)
	{
		nd_handler& ndh = nd_handler::instance();
		ndh.set_actuator_pulse(act, level, duration_ms);
	}

	static cgv::math::quaternion<float> nd_to_cgv_quat(NDAPISpace::quaternion_t nd_q)
	{
		return cgv::math::quaternion<float>(nd_q.w, -nd_q.x, nd_q.y, nd_q.z);
	}

	int get_location() {
		nd_handler& ndh = nd_handler::instance();
		return ndh.get_location(id); 
	}

	bool are_thumb_index_joined() {
		nd_handler& ndh = nd_handler::instance();
		return ndh.are_thumb_index_joined(id);
	}
};

