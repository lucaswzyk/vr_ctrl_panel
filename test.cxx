#include "NDAPI.h"
#include "stdafx.h"
#include <iostream>

#include <cgv/base/register.h>
#include <cgv/render/drawable.h>
#include <cgv_gl/box_wire_renderer.h>
#include <cgv_gl/gl/gl.h>

#include "nd_handler.h"
#include "hand.h"

using namespace std;

class cgv_test
	: public cgv::base::base,
	  public cgv::render::drawable
{
protected:
	string hello;
	vector<hand> hands;
	nd_handler::mode app_mode;

public:

	cgv_test()
		: hello("")
	{}

	string get_type_name(void) const
	{
		return "cgv_test";
	}

	bool self_reflect(cgv::reflect::reflection_handler& rh)
	{
		return rh.reflect_member("hello", hello);
	}

	bool init(cgv::render::context& ctx)
	{
		bool res = true;
		nd_handler* ndh = nd_handler::instance();

		switch (ndh->get_app_mode())
		{
		case nd_handler::BOTH_HANDS:
			hands.push_back(hand(NDAPISpace::LOC_LEFT_HAND));
			hands.push_back(hand(NDAPISpace::LOC_RIGHT_HAND));
			break;
		case nd_handler::LEFT_ONLY:
			hands.push_back(hand(NDAPISpace::LOC_LEFT_HAND));
			break;
		case nd_handler::RIGHT_ONLY:
			hands.push_back(hand(NDAPISpace::LOC_RIGHT_HAND));
			break;
		default:
			break;
		}
		//hand h(NDAPISpace::LOC_RIGHT_HAND);
		res = res && nd_handler::instance()->get_is_connected();

		cgv::render::ref_box_wire_renderer(ctx, 1);

		return res;
	}

	void draw(cgv::render::context& ctx)
	{
		//cgv::render::box_wire_renderer& renderer = cgv::render::ref_box_wire_renderer(ctx);
		//vector<cgv::render::render_types::vec3> extents = { .5, .5, .5 }, positions = { 0, 0, 0 };
		//vector<cgv::math::quaternion<float>> rots = { cgv::math::quaternion<float>(0, 0, 0, 0) };
		//renderer.set_position_array(ctx, positions);
		//renderer.set_extent_array(ctx, extents);
		//renderer.set_rotation_array(ctx, rots);

		//renderer.validate_and_enable(ctx);
		//glDrawArrays(GL_POINTS, 0, GLsizei(positions.size()));
		//renderer.disable(ctx);

		for each (hand h in hands)
		{
			h.draw(ctx);
		}
	}
};

cgv::base::object_registration<cgv_test> cgv_test_reg("");