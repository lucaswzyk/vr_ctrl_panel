#include "hand.h"

// rotate complete part

inline void hand::joint_positions::rotate(int part, quat rotation)
{
	for (size_t i = 0; i < positions[part].size(); i++)
	{
		rotation.rotate(positions[part][i]);
	}
}

// translate complete part

inline void hand::joint_positions::translate(int part, vec3 translation)
{
	for (size_t i = 0; i < positions[part].size(); i++)
	{
		positions[part][i] += translation;
	}
}

// translate complete part along neg. z-axis

inline void hand::joint_positions::translate_neg_z(int part, float z)
{
	translate(part, vec3(0, 0, -z));
}

// translate all positions
// used to move hand from construction origin
// to model view location

inline void hand::joint_positions::translate(vec3 translation)
{
	for (size_t part = 0; part < positions.size(); part++)
	{
		translate(part, translation);
	}
}

// scale all positions
// used to realize hand size at construction origin

inline void hand::joint_positions::scale(float scale)
{
	for (size_t part = 0; part < positions.size(); part++)
	{
		for (size_t i = 0; i < positions[part].size(); i++)
		{
			positions[part][i] *= scale;
		}
	}
}

// generate linearized from positions

inline vector<vec3> hand::joint_positions::make_array()
{
	vector<vec3> result;
	int ind = 0;
	for (size_t i = 0; i < positions.size(); i++)
	{
		for (size_t j = 0; j < positions[i].size(); j++)
		{
			result.push_back(positions[i][j]);
			lin_to_anat[ind++] = pair<int, int>(i, j);
		}
	}

	return result;
}

void hand::init()
{
	vector<quat> identity_vec(NUM_BONES_PER_FINGER, quat(1, 0, 0, 0));
	recursive_rotations = vector<vector<quat>>(NUM_HAND_PARTS, identity_vec);

	// palm
	bone_lengths.push_back(vec3(0));
	palm_resting.push_back(vec3(0));
	recursive_rotations[PALM] = vector<quat>(1, quat(1, 0, 0, 0));

	// thumb
	bone_lengths.push_back(vec3(0, 4, 3));
	palm_resting.push_back(vec3(-5, -2, 2));

	// index
	bone_lengths.push_back(vec3(5, 3, 2));
	palm_resting.push_back(vec3(-3.5, 0, -4));

	// middle
	bone_lengths.push_back(vec3(5, 3.5, 2.5));
	palm_resting.push_back(vec3(-1, 0, -4.5));

	// ring
	bone_lengths.push_back(vec3(4.5, 3.5, 2.5));
	palm_resting.push_back(vec3(1.5, 0, -4.5));

	// pinky
	bone_lengths.push_back(vec3(4, 2.5, 2));
	palm_resting.push_back(vec3(4, 0, -4));

	palm_resting.push_back(vec3(3.5, 0, 2.5));
	palm_resting.push_back(vec3(-3, 0, 3.5));

	if (device.is_left())
	{
		for (size_t i = 0; i < palm_resting.size(); i++)
		{
			vec3 pos = palm_resting[i];
			palm_resting[i] = vec3(-pos.x(), pos.y(), pos.z());
		}
		srs.surface_color = rgb(0, 1, 0);
	}
	else
	{
		srs.surface_color = rgb(1, 0, 0);
	}

	anat_to_actuators[pair<int, int>(THUMB, DISTAL)] = NDAPISpace::ACT_THUMB;
	anat_to_actuators[pair<int, int>(INDEX, DISTAL)] = NDAPISpace::ACT_INDEX;
	anat_to_actuators[pair<int, int>(MIDDLE, DISTAL)] = NDAPISpace::ACT_MIDDLE;
	anat_to_actuators[pair<int, int>(RING, DISTAL)] = NDAPISpace::ACT_RING;
	anat_to_actuators[pair<int, int>(PINKY, DISTAL)] = NDAPISpace::ACT_PINKY;
	anat_to_actuators[pair<int, int>(PALM, INDEX)] = NDAPISpace::ACT_PALM_INDEX_UP;
	anat_to_actuators[pair<int, int>(PALM, MIDDLE)] = NDAPISpace::ACT_PALM_MIDDLE_UP;
	anat_to_actuators[pair<int, int>(PALM, PINKY)] = NDAPISpace::ACT_PALM_PINKY_UP;
	anat_to_actuators[pair<int, int>(PALM, NUM_HAND_PARTS)] = NDAPISpace::ACT_PALM_INDEX_DOWN;
	anat_to_actuators[pair<int, int>(PALM, NUM_HAND_PARTS + 1)] = NDAPISpace::ACT_PALM_PINKY_DOWN;

	cone_inds = vector<GLuint>{
		// palm
		2, 3, 3, 4, 4, 5, 5, 6, 6, 7, 7, 2,
		// thumb
		7, 8, 8, 9, 9, 10,
		// index
		2, 11, 11, 12, 12, 13,
		// middle
		3, 14, 14, 15, 15, 16,
		// ring
		4, 17, 17, 18, 18, 19,
		// pinky
		5, 20, 20, 21, 21, 22
	};
	rcrs.radius = .7 * scale;
	rcrs.surface_color = rgb(1, 1, 1);
}

void hand::update_and_draw(cgv::render::context& ctx, const conn_panel& cp, vec3 pos, mat3 ori)
{
	deliver_interactive_pulse();
	set_pose_and_actuators(cp, pos, ori);
	draw(ctx);
}

inline void hand::set_pose_and_actuators(const conn_panel& cp, vec3 position, mat3 orientation)
{
	set_rotations(orientation);

	pose = joint_positions();
	pose.positions[PALM] = palm_resting;
	pose.rotate(PALM, recursive_rotations[PALM][0]);

	// construct finger from distal to proximal
	for (size_t finger = THUMB; finger < NUM_HAND_PARTS; finger++)
	{
		pose.positions[finger][DISTAL] = vec3(0);
		pose.translate_neg_z(finger, bone_lengths[finger][DISTAL]);
		pose.rotate(finger, recursive_rotations[finger][DISTAL]);

		pose.positions[finger][INTERMED] = vec3(0);
		pose.translate_neg_z(finger, bone_lengths[finger][INTERMED]);
		pose.rotate(finger, recursive_rotations[finger][INTERMED]);

		pose.positions[finger][PROXIMAL] = vec3(0);
		pose.translate_neg_z(finger, bone_lengths[finger][PROXIMAL]);
		pose.rotate(finger, recursive_rotations[finger][PROXIMAL]);

		pose.translate(finger, pose.positions[PALM][finger]);
	}

	pose.scale(scale);
	pose.translate(position);

	// hand pose to conn_panel
	containment_info ci;
	ci.tolerance = scale;
	ci.positions = pose.make_array();
	ci.contacts[0] = device.are_contacts_joined(NDAPISpace::CONT_THUMB, NDAPISpace::CONT_INDEX);
	ci.contacts[1] = device.are_contacts_joined(NDAPISpace::CONT_THUMB, NDAPISpace::CONT_MIDDLE);
	ci.contacts[2] = device.are_contacts_joined(NDAPISpace::CONT_PALM, NDAPISpace::CONT_INDEX);
	ci.contacts[3] = device.are_contacts_joined(NDAPISpace::CONT_PALM, NDAPISpace::CONT_MIDDLE);
	std::map<int, float> touching_indices = cp.check_containments(ci, device.get_location());

	for (auto ind_strength : touching_indices)
	{
		pair<int, int> anatomical = pose.lin_to_anat[ind_strength.first];
		if (anat_to_actuators.count(anatomical))
		{
			device.set_actuator_pulse(anat_to_actuators[anatomical], ind_strength.second);
		}
	}
}

void hand::draw(context& ctx)
{
	vector<vec3> positions = pose.make_array();
	vector<float> radius_array = vector<float>(positions.size(), scale);
	radius_array[0] = 2 * scale;

	sphere_renderer& sr = ref_sphere_renderer(ctx);
	sr.set_position_array(ctx, positions);
	sr.set_radius_array(ctx, radius_array);
	sr.set_render_style(srs);
	sr.render(ctx, 0, positions.size());

	rounded_cone_renderer& rcr = ref_rounded_cone_renderer(ctx);
	rcr.set_position_array(ctx, positions);
	rcr.set_indices(ctx, cone_inds);
	rcr.set_render_style(rcrs);
	rcr.render(ctx, 0, cone_inds.size());
}

inline void hand::set_rotations(mat3 orientation)
{
	vector<quat> imu_rotations = device.get_rel_cgv_rotations();
	quat thumb0_quat = imu_rotations[NDAPISpace::IMULOC_THUMB0];

	quat palm_rot = palm_ref * quat(orientation),
		palm_inv = palm_rot.inverse();
	recursive_rotations[PALM][0] = palm_rot;
	recursive_rotations[THUMB][INTERMED] = thumb0_quat;
	recursive_rotations[THUMB][DISTAL] = thumb0_quat.inverse() * imu_rotations[NDAPISpace::IMULOC_THUMB1];
	recursive_rotations[INDEX][PROXIMAL] = imu_rotations[NDAPISpace::IMULOC_INDEX];
	recursive_rotations[MIDDLE][PROXIMAL] = imu_rotations[NDAPISpace::IMULOC_MIDDLE];
	recursive_rotations[RING][PROXIMAL] = imu_rotations[NDAPISpace::IMULOC_RING];
	recursive_rotations[PINKY][PROXIMAL] = imu_rotations[NDAPISpace::IMULOC_PINKY];

	// split intermediate rotation to all phalanges
	quat rot;
	float roll, pitch, yaw;
	for (size_t finger = INDEX; finger < NUM_HAND_PARTS; finger++)
	{
		rot = palm_inv * recursive_rotations[finger][PROXIMAL];
		roll = atan2(
			2 * (rot.w() * rot.x() + rot.y() * rot.z()),
			1 - 2 * (rot.x() * rot.x() + rot.y() * rot.y())
		);

		if (roll > M_PI / 2)
		{
			roll -= 2 * M_PI;
		}

		if (roll < 0)
		{
			float sinp = 2 * (rot.w() * rot.y() - rot.z() * rot.x()), pitch;
			if (abs(sinp) >= 1)
			{
				pitch = copysign(M_PI / 2, sinp);
			}
			else
			{
				pitch = asin(sinp);
			}
			float yaw = atan2(
				2 * (rot.w() * rot.z() + rot.x() * rot.y()),
				1 - 2 * (rot.y() * rot.y() + rot.z() * rot.z())
			);

			vec3 x(1, 0, 0), y(0, 1, 0), z(0, 0, 1);
			recursive_rotations[finger][PROXIMAL] = palm_rot
				* quat(z, yaw) * quat(y, pitch) * quat(x, rot_split.x() * roll);
			recursive_rotations[finger][PROXIMAL].normalize();
			recursive_rotations[finger][INTERMED] = quat(x, rot_split.y() * roll);
			recursive_rotations[finger][INTERMED].normalize();
			recursive_rotations[finger][DISTAL] = quat(x, min(1.4f, rot_split.z() * roll));
			recursive_rotations[finger][DISTAL].normalize();
		}
	}
}

void hand::calibrate_to_quat(quat q) {
	last_palm_ref = palm_ref;
	palm_ref = q;
	device.calibrate();
}

void hand::restore_last_calibration() {
	palm_ref = last_palm_ref;
	device.restore_last_calibration();
}

inline void hand::set_ack_pulse() {
	for (auto p : anat_to_actuators)
	{
		device.set_actuator_pulse(p.second);
	}
}

void hand::init_interactive_pulse(pulse_kind kind)
{
	reset_interactive_pulse();
	current_pulse = kind;
	pulse_start = chrono::steady_clock::now();
}

inline void hand::deliver_interactive_pulse()
{
	int pulse_stage;
	switch (current_pulse)
	{
	case ACK:
		for (auto p : anat_to_actuators)
		{
			device.set_actuator_pulse(p.second);
		}
		reset_interactive_pulse();
		break;
	case DONE:
		pulse_stage = chrono::duration_cast<chrono::milliseconds>(chrono::steady_clock::now() - pulse_start).count();
		pulse_stage = (anat_to_actuators.size() * pulse_stage) / duration_done_pulse_ms;
		if (pulse_stage >= anat_to_actuators.size())
		{
			reset_interactive_pulse();
		}
		if (pulse_stage == num_delivered_pulses)
		{
			auto it = anat_to_actuators.begin();
			for (size_t i = 0; i < pulse_stage; i++)
			{
				it++;
			}
			cout << it->second << endl;
			device.set_actuator_pulse(it->second, .5f);
			num_delivered_pulses++;
		}
		break;
	case ABORT:
		pulse_stage = chrono::duration_cast<chrono::milliseconds>(chrono::steady_clock::now() - pulse_start).count();
		pulse_stage = (num_part_pulses_abort * pulse_stage) / duration_abort_pulse;
		if (pulse_stage >= num_part_pulses_abort)
		{
			reset_interactive_pulse();
		}
		if (pulse_stage == num_delivered_pulses)
		{
			for (auto p : anat_to_actuators)
			{
				device.set_actuator_pulse(p.second, .5f);
			}
			num_delivered_pulses++;
		}
		break;
	default:
		break;
	}
}

inline void hand::reset_interactive_pulse() {
	current_pulse = NONE;
	num_delivered_pulses = 0;
}
