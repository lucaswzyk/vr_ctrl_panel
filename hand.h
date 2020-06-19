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
	public cgv::render::drawable
{
	typedef cgv::render::render_types::vec3 vec3;
	typedef cgv::render::render_types::mat4 mat4;
	typedef cgv::math::quaternion<float> fquat;

	enum fingers
	{
		THUMB, INDEX, MIDDLE, RING, PINKY, NUM_FINGERS
	};

	enum phalanges
	{
		PROXIMAL, INTERMED, DISTAL, NUM_BONES_PER_FINGER
	};

	struct joint_positions {
		vector<vec3> positions;

		void push_back(vec3 v)
		{
			positions.push_back(v);
		}

		void rotate(fquat rotation)
		{
			for (size_t i = 0; i < positions.size(); i++)
			{
				rotation.rotate(positions[i]);
			}
		}

		void translate(vec3 translation)
		{
			for (size_t i = 0; i < positions.size(); i++)
			{
				positions[i] += translation;
			}
		}

		void translate(float x, float y, float z)
		{
			translate(vec3(x, y, z));
		}

		void add_origin()
		{
			positions.push_back(vec3(0));
		}

		void mirror_at_yz()
		{
			for (size_t i = 0; i < positions.size(); i++)
			{
				vec3 pos = positions[i];
				positions[i] = vec3(-pos.x(), pos.y(), pos.z());
			}
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
	vec3 origin;
	vector<vec3> bone_lengths;
	joint_positions fingers_meet_palm;
	vector<vector<fquat>> recursive_phalanx_rotations;
	fquat palm_rotation;
	cgv::render::sphere_render_style sphere_style;

public:
	hand() {
	}

	hand(NDAPISpace::Location location)
	{
		device = nd_device(location);
		set_geometry();
	}

	string get_type_name(void) const
	{
		return "hand";
	}

	void set_origin(vec3 new_origin)
	{
		origin = new_origin;
	}

	void set_geometry()
	{
		vector<fquat> identity_vec(NUM_BONES_PER_FINGER, fquat(1, 0, 0, 0));

		// thumb
		bone_lengths.push_back(vec3(0.0f, 4.0f, 3.0f));
		fingers_meet_palm.push_back(vec3(5, 3, -2.5));
		recursive_phalanx_rotations.push_back(identity_vec);

		// index
		bone_lengths.push_back(vec3(5.0f, 3.0f, 2.0f));
		fingers_meet_palm.push_back(vec3(3.5, 0, 4));
		recursive_phalanx_rotations.push_back(identity_vec);

		// middle
		bone_lengths.push_back(vec3(5.0f, 3.5f, 2.5f));
		fingers_meet_palm.push_back(vec3(1, 0, 4.5));
		recursive_phalanx_rotations.push_back(identity_vec);

		// ring
		bone_lengths.push_back(vec3(4.5f, 3.5f, 2.5f));
		fingers_meet_palm.push_back(vec3(-1.5, 0, 4));
		recursive_phalanx_rotations.push_back(identity_vec);

		// pinky
		bone_lengths.push_back(vec3(4.0f, 2.5f, 2.0f));
		fingers_meet_palm.push_back(vec3(-4, 0, 4));
		recursive_phalanx_rotations.push_back(identity_vec);

		if (device.is_left())
		{
			fingers_meet_palm.mirror_at_yz();
		}
	}

	void draw(cgv::render::context& ctx)
	{
		joint_positions hand(fingers_meet_palm);
		hand.rotate(palm_rotation);
		hand.translate(origin);

		for (size_t finger = 0; finger < NUM_FINGERS; finger++)
		{
			joint_positions hand_part;
			for (size_t phalanx = 0; phalanx < NUM_BONES_PER_FINGER; phalanx++)
			{
				hand_part.add_origin();
				hand_part.translate(0, 0, bone_lengths[finger][phalanx]);
				hand_part.rotate(recursive_phalanx_rotations[finger][phalanx]);
			}
			hand_part.translate(hand.positions[finger]);
		}

		cgv::render::sphere_renderer& sr = cgv::render::ref_sphere_renderer(ctx);
		sr.set_position_array(ctx, hand.positions);
		sr.validate_and_enable(ctx);
		glDrawArrays(GL_POINTS, 0, hand.positions.size());
		sr.disable(ctx);

		//joint_positions thumb0, thumb1;
		//thumb1.add_origin();
		//thumb1.translate(0, 0, -3);
		//thumb1.rotate(rotations[NDAPISpace::IMULOC_THUMB1]);
		//thumb1.translate(0, 0, -4);
		//thumb0.add_origin();
		//thumb0.translate(0, 0, -4);
		//thumb0.rotate(rotations[NDAPISpace::IMULOC_THUMB0]);
		//thumb0.add_origin();
		//joint_positions thumb = joint_positions::join(thumb0, thumb1);
		//thumb.translate(-5, -3, 2.5);

		//joint_positions index;
		//index.add_origin();
		//index.translate(0, 0, -2);
		//index.add_origin();
		//index.translate(0, 0, -3);
		//index.add_origin();
		//index.translate(0, 0, -5);
		//index.rotate(rotations[NDAPISpace::IMULOC_INDEX]);
		//index.add_origin();
		//index.translate(-3.5, 0, -4);

		//joint_positions middle;
		//middle.add_origin();
		//middle.translate(0, 0, -2.5);
		//middle.add_origin();
		//middle.translate(0, 0, -3.5);
		//middle.add_origin();
		//middle.translate(0, 0, -5);
		//middle.rotate(rotations[NDAPISpace::IMULOC_MIDDLE]);
		//middle.add_origin();
		//middle.translate(-1, 0, -4.5);

		//rotations = device.get_cgv_rotations();
		//joint_positions ring;
		//ring.add_origin();
		//ring.translate(0, 0, -2.5);
		//ring.add_origin();
		//ring.translate(0, 0, -3.5);
		//ring.add_origin();
		//ring.translate(0, 0, -4.5);
		//ring.rotate(rotations[NDAPISpace::IMULOC_RING]);
		//ring.add_origin();
		//ring.translate(1.5, 0, -4);

		//rotations = device.get_cgv_rotations();
		//joint_positions pinky;
		//pinky.add_origin();
		//pinky.translate(0, 0, -2);
		//pinky.add_origin();
		//pinky.translate(0, 0, -2.5);
		//pinky.add_origin();
		//pinky.translate(0, 0, -4);
		//pinky.rotate(rotations[NDAPISpace::IMULOC_PINKY]);
		//pinky.add_origin();
		//pinky.translate(4, 0, -4);

		//hand.append(thumb);
		//hand.append(index);
		//hand.append(middle);
		//hand.append(ring);
		//hand.append(pinky);

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

	void set_rotations()
	{
		vector<fquat> imu_rotations = device.get_cgv_rotations();
		fquat thumb0_quat = imu_rotations[NDAPISpace::IMULOC_THUMB0];
		palm_rotation = imu_rotations[NDAPISpace::IMULOC_PALM];

		recursive_phalanx_rotations[THUMB][INTERMED] = thumb0_quat;
		recursive_phalanx_rotations[THUMB][DISTAL] = thumb0_quat.inverse() * imu_rotations[NDAPISpace::IMULOC_THUMB1];
		recursive_phalanx_rotations[INDEX][PROXIMAL] = imu_rotations[NDAPISpace::IMULOC_INDEX];
		recursive_phalanx_rotations[MIDDLE][PROXIMAL] = imu_rotations[NDAPISpace::IMULOC_MIDDLE];
		recursive_phalanx_rotations[RING][PROXIMAL] = imu_rotations[NDAPISpace::IMULOC_RING];
		recursive_phalanx_rotations[PINKY][PROXIMAL] = imu_rotations[NDAPISpace::IMULOC_PINKY];
	}
};