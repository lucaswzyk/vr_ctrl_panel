#include "NDAPI.h"
#include "stdafx.h"
#include "test_class.h"
#include <iostream>

#include <cgv/base/register.h>
#include <cgv/render/drawable.h>
#include <cgv_gl/box_wire_renderer.h>
#include <cgv_gl/gl/gl.h>

using namespace std;

class cgv_test
	: public cgv::base::base,
	  public cgv::render::drawable
{
protected:
	string hello;
	//test_class tc;

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
		print_test();
		return rh.reflect_member("hello", hello);
	}

	int print_test()
	{
		//NDAPISpace::NDAPI* nd = new NDAPISpace::NDAPI();
		//int stat = nd.connectToServer();

		//int num_dev = nd.getNumberOfDevices();

		//cout << endl << num_dev << endl;

		return 0;
	}

	bool init(cgv::render::context& ctx)
	{
		cgv::render::ref_box_wire_renderer(ctx, 1);

		return true;
	}

	void draw(cgv::render::context& ctx)
	{
		cgv::render::box_wire_renderer& renderer = cgv::render::ref_box_wire_renderer(ctx);
		vector<cgv::render::render_types::vec3> extents = { .5, .5, .5 }, positions = { 0, 0, 0 };
		renderer.set_position_array(ctx, positions);
		renderer.set_extent_array(ctx, extents);
		renderer.validate_and_enable(ctx);

		glDrawArrays(GL_POINTS, 0, GLsizei(positions.size()));
		renderer.disable(ctx);
	}
};

cgv::base::object_registration<cgv_test> cgv_test_reg("");