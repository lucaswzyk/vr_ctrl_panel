#include "label_manager.h"
#include <rect_pack/rect_pack.h>
#include <cgv_gl/gl/gl.h>

namespace cgv {
	namespace nui {

label_manager::label_manager(cgv::media::font::font_face_ptr _font_face, float _font_size)
{
	text_color = rgba(0, 0, 0, 1);
	safety_extension = 12;
	font_face = _font_face;
	font_size = _font_size;
	tex_width = -1;
	tex_height = -1;
	std::vector<uint8_t> label_states;
	packing_outofdate = true;
	texture_outofdate = true;
	texture_content_outofdate = true;
	tex = std::make_shared<cgv::render::texture>("[R,G,B,A]");
	rrs.culling_mode = cgv::render::CM_OFF;
	rrs.illumination_mode = cgv::render::IM_OFF;
	rrs.map_color_to_material = cgv::render::CM_COLOR_AND_OPACITY;
}

void label_manager::set_font_face(cgv::media::font::font_face_ptr _font_face)
{
	font_face = _font_face;
	packing_outofdate = true;
	texture_outofdate = true;
}

void label_manager::set_font_size(float _font_size)
{
	font_size = _font_size;
	packing_outofdate = true;
	texture_outofdate = true;
}

/// set default text color active at begin of each label, defaults to opaque black
void label_manager::set_text_color(const rgba& clr)
{
	text_color = clr;
	texture_outofdate = true;
	for (auto& s : label_states)
		s |= LS_NEW_COLOR;
}


uint32_t label_manager::add_label(const std::string& _text,
	const rgba& bg_clr, int _border_x, int _border_y,
	int _width, int _height)
{
	label l;
	l.text = _text;
	l.background_color = bg_clr;
	l.border_x = _border_x;
	l.border_y = _border_y;
	l.width = _width;
	l.height = _height;
	labels.push_back(l);
	label_states.push_back(LS_NEW_SIZE + LS_NEW_TEXT);
	packing_outofdate = true;
	texture_outofdate = true;
	texture_content_outofdate = true;
	return (uint32_t)(labels.size() - 1);
}

void label_manager::compute_label_sizes()
{
	for (auto& l : labels) {
		if (l.width == -1) {
			int w = (int)ceil(font_face->measure_text_width(l.text, font_size));
			l.width = -(w + 2*l.border_x);
		}
		if (l.height == -1) {
			int h = (int)ceil(1.0f*font_size);
			l.height = -(2*l.border_y + h);
		}
	}
}

void label_manager::fix_label_size(uint32_t li)
{
	if (li >= labels.size())
		return;
	auto& l = labels[li];
	l.width = l.get_width();
	l.height = l.get_height();
}

void label_manager::pack_labels()
{
	std::vector<rect_pack::rectangle> rectangles;
	std::vector<rect_pack::rectangle_size> rectangle_sizes;
	rect_pack::rectangle_size S;
	for (const auto& l : labels) {
		S.width = std::abs(l.width) + 2*safety_extension;
		S.height = std::abs(l.height) + 2*safety_extension;
		rectangle_sizes.push_back(S);
	}
	unsigned width_out, height_out;
	rect_pack::pack_rectangles_iteratively(rectangle_sizes,
		width_out, height_out, rectangles, rect_pack::CS_LongerSideFirst, false, true, rect_pack::PS_Skyline);
	//rect_pack::save_rectangles_html("c:/temp/rect.html", width_out, height_out, rectangles);
	tex_width = width_out;
	tex_height = height_out;
	not_rotated_labels.clear();
	rotated_labels.clear();
	tex_ranges.clear();
	for (const auto& R : rectangles) {
		uint32_t i = uint32_t(tex_ranges.size());
		if (std::abs(labels[i].width) + 2 * safety_extension != R.width)
			rotated_labels.push_back(i);
		else
			not_rotated_labels.push_back(i);
		tex_ranges.push_back(ibox2(ivec2(R.x, R.y), ivec2(R.x + R.width, R.y + R.height)));
	}
	packing_outofdate = false;
}
label_manager::vec4 label_manager::get_texcoord_range(uint32_t label_index)
{
	const auto& R = tex_ranges[label_index];
	float dx = 0.0f;
	if (std::abs(labels[label_index].width) + 2 * safety_extension != R.get_extent()(0))
		dx = -3.0f;
	return vec4(
		(float)(R.get_min_pnt()(0)+safety_extension) / tex_width+dx,
		(float)(tex_height - (R.get_max_pnt()(1) - safety_extension)) / tex_height,
		(float)(R.get_max_pnt()(0) - safety_extension) / tex_width,
		(float)(tex_height - (R.get_min_pnt()(1) + safety_extension)) / tex_height
	);
}

void label_manager::update_label_text(uint32_t i, const std::string& new_text)
{
	labels[i].text = new_text;
	label_states[i] |= LS_NEW_TEXT;
	texture_outofdate = true;
	if (labels[i].width < 0 || labels[i].height < 0) {
		packing_outofdate = true;
		label_states[i] |= LS_NEW_SIZE;
	}
}

void label_manager::update_label_size(uint32_t i, int w, int h)
{
	labels[i].width = w;
	labels[i].height = h;
	label_states[i] |= LS_NEW_SIZE;
	packing_outofdate = true;
}

void label_manager::init(cgv::render::context& ctx)
{
	cgv::render::ref_rectangle_renderer(ctx, 1);
}

bool label_manager::ensure_tex_fbo_combi(cgv::render::context& ctx, cgv::render::texture& tex, cgv::render::frame_buffer& fbo, int width, int height)
{
	bool created = false;
	if (!(tex.is_created() && 
		tex.get_width() == width && tex.get_height() == height)) {
		if (tex.is_created()) {
			fbo.destruct(ctx);
			tex.destruct(ctx);
		}
		tex.create(ctx, cgv::render::TT_2D, width, height);
		created = true;
	}
	if (!fbo.is_created()) {
		fbo.create(ctx, width, height);
		fbo.attach(ctx, tex, 0);
	}
	if (!fbo.is_complete(ctx)) {
		std::cerr << "label_manager::draw_labels ... fbo not complete" << std::endl;
		abort();
	}
	return created;
}

void label_manager::draw_label_backgrounds(cgv::render::context& ctx, const std::vector<uint32_t>& indices, bool all, bool swap)
{
	auto& rr = cgv::render::ref_rectangle_renderer(ctx);
	rr.set_render_style(rrs);
	std::vector<vec4> positions;
	std::vector<vec2> extents;
	std::vector<rgba> colors;
	for (uint32_t i : indices) {
		if (!all && ((label_states[i] & (LS_NEW_TEXT | LS_NEW_COLOR)) == 0))
			continue;
		ibox2 tr = tex_ranges[i];
		if (swap) {
			std::swap(tr.ref_min_pnt()(0), tr.ref_min_pnt()(1));
			std::swap(tr.ref_max_pnt()(0), tr.ref_max_pnt()(1));
		}
		vec4 pos((float)tr.get_center()(0), (float)tr.get_center()(1), 0.0f, 1.0f);
		vec2 ext((float)tr.get_extent()(0), (float)tr.get_extent()(1));
		positions.push_back(pos);
		extents.push_back(ext);
		colors.push_back(labels[i].background_color);
	}
	rr.set_position_array(ctx, positions);
	rr.set_extent_array(ctx, extents);
	rr.set_color_array(ctx, colors);
	rr.render(ctx, 0, positions.size());
}

void label_manager::draw_label_texts(cgv::render::context& ctx, const std::vector<uint32_t>& indices, int height, bool all, bool swap)
{
	glColor4fv(&text_color[0]);
	glEnable(GL_SCISSOR_TEST);
	ctx.enable_font_face(font_face, font_size);
	for (uint32_t i : indices) {
		if (!all && ((label_states[i] & (LS_NEW_TEXT | LS_NEW_COLOR)) == 0))
			continue;
		ibox2 tr = tex_ranges[i];
		if (swap) {
			std::swap(tr.ref_min_pnt()(0), tr.ref_min_pnt()(1));
			std::swap(tr.ref_max_pnt()(0), tr.ref_max_pnt()(1));
		}
		ctx.set_cursor(
			tr.get_min_pnt()(0) + safety_extension + labels[i].border_x,
			tr.get_min_pnt()(1) + safety_extension + labels[i].border_y + (int)(0.75f * font_size));
		glScissor(
			tr.get_min_pnt()(0),
			height - tr.get_max_pnt()(1) - 1,
			tr.get_extent()(0),
			tr.get_extent()(1)
		);
		ctx.output_stream() << labels[i].text;
		ctx.output_stream().flush();
	}
	glDisable(GL_SCISSOR_TEST);
}

void label_manager::draw_labels(cgv::render::context& ctx, bool all)
{
	cgv::media::font::font_face_ptr old_font_face = ctx.get_current_font_face();
	float old_font_size = ctx.get_current_font_size();
	GLboolean is_depth, is_scissor;
	glGetBooleanv(GL_DEPTH_TEST, &is_depth);
	glGetBooleanv(GL_SCISSOR_TEST, &is_scissor);
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_SCISSOR_TEST);
	glClearColor(0.5f, 0.5f, 0.5f, 1);
	ctx.push_window_transformation_array();

	// first draw rotated labels
	if (!rotated_labels.empty()) {
		bool created = ensure_tex_fbo_combi(ctx, tmp_tex, tmp_fbo, tex_height, tex_width);
		tmp_fbo.enable(ctx, 0);
		ctx.set_viewport(ivec4(0, 0, tex_height, tex_width));
		ctx.push_pixel_coords();
		if (created || texture_outofdate)
			glClear(GL_COLOR_BUFFER_BIT);
		draw_label_backgrounds(ctx, rotated_labels, all, true);
		draw_label_texts(ctx, rotated_labels, tex_width, all, true);
		ctx.pop_pixel_coords();
		tmp_fbo.disable(ctx);
		//tmp_tex.write_to_file(ctx, "C:/temp/tmp_tex.bmp");
	}
	// then draw not rotated labels
	bool created = ensure_tex_fbo_combi(ctx, *tex, fbo, tex_width, tex_height);
	fbo.enable(ctx, 0);
	ctx.set_viewport(ivec4(0, 0, tex_width, tex_height));
	ctx.push_pixel_coords();
	glClearColor(0.9f, 0.5f, 0.5f, 1);
	if (created || texture_outofdate)
		glClear(GL_COLOR_BUFFER_BIT);
	draw_label_backgrounds(ctx, not_rotated_labels, all, false);
	draw_label_texts(ctx, not_rotated_labels, tex_height, all, false);
	// copy rotated labels with rectangle renderer to main texture
	if (!rotated_labels.empty()) {
		auto& rr = cgv::render::ref_rectangle_renderer(ctx);
		rr.set_render_style(rrs);
		std::vector<vec3> positions;
		std::vector<vec4> texcoords;
		std::vector<vec2> extents;
		std::vector<rgba> colors;
		std::vector<quat> rotations;
		quat q(0.5f * sqrt(2.0f), 0, 0, 0.5f * sqrt(2.0f));
		for (uint32_t i : rotated_labels) {
			if (!all && ((label_states[i] & (LS_NEW_TEXT | LS_NEW_COLOR)) == 0))
				continue;
			ibox2 tr = tex_ranges[i];
			vec3 pos((float)tr.get_center()(0), (float)tr.get_center()(1), 0.0f);
			vec2 ext((float)tr.get_extent()(1), (float)tr.get_extent()(0));
			vec4 tc(
				(float)tr.get_min_pnt()(0) / tex_width,
				(float)(tex_height - tr.get_min_pnt()(1)) / tex_height,
				(float)tr.get_max_pnt()(0) / tex_width,
				(float)(tex_height - tr.get_max_pnt()(1)) / tex_height);
			std::swap(tc[0], tc[1]);
			std::swap(tc[2], tc[3]);
			tc[0] = 1.0f - tc[0];
			tc[2] = 1.0f - tc[2];
			tc[1] = 1.0f - tc[1];
			tc[3] = 1.0f - tc[3];
			std::swap(tc[0], tc[2]);
			std::swap(tc[1], tc[3]);
			positions.push_back(pos);
			extents.push_back(ext);
			colors.push_back(labels[i].background_color);
			texcoords.push_back(tc);
			rotations.push_back(q);
		}
		rr.set_position_array(ctx, positions);
		rr.set_extent_array(ctx, extents);
		rr.set_color_array(ctx, colors);
		rr.set_texcoord_array(ctx, texcoords);
		rr.set_rotation_array(ctx, rotations);
		tmp_tex.enable(ctx);
		rr.render(ctx, 0, positions.size());
		tmp_tex.disable(ctx);
	}
	ctx.pop_pixel_coords();
	fbo.disable(ctx);
	//tex->write_to_file(ctx, "C:/temp/tex.bmp");

	ctx.pop_window_transformation_array();
	ctx.enable_font_face(old_font_face, old_font_size);
	if (!is_scissor)
		glDisable(GL_SCISSOR_TEST);
	if (is_depth)
		glEnable(GL_DEPTH_TEST);

	texture_outofdate = false;
	texture_content_outofdate = false;
}

void label_manager::destruct(cgv::render::context& ctx)
{
	tex->destruct(ctx);
	tex = 0;
	fbo.destruct(ctx);
	cgv::render::ref_rectangle_renderer(ctx, -1);
}

void label_manager::ensure_texture_uptodate(cgv::render::context& ctx)
{
	if (packing_outofdate) {
		compute_label_sizes();
		pack_labels();
	}
	if (texture_outofdate)
		draw_labels(ctx, packing_outofdate);
}

	}
}