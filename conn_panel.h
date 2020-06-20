#pragma once

#include <cgv/gui/provider.h>
#include <cgv/base/base.h>
#include <cgv/render/drawable.h>
#include <cgv_gl/box_renderer.h>
#include <cgv_gl/gl/gl.h>

using namespace std;

class conn_panel
	: public cgv::base::base,
	  public cgv::render::drawable,
	  public cgv::gui::provider
	
{
protected:
	float width, height, BOX_THICKNESS = .01f;
	float angle_x, angle_y;
	float ty, tz;

	quat quat_y, quat_x;

	vector<vec3> positions, extents, translations;
	vector<quat> rotations;
	vector<rgb> colors;

public:
	conn_panel()
		: width(0), height(0), angle_x(0), angle_y(0), ty(0), tz(0), colors{ rgb(1, 0, 0), rgb(1, 0, 0) }
	{}

	string get_type_name(void) const
	{
		return "conn_panel";
	}

	bool self_reflect(cgv::reflect::reflection_handler& rh)
	{
		return rh.reflect_member("width", width) &&
			rh.reflect_member("height", height) &&
			rh.reflect_member("angle_x", angle_x) &&
			rh.reflect_member("angle_y", angle_y) &&
			rh.reflect_member("ty", ty) &&
			rh.reflect_member("tz", tz);
	}

	void set_geometry()
	{
		float pi = 2 * acos(0.0);
		float rad_x = angle_x * 2 * pi / 360;
		float rad_y = angle_y * 2 * pi / 360;
		positions = vector<vec3>{ vec3(0, 0, 0), vec3(0, 0, 0) };
		extents = vector<vec3>{ vec3(-width, BOX_THICKNESS, height), vec3(width, BOX_THICKNESS, height) };
		quat_x = quat(cos(rad_x), sin(rad_x), .0f, .0f);
		quat_y = quat(cos(rad_y), .0f, sin(rad_y), .0f);
		rotations = vector<quat>{ quat_x * quat_y, quat_x * quat_y.inverse() };
		translations = vector<vec3>{ vec3(0, ty, tz), vec3(0, ty, tz) };
	}

	void draw(cgv::render::context& ctx)
	{
		set_geometry();
		cgv::render::box_renderer& br = cgv::render::ref_box_renderer(ctx);
		br.set_position_is_center(false);
		br.set_position_array(ctx, positions);
		br.set_extent_array(ctx, extents);
		br.set_rotation_array(ctx, rotations);
		br.set_translation_array(ctx, translations);
		br.set_color_array(ctx, colors);
		br.validate_and_enable(ctx);
		glDrawArrays(GL_POINTS, 0, extents.size());
		br.disable(ctx);
	}

	virtual void create_gui() override
	{
		add_member_control(
			this, "width", width, "value_slider",
			"min=.0f;max=1.0f;ticks=false"
		);
		add_member_control(
			this, "height", height, "value_slider",
			"min=.0f;max=1.0f;ticks=false"
		);
		add_member_control(
			this, "angle x", angle_x, "value_slider",
			"min=.0f;max=30.0f;ticks=false"
		);
		add_member_control(
			this, "angle y", angle_y, "value_slider",
			"min=.0f;max=30.0f;ticks=false"
		);
		add_member_control(
			this, "ty", ty, "value_slider",
			"min=-1.0f;max=.0f;ticks=false"
		);
		add_member_control(
			this, "tz", tz, "value_slider",
			"min=-1.0f;max=.0f;ticks=false"
		);
	}
};

cgv::base::object_registration<conn_panel> conn_panel_reg("");