#pragma once

#include <cgv/media/mesh/simple_mesh.h>
#include <cgv_gl/gl/mesh_render_info.h>

using namespace cgv::render;

const char* bridge_mesh_file = "bridge_cleaned.obj";
render_types::mat4 bridge_view_mat = cgv::math::rotate4(vec3(0, 180, 0));

class mesh
{
protected:
	mesh_render_info mri;
	CullingMode cull_mode = CM_OFF;
	IlluminationMode illu_mode = IM_ONE_SIDED;

public:
	bool init(context& ctx)
	{
		bool res = false;
		cgv::media::mesh::simple_mesh<> m;
		if (m.read(bridge_mesh_file))
		{
			if (!m.has_normals())
			{
				m.compute_vertex_normals();
			}
			mri.destruct(ctx);
			mri.construct(ctx, m);
			mri.bind(ctx, ctx.ref_surface_shader_program(true), true);
			res = true;
		}

		return res;
	}

	void draw(context& ctx)
	{
		if (!mri.is_constructed())
		{
			cout << "Trying to render unconstructed mesh. Aborting..." << endl;
			return;
		}

		ctx.push_modelview_matrix();
		ctx.mul_modelview_matrix(bridge_view_mat);
		GLboolean is_culling = glIsEnabled(GL_CULL_FACE);
		GLint cull_face;
		glGetIntegerv(GL_CULL_FACE_MODE, &cull_face);

		if (cull_mode > 0)
		{
			glEnable(GL_CULL_FACE);
			glCullFace(cull_mode == CM_BACKFACE ? GL_BACK : GL_FRONT);
		}
		else
		{
			glDisable(GL_CULL_FACE);
		}
		shader_program& prog = ctx.ref_surface_shader_program(true);
		prog.set_uniform(ctx, "culling_mode", (int)cull_mode);
		prog.set_uniform(ctx, "illumination_mode", (int)illu_mode);
		mri.draw_all(ctx);

		if (is_culling)
		{
			glEnable(GL_CULL_FACE);
		}
		else
		{
			glDisable(GL_CULL_FACE);
		}
		glCullFace(cull_face);
		ctx.pop_modelview_matrix();
	}

	void destruct(context& ctx)
	{
		mri.destruct(ctx);
	}
};