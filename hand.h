#pragma once

#include <cgv/base/node.h>
#include <cgv/signal/rebind.h>
#include <cgv/gui/event_handler.h>
#include <cgv/math/ftransform.h>
#include <cgv/utils/scan.h>
#include <cgv/utils/options.h>
#include <cgv/gui/provider.h>

#include <cgv/render/drawable.h>
#include <cgv/math/quaternion.h>
#include <cgv_gl/sphere_renderer.h>
#include <cgv_gl/gl/gl.h>
#include <iostream>

#include <vr/vr_driver.h>
#include <cg_vr/vr_server.h>
#include <cgv/gui/event_handler.h>

#include "nd_device.h"
#include "nd_handler.h"
#include "conn_panel.h"

using namespace std;

class hand
	: public cgv::base::node,
	public cgv::render::drawable
{
	enum hand_parts
	{
		PALM, THUMB, INDEX, MIDDLE, RING, PINKY, NUM_HAND_PARTS
	};

	enum phalanges
	{
		PROXIMAL, INTERMED, DISTAL, NUM_BONES_PER_FINGER
	};

	struct joint_positions {
		vector<vector<vec3>> positions;

		joint_positions()
		{
			for (size_t i = 0; i < NUM_HAND_PARTS; i++)
			{
				positions.push_back(vector<vec3>(NUM_BONES_PER_FINGER));
			}
		}

		void push_back(int part, vec3 v)
		{
			positions[part].push_back(v);
		}

		void rotate(int part, quat rotation)
		{
			for (size_t i = 0; i < positions[part].size(); i++)
			{
				rotation.rotate(positions[part][i]);
			}
		}

		void translate(int part, vec3 translation)
		{
			for (size_t i = 0; i < positions[part].size(); i++)
			{
				positions[part][i] += translation;
			}
		}

		void translate_neg_z(int part, float z)
		{
			translate(part, vec3(0, 0, -z));
		}

		void translate(vec3 translation)
		{
			for (size_t part = 0; part < positions.size(); part++)
			{
				translate(part, translation);
			}
		}

		void scale(float scale)
		{
			for (size_t part = 0; part < positions.size(); part++)
			{
				for (size_t i = 0; i < positions[part].size(); i++)
				{
					positions[part][i] *= scale;
				}
			}
		}

		vector<vec3> make_array()
		{
			vector<vec3> result;
			for (size_t i = 0; i < positions.size(); i++)
			{
				for (size_t j = 0; j < positions[i].size(); j++)
				{
					result.push_back(positions[i][j]);
				}
			}

			return result;
		}
	};

protected:
	nd_device device;
	vector<vec3> bone_lengths, palm_resting;
	vector<vector<quat>> recursive_rotations;
	vec3 origin;
	joint_positions pose;
	float scale = .01f;
	vector<NDAPISpace::Actuator> actuators;

public:
	hand() 
	{}

	hand(NDAPISpace::Location location)
	{
		device = nd_device(location);
		set_geometry();
	}

	string get_type_name(void) const
	{
		return "hand";
	}

	void set_geometry()
	{
		vector<quat> identity_vec(NUM_BONES_PER_FINGER, quat(1, 0, 0, 0));
		recursive_rotations = vector<vector<quat>>(NUM_HAND_PARTS, identity_vec);

		// palm
		bone_lengths.push_back(vec3());
		palm_resting.push_back(vec3());
		recursive_rotations[PALM] = vector<quat>(1, quat(1, 0, 0, 0));

		// thumb
		bone_lengths.push_back(vec3(0.0f, 4.0f, 3.0f));
		palm_resting.push_back(vec3(-5, -3, 2.5));

		// index
		bone_lengths.push_back(vec3(5.0f, 3.0f, 2.0f));
		palm_resting.push_back(vec3(-3.5, 0, -4));

		// middle
		bone_lengths.push_back(vec3(5.0f, 3.5f, 2.5f));
		palm_resting.push_back(vec3(-1, 0, -4.5));

		// ring
		bone_lengths.push_back(vec3(4.5f, 3.5f, 2.5f));
		palm_resting.push_back(vec3(1.5, 0, -4));

		// pinky
		bone_lengths.push_back(vec3(4.0f, 2.5f, 2.0f));
		palm_resting.push_back(vec3(4, 0, -4));

		palm_resting.push_back(vec3(-3.5, -3, 3));
		palm_resting.push_back(vec3(3, -2, 2.5));

		if (device.is_left())
		{
			for (size_t i = 0; i < palm_resting.size(); i++)
			{
				vec3 pos = palm_resting[i];
				palm_resting[i] = vec3(-pos.x(), pos.y(), pos.z());
			}
		}

		actuators = vector<NDAPISpace::Actuator>{
			NDAPISpace::ACT_THUMB,
			NDAPISpace::ACT_INDEX,
			NDAPISpace::ACT_MIDDLE,
			NDAPISpace::ACT_RING,
			NDAPISpace::ACT_PINKY,
			NDAPISpace::ACT_PALM_INDEX_UP,
			NDAPISpace::ACT_PALM_MIDDLE_UP,
			NDAPISpace::ACT_PALM_PINKY_UP,
			NDAPISpace::ACT_PALM_INDEX_DOWN,
			NDAPISpace::ACT_PALM_PINKY_DOWN
		};
	}

	void set_pose_and_actuators(const conn_panel& cp, vec3 translation, float ascale)
	{
		origin = translation;
		scale = ascale;
		set_rotations();

		pose = joint_positions();
		pose.positions[PALM] = palm_resting;
		pose.rotate(PALM, recursive_rotations[PALM][0]);

		for (size_t finger = THUMB; finger < NUM_HAND_PARTS; finger++)
		{
			pose.positions[finger][DISTAL] = vec3(0);
			pose.translate_neg_z(finger, bone_lengths[finger][DISTAL]);
			pose.rotate(finger, recursive_rotations[finger][DISTAL]);

			pose.positions[finger][INTERMED] = vec3(0);
			pose.translate_neg_z(finger, bone_lengths[finger][INTERMED]);
			pose.rotate(finger, recursive_rotations[finger][INTERMED]);

			pose.positions[finger][PROXIMAL] = vec3(0);
			pose.translate_neg_z(finger, bone_lengths[finger][PROXIMAL]);
			pose.rotate(finger, recursive_rotations[finger][PROXIMAL]);

			pose.translate(finger, pose.positions[PALM][finger]);
		}

		pose.scale(scale);
		pose.translate(origin);

		vector<vec3> touching_points = vector<vec3>{
			pose.positions[THUMB][DISTAL],
			pose.positions[INDEX][DISTAL],
			pose.positions[MIDDLE][DISTAL],
			pose.positions[RING][DISTAL],
			pose.positions[PINKY][DISTAL],
			pose.positions[PALM][INDEX],
			.5f * (pose.positions[PALM][MIDDLE] + pose.positions[PALM][RING]),
			pose.positions[PALM][PINKY],
			pose.positions[PALM][NUM_HAND_PARTS],
			pose.positions[PALM][NUM_HAND_PARTS + 1]
		};

		std::set<int> touching_indices = cp.check_containments(touching_points, scale);

		for each (int ind in touching_indices)
		{
			device.set_actuator_pulse(actuators[ind]);
		}
	}

	void draw(cgv::render::context& ctx)
	{
		vector<vec3> positions = pose.make_array();
		vector<float> radius_array = vector<float>(positions.size(), scale);
		radius_array[0] = 2 * scale;

		cgv::render::sphere_renderer& sr = cgv::render::ref_sphere_renderer(ctx);
		sr.set_position_array(ctx, positions);
		sr.set_radius_array(ctx, radius_array);
		sr.validate_and_enable(ctx);
		glDrawArrays(GL_POINTS, 0, positions.size());
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
		vector<quat> imu_rotations = device.get_cgv_rotations();
		quat thumb0_quat = imu_rotations[NDAPISpace::IMULOC_THUMB0];

		recursive_rotations[PALM][0] = imu_rotations[NDAPISpace::IMULOC_PALM];
		recursive_rotations[THUMB][INTERMED] = thumb0_quat;
		recursive_rotations[THUMB][DISTAL] = thumb0_quat.inverse() * imu_rotations[NDAPISpace::IMULOC_THUMB1];
		recursive_rotations[INDEX][PROXIMAL] = imu_rotations[NDAPISpace::IMULOC_INDEX];
		recursive_rotations[MIDDLE][PROXIMAL] = imu_rotations[NDAPISpace::IMULOC_MIDDLE];
		recursive_rotations[RING][PROXIMAL] = imu_rotations[NDAPISpace::IMULOC_RING];
		recursive_rotations[PINKY][PROXIMAL] = imu_rotations[NDAPISpace::IMULOC_PINKY];
	}
};