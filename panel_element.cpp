//#include "panel_element.h"
//#pragma once
//
//#include <cgv/render/drawable.h>
//#include <set>
//
//typedef cgv::render::render_types::vec3 vec3;
//typedef cgv::render::render_types::quat quat;
//typedef cgv::render::render_types::rgb rgb;
//
//using namespace std;
//
//
//class panel_node
//{
//protected:
//	panel_node* parent;
//	vector<panel_node*> children;
//
//	vec3 position, extent, translation;
//	quat rotation;
//	rgb color;
//
//	float height;
//
//	bool has_been_touched;
//
//	quat IDENTITY_QUAT = quat(vec3(1, 0, 0), 0);
//
//public:
//	panel_node() : panel_node(vec3(0), vec3(0), vec3(0), vec3(0), rgb(0), nullptr) {};
//
//	// if a_extent.y() <= 0, the box height will be chosen slightly bigger than the parent's
//	panel_node(vec3 a_position, vec3 a_extent, vec3 a_translation,
//		vec3 angles, rgb a_color,
//		panel_node* parent_ptr)
//	{
//		add_to_tree(parent_ptr);
//		set_geometry(a_position, a_extent, a_translation, angles, a_color);
//	};
//
//	void add_to_tree(panel_node* parent_ptr)
//	{
//		parent = parent_ptr;
//		if (parent) parent->children.push_back(this);
//	}
//
//	void set_geometry(vec3 a_position, vec3 a_extent, vec3 a_translation,
//		vec3 angles, rgb a_color)
//	{
//		const quat rot_parent = parent ? parent->rotation : quat(vec3(1, 0, 0), 0);
//		quat quat_x, quat_y, quat_z;
//		quat_x = quat(vec3(1, 0, 0), cgv::math::deg2rad(angles.x()));
//		quat_y = quat(vec3(0, 1, 0), cgv::math::deg2rad(angles.y()));
//		quat_z = quat(vec3(0, 0, 1), cgv::math::deg2rad(angles.z()));
//		rotation = rot_parent * quat_z * quat_y * quat_x;
//
//		rot_parent.rotate(a_position);
//		vec3 pos_parent = parent ? parent->position : vec3(0);
//		position = a_position + pos_parent;
//
//		if (a_extent.y() <= 0)
//		{
//			float height_parent = parent ? parent->height : .0099f;
//			height = height_parent + .0001f;
//			extent = vec3(a_extent.x(), height, a_extent.z());
//			extent.abs();
//		}
//		else
//		{
//			height = a_extent.y();
//		}
//
//		vec3 t_parent = parent ? parent->translation : vec3(0);
//		translation = a_translation + t_parent;
//
//		color = a_color;
//	}
//
//	virtual set<int> check_containments(vector<vec3> vecs, float tolerance)
//	{
//		set<int> result;
//
//		for (size_t i = 0; i < vecs.size(); i++)
//		{
//			if (contains(vecs[i], tolerance))
//			{
//				result.insert(i);
//			}
//		}
//
//		for (size_t i = 0; i < children.size(); i++)
//		{
//			set<int> child_result = children[i]->check_containments(vecs, tolerance);
//			result.insert(child_result.begin(), child_result.end());
//		}
//
//		if (result.size() > 0) on_touch();
//		has_been_touched = result.size() > 0;
//
//		return result;
//	}
//
//	virtual bool contains(vec3 v, float tolerance)
//	{
//		v -= position + translation;
//		rotation.inverse().rotate(v);
//
//		bool is_contained = -.5 * extent.x() - tolerance <= v.x()
//			&& -.5 * extent.y() - tolerance <= v.y()
//			&& -.5 * extent.z() - tolerance <= v.z()
//			&& v.x() <= .5 * extent.x() + tolerance
//			&& v.y() <= .5 * extent.y() + tolerance
//			&& v.z() <= .5 * extent.z() + tolerance;
//
//		return is_contained;
//	}
//
//	virtual void on_touch() {};
//
//	vector<vec3> get_positions_rec()
//	{
//		vector<vec3> result{ position };
//		for (size_t i = 0; i < children.size(); i++)
//		{
//			vector<vec3> child_result = children[i]->get_positions_rec();
//			for (size_t j = 0; j < child_result.size(); j++)
//			{
//				result.push_back(child_result[j]);
//			}
//		}
//
//		return result;
//	}
//
//	vector<vec3> get_extents_rec()
//	{
//		vector<vec3> result{ extent };
//		for (size_t i = 0; i < children.size(); i++)
//		{
//			vector<vec3> child_result = children[i]->get_extents_rec();
//			for (size_t j = 0; j < child_result.size(); j++)
//			{
//				result.push_back(child_result[j]);
//			}
//		}
//
//		return result;
//	}
//
//	vector<vec3> get_translations_rec()
//	{
//		vector<vec3> result{ translation };
//		for (size_t i = 0; i < children.size(); i++)
//		{
//			vector<vec3> child_result = children[i]->get_translations_rec();
//			for (size_t j = 0; j < child_result.size(); j++)
//			{
//				result.push_back(child_result[j]);
//			}
//		}
//
//		return result;
//	}
//
//	vector<quat> get_rotations_rec()
//	{
//		vector<quat> result{ rotation };
//		for (size_t i = 0; i < children.size(); i++)
//		{
//			vector<quat> child_result = children[i]->get_rotations_rec();
//			for (size_t j = 0; j < child_result.size(); j++)
//			{
//				result.push_back(child_result[j]);
//			}
//		}
//
//		return result;
//	}
//
//	vector<rgb> get_colors_rec()
//	{
//		vector<rgb> result{ color };
//		for (size_t i = 0; i < children.size(); i++)
//		{
//			vector<rgb> child_result = children[i]->get_colors_rec();
//			for (size_t j = 0; j < child_result.size(); j++)
//			{
//				result.push_back(child_result[j]);
//			}
//		}
//
//		return result;
//	}
//
//	rgb get_color() { return color; }
//	void set_color(rgb a_color) { color = a_color; }
//};
//
//class color_switch_button : public panel_node
//{
//public:
//	using panel_node::panel_node;
//
//	void on_touch()
//	{
//		if (!has_been_touched)
//		{
//			rgb tmp = color;
//			color = parent->get_color();
//			parent->set_color(tmp);
//			has_been_touched = true;
//		}
//	}
//};
//
//class slider : public panel_node
//{
//protected:
//	float value, value_tolerance, controlled_val_max;
//	float* controlled_val;
//	rgb active_color;
//
//	vector<vec3> last_contained_pos;
//
//	int NUM_INDICATOR_FIELDS = 7;
//	float BORDER_FRAC = .1f, TOLERANCE_NUMERATOR = .1f;
//
//public:
//	// needs to be constructed with position on bottom left and extent to top right corner
//	slider(vec3 a_position, vec3 a_extent, vec3 a_translation,
//		vec3 angles, rgb base_color, rgb val_color,
//		float* a_controlled_val, float a_controlled_val_max,
//		panel_node* parent_ptr)
//	{
//		add_to_tree(parent_ptr);
//		set_geometry(a_position, a_extent, a_translation, angles, base_color);
//
//		value = 0;
//		controlled_val = a_controlled_val;
//		controlled_val_max = a_controlled_val_max;
//		value_tolerance = abs(TOLERANCE_NUMERATOR / (a_extent.z() - a_position.z()));
//		active_color = val_color;
//
//		float border = BORDER_FRAC * a_extent.x();
//		float z_frac = a_extent.z() / NUM_INDICATOR_FIELDS;
//		vec3 indicator_extent(a_extent.x() - border, 0, z_frac - border);
//
//		float first_z = a_position.z() + .5f * (a_extent.z() - z_frac);
//		for (size_t i = 0; i < NUM_INDICATOR_FIELDS; i++)
//		{
//			new panel_node(
//				vec3(0, 0, first_z - i * z_frac),
//				indicator_extent,
//				vec3(0), vec3(0), base_color, this
//			);
//		}
//	}
//
//	void on_touch()
//	{
//		//cout << "contained" << endl;
//		if (last_contained_pos.size() >= 1)
//		{
//			float new_value = vec_to_val(last_contained_pos[0]);
//			if (abs(new_value - value) < value_tolerance)
//			{
//				value = new_value;
//				*controlled_val = controlled_val_max * value;
//				set_indicator_colors();
//			}
//		}
//	}
//
//	float vec_to_val(vec3 v)
//	{
//		float fraction = ((position.z() + .5f * extent.z()) - v.z()) / extent.z();
//
//		return min(1.0f, max(.0f, fraction));
//	}
//
//	void set_indicator_colors()
//	{
//		float val = value;
//		float indicator_fields_frac = 1.0f / NUM_INDICATOR_FIELDS;
//		for (size_t i = 0; i < children.size(); i++)
//		{
//			if (val > indicator_fields_frac)
//			{
//				children[i]->set_color(active_color);
//				val -= indicator_fields_frac;
//			}
//			else if (value > 0)
//			{
//				val = val * NUM_INDICATOR_FIELDS;
//				children[i]->set_color(val * active_color + (1 - val) * color);
//				val = .0f;
//			}
//			else
//			{
//				children[i]->set_color(color);
//			}
//		}
//	}
//
//	virtual bool contains(vec3 v, float tolerance) override
//	{
//		v -= position + translation;
//		rotation.inverse().rotate(v);
//
//		bool is_contained = -.5 * extent.x() - tolerance <= v.x()
//			&& -.5 * extent.y() - tolerance <= v.y()
//			&& -.5 * extent.z() - tolerance <= v.z()
//			&& v.x() <= .5 * extent.x() + tolerance
//			&& v.y() <= .5 * extent.y() + tolerance
//			&& v.z() <= .5 * extent.z() + tolerance;
//
//		if (is_contained) last_contained_pos.push_back(v);
//
//		return is_contained;
//	}
//
//	virtual set<int> check_containments(vector<vec3> vecs, float tolerance) override
//	{
//		last_contained_pos.clear();
//		return panel_node::check_containments(vecs, tolerance);
//	}
//};
//
//class lever
//	: public panel_node
//{
//protected:
//	float max_deflection;
//
//public:
//	// a_position - midpoint between lever arms
//	// a_extent - lever length, handle width, thickness
//	// angles - max rot. around own x in each direction, rot. around parent y, rot. around own z
//	lever(vec3 a_position, vec3 a_extent, vec3 a_translation,
//		vec3 angles, rgb base_color, rgb val_color,
//		float* a_controlled_val, float a_controlled_val_max,
//		panel_node* parent_ptr)
//	{
//		add_to_tree(parent_ptr);
//		max_deflection = angles.x();
//		a_position.y() += a_extent.y();
//		set_geometry(a_position, a_extent, a_translation, angles, base_color);
//
//		// add handle
//		new panel_node()
//	}
//};