#pragma once

#include <cgv/render/render_types.h>
#include <vr/vr_state.h>

using namespace cgv::render;

class math_conversion :
	public render_types
{
public:
	static vec3 position_from_pose(const float pose[12], int col=3)
	{
		return vec3(pose[3 * col], pose[3 * col + 1], pose[3 * col + 2]);
	}

	static vec4 hom_pos(vec3 v)
	{
		return vec4(v.x(), v.y(), v.z(), 1.0f);
	}

	static vec4 hom_pos(const float pose[12])
	{
		return hom_pos(position_from_pose(pose));
	}

	static vec3 inhom_pos(vec4 v)
	{
		return vec3(v.x(), v.y(), v.z()) / v.w();
	}

	static vec4 hom_dir(vec3 dir)
	{
		return vec4(dir.x(), dir.y(), dir.z(), .0f);
	}

	static vec3 inhom_dir(vec4 dir)
	{
		return vec3(dir.x(), dir.y(), dir.z());
	}

	static vec3 ave_pos(const vr::vr_controller_state* ctrls)
	{
		vec3 result(0);
		int num_tracked = 0;
		for (size_t i = 0; i < 4; i++)
		{
			if (ctrls[i].status == vr::VRS_TRACKED)
			{
				result += position_from_pose(ctrls[i].pose);
				num_tracked++;
			}
		}

		return result / num_tracked;
	}
};
