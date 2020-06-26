#pragma once

#include <random>
#include <chrono>

#include <cgv/render/drawable.h>
#include <cgv_gl/sphere_renderer.h>
#include <cgv_gl/gl/gl.h>
#include <cgv/math/ftransform.h>

using namespace std;

class stars_sphere
	: public cgv::render::drawable
{
	size_t num_stars;
	float r_out, r_in;
	
	vec3* directions;
	vec3* positions;
	float* distances;
	float* radii;
	rgb* colors;

	mt19937 gen;
	uniform_real_distribution<float> dis_alpha, dis_beta;
	normal_distribution<float> dis_radii;

	float speed, max_speed;
	quat rotation;

	chrono::steady_clock::time_point last_update;
	float ms_per_step;

	mat4 model_view_mat;

	void update()
	{
		chrono::steady_clock::time_point now = chrono::steady_clock::now();
		float steps_elapsed = chrono::duration_cast<chrono::milliseconds>(now - last_update).count();
		float distance_elapsed = speed * steps_elapsed;
		if (distance_elapsed > r_out - r_in)
		{
			cout << "Rendering too slow. Not updating" << endl;
			last_update = now;
			return;
		}

		for (size_t i = 0; i < num_stars; i++)
		{
			distances[i] += distance_elapsed;

			if (distances[i] > r_out)
			{
				float alpha = dis_alpha(gen), beta = dis_beta(gen);
				directions[i] = vec3(
					cos(alpha) * cos(beta),
					cos(alpha) * sin(beta),
					sin(alpha)
				);
				distances[i] -= r_out - r_in;
				radii[i] = max(.0f, dis_radii(gen));
			}
		}

		last_update = now;
	}

	void init_stars()
	{
		uniform_real_distribution<float> dis_distances = uniform_real_distribution<float>(r_in, r_out);
		for (size_t i = 0; i < num_stars; i++)
		{
			float alpha = dis_alpha(gen), beta = dis_beta(gen);
			directions[i] = vec3(
				cos(alpha) * cos(beta),
				cos(alpha) * sin(beta),
				sin(alpha)
			);
			distances[i] = dis_distances(gen);
			radii[i] = max(.0f, dis_radii(gen));
		}
		last_update = chrono::steady_clock::now();
	}

public:
	stars_sphere(float a_r_in, float a_r_out, vec3 origin, 
		int a_num_stars = 100, float a_speed = 0, float a_max_speed = .01f,
		float star_rad_mean = .01f, float star_rad_deviation = .005f)
	{
		r_out = a_r_out;
		r_in = a_r_in;

		num_stars = a_num_stars;
		directions = new vec3[num_stars];
		positions = new vec3[num_stars];
		distances = new float[num_stars];
		radii = new float[num_stars];
		colors = new rgb[num_stars]{ rgb(1, 1, 1) };

		float pi_half = acos(.0f);
		gen = mt19937(random_device()());
		dis_alpha = uniform_real_distribution<float>(-pi_half, .0f);
		dis_beta = uniform_real_distribution<float>(.0f, 4 * pi_half);
		dis_radii = normal_distribution<float>(star_rad_mean, star_rad_deviation);

		speed = a_speed;
		max_speed = a_max_speed;
		model_view_mat.identity();
		model_view_mat *= cgv::math::translate4(origin);

		init_stars();
	}

	void set_speed(float new_speed) { speed = new_speed; }
	float* get_speed_ptr() { return &speed; }
	float get_max_speed() { return max_speed; }
	void draw(cgv::render::context& ctx)
	{
		update();
		for (size_t i = 0; i < num_stars; i++)
		{
			positions[i] = directions[i] * distances[i];
		}

		ctx.push_modelview_matrix();
		ctx.mul_modelview_matrix(model_view_mat);
		cgv::render::sphere_renderer& sr = cgv::render::ref_sphere_renderer(ctx);
		sr.set_position_array(ctx, positions, num_stars);
		sr.set_radius_array(ctx, radii, num_stars);
		sr.set_color_array(ctx, colors, num_stars);
		sr.validate_and_enable(ctx);
		glDrawArrays(GL_POINTS, 0, num_stars);
		sr.disable(ctx);
		ctx.pop_modelview_matrix();
	}
};

