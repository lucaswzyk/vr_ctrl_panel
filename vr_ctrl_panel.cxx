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

	vector<hand> hands;
	vector<short> tracker_ids;
	bool trackers_assigned;
	nd_handler::mode app_mode;
	vector<vec3> hand_translations;
	float hand_scale = .01f;

	cgv::render::mesh_render_info mri;
	bool is_load_mesh, is_render_mesh;

	conn_panel panel;

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
			tracker_ids = vector<short>(hands.size(), -1);
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
			hands.push_back(hand(NDAPISpace::LOC_LEFT_HAND));
			hands.push_back(hand(NDAPISpace::LOC_RIGHT_HAND));
			break;
		case nd_handler::LEFT_ONLY:
			hands.push_back(hand(NDAPISpace::LOC_LEFT_HAND));
			break;
		case nd_handler::RIGHT_ONLY:
			hands.push_back(hand(NDAPISpace::LOC_RIGHT_HAND));
			break;
		default:
			break;
		}
		hand_translations = vector<vec3>(hands.size(), vec3(0));
		tracker_ids = vector<short>(hands.size(), -1);
		model_view_mat.identity();
		physical_to_world.identity();

		return res;
	}

	void draw(cgv::render::context& ctx)
	{
		ctx.push_modelview_matrix();
		ctx.mul_modelview_matrix(model_view_mat);

		//auto t0 = std::chrono::steady_clock::now();
		for (size_t i = 0; i < hands.size(); i++)
		{
			hands[i].set_pose_and_actuators(panel, hand_translations[i], hand_scale);
			hands[i].draw(ctx);
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

			vec4 hom_translation = physical_to_world * cgv::math::hom(vrpe.get_position());
			vec3 translation = vec3(hom_translation.x(), hom_translation.y(), hom_translation.z());

			if (trackers_assigned)
			{
				for (size_t i = 0; i < hands.size(); i++)
				{
					if (tracker_ids[i] == t_id)
					{
						hand_translations[i] = translation;
					}
				}
			}
			else
			{
				try_to_assign_tracker(t_id, translation);
			}
			
			return true;
		}

		return false;
	}

	void try_to_assign_tracker(short t_id, vec3 translation)
	{
		trackers_assigned = true;
		for (size_t i = 0; i < hands.size(); i++)
		{
			if (translation.x() >= 0 && hands[i].get_location() == NDAPISpace::LOC_LEFT_HAND
				|| translation.x() < 0 && hands[i].get_location() == NDAPISpace::LOC_RIGHT_HAND)
			{
				tracker_ids[i] = t_id;
				hand_translations[i] = translation;
			}
			trackers_assigned &= tracker_ids[i] > 0;
		}

		update_member(&trackers_assigned);
	}

	virtual void stream_help(std::ostream& os) override
	{
	}

	// Inherited via provider
	virtual void create_gui() override
	{
		add_member_control(this, "load mesh", is_load_mesh);
		add_member_control(this, "render mesh", is_render_mesh);
		add_member_control(this, "trackers assigned", trackers_assigned);

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