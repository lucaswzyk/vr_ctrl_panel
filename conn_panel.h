#pragma once

#include <cgv/gui/provider.h>
#include <cgv/base/base.h>
#include <cgv/render/drawable.h>
#include <cgv_gl/box_renderer.h>
#include <cgv_gl/gl/gl.h>

#include "panel_element.h"

using namespace std;

class conn_panel
	: public cgv::base::base,
	  public cgv::render::drawable,
	  public cgv::gui::provider
	
{
protected:
	float width, height;
	float angle_x, angle_y, angle_z;
	float ty, tz;

	panel_node* panel;

public:
	conn_panel()
		: width(0), height(0), 
		  angle_x(0), angle_y(0), angle_z(0), 
		  ty(0), tz(0)
	{}

	conn_panel(float width, float height,
		float angle_x, float angle_y, float angle_z,
		float ty, float tz)
		: width(width), height(height),
		  angle_x(angle_x), angle_y(angle_y), angle_z(angle_z),
		  ty(ty), tz(tz)
	{
		panel = new panel_surface(width, height, angle_x, angle_y, angle_z, ty, tz);
		panel->add_child(new button(.1f, .1f, .25f, .1f, rgb(0, 1, 1)));
		//panel->add_child(new button(.01f, .01f, .1f, .1f, rgb(1, 1, 0)));
		//panel = new button(.01f, .01f, .1f, .1f, rgb(0, 1, 1));
	}

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
			rh.reflect_member("angle_z", angle_z) &&
			rh.reflect_member("ty", ty) &&
			rh.reflect_member("tz", tz);
	}

	void draw(cgv::render::context& ctx)
	{
		vector<vec3> positions = panel->get_positions();// vector<vec3>(1, vec3(0));
		vector<vec3> extents = panel->get_extents();// vector<vec3>(1, vec3(1, 1, 1));
		vector<quat> rotations = panel->get_rotations();// vector<quat>(1, quat(1, 0, 0, 0));
		vector<vec3> translations = panel->get_translations();// vector<vec3>(1, vec3(0, 0, 0));
		vector<rgb> colors = panel->get_colors();// vector<rgb>(1, rgb(1, 0, 0));
		//vector<vec3> v = panel->get_positions();
		//v = panel->get_extents();
		//vector<quat> rot = panel->get_rotations();
		//v = panel->get_translations();
		cgv::render::box_renderer& br = cgv::render::ref_box_renderer(ctx);
		br.set_position_is_center(false);
		br.set_position_array(ctx, positions);
		br.set_extent_array(ctx, extents);
		br.set_rotation_array(ctx, rotations);
		br.set_translation_array(ctx, translations);
		br.set_color_array(ctx, colors);
		br.validate_and_enable(ctx);
		int size = panel->get_positions().size();
		glDrawArrays(GL_POINTS, 0, panel->get_positions().size());
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
			this, "angle z", angle_z, "value_slider",
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

	bool contains(vec3 v) const
	{
		return panel->contains(v);
	}
};

//cgv::base::object_registration<conn_panel> conn_panel_reg("");