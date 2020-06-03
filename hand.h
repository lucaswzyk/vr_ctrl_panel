#pragma once

#include <cgv/base/node.h>
#include <cgv/signal/rebind.h>
#include <cgv/gui/event_handler.h>
#include <cgv/math/ftransform.h>
#include <cgv/utils/scan.h>
#include <cgv/utils/options.h>
#include <cgv/gui/provider.h>

#include <cgv/math/quaternion.h>
#include <cgv/render/drawable.h>
#include <cgv_gl/box_wire_renderer.h>
#include <cgv_gl/rounded_cone_renderer.h>
#include <cgv_gl/sphere_renderer.h>
#include <cgv_gl/gl/gl.h>
#include <iostream>

#include <vr/vr_driver.h>
#include <cg_vr/vr_server.h>
#include <cgv/gui/event_handler.h>

#include "nd_device.h"
#include "nd_handler.h"

using namespace std;

class hand
	: public cgv::base::node,
	public cgv::render::drawable,
	public cgv::gui::event_handler,
	public cgv::gui::provider
{
	enum hand_parts
	{
		PALM, THUMB0, THUMB1, INDEX, MIDDLE, RING, PINKY
	};

	struct joint_positions {
		vector<cgv::render::render_types::vec3> positions;

		void rotate(cgv::math::quaternion<float> rotation)
		{
			for (size_t i = 0; i < positions.size(); i++)
			{
				rotation.rotate(positions[i]);
			}
		}

		void translate(cgv::render::render_types::vec3 translation)
		{
			for (size_t i = 0; i < positions.size(); i++)
			{
				positions[i] += translation;
			}
		}

		void translate(float x, float y, float z)
		{
			translate(cgv::render::render_types::vec3(x, y, z));
		}

		void add_origin()
		{
			positions.push_back(cgv::render::render_types::vec3(0));
		}

		void append(joint_positions appendix)
		{
			for (size_t i = 0; i < appendix.positions.size(); i++)
			{
				positions.push_back(appendix.positions[i]);
			}
		}

		static joint_positions join(joint_positions pos1, joint_positions pos2)
		{
			joint_positions result;

			for (size_t i = 0; i < pos1.positions.size(); i++)
			{
				result.positions.push_back(pos1.positions[i]);
			}
			for (size_t i = 0; i < pos2.positions.size(); i++)
			{
				result.positions.push_back(pos2.positions[i]);
			}

			return result;
		}
	};

protected:
	nd_device device;
	cgv::render::render_types::vec3 origin;
	vector<cgv::math::quaternion<float>> rotations;
	cgv::math::fmat<float, 4, 4> identity, palm_rotation_mat;
	cgv::render::sphere_render_style sphere_style;

	void on_device_change(void* kit_handle, bool attach)
	{
		void* last_kit_handle = 0;
		if (attach) {
			if (last_kit_handle == 0) {
				vr::vr_kit* kit_ptr = vr::get_vr_kit(kit_handle);
				if (kit_ptr) {
					last_kit_handle = kit_handle;
					//left_deadzone_and_precision = kit_ptr->get_controller_throttles_and_sticks_deadzone_and_precision(0);
					//cgv::gui::ref_vr_server().provide_controller_throttles_and_sticks_deadzone_and_precision(kit_handle, 0, &left_deadzone_and_precision);
					//post_recreate_gui();
				}
			}
		}
		else {
			if (kit_handle == last_kit_handle) {
				last_kit_handle = 0;
				//post_recreate_gui();
			}
		}
	}

public:
	hand() {
	}

	hand(NDAPISpace::Location location)
	{
		connect(cgv::gui::ref_vr_server().on_device_change, this, &hand::on_device_change);
		device = nd_device(location);
		set_geometry();
	}

	string get_type_name(void) const
	{
		return "hand";
	}

	void set_origin(cgv::render::render_types::vec3 new_origin)
	{
		origin = new_origin;
	}

	void set_geometry()
	{
		//// palm
		//palm_position.push_back(cgv::render::render_types::vec3(0, 0, 0));
		//palm_extent.push_back(cgv::render::render_types::vec3(.4, .1, .5));
		//palm_rotation.push_back(cgv::math::quaternion<float>(0, 0, 0, 1));

		//// thumb
		//thumb_position.push_back(cgv::render::render_types::vec3(0));
		//thumb_extent.push_back(cgv::render::render_types::vec3(-.1, .1, -.5));
		//thumb_rotation.push_back(cgv::math::quaternion<float>(0, 0, 0, 1));
		//thumb_mv_mat.identity();
		//thumb_mv_mat(0, 3) = -.2;
		//thumb_mv_mat(1, 3) = -.05;
		//thumb_mv_mat(2, 3) = .15;

		//// index
		//index_position.push_back(cgv::render::render_types::vec3(0));
		//index_extent.push_back(cgv::render::render_types::vec3(-.1, .1, -.6));
		//index_rotation.push_back(cgv::math::quaternion<float>(0, 0, 0, 1));
		//index_mv_mat.identity();
		//index_mv_mat(0, 3) = -.1;
		//index_mv_mat(1, 3) = -.05;
		//index_mv_mat(2, 3) = -.25;

		//// middle
		//middle_position.push_back(cgv::render::render_types::vec3(0));
		//middle_extent.push_back(cgv::render::render_types::vec3(-.1, .1, -.6));
		//middle_rotation.push_back(cgv::math::quaternion<float>(0, 0, 0, 1));
		//middle_mv_mat.identity();
		//middle_mv_mat(1, 3) = -.05;
		//middle_mv_mat(2, 3) = -.25;

		//// ring
		//ring_position.push_back(cgv::render::render_types::vec3(0));
		//ring_extent.push_back(cgv::render::render_types::vec3(-.1, .1, -.6));
		//ring_rotation.push_back(cgv::math::quaternion<float>(0, 0, 0, 1));
		//ring_mv_mat.identity();
		//ring_mv_mat(0, 3) = .1;
		//ring_mv_mat(1, 3) = -.05;
		//ring_mv_mat(2, 3) = -.25;

		//// pinky
		//pinky_position.push_back(cgv::render::render_types::vec3(0));
		//pinky_extent.push_back(cgv::render::render_types::vec3(-.1, .1, -.6));
		//pinky_rotation.push_back(cgv::math::quaternion<float>(0, 0, 0, 1));
		//pinky_mv_mat.identity();
		//pinky_mv_mat(0, 3) = .2;
		//pinky_mv_mat(1, 3) = -.05;
		//pinky_mv_mat(2, 3) = -.25;

		identity.identity();
	}

	void draw(cgv::render::context& ctx)
	{
		rotations = device.get_cgv_rotations();

		joint_positions thumb0, thumb1;
		thumb1.add_origin();
		thumb1.translate(0, 0, -3);
		thumb1.rotate(rotations[NDAPISpace::IMULOC_THUMB1]);
		thumb1.translate(0, 0, -4);
		thumb0.add_origin();
		thumb0.translate(0, 0, -4);
		thumb0.rotate(rotations[NDAPISpace::IMULOC_THUMB0]);
		thumb0.add_origin();
		joint_positions thumb = joint_positions::join(thumb0, thumb1);
		thumb.translate(-5, -3, 2.5);

		joint_positions index;
		index.add_origin();
		index.translate(0, 0, -2);
		index.add_origin();
		index.translate(0, 0, -3);
		index.add_origin();
		index.translate(0, 0, -5);
		index.rotate(rotations[NDAPISpace::IMULOC_INDEX]);
		index.add_origin();
		index.translate(-3.5, 0, -4);

		joint_positions middle;
		middle.add_origin();
		middle.translate(0, 0, -2.5);
		middle.add_origin();
		middle.translate(0, 0, -3.5);
		middle.add_origin();
		middle.translate(0, 0, -5);
		middle.rotate(rotations[NDAPISpace::IMULOC_MIDDLE]);
		middle.add_origin();
		middle.translate(-1, 0, -4.5);

		rotations = device.get_cgv_rotations();
		joint_positions ring;
		ring.add_origin();
		ring.translate(0, 0, -2.5);
		ring.add_origin();
		ring.translate(0, 0, -3.5);
		ring.add_origin();
		ring.translate(0, 0, -4.5);
		ring.rotate(rotations[NDAPISpace::IMULOC_RING]);
		ring.add_origin();
		ring.translate(1.5, 0, -4);

		rotations = device.get_cgv_rotations();
		joint_positions pinky;
		pinky.add_origin();
		pinky.translate(0, 0, -2);
		pinky.add_origin();
		pinky.translate(0, 0, -2.5);
		pinky.add_origin();
		pinky.translate(0, 0, -4);
		pinky.rotate(rotations[NDAPISpace::IMULOC_PINKY]);
		pinky.add_origin();
		pinky.translate(4, 0, -4);

		joint_positions hand;
		hand.add_origin();
		hand.append(thumb);
		hand.append(index);
		hand.append(middle);
		hand.append(ring);
		hand.append(pinky);
		hand.rotate(rotations[NDAPISpace::IMULOC_PALM]);
		hand.translate(10.0f*origin);

		cgv::render::sphere_renderer& sr = cgv::render::ref_sphere_renderer(ctx);
		sr.set_position_array(ctx, hand.positions);
		sr.validate_and_enable(ctx);
		glDrawArrays(GL_POINTS, 0, hand.positions.size());
		sr.disable(ctx);

		//draw_part(ctx, identity, palm_position, palm_extent, palm_rotation, true);

		// enter palm coord system

		//cgv::render::rounded_cone_renderer& rcr = cgv::render::ref_rounded_cone_renderer(ctx);
		//cgv::render::rounded_cone_renderer::cone c;
		//c.start = cgv::math::fvec<float, 4U>(0, 0, 0, 1);
		//c.end = cgv::math::fvec<float, 4U>(0, 0, 1, 1);
		//vector<cgv::render::rounded_cone_renderer::cone> cones;
		//cones.push_back(c);
		//rcr.set_cone_array(ctx, cones);
		//rcr.validate_and_enable(ctx);
		//glDrawArrays(GL_POINTS, 0, 1);
		//rcr.disable(ctx);

		//draw_part(ctx, thumb_mv_mat, thumb_position, thumb_extent, thumb_rotation, false);
		//draw_part(ctx, index_mv_mat, index_position, index_extent, index_rotation, false);
		//draw_part(ctx, middle_mv_mat, middle_position, middle_extent, middle_rotation, false);
		//draw_part(ctx, ring_mv_mat, ring_position, ring_extent, ring_rotation, false);
		//draw_part(ctx, pinky_mv_mat, pinky_position, pinky_extent, pinky_rotation, false);

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

	// Inherited via event_handler
	virtual bool handle(cgv::gui::event& e) override;
	virtual void stream_help(std::ostream& os) override;

	// Inherited via provider
	virtual void create_gui() override;
};