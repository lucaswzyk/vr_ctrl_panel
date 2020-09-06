#include "space.h"

void space::update()
{
	// to ensure realistic movement independent of frame rate
	chrono::steady_clock::time_point now = chrono::steady_clock::now();
	float ms_elapsed = chrono::duration_cast<chrono::milliseconds>(now - last_update).count();

	float distance_elapsed = speed_ahead * ms_elapsed;
	vec3 angles = vec3(speed_pitch, speed_yaw, speed_roll);
	mat3 rotation = cgv::math::rotate3(ms_elapsed * angles),
		inv_rotation = cgv::math::rotate3(-2 * r_out * angles);

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
			distance_elapsed *= target_speed_ratio;
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
			float spawn_ratio = i < num_stars ? spawn_ratio_stars : spawn_ratio_targets;
			p = spawn_ratio * r_out * vec3(
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

// if a target has been hit, it is mirrored at the midpoint of the shell

void space::fire() {
	is_phaser_firing = true;

	for (size_t i = num_stars; i < num_stars + num_targets; i++)
	{
		vec3 t_pos = positions[i];
		if (t_pos.length() < r_out)
		{
			float a = cgv::math::dot(t_pos, phaser_directions[0]);
			vec3 t_center_ray0 = t_pos - phaser_positions[1]
				- cgv::math::dot(t_pos, phaser_directions[0]) * phaser_directions[0],
				t_center_ray1 = t_pos - phaser_positions[3]
				- cgv::math::dot(t_pos, phaser_directions[1]) * phaser_directions[1];
			if (t_center_ray0.length() < radii[i] || t_center_ray1.length() < radii[i])
			{
				positions[i] = -positions[i] - 2.0f * origin;
			}
		}
	}
}

void space::init()
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
		vec3(-5.0f, 10.0f, .0f),
		vec3(phaser_loc.x(), phaser_loc.y(), phaser_loc.z()) - origin,
		vec3(5.0f, 10.0f, .0f)
	};
	phaser_directions = {
		phaser_positions[0] - phaser_positions[1],
		phaser_positions[2] - phaser_positions[3] };
	phaser_directions[0].normalize();
	phaser_directions[1].normalize();

	rcrs.surface_color = rgb(.73f, .27f, .07f);

	last_update = chrono::steady_clock::now();
}

space::space(float a_r_in, float a_r_out)
{
	r_out = a_r_out;
	r_in = a_r_in;

	positions = new vec3[num_stars + max_num_targets];
	radii = new float[num_stars + max_num_targets];
	colors = new rgb[num_stars + max_num_targets];

	gen = mt19937(random_device()());
	dis_angles = uniform_real_distribution<float>(-M_PI_2, M_PI_2);
	dis_radii = normal_distribution<float>(star_rad_mean, star_rad_deviation);

	speed_ahead = 0;
	speed_pitch = 0;
	speed_yaw = 0;
	speed_roll = 0;

	origin = vec3(0, 0, -r_out);

	num_targets = 0;
	is_phaser_firing = false;

	init();
}

void space::draw(context& ctx)
{
	update();

	ctx.push_modelview_matrix();
	ctx.mul_modelview_matrix(model_view_mat);

	sphere_renderer& sr = ref_sphere_renderer(ctx);
	sr.set_position_array(ctx, positions, num_stars + num_targets);
	sr.set_radius_array(ctx, radii, num_stars + num_targets);
	sr.set_color_array(ctx, colors, num_stars + num_targets);
	sr.set_render_style(srs);
	sr.render(ctx, 0, num_stars + num_targets);

	if (is_phaser_firing)
	{
		rounded_cone_renderer& rcr = ref_rounded_cone_renderer(ctx);
		rcr.set_position_array(ctx, phaser_positions);
		rcr.set_indices(ctx, phaser_indices);
		rcr.set_radius_array(ctx, phaser_radii);
		rcr.set_render_style(rcrs);
		rcr.render(ctx, 0, phaser_indices.size());
		is_phaser_firing = false;
	}

	ctx.pop_modelview_matrix();
}

float space::get_new_radius() {
	float result = 0;
	while (result <= 0)
	{
		result = dis_radii(gen);
	}

	return result;
}
