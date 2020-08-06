//#pragma once
//
//#include <random>
//#include <chrono>
//
//#include <cgv/render/drawable.h>
//#include <cgv_gl/sphere_renderer.h>
//#include <cgv_gl/gl/gl.h>
//#include <cgv/math/ftransform.h>
//
//#include "math_conversion.h"
//
//using namespace std;
//
//class stars_sphere
//	: public cgv::render::drawable
//{
//	float r_out, r_in;
//
//	vec3* positions;
//	float* radii;
//	rgb* colors;
//
//	mt19937 gen;
//	normal_distribution<float> dis_angle;
//	normal_distribution<float> dis_radii;
//
//	float speed_ahead, speed_pitch, speed_yaw, speed_roll;
//
//	const size_t num_stars = 100;
//	const float max_speed_ahead = .001f,
//		max_angular_speed = .1f,
//		pi_half = acos(.0f);
//
//	chrono::steady_clock::time_point last_update;
//	float ms_per_step;
//
//	mat4 model_view_mat;
//	vec3 origin;
//
//	void update()
//	{
//		chrono::steady_clock::time_point now = chrono::steady_clock::now();
//		float steps_elapsed = chrono::duration_cast<chrono::milliseconds>(now - last_update).count();
//
//		float distance_elapsed = speed_ahead * steps_elapsed;
//		mat3 rotation = cgv::math::rotate3(vec3(speed_pitch, speed_yaw, speed_roll));
//
//		if (distance_elapsed > r_out - r_in)
//		{
//			cout << "Rendering too slow. Not updating" << endl;
//			last_update = now;
//			return;
//		}
//
//		vec3 p;
//		for (size_t i = 0; i < num_stars; i++)
//		{
//			p = positions[i];
//			float new_length = p.length() + p.z() * distance_elapsed;
//			p -= origin;
//			float global_length = p.length();
//
//			if (global_length < r_in)
//			{
//				p = -p;
//			}
//			else if (global_length > r_out)
//			{
//				float alpha = get_new_angle(), beta = get_new_angle();
//				positions[i] = vec3(
//					sin(alpha) * cos(beta),
//					sin(beta),
//					cos(alpha) * cos(beta)
//				);
//				positions[i] *= new_length - r_out + r_in;
//				radii[i] = max(.0f, dis_radii(gen));
//			}
//			else
//			{
//				p = rotation * p + origin;
//			}
//
//			positions[i] = p;
//		}
//
//		last_update = now;
//	}
//
//	void init_stars()
//	{
//		uniform_real_distribution<float> dis_distances = uniform_real_distribution<float>(r_in, r_out);
//		for (size_t i = 0; i < num_stars; i++)
//		{
//			float alpha = 2 * get_new_angle(), beta = get_new_angle();
//			positions[i] = vec3(
//				sin(alpha) * cos(beta),
//				sin(beta),
//				cos(alpha) * cos(beta)
//			);
//			positions[i] *= dis_distances(gen);
//			radii[i] = get_new_radius();
//		}
//		last_update = chrono::steady_clock::now();
//	}
//
//public:
//	stars_sphere(float a_r_in, float a_r_out,
//		float star_rad_mean = .01f, float star_rad_deviation = .005f)
//	{
//		r_out = a_r_out;
//		r_in = a_r_in;
//
//		positions = new vec3[num_stars];
//		radii = new float[num_stars];
//		colors = new rgb[num_stars]{ rgb(1, 1, 1) };
//
//		gen = mt19937(random_device()());
//		dis_angle = normal_distribution<float>(0, pi_half / 2);
//		dis_radii = normal_distribution<float>(star_rad_mean, star_rad_deviation);
//
//		speed_ahead = a_speed;
//		speed_pitch = 0;
//		speed_yaw = 0;
//		speed_roll = 0;
//
//		origin = vec3(0, 0, -r_out);
//		model_view_mat.identity();
//		model_view_mat *= cgv::math::translate4(origin);
//
//		init_stars();
//	}
//
//	void set_speed_ahead(float new_speed) { speed_ahead = new_speed; }
//	float get_max_speed_ahead() { return max_speed_ahead; }
//
//	void draw(cgv::render::context& ctx)
//	{
//		update();
//
//		ctx.push_modelview_matrix();
//		ctx.mul_modelview_matrix(model_view_mat);
//		cgv::render::sphere_renderer& sr = cgv::render::ref_sphere_renderer(ctx);
//		sr.set_position_array(ctx, positions, num_stars);
//		sr.set_radius_array(ctx, radii, num_stars);
//		sr.set_color_array(ctx, colors, num_stars);
//		sr.validate_and_enable(ctx);
//		glDrawArrays(GL_POINTS, 0, num_stars);
//		sr.disable(ctx);
//		ctx.pop_modelview_matrix();
//	}
//
//	float get_new_angle() {
//		float result = pi_half;
//		while (result <= -pi_half || result >= pi_half)
//		{
//			result = dis_angle(gen);
//		}
//
//		return result;
//	}
//
//	float get_new_radius() {
//		float result = 0;
//		while (result <= 0)
//		{
//			result = dis_radii(gen);
//		}
//
//		return result;
//	}
//
//	static void set_speed_ahead(stars_sphere* s, float val) { s->speed_ahead = val * s->max_speed_ahead; }
//	static void set_speed_pitch(stars_sphere* s, float val) { s->speed_pitch = val * s->max_angular_speed; }
//	static void set_speed_yaw(stars_sphere* s, float val) { s->speed_yaw = val * s->max_angular_speed; }
//	static void set_speed_roll(stars_sphere* s, float val) { s->speed_roll = val * s->max_angular_speed; }
//};
//
