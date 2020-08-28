#pragma once

#include "NDAPI.h"
#include "stdafx.h"
#include <iostream>
#include <fstream>

#include <cgv/base/register.h>
#include <cgv/render/drawable.h>
#include <cgv_gl/gl/mesh_render_info.h>
#include <cgv/media/mesh/simple_mesh.h>
#include <cgv/media/color_scale.h>
#include <cgv/gui/event_handler.h>
#include <cgv/gui/provider.h>
#include <cgv_gl/box_renderer.h>
#include <cgv_gl/sphere_renderer.h>
#include <cgv_gl/rounded_cone_renderer.h>
#include <cg_vr/vr_server.h>
#include <cg_vr/vr_events.h>
#include <cgv/signal/signal.h>

#include <chrono>

#include "nd_handler.h"
#include "hand.h"
#include "mesh.h"
#include "math_conversion.h"
#include "head_up_display.h"

using namespace std;

enum calibration_stage
{
	NOT_CALIBRATING, REQUESTED, USER_POS, USER_POS_MAN, GLOVES, PANEL, EXPORT, NUM_CALIBRATION_STEPS, ABORT = -1
};

using namespace cgv::render;
using namespace cgv::math;

class vr_ctrl_panel
	: public cgv::base::base,
	public cgv::render::drawable,
	public cgv::gui::event_handler,
	public cgv::gui::provider
{
	struct calibration
	{
		// settings that can be calibrated
		mat4 model_view_mat, world_to_model, bridge_view_mat;
		vector<mat3> tracker_refs;
		map<int, int> tr_assign;
		vec3 user_position, z_dir;

		// bools determining rendered objects
		bool load_bridge,
			render_hands,
			render_panel,
			render_bridge;

		// vars used during calibration
		int stage;
		hand* cal_hand;
		bool is_signal_invalid;
		chrono::steady_clock::time_point last_tp;
		vector<mat3> last_tracker_refs;

		// constants
		int request_dur = 1000,
			hand_calibration_prep = 5000;
		vec3 hand_vs_panel_for_calibration = vec3(.0f, .0f, -.2f);

		calibration()
		{
			model_view_mat.identity();
			world_to_model.identity();
			bridge_view_mat = rotate4(vec3(0, 180, 0));
			tracker_refs = vector<mat3>(2);
			tracker_refs[0].identity();
			tracker_refs[1].identity();
			user_position = vec3(0);
			z_dir = vec3(0);
			z_dir.z() = 1.0f;

			load_bridge = true;
			render_hands = true;
			render_panel = true;
			render_bridge = true;

			stage = NOT_CALIBRATING;
			cal_hand = nullptr;
			is_signal_invalid = false;
		}
	};

protected:
	// hands
	vector<hand*> hands;
	vector<NDAPISpace::Location> existing_hand_locs;
	vector<vec3> hand_positions;
	vector<mat3> hand_orientations;

	// panel
	conn_panel panel;

	// bridge mesh
	mesh bridge;

	// head up display
	head_up_display hd;
	vec3 hd_position;
	mat3 hd_orientation;

	// calibration
	calibration c, last_cal;

public:
	vr_ctrl_panel()
	{}

	string get_type_name(void) const
	{
		return "vr_ctrl_panel";
	}

	bool self_reflect(cgv::reflect::reflection_handler& rh)
	{
		return rh.reflect_member("render_hands", c.render_hands)
			&& rh.reflect_member("render_panel", c.render_panel)
			&& rh.reflect_member("load_bridge", c.load_bridge)
			&& rh.reflect_member("render_bridge", c.render_bridge);
	}

	void on_set(void* member_ptr)
	{
		update_member(member_ptr);
	}

	bool init(context& ctx);

	void update_calibration(vr::vr_kit_state state, int t_index);

	void next_calibration_stage(bool set_interactive_pulse = true, bool invalidate_ack = true);

	void reset_calibration_stage();

	void abort_calibration(bool restore);

	void calibrate_new_z(const vr::vr_kit_state& state);

	void calibrate_model_view(vec3 panel_origin);

	void assign_trackers(cgv::gui::vr_pose_event& vrpe);

	void reset_tracker_assigns();

	void export_calibration();

	void load_calibration();

	void set_boolean(bool& b, bool new_val);

	void init_frame(context& ctx);

	void draw(context& ctx);

	void destruct(context& ctx);

	// Inherited via event_handler
	virtual bool handle(cgv::gui::event& e) override;

	virtual void stream_help(std::ostream& os) override {}

	// Inherited via provider
	virtual void create_gui() override;
};

cgv::base::object_registration<vr_ctrl_panel> vr_ctrl_panel_reg("");