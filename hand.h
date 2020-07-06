#pragma once

#include <cgv/base/node.h>
#include <cgv/signal/rebind.h>
#include <cgv/gui/event_handler.h>
#include <cgv/math/ftransform.h>
#include <cgv/utils/scan.h>
#include <cgv/utils/options.h>
#include <cgv/gui/provider.h>

#include <cgv/render/drawable.h>
#include <cgv/math/quaternion.h>
#include <cgv_gl/sphere_renderer.h>
#include <cgv_gl/gl/gl.h>
#include <iostream>
#include <chrono>

#include <vr/vr_driver.h>
#include <cg_vr/vr_server.h>
#include <cgv/gui/event_handler.h>

#include "nd_device.h"
#include "nd_handler.h"
#include "conn_panel.h"

using namespace std;

class hand
	: public cgv::base::node,
	public cgv::render::drawable
{
	enum hand_parts
	{
		PALM, THUMB, INDEX, MIDDLE, RING, PINKY, NUM_HAND_PARTS
	};

	enum phalanges
	{
		PROXIMAL, INTERMED, DISTAL, NUM_BONES_PER_FINGER
	};

	struct joint_positions {
		vector<vector<vec3>> positions;

		joint_positions()
		{
			for (size_t i = 0; i < NUM_HAND_PARTS; i++)
			{
				positions.push_back(vector<vec3>(NUM_BONES_PER_FINGER));
			}
		}

		void push_back(int part, vec3 v)
		{
			positions[part].push_back(v);
		}

		void rotate(int part, quat rotation)
		{
			for (size_t i = 0; i < positions[part].size(); i++)
			{
				rotation.rotate(positions[part][i]);
			}
		}

		void translate(int part, vec3 translation)
		{
			for (size_t i = 0; i < positions[part].size(); i++)
			{
				positions[part][i] += translation;
			}
		}

		void translate_neg_z(int part, float z)
		{
			translate(part, vec3(0, 0, -z));
		}

		void translate(vec3 translation)
		{
			for (size_t part = 0; part < positions.size(); part++)
			{
				translate(part, translation);
			}
		}

		void scale(float scale)
		{
			for (size_t part = 0; part < positions.size(); part++)
			{
				for (size_t i = 0; i < positions[part].size(); i++)
				{
					positions[part][i] *= scale;
				}
			}
		}

		vector<vec3> make_array()
		{
			vector<vec3> result;
			for (size_t i = 0; i < positions.size(); i++)
			{
				for (size_t j = 0; j < positions[i].size(); j++)
				{
					result.push_back(positions[i][j]);
				}
			}

			return result;
		}
	};

public:
	enum pulse_kind
	{
		NONE, ACK, DONE, ABORT
	};

protected:
	nd_device device;
	vector<vec3> bone_lengths, palm_resting;
	vector<vector<quat>> recursive_rotations;
	vec3 origin;
	joint_positions pose;
	float scale = .01f;
	vector<NDAPISpace::Actuator> actuators;
	cgv::render::sphere_render_style srs;
	bool man_ack;

	chrono::steady_clock::time_point pulse_start;
	pulse_kind current_pulse;
	int num_delivered_pulses, num_part_pulses_abort = 3, duration_done_pulse_ms = 1000, duration_abort_pulse = 600;

public:
	hand() 
	{}

	hand(NDAPISpace::Location location)
		: man_ack(false), current_pulse(NONE)
	{
		device = nd_device(location);
		set_geometry();
	}

	string get_type_name(void) const
	{
		return "hand";
	}

	void set_geometry()
	{
		vector<quat> identity_vec(NUM_BONES_PER_FINGER, quat(1, 0, 0, 0));
		recursive_rotations = vector<vector<quat>>(NUM_HAND_PARTS, identity_vec);

		// palm
		bone_lengths.push_back(vec3(0));
		palm_resting.push_back(vec3(0));
		recursive_rotations[PALM] = vector<quat>(1, quat(1, 0, 0, 0));

		// thumb
		bone_lengths.push_back(vec3(0, 4, 3));
		palm_resting.push_back(vec3(-5, -3, 2.5));

		// index
		bone_lengths.push_back(vec3(5, 3, 2));
		palm_resting.push_back(vec3(-3.5, 0, -4));

		// middle
		bone_lengths.push_back(vec3(5, 3.5, 2.5));
		palm_resting.push_back(vec3(-1, 0, -4.5));

		// ring
		bone_lengths.push_back(vec3(4.5, 3.5, 2.5));
		palm_resting.push_back(vec3(1.5, 0, -4));

		// pinky
		bone_lengths.push_back(vec3(4, 2.5, 2));
		palm_resting.push_back(vec3(4, 0, -4));

		palm_resting.push_back(vec3(3.5, 3, 3));
		palm_resting.push_back(vec3(3, -2, 2.5));

		if (device.is_left())
		{
			for (size_t i = 0; i < palm_resting.size(); i++)
			{
				vec3 pos = palm_resting[i];
				palm_resting[i] = vec3(-pos.x(), pos.y(), pos.z());
			}
			srs.surface_color = rgb(1, 0, 0);
		}
		else
		{
			srs.surface_color = rgb(0, 0, 1);
		}

		actuators = vector<NDAPISpace::Actuator>{
			NDAPISpace::ACT_THUMB,
			NDAPISpace::ACT_INDEX,
			NDAPISpace::ACT_MIDDLE,
			NDAPISpace::ACT_RING,
			NDAPISpace::ACT_PINKY,
			NDAPISpace::ACT_PALM_INDEX_UP,
			NDAPISpace::ACT_PALM_MIDDLE_UP,
			NDAPISpace::ACT_PALM_PINKY_UP,
			NDAPISpace::ACT_PALM_INDEX_DOWN,
			NDAPISpace::ACT_PALM_PINKY_DOWN
		};
	}

	void update_and_draw(cgv::render::context& ctx, const conn_panel& cp, vec3 translation, float ascale)
	{
		deliver_interactive_pulse();
		set_pose_and_actuators(cp, translation, ascale);
		draw(ctx);
	}

	void set_pose_and_actuators(const conn_panel& cp, vec3 translation, float ascale)
	{
		origin = translation;
		scale = ascale;
		set_rotations();

		pose = joint_positions();
		pose.positions[PALM] = palm_resting;
		pose.rotate(PALM, recursive_rotations[PALM][0]);

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
		pose.translate(origin);

		vector<vec3> touching_points = vector<vec3>{
			pose.positions[THUMB][DISTAL],
			pose.positions[INDEX][DISTAL],
			pose.positions[MIDDLE][DISTAL],
			pose.positions[RING][DISTAL],
			pose.positions[PINKY][DISTAL],
			pose.positions[PALM][INDEX],
			.5f * (pose.positions[PALM][MIDDLE] + pose.positions[PALM][RING]),
			pose.positions[PALM][PINKY],
			pose.positions[PALM][NUM_HAND_PARTS],
			pose.positions[PALM][NUM_HAND_PARTS + 1]
		};

		std::set<int> touching_indices = cp.check_containments(touching_points, scale);

		for each (int ind in touching_indices)
		{
			device.set_actuator_pulse(actuators[ind]);
		}
	}

	void draw(cgv::render::context& ctx)
	{
		vector<vec3> positions = pose.make_array();
		vector<float> radius_array = vector<float>(positions.size(), scale);
		radius_array[0] = 2 * scale;

		cgv::render::sphere_renderer& sr = cgv::render::ref_sphere_renderer(ctx);
		sr.set_position_array(ctx, positions);
		sr.set_radius_array(ctx, radius_array);
		sr.set_render_style(srs);
		sr.validate_and_enable(ctx);
		glDrawArrays(GL_POINTS, 0, positions.size());
		sr.disable(ctx);
	}

	void set_rotations()
	{
		vector<quat> imu_rotations = device.get_rel_cgv_rotations();
		quat thumb0_quat = imu_rotations[NDAPISpace::IMULOC_THUMB0];

		recursive_rotations[PALM][0] = imu_rotations[NDAPISpace::IMULOC_PALM].inverse();
		recursive_rotations[THUMB][INTERMED] = thumb0_quat.inverse();
		recursive_rotations[THUMB][DISTAL] = thumb0_quat * imu_rotations[NDAPISpace::IMULOC_THUMB1].inverse();
		recursive_rotations[INDEX][PROXIMAL] = imu_rotations[NDAPISpace::IMULOC_INDEX].inverse();
		recursive_rotations[MIDDLE][PROXIMAL] = imu_rotations[NDAPISpace::IMULOC_MIDDLE].inverse();
		recursive_rotations[RING][PROXIMAL] = imu_rotations[NDAPISpace::IMULOC_RING].inverse();
		recursive_rotations[PINKY][PROXIMAL] = imu_rotations[NDAPISpace::IMULOC_PINKY].inverse();
	}

	int get_location() { return device.get_location(); }
	bool* get_man_ack() { return &man_ack; }
	void calibrate_to_quat(quat q) { device.calibrate_to_quat(q); }
	void restore_last_calibration() { device.restore_last_calibration(); }

	bool is_in_ack_pose() { return device.are_contacts_joined(NDAPISpace::CONT_THUMB, NDAPISpace::CONT_INDEX) || man_ack; }
	void set_ack_pulse() {
		for (auto act : actuators)
		{
			device.set_actuator_pulse(act);
		}
	}
	bool is_in_decl_pose() { return device.are_contacts_joined(NDAPISpace::CONT_THUMB, NDAPISpace::CONT_MIDDLE); }

	void init_interactive_pulse(pulse_kind kind)
	{
		reset_interactive_pulse();
		current_pulse = kind;
		pulse_start = chrono::steady_clock::now();
	}

	void deliver_interactive_pulse()
	{
		int pulse_stage;
		switch (current_pulse)
		{
		case ACK:
			for (auto act : actuators)
			{
				device.set_actuator_pulse(act);
			}
			reset_interactive_pulse();
			break;
		case DONE:
			pulse_stage = chrono::duration_cast<chrono::milliseconds>(chrono::steady_clock::now() - pulse_start).count();
			pulse_stage = (actuators.size() * pulse_stage) / duration_done_pulse_ms;
			if (pulse_stage >= actuators.size())
			{
				reset_interactive_pulse();
			}
			if (pulse_stage == num_delivered_pulses)
			{
				device.set_actuator_pulse(actuators[pulse_stage]);
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
				for (auto act : actuators)
				{
					device.set_actuator_pulse(act, .5f);
				}
				num_delivered_pulses++;
			}
			break;
		default:
			break;
		}
	}

	void reset_interactive_pulse() {
		current_pulse = NONE;
		num_delivered_pulses = 0;
	}
};