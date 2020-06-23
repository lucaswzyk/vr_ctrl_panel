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
	  public cgv::render::drawable
{
protected:
	panel_node* panel_tree;

public:
	conn_panel()
	{
		panel_tree = new panel_node();
		panel_node* left_panel = new panel_node(
			vec3(0), vec3(-.5f, .0f, .3f), vec3(.0f, -.625f, -.93f),
			vec3(12.4f, 2.98f, 1.3f), rgb(1, 0, 0), 
			panel_tree
		);
		color_switch_button* left_button = new color_switch_button(
			vec3(-.25f, .0f, .1f), vec3(-.1f, .0f, .1f), vec3(0),
			vec3(0), rgb(1, 1, 0),
			left_panel
		);
		panel_node* right_panel = new panel_node(
			vec3(0), vec3(.5f, .0f, .3f), vec3(.0f, -.625f, -.93f),
			vec3(12.4f, -2.98f, -1.3f), rgb(0, 1, 0),
			panel_tree
		);
		color_switch_button* right_button = new color_switch_button(
			vec3(.25f, .0f, .1f), vec3(.1f, .0f, .1f), vec3(0),
			vec3(0), rgb(0, 1, 1),
			right_panel
		);
		slider* left_slider = new slider(
			vec3(-.1f, 0, .2f), vec3(.05f, 0, -.15f), vec3(0),
			vec3(0), rgb(0, 0, 1), rgb(1.0f, .65f, .0f),
			left_panel
		);
		//panel = new panel_surface(width, height, angle_x, angle_y, angle_z, ty, tz);
		//panel->add_child(new color_switch_button(.1f, .1f, .25f, .1f, rgb(0, 1, 1)));
		//panel->add_child(new button(.01f, .01f, .1f, .1f, rgb(1, 1, 0)));
		//panel = new button(.01f, .01f, .1f, .1f, rgb(0, 1, 1));
	}

	string get_type_name(void) const
	{
		return "conn_panel";
	}

	void draw(cgv::render::context& ctx)
	{
		vector<vec3> positions = panel_tree->get_positions_rec();// vector<vec3>(1, vec3(0));
		vector<vec3> extents = panel_tree->get_extents_rec();// vector<vec3>(1, vec3(1, 1, 1));
		vector<quat> rotations = panel_tree->get_rotations_rec();// vector<quat>(1, quat(1, 0, 0, 0));
		vector<vec3> translations = panel_tree->get_translations_rec();// vector<vec3>(1, vec3(0, 0, 0));
		vector<rgb> colors = panel_tree->get_colors_rec();// vector<rgb>(1, rgb(1, 0, 0));
		cgv::render::box_renderer& br = cgv::render::ref_box_renderer(ctx);
		br.set_position_is_center(false);
		br.set_position_array(ctx, positions);
		br.set_extent_array(ctx, extents);
		br.set_rotation_array(ctx, rotations);
		br.set_translation_array(ctx, translations);
		br.set_color_array(ctx, colors);
		br.validate_and_enable(ctx);
		glDrawArrays(GL_POINTS, 0, positions.size());
		br.disable(ctx);
	}

	std::set<int> check_containments(vector<vec3> vecs) const
	{
		return panel_tree->check_containments(vecs);
	}
};

//cgv::base::object_registration<conn_panel> conn_panel_reg("");