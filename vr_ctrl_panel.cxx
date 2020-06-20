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
	vector<hand> hands;
	nd_handler::mode app_mode;

	cgv::render::mesh_render_info mri;
	bool is_load_mesh, is_render_mesh;

	mat4 bridge_view_mat, hand_view_mat;
	vec3 hand_translation;

public:
	vr_ctrl_panel()
		: is_load_mesh(true), is_render_mesh(true)
	{
		bridge_view_mat.identity();
		hand_view_mat.identity();
	}

	string get_type_name(void) const
	{
		return "vr_ctrl_panel";
	}

	bool self_reflect(cgv::reflect::reflection_handler& rh)
	{
		return rh.reflect_member("hand_translation_x", hand_translation(0)) &&
			rh.reflect_member("hand_translation_y", hand_translation(1)) &&
			rh.reflect_member("hand_translation_z", hand_translation(2)) &&
			rh.reflect_member("is_load_mesh", is_load_mesh) &&
			rh.reflect_member("is_render_mesh", is_render_mesh);
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
		//hand_view_mat *= cgv::math::translate4(0.2f, -.68f, -.72f); 
		//hand_view_mat *= cgv::math::rotate4(180.0f, 0.0f, 1.0f, 0.0f);
		hand_view_mat *= cgv::math::scale4(.01f, .01f, .01f); 

		if (is_load_mesh)
		{
			load_bridge_mesh(ctx);
		}

		return res;
	}

	void draw(cgv::render::context& ctx)
	{
		auto t0 = std::chrono::steady_clock::now();
		ctx.push_modelview_matrix();
		ctx.mul_modelview_matrix(hand_view_mat);
		for each (hand h in hands)
		{
			h.draw(ctx);
		}
		ctx.pop_modelview_matrix();
		auto t1 = std::chrono::steady_clock::now();

		if (is_render_mesh && mri.is_constructed())
		{
			ctx.push_modelview_matrix();
			ctx.mul_modelview_matrix(bridge_view_mat);
			mri.render_mesh(ctx, ctx.ref_surface_shader_program(true));
			ctx.pop_modelview_matrix();
		}
		auto t2 = std::chrono::steady_clock::now();
		//cout << "hand: " << std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0).count() << endl;
		//cout << "mesh: " << std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1).count() << endl << endl;
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
			vec3 origin = vrpe.get_position();
			hand_view_mat(0, 3) = origin(0) + hand_translation(0);
			hand_view_mat(1, 3) = origin(1) + hand_translation(1);
			hand_view_mat(2, 3) = origin(2) + hand_translation(2);
			return true;
		}

		return false;
	}

	virtual void stream_help(std::ostream& os) override
	{
	}

	// Inherited via provider
	virtual void create_gui() override
	{
		add_control(
			"hand pos x", hand_view_mat(0, 3), "value_slider",
			"min=-1.0f;max=1.0f;ticks=false"
		).operator->();
		add_control(
			"hand pos y", hand_view_mat(1, 3), "value_slider",
			"min=-1.0f;max=1.0f;ticks=false"
		).operator->();
		add_control(
			"hand pos z", hand_view_mat(2, 3), "value_slider",
			"min=-1.0f;max=1.0f;ticks=false"
		).operator->();

		add_member_control(this, "render mesh", is_render_mesh);
	}

	void load_bridge_mesh(cgv::render::context& ctx, const char* file = "C:/Users/CC42D/src/vr_ctrl_panel/git/bridge.obj")
	{
		cgv::media::mesh::simple_mesh<> m;
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
		}
		bridge_view_mat.identity();
		bridge_view_mat *= cgv::math::rotate4(180.0f, 0.0f, 1.0f, 0.0f);
		bridge_view_mat *= cgv::math::translate4(0.0f, -1.5f, -2.7f);
	}
};

cgv::base::object_registration<vr_ctrl_panel> vr_ctrl_panel_reg("");