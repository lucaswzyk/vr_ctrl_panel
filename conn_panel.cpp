#include "conn_panel.h"
//
//#pragma once
//
//#include <cgv/render/drawable.h>
//
//typedef cgv::render::render_types::vec3 vec3;
//typedef cgv::render::render_types::quat quat;
//typedef cgv::render::render_types::rgb rgb;
//
//using namespace std;
//
//class panel_node
//{
//protected:
//	panel_node* parent;
//	vector<panel_node*> children;
//
//	vector<vec3> positions, extents, translations;
//	vector<quat> rotations;
//	vector<rgb> colors;
//
//public:
//	void set_parent(panel_node* p)
//	{
//		parent = p;
//	}
//
//	void add_child(panel_node* c)
//	{
//		c->set_parent(this);
//		children.push_back(c);
//	}
//
//	virtual vector<vec3> get_positions()
//	{
//		return positions;
//	}
//
//	virtual vector<vec3> get_extents()
//	{
//		return extents;
//	}
//
//	virtual vector<quat> get_rotations()
//	{
//		return rotations;
//	}
//
//	virtual vector<vec3> get_translations()
//	{
//		return translations;
//	}
//
//	virtual vector<rgb> get_colors()
//	{
//		return colors;
//	}
//
//	virtual bool contains(vec3 v)
//	{
//		return false;
//	}
//};
//
//class panel_surface : public panel_node
//{
//	float width, height, BOX_THICKNESS = 0.01f;
//	float angle_x, angle_y, angle_z;
//	float ty, tz;
//
//public:
//	panel_surface(float width, float height,
//		float angle_x, float angle_y, float angle_z,
//		float ty, float tz)
//		: width(width), height(height),
//		angle_x(angle_x), angle_y(angle_y), angle_z(angle_z),
//		ty(ty), tz(tz)
//	{
//		set_geometry();
//	};
//
//	void set_geometry()
//	{
//		float pi = 2 * acos(0.0);
//		float rad_x = angle_x * 2 * pi / 360;
//		float rad_y = angle_y * 2 * pi / 360;
//		float rad_z = angle_z * 2 * pi / 360;
//		positions = vector<vec3>{ vec3(0, 0, 0), vec3(0, 0, 0) };
//		extents = vector<vec3>{ vec3(-width, BOX_THICKNESS, height), vec3(width, BOX_THICKNESS, height) };
//
//		quat quat_x, quat_y, quat_z, left_quat, right_quat;
//		quat_x = quat(cos(rad_x), sin(rad_x), .0f, .0f);
//		quat_y = quat(cos(rad_y), .0f, sin(rad_y), .0f);
//		quat_z = quat(cos(rad_z), .0f, .0f, sin(rad_z));
//		left_quat = quat_x * quat_z.inverse() * quat_y;
//		right_quat = quat_x * quat_z * quat_y.inverse();
//		rotations = vector<quat>{ left_quat, right_quat };
//
//		translations = vector<vec3>{ vec3(0, ty, tz), vec3(0, ty, tz) };
//		colors = vector<rgb>{ rgb(1, 0, 0), rgb(1, 0, 0) };
//	}
//
//	vector<vec3> get_positions()
//	{
//		vector<vec3> result = positions;
//
//		for (size_t i = 0; i < children.size(); i++)
//		{
//			vector<vec3> c_pos = children[i]->get_positions();
//			for (size_t j = 0; j < c_pos.size(); j++)
//			{
//				result.push_back(c_pos[j] + positions[c_pos[j].x() <= 0 ? 0 : 1]);
//			}
//		}
//
//		return result;
//	}
//
//	vector<vec3> get_extents()
//	{
//		vector<vec3> result = extents;
//
//		for (size_t i = 0; i < children.size(); i++)
//		{
//			vector<vec3> c_pos = children[i]->get_positions();
//			vector<vec3> c_ext = children[i]->get_extents();
//			for (size_t j = 0; j < c_ext.size(); j++)
//			{
//				vec3 pos = c_pos[j] + positions[c_pos[j].x() <= 0 ? 0 : 1];
//				result.push_back(c_ext[j] + pos);
//			}
//		}
//
//		return result;
//	}
//
//	vector<vec3> get_translations()
//	{
//		vector<vec3> result = translations;
//
//		for (size_t i = 0; i < children.size(); i++)
//		{
//			vector<vec3> c_trans = children[i]->get_translations();
//			for (size_t j = 0; j < c_trans.size(); j++)
//			{
//				result.push_back(c_trans[j] + translations[0]);
//			}
//		}
//
//		return result;
//	}
//
//	vector<quat> get_rotations()
//	{
//		vector<quat> result = rotations;
//
//		for (size_t i = 0; i < children.size(); i++)
//		{
//			vector<vec3> c_pos = children[i]->get_positions();
//			vector<quat> c_rot = children[i]->get_rotations();
//			for (size_t j = 0; j < c_rot.size(); j++)
//			{
//				vec3 v = c_pos[j];
//				int side = v.x() <= 0 ? 0 : 1;
//				result.push_back(rotations[side] * c_rot[j]);
//			}
//		}
//
//		return result;
//	}
//
//	vector<rgb> get_colors()
//	{
//		vector<rgb> result = colors;
//
//		for (size_t i = 0; i < children.size(); i++)
//		{
//			vector<rgb> c_col = children[i]->get_colors();
//			for (size_t j = 0; j < c_col.size(); j++)
//			{
//				result.push_back(c_col[j]);
//			}
//		}
//
//		return result;
//	}
//
//	bool contains(vec3 v)
//	{
//		v -= translations[0];
//		if (v.x() <= 0)
//		{
//			rotations[0].inverse().rotate(v);
//			return v.x() <= 0 && v.x() >= extents[0].x()
//				&& v.y() >= 0 && v.y() <= extents[0].y()
//				&& v.z() >= 0 && v.z() <= extents[0].z();
//		}
//		else
//		{
//			rotations[1].inverse().rotate(v);
//			return v.x() >= 0 && v.x() <= extents[0].x()
//				&& v.y() >= 0 && v.y() <= extents[0].y()
//				&& v.z() >= 0 && v.z() <= extents[0].z();
//		}
//	}
//};
//
//class button : public panel_node
//{
//	float width, height;
//	float x_on_parent, y_on_parent;
//	float BUTTON_THICKNESS = .011f;
//
//public:
//	button(float awidth, float aheight, float a_x_on_parent, float a_y_on_parent, rgb color)
//		: width(awidth), height(aheight), x_on_parent(a_x_on_parent), y_on_parent(a_y_on_parent)
//	{
//		set_geometry();
//		colors.push_back(color);
//	};
//
//	void set_geometry()
//	{
//		positions = vector<vec3>(1, vec3(x_on_parent, 0, y_on_parent));
//		extents = vector<vec3>(1, vec3(width, BUTTON_THICKNESS, height));
//		rotations = vector<quat>(1, quat(1, 0, 0, 0));
//		translations = vector<vec3>(1, vec3(0, 0, 0));
//	}
//};
//
