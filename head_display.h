#pragma once

#include <cgv/base/node.h>
#include <cgv/render/context.h>
#include <cgv/render/drawable.h>
#include <cgv/render/shader_program.h>
#include <cgv_gl/rectangle_renderer.h>
#include <cg_nui/label_manager.h>
#include <sstream>
#include <iomanip>
#include <string>

using namespace std;

class head_display
	: public cgv::render::drawable
{
protected:
	cgv::nui::label_manager lm;

	vec3 position;
	quat orientation;
	vec2 extents;
	vec4 texture_ranges;
	float scale;
	bool is_visible;

	plane_render_style prs;

	void construct()
	{
		cgv::media::font::font_ptr f = cgv::media::font::find_font("Arial");
		lm.set_font_face(f->get_font_face(cgv::media::font::FFA_REGULAR));
		lm.set_font_size(20);
		lm.set_text_color(rgba(0, 0, 0, 1));
		lm.add_label("Hello", rgba(1, 0, 0, 1));
		lm.compute_label_sizes();
		lm.pack_labels();

		const auto& l = lm.get_label(0);
		position = vec3(0);
		orientation = quat(vec3(0, 1, 0), 0);
		extents = scale * vec2(l.get_width(), l.get_height());
		texture_ranges = lm.get_texcoord_range(0);
	}

public:
	head_display()
		: scale(1.0f), is_visible(false)
	{
		prs.illumination_mode = cgv::render::IM_OFF;
		construct();
	}

	void draw(context& ctx)
	{
		lm.ensure_texture_uptodate(ctx);
		auto& rr = ref_rectangle_renderer(ctx);
		rr.set_position_array(ctx, vector<vec3>{position});
		rr.set_rotation_array(ctx, vector<quat>{orientation});
		rr.set_extent_array(ctx, vector<vec2>{extents});
		rr.set_texcoord_array(ctx, vector<vec4>{texture_ranges});
		if (rr.validate_and_enable(ctx))
		{
			lm.get_texture()->enable(ctx);
			rr.draw(ctx, 0, lm.get_nr_labels());
			lm.get_texture()->disable(ctx);
			rr.disable(ctx);
		}
	}

	void set_visible(bool a_visible = true) { is_visible = a_visible; }
	void set_text(string bla) {}
	void set_pose(mat4 pose_mat){}
};