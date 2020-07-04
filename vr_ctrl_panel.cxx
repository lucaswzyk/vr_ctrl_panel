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

#include <chrono>

#include "nd_handler.h"
#include "hand.h"

using namespace std;

extern "C"
{
	__declspec(dllexport) int AmdPowerXpressRequestHighPerformance = 1;
}

enum calibration_mode
{
	NOT_CALIBRATING, REQUESTED, CHEST_CLICK, GLOVES1, GLOVES2, PANEL, NUM_CALIBRATION_STEPS
};

class vr_ctrl_panel
	: public cgv::base::base,
	  public cgv::render::drawable,
	  public cgv::gui::event_handler,
	  public cgv::gui::provider
{
protected:
	vec3 origin, user_position;
	float angle_y;
	mat4 model_view_mat, physical_to_world;

	vector<hand*> hands;
	vector<NDAPISpace::Location> existing_hand_locs;
	int num_hands;
	map<int, int> tracker_assigns;

	// TODO combine these
	vector<vec3> hand_translations;
	bool trackers_assigned;
	nd_handler::mode app_mode;
	float hand_scale = .01f;

	cgv::render::mesh_render_info mri;
	bool is_load_mesh, is_render_mesh;

	conn_panel panel;

	// calibration
	bool is_ack_deprecated;
	float calibration_pose[12];
	chrono::steady_clock::time_point calibration_time;
	int calibration_request_duration_ms, hand_calibration_duration_ms, 
		calibrating_tracker, current_calibration_stage;
	vec3 hand_vs_panel_for_calibration = vec3(.0f, .1f, .15f);

public:
	vr_ctrl_panel()
		: is_load_mesh(true), is_render_mesh(true), 
		  angle_y(.0f), app_mode(nd_handler::NO_DEVICE), trackers_assigned(false)
	{}

	string get_type_name(void) const
	{
		return "vr_ctrl_panel";
	}

	bool self_reflect(cgv::reflect::reflection_handler& rh)
	{
		return rh.reflect_member("is_load_mesh", is_load_mesh)
			&& rh.reflect_member("is_render_mesh", is_render_mesh);
	}

	void on_set(void* member_ptr)
	{
		if (member_ptr == &origin.x() 
			|| member_ptr == &origin.y()
			|| member_ptr == &origin.z() 
			|| member_ptr == &angle_y)
		{
			model_view_mat.identity();
			model_view_mat *= cgv::math::translate4(origin);
			model_view_mat *= cgv::math::rotate4(angle_y, vec3(0, 1, 0));

			physical_to_world.identity();
			physical_to_world *= cgv::math::rotate4(-angle_y, vec3(0, 1, 0));
			physical_to_world *= cgv::math::translate4(-origin);
		}

		if (member_ptr == &trackers_assigned)
		{
			tracker_assigns = map<int, int>();
		}

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
			num_hands = 2;
			break;
		case nd_handler::LEFT_ONLY:
			hands.push_back(nullptr);
			hands.push_back(new hand(NDAPISpace::LOC_LEFT_HAND));
			existing_hand_locs.push_back(NDAPISpace::LOC_LEFT_HAND);
			num_hands = 1;
			break;
		case nd_handler::RIGHT_ONLY:
			hands.push_back(new hand(NDAPISpace::LOC_RIGHT_HAND));
			existing_hand_locs.push_back(NDAPISpace::LOC_RIGHT_HAND);
			hands.push_back(nullptr);
			num_hands = 1;
			break;
		default:
			num_hands = 0;
			break;
		}
		hand_translations = vector<vec3>(hands.size(), vec3(0));
		model_view_mat.identity();
		physical_to_world.identity();

		current_calibration_stage = NOT_CALIBRATING;
		calibration_request_duration_ms = 1000;
		hand_calibration_duration_ms = 2000;

		return res;
	}

	void update_calibration(vr::vr_kit_state state, int t_id)
	{
		if (current_calibration_stage > NOT_CALIBRATING && t_id != calibrating_tracker)
		{
			return;
		}

		bool is_calibrating_hand_ack = hands[tracker_assigns[t_id]]->is_in_ack_pose();
		float time_to_calibration;

		switch (current_calibration_stage)
		{
		case NOT_CALIBRATING:
			if (!is_ack_deprecated && is_calibrating_hand_ack)
			{
				calibrating_tracker = t_id;
				calibration_time = chrono::steady_clock::now();
				current_calibration_stage++;
			}
			break;
		case REQUESTED:
			if (!is_calibrating_hand_ack)
			{
				reset_calibration_stage();
			}
			else
			{
				time_to_calibration = calibration_request_duration_ms - 
					chrono::duration_cast<chrono::milliseconds>(chrono::steady_clock::now() - calibration_time).count();
				cout << "Calibration in " << to_string(time_to_calibration / 1000) << "s..." << endl;
				if (time_to_calibration <= 0)
				{
					next_calibration_stage();
				}
			}
			break;
		case CHEST_CLICK:
			cout << "Tap index and thumb in front of chest" << endl;
			if (!is_ack_deprecated && is_calibrating_hand_ack)
			{
				user_position = position_from_pose(state.controller[t_id].pose);
				next_calibration_stage();
			}
			break;
		case GLOVES1:
			cout << "Move your hands as shown, fingers together, tap thumb and index when ready" << endl;
			for (auto loc : existing_hand_locs)
			{
				hands[loc]->calibrate();
			}
			if (!is_ack_deprecated && is_calibrating_hand_ack)
			{
				next_calibration_stage();
			}
			break;
		case GLOVES2:
			for (auto loc : existing_hand_locs)
			{
				hands[loc]->calibrate();
			}
			calibrate(state);
			time_to_calibration = hand_calibration_duration_ms -
				chrono::duration_cast<chrono::milliseconds>(chrono::steady_clock::now() - calibration_time).count();
			cout << "Calibration in " << to_string(time_to_calibration / 1000) << "s..." << endl;
			if (time_to_calibration <= 0)
			{
				next_calibration_stage();
			}
			break;
		case PANEL:
			calibrate(state);
			if (!is_ack_deprecated && is_calibrating_hand_ack)
			{
				reset_calibration_stage();
			}
		default:
			break;
		}

		is_ack_deprecated = is_ack_deprecated && is_calibrating_hand_ack;
	}

	void next_calibration_stage()
	{
		calibration_time = chrono::steady_clock::now();
		is_ack_deprecated = true;
		current_calibration_stage++;
	}

	void reset_calibration_stage()
	{
		is_ack_deprecated = true;
		current_calibration_stage = NOT_CALIBRATING;
	}

	void calibrate(const vr::vr_kit_state& state)
	{
		vec3 new_z(0);
		vec3 new_y(0, 1, 0);
		for (auto assign : tracker_assigns)
		{
			new_z += position_from_pose(state.controller[assign.first].pose);
			hands[assign.second]->calibrate();
		}
		new_z /= tracker_assigns.size();
		new_z -= user_position;
		new_z.y() = .0f;
		new_z.normalize();
		new_z = -new_z;
		vec3 new_x = cgv::math::cross(new_y, new_z);

		model_view_mat.identity();
		model_view_mat.set_col(0, hom_dir(new_x));
		model_view_mat.set_col(1, hom_dir(new_y));
		model_view_mat.set_col(2, hom_dir(new_z));
		model_view_mat.set_col(3, hom_pos(state.controller[calibrating_tracker].pose));
		vec4 center_translation = hom_pos(-panel_pos_on_bridge - hand_vs_panel_for_calibration);
		model_view_mat *= cgv::math::translate4(inhom_pos(center_translation));

		physical_to_world = cgv::math::inv(model_view_mat);
	}

	void draw(cgv::render::context& ctx)
	{
		ctx.push_modelview_matrix();
		ctx.mul_modelview_matrix(model_view_mat);

		//auto t0 = std::chrono::steady_clock::now();
		for (auto loc : existing_hand_locs)
		{
			hands[loc]->set_pose_and_actuators(panel, hand_translations[loc], hand_scale);
			hands[loc]->draw(ctx);
		}
		//auto t1 = std::chrono::steady_clock::now();

		if (is_load_mesh && !mri.is_constructed())
		{
			load_bridge_mesh(ctx);
		}

		if (is_render_mesh && mri.is_constructed())
		{
			//ctx.push_modelview_matrix();
			//ctx.mul_modelview_matrix(bridge_view_mat);
			//mri.render_mesh(ctx, ctx.ref_surface_shader_program(true));
			//ctx.pop_modelview_matrix();
		}

		panel.draw(ctx);
		//auto t2 = std::chrono::steady_clock::now();
		//cout << "hand: " << std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0).count() << endl;
		//cout << "mesh: " << std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1).count() << endl << endl;
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

			vec3 position = inhom_pos(physical_to_world * hom_pos(vrpe.get_position()));
			if (tracker_assigns.size() == num_hands && tracker_assigns.count(t_id))
			{
				int tracker_loc = tracker_assigns[t_id];
				hand_translations[tracker_loc] = position;
				update_calibration(vrpe.get_state(), t_id);
			}
			else
			{
				reset_calibration_stage();
				assign_trackers(t_id, position);
			}
			
			return true;
		}

		return false;
	}

	void assign_trackers(int id, vec3 position)
	{
		cout << "Assigning trackers" << endl;
		if (tracker_assigns.size() == existing_hand_locs.size())
		{
			cout << "Resetting tracker assignments" << endl;
			tracker_assigns = map<int, int>();
		}

		if (tracker_assigns.empty())
		{
			int loc = existing_hand_locs[0];
			tracker_assigns[id] = loc;
			hand_translations[loc] = position;
		}
		else
		{
			int other_id = tracker_assigns.begin()->first;
			vec3 other_position = hand_translations[tracker_assigns[other_id]];
			if (other_position.x() < position.x())
			{
				tracker_assigns[id] = NDAPISpace::LOC_RIGHT_HAND;
				tracker_assigns[other_id] = NDAPISpace::LOC_LEFT_HAND;
			}
			else
			{
				tracker_assigns[id] = NDAPISpace::LOC_LEFT_HAND;
				tracker_assigns[other_id] = NDAPISpace::LOC_RIGHT_HAND;
			}
		}
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
		add_member_control(this, "load mesh", is_load_mesh);
		add_member_control(this, "render mesh", is_render_mesh);
		add_member_control(this, "trackers assigned", trackers_assigned);

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
	}
};

cgv::base::object_registration<vr_ctrl_panel> vr_ctrl_panel_reg("");