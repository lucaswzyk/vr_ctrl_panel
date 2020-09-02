#pragma once

#include <cgv/base/node.h>
#include <cgv/render/context.h>
#include <cgv/render/drawable.h>
#include <cgv/render/shader_program.h>
#include <cgv_gl/rectangle_renderer.h>
#include <cg_nui/label_manager.h>
#include <cg_vr/vr_events.h>
#include <sstream>
#include <iomanip>
#include <string>

using namespace std;

class headup_display
	: public cgv::render::drawable
{
protected:
	cgv::nui::label_manager lm;

	vector<vec3> position;
	vector<quat> orientation;
	vector<vec2> extents;
	vector<vec4> texture_ranges;
	float scale;
	bool is_visible;

	plane_render_style prs;

	void construct()
	{
		cgv::media::font::font_ptr f = cgv::media::font::find_font("Arial");
		lm.set_font_face(f->get_font_face(cgv::media::font::FFA_REGULAR));
		lm.set_font_size(36);
		lm.set_text_color(rgba(0, 0, 0, 1));
		lm.add_label("init", rgba(1, 1, 1, .1f), 4, 4, 800, 90);

		position.push_back(vec3(0));
		orientation.push_back(quat(vec3(0, 1, 0), 0));

		lm.pack_labels();
		const auto& l = lm.get_label(0);
		extents.push_back(scale * vec2(l.get_width(), l.get_height()));
		texture_ranges.push_back(lm.get_texcoord_range(0));
	}

	void set_pose(vec3 pos, mat3 ori)
	{
		vec3 vs_hmd = ori * vec3(.0f, .1f, -.6f);
		position[0] = pos + vs_hmd;
		orientation[0] = ori;
	}

	void set_visible(bool a_visible = true) { is_visible = a_visible; }

public:
	headup_display()
		: scale(.001f), is_visible(false)
	{
		prs.illumination_mode = cgv::render::IM_OFF;
		construct();
	}

	void draw(context& ctx, vec3 p, mat3 ori)
	{
		if (!is_visible)
		{
			return;
		}

		set_pose(p, ori);

		lm.ensure_texture_uptodate(ctx);
		auto& rr = ref_rectangle_renderer(ctx);
		rr.set_position_array(ctx, position);
		rr.set_rotation_array(ctx, orientation);
		rr.set_extent_array(ctx, extents);
		rr.set_texcoord_array(ctx, texture_ranges);
		if (rr.validate_and_enable(ctx))
		{
			lm.get_texture()->enable(ctx);
			rr.draw(ctx, 0, lm.get_nr_labels());
			lm.get_texture()->disable(ctx);
			rr.disable(ctx);
		}
	}

	void set_text(string s) 
	{
		if (s.compare(""))
		{
			cout << "Headup display: " << s << endl;
			lm.update_label_text(0, s);
			set_visible();
		}
		else
		{
			set_visible(false);
		}
	}
};