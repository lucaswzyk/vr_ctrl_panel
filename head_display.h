#pragma once

#include <cgv/render/drawable.h>
#include <cgv/render/shader_program.h>
#include <cgv/render/frame_buffer.h>

#include <string>

using namespace std;

class head_display
	: public cgv::render::drawable
{
protected:
	string text;
	unsigned int text_size;
	ivec2 pos, fb_res;

	cgv::media::font::font_face_ptr ff;
	cgv::render::frame_buffer fb;
	cgv::render::texture texture;
	cgv::media::color<float> bgcolor;

	mat4 model_view_mat;
	bool is_visible;

public:
	head_display()
		: text(""), is_visible(false)
	{}

	void set_text(string a_text) { text = a_text; }
	void set_visible(bool is_vis = true) { is_visible = is_vis; }
	void set_pose(mat4 a_mat) { model_view_mat = a_mat; }

	bool init(cgv::render::context& ctx)
	{
		bool success = true;

		texture.set_width(fb_res.x());
		texture.set_height(fb_res.y());
		success = texture.create(ctx) && success;

		success = fb.create(ctx, fb_res.x(), fb_res.y()) && success;
		success = fb.attach(ctx, texture) && success;
		success = fb.is_complete(ctx) && success;

		cgv::render::shader_program& default_sh = ctx.ref_default_shader_program(true);
		init_unit_square();
		cgv::render::type_descriptor vec2type = cgv::render::element_descriptor_traits<cgv::render::render_types::vec2>::get_type_descriptor()
	}

	void draw(cgv::render::context& ctx)
	{
		ctx.push_modelview_matrix();
		ctx.mul_modelview_matrix(model_view_mat);

		ctx.ref_default_shader_program().enable(ctx);
		ctx.output_stream() << text;
		ctx.output_stream().flush();
		ctx.ref_default_shader_program().disable(ctx);

		ctx.pop_modelview_matrix();
	}
};