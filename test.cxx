#include "NDAPI.h"
#include "stdafx.h"
#include <iostream>

#include <cgv/base/register.h>
#include <cgv/render/drawable.h>

#include "nd_handler.h"
#include "hand.h"

using namespace std;

extern "C"
{
	__declspec(dllexport) int AmdPowerXpressRequestHighPerformance = 1;
}

class cgv_test
	: public cgv::base::base,
	  public cgv::render::drawable,
	  public cgv::gui::event_handler
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
		cgv::gui::connect_vr_server(false);
		bool res = true;
		nd_handler& ndh = nd_handler::instance();

		switch (ndh.get_app_mode())
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
		res = res && nd_handler::instance().get_is_connected();

		//cgv::render::ref_rounded_cone_renderer(ctx, 1);
		cgv::render::ref_sphere_renderer(ctx, 1);

		//cgv::gui::connect_vr_server(true);

		return res;
	}

	void draw(cgv::render::context& ctx)
	{
		for each (hand h in hands)
		{
			h.draw(ctx);
		}
	}

	void clear(cgv::render::context& ctx)
	{
		//cgv::render::ref_rounded_cone_renderer(ctx, -1);
		cgv::render::ref_sphere_renderer(ctx, -1);
	}

	// Inherited via event_handler
	virtual bool handle(cgv::gui::event& e) override
	{
		if ((e.get_flags() & cgv::gui::EF_VR) == 0)
		{
			return false;
		}

		if (e.get_kind() == cgv::gui::EID_POSE)
		{
			cgv::gui::vr_pose_event& vrpe = static_cast<cgv::gui::vr_pose_event&>(e);
			cgv::render::render_types::vec3 origin = vrpe.get_position();
			hands[0].set_origin(origin);
			return true;
		}

		return false;
	}

	virtual void stream_help(std::ostream& os) override
	{
	}
};

cgv::base::object_registration<cgv_test> cgv_test_reg("");