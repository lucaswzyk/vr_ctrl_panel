#pragma once

#include<cgv/render/render_types.h>

#include "nd_handler.h"

using namespace std;
typedef cgv::math::quaternion<float> quat;
typedef cgv::render::render_types::vec3 vec3;

class nd_device
{
protected:
	// left or right hand
	NDAPISpace::Location location;
	// ND internal ID and number of inertial sensors
	int id, num_imus;
	
	// needs to be array for get_rotations()
	NDAPISpace::imu_sensor_t* imu_vals;
	// quats saved for calibration ("new unit quat")
	vector<quat> ref_quats, prev_ref_quats;

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

		ref_quats = vector<quat>(num_imus, quat(1, 0, 0, 0));
		prev_ref_quats = ref_quats;
	}

	vector<quat> get_raw_cgv_rotations();

	// rotations relative to ref_quats
	vector<quat> get_rel_cgv_rotations();

	bool is_left() { return location == NDAPISpace::LOC_LEFT_HAND; }

	void set_actuator_pulse(NDAPISpace::Actuator act, float level = .1, float duration_ms = 100);

	// converting quats from NDAPI to cgv space
	static quat nd_to_cgv_quat(NDAPISpace::quaternion_t nd_q);

	int get_location();

	bool are_contacts_joined(NDAPISpace::Contact c1, NDAPISpace::Contact c2);

	// set "new unit"
	void calibrate();

	void restore_last_calibration() { ref_quats = prev_ref_quats; }
};

