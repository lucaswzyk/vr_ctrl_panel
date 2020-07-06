#include "NDAPI.h"
#include "stdafx.h"
#include <iostream>

#include <cgv/base/register.h>
#include <cgv/render/drawable.h>
#include <cgv_gl/gl/mesh_render_info.h>
#include <cgv/media/mesh/simple_mesh.h>
#include <cgv/media/color_scale.h>
#include <cgv/gui/event_handler.h>
#include <cgv/gui/provider.h>
#include <cgv_gl/box_renderer.h>
#include <cgv_gl/sphere_renderer.h>
#include <cg_vr/vr_server.h>
#include <cg_vr/vr_events.h>
#include <cgv/signal/signal.h>

#include <chrono>

#include "nd_handler.h"
#include "hand.h"

using namespace std;

extern "C"
{
	__declspec(dllexport) int AmdPowerXpressRequestHighPerformance = 1;
}

enum calibration_stage
{
	NOT_CALIBRATING, REQUESTED, USER_POS, GLOVES, PANEL, NUM_CALIBRATION_STEPS, ABORT = -1
};


class vr_ctrl_panel
	: public cgv::base::base,
	  public cgv::render::drawable,
	  public cgv::gui::event_handler,
	  public cgv::gui::provider
{
	struct calibration
	{
		// settings that can be calibrated
		mat4 model_view_mat, world_to_model;
		map<int, int> tr_assign;

		// bools determining rendered objects
		bool is_load_bridge,
			is_render_hands,
			is_render_panel,
			is_render_bridge;

		// vars used during calibration
		int stage, t_id;
		bool is_signal_invalid, is_using_hmd;
		vec3 user_position, z_dir;
		chrono::steady_clock::time_point last_tp;

		// constants
		int request_dur = 1000,
		hand_calibration_prep = 5000;
		vec3 hand_vs_panel_for_calibration = vec3(.0f, .1f, .2f);
	};

protected:
	// hands
	vector<hand*> hands;
	vector<NDAPISpace::Location> existing_hand_locs;
	vector<vec3> hand_translations;
	float hand_scale = .01f;

	// panel
	conn_panel panel;

	// bridge mesh
	cgv::render::mesh_render_info mri;

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
		return rh.reflect_member("is_load_bridge", c.is_load_bridge)
			&& rh.reflect_member("is_render_mesh", c.is_render_bridge)
			&& rh.reflect_member("is_using_hmd", c.is_using_hmd);
	}

	void on_set(void* member_ptr)
	{
		update_member(member_ptr);
	}

	bool init(cgv::render::context& ctx)
	{
		bool res = true;

		cgv::gui::connect_vr_server(false);
		nd_handler& ndh = nd_handler::instance();
		res = res && nd_handler::instance().get_is_connected();

		//cgv::render::ref_rounded_cone_renderer(ctx, 1);
		cgv::render::ref_box_renderer(ctx, 1);
		cgv::render::ref_sphere_renderer(ctx, 1);

		switch (ndh.get_app_mode())
		{
		case nd_handler::BOTH_HANDS:
			hands.push_back(new hand(NDAPISpace::LOC_RIGHT_HAND));
			existing_hand_locs.push_back(NDAPISpace::LOC_RIGHT_HAND);
			hands.push_back(new hand(NDAPISpace::LOC_LEFT_HAND));
			existing_hand_locs.push_back(NDAPISpace::LOC_LEFT_HAND);
			break;
		case nd_handler::LEFT_ONLY:
			hands.push_back(nullptr);
			hands.push_back(new hand(NDAPISpace::LOC_LEFT_HAND));
			existing_hand_locs.push_back(NDAPISpace::LOC_LEFT_HAND);
			break;
		case nd_handler::RIGHT_ONLY:
			hands.push_back(new hand(NDAPISpace::LOC_RIGHT_HAND));
			existing_hand_locs.push_back(NDAPISpace::LOC_RIGHT_HAND);
			hands.push_back(nullptr);
			break;
		default:
			break;
		}
		hand_translations = vector<vec3>(hands.size(), vec3(0));
		c.model_view_mat.identity();
		c.world_to_model.identity();

		c.stage = NOT_CALIBRATING;

		return res;
	}

	void update_calibration(vr::vr_kit_state state, int t_id)
	{
		if (c.stage > NOT_CALIBRATING && t_id != c.t_id)
		{
			return;
		}

		bool is_calibrating_hand_decl = hands[t_id]->is_in_decl_pose();
		if (c.stage > NOT_CALIBRATING && is_calibrating_hand_decl)
		{
			c.is_signal_invalid = true;
			c.stage = ABORT;
		}

		bool is_calibrating_hand_ack = hands[t_id]->is_in_ack_pose();
		float time_to_calibration;

		switch (c.stage)
		{
		case NOT_CALIBRATING:
			if (!c.is_signal_invalid && is_calibrating_hand_ack)
			{
				c.t_id = t_id;
				next_calibration_stage(false, false);
			}
			break;
		case REQUESTED:
			if (!is_calibrating_hand_ack)
			{
				reset_calibration_stage();
			}
			else
			{
				time_to_calibration = c.request_dur - 
					chrono::duration_cast<chrono::milliseconds>(chrono::steady_clock::now() - c.last_tp).count();
				cout << "Calibration in " << to_string(time_to_calibration / 1000) << "s..." << endl;
				if (time_to_calibration <= 0)
				{
					last_cal = c;
					set_boolean(c.is_render_panel, false);
					set_boolean(c.is_render_bridge, false);
					next_calibration_stage();
				}
			}
			break;
		case USER_POS:
			if (c.is_using_hmd)
			{
				c.user_position = position_from_pose(state.hmd.pose);
				next_calibration_stage(false, true);
			}
			else
			{
				cout << "Tap index and thumb above your head" << endl;
				if (!c.is_signal_invalid && is_calibrating_hand_ack)
				{
					c.user_position = position_from_pose(state.controller[t_id].pose);
					next_calibration_stage();
				}
			}
			break;
		case GLOVES:
			time_to_calibration = c.hand_calibration_prep -
				chrono::duration_cast<chrono::milliseconds>(chrono::steady_clock::now() - c.last_tp).count();
			cout << "Move your hands as shown, fingers together, tap thumb and index when ready" << endl
				 << "Calibration in " << to_string(time_to_calibration / 1000) << "s..." << endl;
			calibrate_new_z(state);
			for (auto loc : existing_hand_locs)
			{
				hands[loc]->calibrate_to_quat(quat(c.z_dir.z(), 0, -c.z_dir.x(), 0));
			}
			if (time_to_calibration <= 0)
			{
				set_boolean(c.is_render_panel, true);
				set_boolean(c.is_render_bridge, true);
				next_calibration_stage();
			}
			break;
		case PANEL:
			cout << "Acknowledge when done" << endl;
			calibrate_model_view(position_from_pose(state.controller[c.t_id].pose));
			if (!c.is_signal_invalid && is_calibrating_hand_ack)
			{
				next_calibration_stage();
			}
			break;
		case ABORT:
			cout << "Restore last calibration (index) or go with this one (middle)?" << endl;
			if (!c.is_signal_invalid && is_calibrating_hand_ack)
			{
				abort_calibration(true);
			}
			else if (!c.is_signal_invalid && is_calibrating_hand_decl)
			{
				abort_calibration(false);
			}
			break;
		default:
			break;
		}

		c.is_signal_invalid = c.is_signal_invalid && (is_calibrating_hand_ack || is_calibrating_hand_decl);
	}

	void next_calibration_stage(bool set_interactive_pulse=true, bool invalidate_ack=true)
	{
		c.stage++;
		if (c.stage == NUM_CALIBRATION_STEPS)
		{
			reset_calibration_stage();
			for (auto loc : existing_hand_locs)
			{
				hands[loc]->init_interactive_pulse(hand::DONE);
			}
		}
		else
		{
			c.last_tp = chrono::steady_clock::now();
			c.is_signal_invalid = invalidate_ack ? true : c.is_signal_invalid;
			if (set_interactive_pulse)
			{
				hands[c.tr_assign[c.t_id]]->init_interactive_pulse(hand::ACK);
			}
		}
	}

	void reset_calibration_stage()
	{
		c.is_signal_invalid = true;
		c.stage = NOT_CALIBRATING;
	}

	void abort_calibration(bool restore)
	{
		for (auto loc : existing_hand_locs)
		{
			hands[loc]->init_interactive_pulse(hand::ABORT);
			if (restore)
			{
				hands[loc]->restore_last_calibration();
			}
		}
		c = last_cal;
		reset_calibration_stage();
	}

	void calibrate_new_z(const vr::vr_kit_state& state)
	{
		vec3 hands_ave(0);
		for (auto assign : c.tr_assign)
		{
			hands_ave += position_from_pose(state.controller[assign.first].pose);
		}
		hands_ave /= c.tr_assign.size();
		c.z_dir = c.user_position - hands_ave;
		c.z_dir.y() = .0f;
		c.z_dir.normalize();
	}

	void calibrate_model_view(vec3 panel_origin)
	{
		mat4 new_model_view_mat;
		new_model_view_mat.identity();
		float rad_to_deg = 45.0f / atan(1.0f);
		float angle_y = acos(c.z_dir.z()) * rad_to_deg;
		if (c.z_dir.x() < 0)
		{
			angle_y = -angle_y;
		}
		new_model_view_mat *= cgv::math::rotate4(vec3(0, angle_y, 0));
		new_model_view_mat.set_col(3, hom_pos(panel_origin));
		vec4 center_translation = hom_pos(-panel_pos_on_bridge - c.hand_vs_panel_for_calibration);
		new_model_view_mat *= cgv::math::translate4(inhom_pos(center_translation));

		c.model_view_mat = new_model_view_mat;
		c.world_to_model = cgv::math::inv(new_model_view_mat);
	}

	void set_boolean(bool& b, bool new_val)
	{
		b = new_val;
		on_set(&b);
	}

	void draw(cgv::render::context& ctx)
	{
		ctx.push_modelview_matrix();
		ctx.mul_modelview_matrix(c.model_view_mat);

		//auto t0 = std::chrono::steady_clock::now();
		for (auto loc : existing_hand_locs)
		{
			hands[loc]->update_and_draw(ctx, panel, hand_translations[loc], hand_scale);
		}
		//auto t1 = std::chrono::steady_clock::now();

		if (c.is_load_bridge)
		{
			load_bridge_mesh(ctx);
		}

		if (c.is_render_bridge && mri.is_constructed())
		{
			//ctx.push_modelview_matrix();
			//ctx.mul_modelview_matrix(bridge_view_mat);
			//mri.render_mesh(ctx, ctx.ref_surface_shader_program(true));
			//ctx.pop_modelview_matrix();
		}

		if (c.is_render_panel)
		{
			panel.draw(ctx);
		}
		auto t2 = std::chrono::steady_clock::now();
		/*cout << "hand: " << std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0).count() << endl;
		cout << "mesh: " << std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1).count() << endl << endl;*/
		ctx.pop_modelview_matrix();
	}

	void clear(cgv::render::context& ctx)
	{
		//cgv::render::ref_rounded_cone_renderer(ctx, -1);
		//cgv::render::ref_sphere_renderer(ctx, -1);
		//cgv::render::ref_box_renderer(ctx, -1);
	}

	// Inherited via event_handler
	virtual bool handle(cgv::gui::event& e) override
	{
		if ((e.get_flags() & cgv::gui::EF_VR) == 0)
		{
			return false;
		}

		if (e.get_kind() == cgv::gui::EID_POSE)
		{
			cgv::gui::vr_pose_event& vrpe = static_cast<cgv::gui::vr_pose_event&>(e);
			int t_id = vrpe.get_trackable_index();
			if (t_id == -1) return false;

			if (!c.tr_assign.size())
			{
				assign_trackers(vrpe.get_state());
			}
			else if (!c.tr_assign.count(t_id))
			{
				reset_tracker_assigns();
				assign_trackers(vrpe.get_state());
			}

			vec3 position = inhom_pos(c.world_to_model * hom(vrpe.get_position()));
			if (c.tr_assign.size() && c.tr_assign.count(t_id))
			{
				int tracker_loc = c.tr_assign[t_id];
				hand_translations[tracker_loc] = position;
				update_calibration(vrpe.get_state(), t_id);
			}
			
			return true;
		}

		return false;
	}

	void assign_trackers(const vr::vr_kit_state& state)
	{
		if (c.tr_assign.size())
		{
			cout << "Trackers already assigned" << endl;
			return;
		}

		if (!existing_hand_locs.size())
		{
			cout << "No hands to assign trackers to" << endl;
		}
		else
		{
			cout << "Assigning trackers" << endl;

			map<float, int, greater<float>> prio_map1, prio_map2;
			for (size_t i = 0; i < 4; i++)
			{
				prio_map1[state.controller[i].pose[10]] = i;
			}

			if (existing_hand_locs.size() == 1)
			{
				c.tr_assign[prio_map1.begin()->second] = existing_hand_locs[0];
			}
			else
			{
				auto it = prio_map1.begin();
				c.tr_assign[it++->second] = NDAPISpace::LOC_RIGHT_HAND;
				c.tr_assign[it->second] = NDAPISpace::LOC_LEFT_HAND;
			}
		}
	}

	void reset_tracker_assigns() {
		cout << "Resetting tracker assignments" << endl;
		c.tr_assign = map<int, int>();
	}

	vec3 position_from_pose(const float pose[12])
	{
		return vec3(pose[9], pose[10], pose[11]);
	}
	
	vec4 hom_pos(vec3 v) { return vec4(v.x(), v.y(), v.z(), 1.0f); }
	vec4 hom_pos(const float pose[12]) { return hom_pos(position_from_pose(pose)); }
	vec3 inhom_pos(vec4 v) { return vec3(v.x(), v.y(), v.z()) / v.w(); }
	vec4 hom_dir(vec3 dir) { return vec4(dir.x(), dir.y(), dir.z(), .0f); }
	vec3 inhom_dir(vec4 dir) { return vec3(dir.x(), dir.y(), dir.z()); }

	virtual void stream_help(std::ostream& os) override
	{
	}

	// Inherited via provider
	virtual void create_gui() override
	{
		add_member_control(this, "render hands", c.is_render_hands, "toggle");
		add_member_control(this, "render panel", c.is_render_panel, "toggle");
		add_member_control(this, "render bridge", c.is_render_bridge, "toggle");
		add_member_control(this, "load bridge mesh", c.is_load_bridge, "toggle");
		cgv::signal::connect_copy(add_button("reassign trackers")->click, rebind(this, &vr_ctrl_panel::reset_tracker_assigns));
		
		for (auto loc : existing_hand_locs)
		{
			string loc_string = loc == NDAPISpace::LOC_RIGHT_HAND ? "right" : "left";
			add_member_control(this, loc_string, *(hands[loc]->get_man_ack()));
		}
	}

	void load_bridge_mesh(cgv::render::context& ctx, const char* file = "bridge_cleaned.obj")
	{
		/*cgv::media::mesh::simple_mesh<> m;
		if (m.read(file))
		{
			if (!m.has_colors())
			{
				m.ensure_colors(cgv::media::CT_RGB, m.get_nr_positions());
				double int_part;
				for (unsigned i = 0; i < m.get_nr_positions(); ++i)
					m.set_color(i, cgv::media::color_scale(modf(20 * double(i) / (m.get_nr_positions() - 1), &int_part)));
			}
			mri.construct_vbos(ctx, m);
			mri.bind(ctx, ctx.ref_surface_shader_program(true));
		}*/
		//bridge_view_mat.identity();
		//bridge_view_mat *= cgv::math::rotate4(180.0f, 0.0f, 1.0f, 0.0f);
		//bridge_view_mat *= cgv::math::translate4(0.0f, -1.5f, -2.7f);
		c.is_load_bridge = false;
	}
};

cgv::base::object_registration<vr_ctrl_panel> vr_ctrl_panel_reg("");