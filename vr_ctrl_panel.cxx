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

class vr_ctrl_panel
	: public cgv::base::base,
	  public cgv::render::drawable,
	  public cgv::gui::event_handler,
	  public cgv::gui::provider
{
protected:
	vec3 origin;
	float angle_y;
	mat4 model_view_mat, physical_to_world;

	vector<hand*> hands;
	int num_hands;
	map<int, int> tracker_assigns;

	// TODO combine these
	vector<vec3> hand_translations, raw_positions;
	bool trackers_assigned;
	nd_handler::mode app_mode;
	float hand_scale = .01f;

	cgv::render::mesh_render_info mri;
	bool is_load_mesh, is_render_mesh;

	conn_panel panel;

	bool calibration_requested, is_calibrating;
	chrono::steady_clock::time_point calibration_request_time, calibration_start;
	int calibration_request_duration_ms, calibration_duration_ms;

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
			&& rh.reflect_member("is_render_mesh", is_render_mesh)
			&& rh.reflect_member("origin_x", origin.x())
			&& rh.reflect_member("origin_y", origin.y())
			&& rh.reflect_member("origin_z", origin.z())
			&& rh.reflect_member("angle_y", angle_y);
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

		if (member_ptr == &is_calibrating)
		{
			calibration_start = chrono::steady_clock::now();
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
			hands.push_back(new hand(NDAPISpace::LOC_LEFT_HAND));
			num_hands = 2;
			break;
		case nd_handler::LEFT_ONLY:
			hands.push_back(nullptr);
			hands.push_back(new hand(NDAPISpace::LOC_LEFT_HAND));
			num_hands = 1;
			break;
		case nd_handler::RIGHT_ONLY:
			hands.push_back(new hand(NDAPISpace::LOC_RIGHT_HAND));
			hands.push_back(nullptr);
			num_hands = 1;
			break;
		default:
			num_hands = 0;
			break;
		}
		hand_translations = vector<vec3>(hands.size(), vec3(0));
		raw_positions = vector<vec3>(hands.size(), vec3(0));
		model_view_mat.identity();
		physical_to_world.identity();

		calibration_duration_ms = 100000;

		return res;
	}

	void update_calibration()
	{
		bool all_thumb_index_joined = true;
		for (size_t i = 0; i < hands.size(); i++)
		{
			if (!hands[i])
			{
				all_thumb_index_joined &= hands[i]->are_thumb_index_joined();
			}
		}

		if (calibration_requested)
		{
			int since_request_ms = chrono::duration_cast<chrono::milliseconds>(chrono::steady_clock::now() - calibration_request_time).count();
			if (calibration_request_duration_ms < since_request_ms)
			{
				cout << "Calibration" << endl;
			}
		}
		else if (all_thumb_index_joined)
		{
			calibration_request_time = chrono::steady_clock::now();
		}
	}

	void draw(cgv::render::context& ctx)
	{
		ctx.push_modelview_matrix();
		ctx.mul_modelview_matrix(model_view_mat);

		//auto t0 = std::chrono::steady_clock::now();
		for (size_t i = 0; i < hands.size(); i++)
		{
			if (hands[i])
			{
				hands[i]->set_pose_and_actuators(panel, hand_translations[i], hand_scale);
				hands[i]->draw(ctx);
			}
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

			vec3 position = inhom(physical_to_world * hom(vrpe.get_position()));
			if (tracker_assigns.size() == num_hands && tracker_assigns.count(t_id))
			{
				int tracker_loc = tracker_assigns[t_id];
				hand_translations[tracker_loc] = position;
				raw_positions[tracker_loc] = vrpe.get_position();

				if (is_calibrating)
				{
					set_physical_to_world();
					int since_calibration_start = chrono::duration_cast<chrono::milliseconds>(chrono::steady_clock::now() - calibration_start).count();
					is_calibrating = since_calibration_start < calibration_duration_ms;
					update_member(&is_calibrating);
				}
			}
			else
			{
				assign_trackers(t_id, position);
			}
			
			return true;
		}

		return false;
	}

	void assign_trackers(int id, vec3 position)
	{
		if (tracker_assigns.empty())
		{
			int loc = hands[0] ? NDAPISpace::LOC_RIGHT_HAND : NDAPISpace::LOC_LEFT_HAND;
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

	void set_physical_to_world()
	{
		model_view_mat.identity();
		model_view_mat *= cgv::math::translate4(.5f * (raw_positions[0] + raw_positions[1]));
		vec3 diff = raw_positions[0] - raw_positions[1];
		float angle = cgv::math::dot(diff, vec3(1, 0, 0)) / diff.length();
		angle = acos(angle) * 45 / atan(1.0f);
		if (cgv::math::cross(vec3(1, 0, 0), diff).y() < 0)
		{
			angle = -angle;
		}
		model_view_mat *= cgv::math::rotate4(angle, vec3(0, 1, 0));
		model_view_mat *= cgv::math::translate4(-vec3(.005f, .875f, 3.63f));
	}

	vec3 position_from_pose(const float pose[12])
	{
		return vec3(pose[9], pose[10], pose[12]);
	}
	
	vec4 hom(vec3 v) { return vec4(v.x(), v.y(), v.z(), 1.0f); }
	vec3 inhom(vec4 v) { return vec3(v.x(), v.y(), v.z()) / v.w(); }

	virtual void stream_help(std::ostream& os) override
	{
	}

	// Inherited via provider
	virtual void create_gui() override
	{
		add_member_control(this, "load mesh", is_load_mesh);
		add_member_control(this, "render mesh", is_render_mesh);
		add_member_control(this, "trackers assigned", trackers_assigned);
		add_member_control(this, "calibrate", is_calibrating);

		add_member_control(this, "origin x", origin.x());
		add_member_control(
			this, "origin x", origin.x(),
			"slider", "min=-2.0f;max=2.0f"
		);
		add_member_control(this, "origin y", origin.y());
		add_member_control(
			this, "origin y", origin.y(),
			"slider", "min=-2.0f;max=2.0f"
		);
		add_member_control(this, "origin z", origin.z());
		add_member_control(
			this, "origin z", origin.z(),
			"slider", "min=-2.0f;max=2.0f"
		);
		add_member_control(this, "angle y", angle_y);
		add_member_control(
			this, "angle y", angle_y,
			"slider", "min=-180.0f;max=180.0f"
		);
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