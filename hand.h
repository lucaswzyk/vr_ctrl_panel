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
#include <cgv_gl/rounded_cone_renderer.h>
#include <cgv_gl/gl/gl.h>
#include <iostream>
#include <chrono>

#include <vr/vr_driver.h>
#include <cg_vr/vr_server.h>
#include <cgv/gui/event_handler.h>

#include "nd_device.h"
#include "nd_handler.h"
#include "conn_panel.h"
#include "math_conversion.h"

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
		// positions of hand joints in model space
		// structure: hand_part<phalanx<position>>
		vector<vector<vec3>> positions;
		// positions linearized in hand_part major format
		// set by make_array() 
		vector<vec3> linearized;
		// correspondence of indices in linearized
		// to double indices in positions
		map<int, pair<int, int>> lin_to_anat;

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

		// rotate complete part
		void rotate(int part, quat rotation);

		// translate complete part
		void translate(int part, vec3 translation);

		// translate complete part along neg. z-axis
		void translate_neg_z(int part, float z);

		// translate all positions
		// used to move hand from construction origin
		// to model view location
		void translate(vec3 translation);

		// scale all positions
		// used to realize hand size at construction origin
		void scale(float scale);

		// generate linearized from positions
		vector<vec3> make_array();
	};

public:
	enum pulse_kind
	{
		NONE, ACK, DONE, ABORT
	};

protected:
	// glove
	nd_device device;

	// geometry
	joint_positions pose;
	vector<vector<quat>> recursive_rotations;
	vector<vec3> bone_lengths, palm_resting;
	vector<GLuint> cone_inds;
	const vec3 rot_split = vec3(.5f, .5f, .25f);

	// calibration
	quat last_palm_ref, palm_ref;
	const float scale = .007f;
	
	// actuators and pulses
	map<pair<int, int>, NDAPISpace::Actuator> anat_to_actuators;
	chrono::steady_clock::time_point pulse_start;
	pulse_kind current_pulse;
	int num_delivered_pulses;
	const int num_part_pulses_abort = 3, 
		duration_done_pulse_ms = 1000, 
		duration_abort_pulse = 600;

	// rendering
	sphere_render_style srs;
	rounded_cone_render_style rcrs;

public:
	hand() 
	{}

	hand(NDAPISpace::Location location, mat3 a_palm_ref)
		: current_pulse(NONE)
	{
		device = nd_device(location);
		calibrate_to_mat(a_palm_ref);
		last_palm_ref = palm_ref;
		init();
	}

	string get_type_name(void) const
	{
		return "hand";
	}

	void init();

	void update_and_draw(cgv::render::context& ctx, const conn_panel& cp, vec3 pos, mat3 ori);

	void set_pose_and_actuators(const conn_panel& cp, vec3 position, mat3 orientation);

	void draw(context& ctx);

	void set_rotations(mat3 orientation);

	int get_location() { return device.get_location(); }

	void calibrate_to_mat(mat3 ref_mat);

	void restore_last_calibration();

	bool is_in_ack_pose() { return device.are_contacts_joined(NDAPISpace::CONT_THUMB, NDAPISpace::CONT_INDEX); }
	bool is_in_decl_pose() { return device.are_contacts_joined(NDAPISpace::CONT_THUMB, NDAPISpace::CONT_MIDDLE); }
	bool is_in_choice1_pose() { return device.are_contacts_joined(NDAPISpace::CONT_PALM, NDAPISpace::CONT_INDEX); }
	bool is_in_choice2_pose() { return device.are_contacts_joined(NDAPISpace::CONT_PALM, NDAPISpace::CONT_MIDDLE); }

	void set_ack_pulse();

	void init_interactive_pulse(pulse_kind kind);

	void deliver_interactive_pulse();

	void reset_interactive_pulse();
};