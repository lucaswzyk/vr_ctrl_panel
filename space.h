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
	float r_out, r_in;
	
	// positions of stars and target (last max_num_targets indices)
	vec3* positions;
	float* radii;
	rgb* colors;

	mt19937 gen;
	uniform_real_distribution<float> dis_angles;
	normal_distribution<float> dis_radii;

	float speed_ahead, speed_pitch, speed_yaw, speed_roll;

	const size_t num_stars = 100, max_num_targets = 3;
	const float max_speed_ahead = .1f,
		max_angular_speed = .02f,
		pi_half = acos(.0f),
		star_rad_mean = .05f, star_rad_deviation = .01f,
		target_radius = 50.0f;
	const rgb star_color = rgb(1.0f, 1.0f, 1.0f),
			  target_color = rgb(1.0f, .0f, .0f);

	int num_targets;
	bool is_firing;
	const vec3 phaser_loc = vec3(2.5f, .0f, -6.0f);
	vector<vec3> phaser_positions, phaser_directions;
	const vector<GLuint> phaser_indices = { 0, 1, 2, 3 };
	const vector<float> phaser_radii = { .01f, .0f, .01f, .0f };

	chrono::steady_clock::time_point last_update;

	mat4 model_view_mat;
	vec3 origin;

	sphere_render_style srs;
	rounded_cone_render_style rcrs;

	void update_positions()
	{
		// to ensure realistic movement independent of frame rate
		chrono::steady_clock::time_point now = chrono::steady_clock::now();
		float ms_elapsed = chrono::duration_cast<chrono::milliseconds>(now - last_update).count();
		
		float distance_elapsed = speed_ahead * ms_elapsed;
		vec3 angles = vec3(speed_roll, speed_pitch, speed_yaw);
		mat3 rotation = cgv::math::rotate3(ms_elapsed * angles),
			 inv_rotation = cgv::math::rotate3(-r_out / 2 * angles);

		if (distance_elapsed > 100.0f)
		{
			cout << "Rendering too slow. Not updating" << endl;
			last_update = now;
			return;
		}

		// TODO inner radius (in latex too)
		// TODO dynamic radii (distance)
		vec3 p;
		for (size_t i = 0; i < num_stars + num_targets; i++)
		{
			if (i == num_stars)
			{
				distance_elapsed /= 3.0f;
			}
			// the following lines model dead ahead movement
			p = positions[i];
			float l = p.length();
			p.normalize();
			// now, p.z() is the cosine of the angle between
			// p and the pos. z-axis
			p *= l + p.z() * distance_elapsed;
			// translate p to model space
			p += origin;

			// check if p is outside the shell
			if (p.length() > r_out)
			{
				// get random angles from uniform distribution
				float alpha = dis_angles(gen), beta = dis_angles(gen);
				// init new position on spawn sphere
				p = r_out / 10 * vec3(
					sin(alpha) * cos(beta),
					sin(beta),
					cos(alpha) * cos(beta)
				) + origin;
				positions[i] = inv_rotation * p - origin;
			}
			else if (p.length() - radii[i] < r_in)
			{
				positions[i] = -p - origin;
			}
			else
			{
				positions[i] = rotation * p - origin;
			}
		}

		for (size_t i = 0; i < num_targets; i++)
		{
			float dist_frac = 1.0f - (positions[num_stars + i] + origin).length() / r_out;
			dist_frac = sqrt(dist_frac);
			radii[num_stars + i] = dist_frac * target_radius;
		}

		last_update = now;
	}

	void fire() {
		is_firing = true;

		for (size_t i = num_stars; i < num_stars + num_targets; i++)
		{
			vec3 t_pos = positions[i];
			float a = cgv::math::dot(t_pos, phaser_directions[0]);
			vec3 t_center_ray0 = t_pos - cgv::math::dot(t_pos, phaser_directions[0]) * phaser_directions[0],
				 t_center_ray1 = t_pos - cgv::math::dot(t_pos, phaser_directions[1]) * phaser_directions[1];
			if (t_center_ray0.length() < radii[i] || t_center_ray1.length() < radii[i])
			{
				for (size_t j = i + 1; j < num_stars + num_targets; j++)
				{
					positions[j - 1] = positions[j];
				}
				num_targets--;
			}
		}
	}

	void init()
	{
		model_view_mat.identity();
		model_view_mat *= cgv::math::translate4(origin);

		uniform_real_distribution<float> dis_distances = uniform_real_distribution<float>(r_in, r_out);
		for (size_t i = 0; i < num_stars + max_num_targets; i++)
		{
			float alpha = 2 * dis_angles(gen), beta = dis_angles(gen);
			positions[i] = vec3(
				sin(alpha) * cos(beta),
				sin(beta),
				cos(alpha) * cos(beta)
			);
			positions[i] *= dis_distances(gen);
			positions[i] -= origin;
			if (i < num_stars)
			{
				radii[i] = get_new_radius();
				colors[i] = rgb(1);
			}
			else
			{
				radii[i] = target_radius;
				colors[i] = target_color;
			}
		}

		num_targets = 0;
		phaser_positions = {
			vec3(-phaser_loc.x(), phaser_loc.y(), phaser_loc.z()) - origin,
			vec3(0),
			//vec3(-10.0f, 10.0f, .0f),
			vec3(phaser_loc.x(), phaser_loc.y(), phaser_loc.z()) - origin,
			//vec3(10.0f, 10.0f, .0f)
			vec3(0)
		};
		phaser_directions = { phaser_positions[1], phaser_positions[2] };
		phaser_directions[0].normalize();
		phaser_directions[1].normalize();

		rcrs.surface_color = rgb(.73f, .27f, .07f);

		last_update = chrono::steady_clock::now();
	}

public:
	space(float a_r_in, float a_r_out)
	{
		r_out = a_r_out;
		r_in = a_r_in;

		positions = new vec3[num_stars + max_num_targets];
		radii = new float[num_stars + max_num_targets];
		colors = new rgb[num_stars + max_num_targets];

		gen = mt19937(random_device()());
		dis_angles = uniform_real_distribution<float>(-pi_half, pi_half);
		dis_radii = normal_distribution<float>(star_rad_mean, star_rad_deviation);

		speed_ahead = 0;
		speed_pitch = 0;
		speed_yaw = 0;
		speed_roll = 0;

		origin = vec3(0, 0, -r_out);

		num_targets = 0;
		is_firing = false;

		init();
	}

	void set_speed_ahead(float new_speed) { speed_ahead = new_speed; }
	float get_max_speed_ahead() { return max_speed_ahead; }

	void draw(context& ctx)
	{
		update_positions();

		ctx.push_modelview_matrix();
		ctx.mul_modelview_matrix(model_view_mat);

		sphere_renderer& sr = ref_sphere_renderer(ctx);
		sr.set_position_array(ctx, positions, num_stars + num_targets);
		sr.set_radius_array(ctx, radii, num_stars + num_targets);
		sr.set_color_array(ctx, colors, num_stars + num_targets);
		sr.set_render_style(srs);
		sr.render(ctx, 0, num_stars + num_targets);

		if (is_firing)
		{
			rounded_cone_renderer& rcr = ref_rounded_cone_renderer(ctx);
			rcr.set_position_array(ctx, phaser_positions);
			rcr.set_indices(ctx, phaser_indices);
			rcr.set_radius_array(ctx, phaser_radii);
			rcr.set_render_style(rcrs);
			rcr.render(ctx, 0, phaser_indices.size());
			is_firing = false;
		}

		ctx.pop_modelview_matrix();
	}

	float get_new_radius() {
		float result = 0;
		while (result <= 0)
		{
			result = dis_radii(gen);
		}

		return result;
	}
	
	static void set_speed_ahead(space* s, float val) { s->speed_ahead = val * s->max_speed_ahead; }
	static void set_speed_pitch(space* s, float val) { s->speed_pitch = val * s->max_angular_speed; }
	static void set_speed_yaw(space* s, float val) { s->speed_yaw = val * s->max_angular_speed; }
	static void set_speed_roll(space* s, float val) { s->speed_roll = val * s->max_angular_speed; }

	static void add_target(space* s) {
		if (s->num_targets < s->max_num_targets)
		{
			s->num_targets++;
		}
	}
	static void static_fire(space* s) { s->fire(); }
};

