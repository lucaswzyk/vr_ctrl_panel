#pragma once

#include <cgv/gui/provider.h>
#include <cgv/base/base.h>
#include <cgv/render/drawable.h>
#include <cgv_gl/box_renderer.h>
#include <cgv_gl/gl/gl.h>

#include "panel_element.h"
#include "space.h"

using namespace std;

vec3 const panel_pos_on_bridge = vec3(-.005f, .88f, -3.627f);

class conn_panel
	: public cgv::base::base,
	  public cgv::render::drawable
{
protected:
	panel_node* panel_tree;
	stars_sphere* controlled_stars_sphere;

public:

	conn_panel()
	{
		controlled_stars_sphere = new stars_sphere(1.0f, 20.0f, vec3(.0f, .0f, -25.0f));
		panel_tree = new panel_node();

		vec3 left_ext(.48, .0f, -.3);
		vec3 left_rot(25, 6.3f, .2f);
		vec3 left_tr = panel_pos_on_bridge - .5f * cgv::math::rotate3(left_rot) * left_ext;
		panel_node* left_panel = new panel_node(
			vec3(0), left_ext, left_tr,
			left_rot, rgb(1, 0, 0), 
			panel_tree
		);
		color_switch_button* left_button = new color_switch_button(
			vec3(0), vec3(.05f, .0f, .05f), vec3(0),
			vec3(0), rgb(1, 1, 0),
			left_panel
		);
		vec3 right_ext(-left_ext.x(), left_ext.y(), left_ext.z());
		vec3 right_rot(left_rot.x(), -left_rot.y(), -left_rot.z());
		vec3 right_tr = panel_pos_on_bridge - .5f * cgv::math::rotate3(right_rot) * right_ext;
		panel_node* right_panel = new panel_node(
			vec3(0), right_ext, right_tr,
			right_rot, rgb(0, 1, 0),
			panel_tree
		);
		color_switch_button* right_button = new color_switch_button(
			vec3(0), vec3(.05f, .0f, .05f), vec3(0),
			vec3(0), rgb(0, 1, 1),
			right_panel
		);
		slider* left_slider = new slider(
			vec3(.1f, .0f, .0f), vec3(.05f, 0, .15f), vec3(0),
			vec3(0), rgb(0, 0, 1), rgb(1.0f, .65f, .0f),
			controlled_stars_sphere->get_speed_ptr(), controlled_stars_sphere->get_max_speed(),
			left_panel
		);
	}

	string get_type_name(void) const
	{
		return "conn_panel";
	}

	void draw(cgv::render::context& ctx)
	{
		vector<vec3> positions = panel_tree->get_positions_rec();
		vector<vec3> extents = panel_tree->get_extents_rec();
		vector<quat> rotations = panel_tree->get_rotations_rec();
		vector<vec3> translations = panel_tree->get_translations_rec();
		vector<rgb> colors = panel_tree->get_colors_rec();
		cgv::render::box_renderer& br = cgv::render::ref_box_renderer(ctx);
		br.set_position_is_center(true);
		br.set_position_array(ctx, positions);
		br.set_extent_array(ctx, extents);
		br.set_rotation_array(ctx, rotations);
		br.set_translation_array(ctx, translations);
		br.set_color_array(ctx, colors);
		br.validate_and_enable(ctx);
		glDrawArrays(GL_POINTS, 0, positions.size());
		br.disable(ctx);

		controlled_stars_sphere->draw(ctx);
	}

	std::set<int> check_containments(vector<vec3> vecs, float tolerance) const
	{
		return panel_tree->check_containments(vecs, tolerance);
	}
};