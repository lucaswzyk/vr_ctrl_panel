#pragma once

#include <cgv/math/quaternion.h>
#include <cgv/render/drawable.h>
#include <cgv_gl/box_wire_renderer.h>
#include <cgv_gl/gl/gl.h>

#include "nd_device.h"

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
		thumb_position, thumb_extent;
	cgv::math::fmat<float, 4, 4> z_mirror;

public:
	hand() {}

	hand(NDAPISpace::Location location)
	{
		device = nd_device(location);
		set_geometry();
	}

	void set_geometry()
	{
		palm_position.push_back(cgv::render::render_types::vec3(0, 0, 0));
		thumb_position.push_back(cgv::render::render_types::vec3(-.5, 0, 0));
		//positions.push_back(cgv::render::render_types::vec3(0, 0, 0));
		//positions.push_back(cgv::render::render_types::vec3(0, 0, 0));
		//positions.push_back(cgv::render::render_types::vec3(0, 0, 0));
		//positions.push_back(cgv::render::render_types::vec3(0, 0, 0));

		palm_extent.push_back(cgv::render::render_types::vec3(.5, .1, .7));
		thumb_extent.push_back(cgv::render::render_types::vec3(.1, .1, .1));

		z_mirror.identity();
		z_mirror(1, 1) = -1;
	}

	void draw(cgv::render::context& ctx)
	{
		//ctx.push_modelview_matrix();
		//ctx.mul_modelview_matrix(z_mirror);

		vector<cgv::math::quaternion<float>> rotations = device.get_cgv_rotations(),
			palm_rotation, thumb_rotation;

		palm_rotation.push_back(rotations[0]);
		thumb_rotation.push_back(rotations[2]);

		cgv::render::box_wire_renderer& renderer = cgv::render::ref_box_wire_renderer(ctx);
		//renderer.set_position_array(ctx, palm_position);
		//renderer.set_extent_array(ctx, palm_extent);
		//renderer.set_rotation_array(ctx, palm_rotation);

		//renderer.validate_and_enable(ctx);
		//glDrawArrays(GL_POINTS, 0, GLsizei(palm_position.size()));
		//renderer.disable(ctx);

		//ctx.push_modelview_matrix();
		//cgv::math::fmat<float, 4, 4> palm_rot_mat;
		//rotations[0].put_homogeneous_matrix(palm_rot_mat);
		//ctx.mul_modelview_matrix(palm_rot_mat);

		renderer.set_position_is_center(true);
		renderer.set_position_array(ctx, thumb_position);
		renderer.set_extent_array(ctx, thumb_extent);
		renderer.set_rotation_array(ctx, thumb_rotation);

		renderer.validate_and_enable(ctx);
		glDrawArrays(GL_POINTS, 0, GLsizei(thumb_position.size()));
		renderer.disable(ctx);

		//ctx.pop_modelview_matrix();
		//renderer.set_position_is_center(true);
		//ctx.pop_modelview_matrix();
	}
};

