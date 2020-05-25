#pragma once

#include <cgv/math/quaternion.h>
#include <cgv/render/drawable.h>
#include <cgv_gl/box_wire_renderer.h>
#include <cgv_gl/gl/gl.h>
#include <iostream>

#include "nd_device.h"
#include "nd_handler.h"

using namespace std;

class hand
	: public cgv::render::drawable
{
	enum hand_parts
	{
		PALM, THUMB0, THUMB1, INDEX, MIDDLE, RING, PINKY
	};

protected:
	nd_device device;
	vector<cgv::render::render_types::vec3> palm_extent, palm_position,
		thumb_position, thumb_extent,
		index_position, index_extent,
		middle_position, middle_extent,
		ring_position, ring_extent,
		pinky_position, pinky_extent;
	vector<cgv::math::quaternion<float>> rotations,
		palm_rotation,
		thumb_rotation,
		index_rotation,
		middle_rotation,
		ring_rotation,
		pinky_rotation;
	cgv::math::fmat<float, 4, 4> identity,
		palm_rotation_mat,
		thumb_mv_mat,
		index_mv_mat,
		middle_mv_mat,
		ring_mv_mat,
		pinky_mv_mat;

public:
	hand() {}

	hand(NDAPISpace::Location location)
	{
		device = nd_device(location);
		set_geometry();
	}

	void set_geometry()
	{
		// palm
		palm_position.push_back(cgv::render::render_types::vec3(0, 0, 0));
		palm_extent.push_back(cgv::render::render_types::vec3(.4, .1, .5));
		palm_rotation.push_back(cgv::math::quaternion<float>(0, 0, 0, 1));
		
		// thumb
		thumb_position.push_back(cgv::render::render_types::vec3(0));
		thumb_extent.push_back(cgv::render::render_types::vec3(-.1, .1, -.5));
		thumb_rotation.push_back(cgv::math::quaternion<float>(0, 0, 0, 1));
		thumb_mv_mat.identity();
		thumb_mv_mat(0, 3) = -.2;
		thumb_mv_mat(1, 3) = -.05;
		thumb_mv_mat(2, 3) = .15;

		// index
		index_position.push_back(cgv::render::render_types::vec3(0));
		index_extent.push_back(cgv::render::render_types::vec3(-.1, .1, -.6));
		index_rotation.push_back(cgv::math::quaternion<float>(0, 0, 0, 1));
		index_mv_mat.identity();
		index_mv_mat(0, 3) = -.1;
		index_mv_mat(1, 3) = -.05;
		index_mv_mat(2, 3) = -.25;

		// middle
		middle_position.push_back(cgv::render::render_types::vec3(0));
		middle_extent.push_back(cgv::render::render_types::vec3(-.1, .1, -.6));
		middle_rotation.push_back(cgv::math::quaternion<float>(0, 0, 0, 1));
		middle_mv_mat.identity();
		middle_mv_mat(1, 3) = -.05;
		middle_mv_mat(2, 3) = -.25;

		// ring
		ring_position.push_back(cgv::render::render_types::vec3(0));
		ring_extent.push_back(cgv::render::render_types::vec3(-.1, .1, -.6));
		ring_rotation.push_back(cgv::math::quaternion<float>(0, 0, 0, 1));
		ring_mv_mat.identity();
		ring_mv_mat(0, 3) = .1;
		ring_mv_mat(1, 3) = -.05;
		ring_mv_mat(2, 3) = -.25;
		
		// pinky
		pinky_position.push_back(cgv::render::render_types::vec3(0));
		pinky_extent.push_back(cgv::render::render_types::vec3(-.1, .1, -.6));
		pinky_rotation.push_back(cgv::math::quaternion<float>(0, 0, 0, 1));
		pinky_mv_mat.identity();
		pinky_mv_mat(0, 3) = .2;
		pinky_mv_mat(1, 3) = -.05;
		pinky_mv_mat(2, 3) = -.25;

		identity.identity();
	}

	void draw(cgv::render::context& ctx)
	{
		set_rotations();

		draw_part(ctx, identity, palm_position, palm_extent, palm_rotation, true);

		// enter palm coord system
		ctx.push_modelview_matrix();
		rotations[0].put_homogeneous_matrix(palm_rotation_mat);
		ctx.mul_modelview_matrix(palm_rotation_mat);

		draw_part(ctx, thumb_mv_mat, thumb_position, thumb_extent, thumb_rotation, false);
		draw_part(ctx, index_mv_mat, index_position, index_extent, index_rotation, false);
		draw_part(ctx, middle_mv_mat, middle_position, middle_extent, middle_rotation, false);
		draw_part(ctx, ring_mv_mat, ring_position, ring_extent, ring_rotation, false);
		draw_part(ctx, pinky_mv_mat, pinky_position, pinky_extent, pinky_rotation, false);

		// exit palm coord system
		ctx.pop_modelview_matrix();
	}

	void set_rotations()
	{
		rotations = device.get_cgv_rotations();

		palm_rotation[0] = rotations[0];
		thumb_rotation[0] = rotations[1];
		index_rotation[0] = rotations[3];
		middle_rotation[0] = rotations[4];
		ring_rotation[0] = rotations[5];
		pinky_rotation[0] = rotations[6];

		cgv::math::fmat<float, 3, 3> index_mat;
		index_rotation[0].put_matrix(index_mat);
		cout << index_mat(1, 1) << endl;
		if (index_mat(1, 1) < 0)
		{
			nd_handler* ndh = nd_handler::instance();
			ndh->set_index_pulse();
		}
	}

	void draw_part(cgv::render::context& ctx,
		cgv::math::fmat<float, 4, 4> model_view_mat,
		vector<cgv::render::render_types::vec3> position,
		vector<cgv::render::render_types::vec3> extent,
		vector<cgv::math::quaternion<float>> rotation,
		bool position_is_center)
	{
		ctx.push_modelview_matrix();
		ctx.mul_modelview_matrix(model_view_mat);

		cgv::render::box_wire_renderer& renderer = cgv::render::ref_box_wire_renderer(ctx);
		renderer.set_position_is_center(position_is_center);
		renderer.set_position_array(ctx, position);
		renderer.set_extent_array(ctx, extent);
		renderer.set_rotation_array(ctx, rotation);

		renderer.validate_and_enable(ctx);
		glDrawArrays(GL_POINTS, 0, 1);
		renderer.disable(ctx);

		ctx.pop_modelview_matrix();
	}
};

