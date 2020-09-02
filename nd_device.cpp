#include "nd_device.h"

inline vector<quat> nd_device::get_raw_cgv_rotations()
{
	nd_handler& ndh = nd_handler::instance();
	ndh.get_rotations(imu_vals, num_imus, id);

	vector<quat> cgv_rotations;
	NDAPISpace::quaternion_t nd_quat;
	for (size_t i = 0; i < num_imus; i++)
	{
		nd_quat = imu_vals[i].rawRotation;
		cgv_rotations.push_back(nd_to_cgv_quat(nd_quat));
	}

	return cgv_rotations;
}

vector<quat> nd_device::get_rel_cgv_rotations()
{
	vector<quat> rotations = get_raw_cgv_rotations();
	for (size_t i = 0; i < num_imus; i++)
	{
		rotations[i] = ref_quats[i] * rotations[i];
	}

	return rotations;
}

// set "new unit"


// converting quats from NDAPI to cgv space

void nd_device::set_actuator_pulse(NDAPISpace::Actuator act, float level, float duration_ms)
{
	nd_handler& ndh = nd_handler::instance();
	ndh.set_actuator_pulse(id, act, level, duration_ms);
}

quat nd_device::nd_to_cgv_quat(NDAPISpace::quaternion_t nd_q)
{
	if (nd_q.w || nd_q.x || nd_q.y || nd_q.z)
	{
		// NDAPI coords are x - right, y - up, z - front
		// --> wrong angle sign around x and y
		return quat(nd_q.w, -nd_q.x, -nd_q.y, nd_q.z);
	}
	else
	{
		// because the nd quats are (0, 0, 0, 0) for virtual devices
		return quat(vec3(0, 1, 0), 0);
	}
}

int nd_device::get_location() {
	nd_handler& ndh = nd_handler::instance();
	return ndh.get_location(id);
}

bool nd_device::are_contacts_joined(NDAPISpace::Contact c1, NDAPISpace::Contact c2) {
	nd_handler& ndh = nd_handler::instance();
	return ndh.are_contacts_joined(c1, c2, id);
}

void nd_device::calibrate()
{
	prev_ref_quats = ref_quats;
	ref_quats = get_raw_cgv_rotations();
	for (size_t i = 0; i < num_imus; i++)
	{
		ref_quats[i] = ref_quats[i].inverse();
		ref_quats[i].normalize();
	}
}
