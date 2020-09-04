#pragma once

#include <random>
#include <chrono>

#include <cgv/render/drawable.h>
#include <cgv_gl/sphere_renderer.h>
#include <cgv_gl/rounded_cone_renderer.h>
#include <cgv_gl/gl/gl.h>
#include <cgv/math/ftransform.h>

#include "math_conversion.h"

using namespace std;

class space
	: public cgv::render::drawable
{
	// shell geometry
	float r_out, r_in;
	const size_t num_stars = 100, max_num_targets = 5;
	const float max_speed_ahead = .1f,
		max_angular_speed = .01f,
		star_rad_mean = .05f, star_rad_deviation = .01f,
		target_radius = 50.0f, target_speed_ratio = .4f,
		spawn_ratio_stars = .2f, spawn_ratio_targets = .001f;
	const rgb star_color = rgb(1.0f, 1.0f, 1.0f),
			  target_color = rgb(.0f, 1.0f, .0f);
	
	// positions of stars and targets (last max_num_targets indices)
	vec3* positions;
	float* radii;
	rgb* colors;
	int num_targets;

	// for updating 
	float speed_ahead, speed_pitch, speed_yaw, speed_roll;
	chrono::steady_clock::time_point last_update;
	mt19937 gen;
	uniform_real_distribution<float> dis_angles;
	normal_distribution<float> dis_radii;

	// phasers
	bool is_phaser_firing;
	const vec3 phaser_loc = vec3(2.5f, .0f, -6.0f);
	vector<vec3> phaser_positions, phaser_directions;
	const vector<GLuint> phaser_indices = { 0, 1, 2, 3 };
	const vector<float> phaser_radii = { .01f, .0f, .01f, .0f };

	// rendering
	mat4 model_view_mat;
	vec3 origin;
	sphere_render_style srs;
	rounded_cone_render_style rcrs;

	void update();

	// if a target has been hit, it is mirrored at the midpoint of the shell
	void fire();

	void init();

public:
	space(float a_r_in, float a_r_out);

	void draw(context& ctx);
	
	static void set_speed_ahead(space* s, float val) { s->speed_ahead = val * s->max_speed_ahead; }
	static void set_speed_pitch(space* s, float val) { s->speed_pitch = val * s->max_angular_speed; }
	static void set_speed_yaw(space* s, float val) { s->speed_yaw = val * s->max_angular_speed; }
	static void set_speed_roll(space* s, float val) { s->speed_roll = val * s->max_angular_speed; }

	static void toggle_targets(space* s) {
		if (s->num_targets == 0)
		{
			s->num_targets = s->max_num_targets;
		}
		else
		{
			s->num_targets = 0;
		}
	}

	static void static_fire(space* s) { s->fire(); }

	float get_new_radius();
};

