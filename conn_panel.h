#pragma once

#include <cgv/gui/provider.h>
#include <cgv/base/base.h>
#include <cgv/render/drawable.h>
#include <cgv_gl/box_renderer.h>
#include <cgv_gl/gl/gl.h>

#include "panel_element.h"
#include "space.h"

using namespace std;

vec3 const panel_pos_on_bridge = vec3(-.005f, .885f, -3.627f);

class conn_panel
	: public cgv::render::drawable
{
protected:
	panel_node* panel_tree;
	space* controlled_space;

public:

	conn_panel()
	{
		controlled_space = new space(10.0f, 1000.0f);
		panel_tree = new panel_node();

		vec3 left_ext(-.48, .0f, .3);
		vec3 left_rot(24.8, 6.3f, .15f);
		panel_node* left_panel = new panel_node(
			panel_pos_on_bridge, left_ext, .5f * left_ext,
			left_rot, rgb(0), 
			panel_tree
		);
		pos_neg_slider* yaw_slider = new pos_neg_slider(
			vec3(-.1f, .0f, -.05f), vec3(.05f, 0, .15f), vec3(0),
			vec3(.0f, 90.0f, .0f), rgb(.0f, .06f, .93f), rgb(1.0f, .6f, .0f),
			controlled_space, space::set_speed_yaw,
			left_panel
		);
		pos_neg_slider* pitch_slider = new pos_neg_slider(
			vec3(.1f, .0f, .0f), vec3(.05f, 0, .15f), vec3(0),
			vec3(.0f, 0.0f, .0f), rgb(.0f, .06f, .93f), rgb(1.0f, .6f, .0f),
			controlled_space, space::set_speed_pitch,
			left_panel
		);
		pos_neg_slider* roll_slider = new pos_neg_slider(
			vec3(-.1f, .0f, .05f), vec3(.05f, 0, .15f), vec3(0),
			vec3(.0f, -90.0f, .0f), rgb(.0f, .06f, .93f), rgb(1.0f, .6f, .0f),
			controlled_space, space::set_speed_roll,
			left_panel
		);

		vec3 right_ext(-left_ext.x(), left_ext.y(), left_ext.z());
		vec3 right_rot(left_rot.x(), -left_rot.y(), -left_rot.z());
		panel_node* right_panel = new panel_node(
			panel_pos_on_bridge, right_ext, .5f * right_ext,
			right_rot, rgb(0),
			panel_tree
		);
		lever* right_lever = new lever(
			vec3(0), vec3(.1f, .1f, .01f), vec3(0),
			vec3(60.0f, .0f, .0f), rgb(.8f, .87f, 1.0f),
			controlled_space, space::set_speed_ahead, 
			right_panel);
		button* toggle_targets_button = new button(
			vec3(.1f, .0f, .0f), vec3(.05f, .0f, .05f), vec3(0),
			vec3(0), rgb(.0f, .06f, .93f), rgb(.0f, 1.0f, .0f),
			controlled_space, space::toggle_targets,
			right_panel
		);
		button* fire_button = new hold_button(
			vec3(-.1f, .0f, .0f), vec3(.05f, .0f, .05f), vec3(0),
			vec3(0), rgb(1.0f, .6f, .0f), rgb(.53f, .13f, .07f),
			controlled_space, space::static_fire,
			right_panel
		);
	}
	
	void draw(cgv::render::context& ctx)
	{
		group_geometry gg = panel_tree->get_geometry_rec();
		cgv::render::box_renderer& br = cgv::render::ref_box_renderer(ctx);
		br.set_position_is_center(true);
		br.set_position_array(ctx, gg.positions);
		br.set_extent_array(ctx, gg.extents);
		br.set_rotation_array(ctx, gg.rotations);
		br.set_translation_array(ctx, gg.translations);
		br.set_color_array(ctx, gg.colors);
		br.validate_and_enable(ctx);
		glDrawArrays(GL_POINTS, 0, gg.positions.size());
		br.disable(ctx);

		controlled_space->draw(ctx);
	}

	std::map<int, float> check_containments(containment_info ci, int hand_loc) const
	{
		return panel_tree->check_containments(ci, hand_loc);
	}
};