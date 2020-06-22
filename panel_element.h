#pragma once

#include <cgv/render/drawable.h>
#include <set>

typedef cgv::render::render_types::vec3 vec3;
typedef cgv::render::render_types::quat quat;
typedef cgv::render::render_types::rgb rgb;

using namespace std;


class panel_node
{
protected:
	panel_node* parent;
	vector<panel_node*> children;

	vec3 position, extent, translation;
	quat rotation;
	rgb color;

	float height;
	bool has_been_touched;

	quat IDENTITY_QUAT = quat(1, 0, 0, 0);

public:
	panel_node() : panel_node(vec3(0), vec3(0), vec3(0), vec3(0), rgb(0), nullptr) {};

	// if a_extent.y() <= 0, the box height will be chosen slightly bigger than the parent's
	panel_node(vec3 a_position, vec3 a_extent, vec3 a_translation,
		vec3 angles, rgb a_color,
		panel_node* parent_ptr)
	{
		parent = parent_ptr;
		vec3 pos_parent = parent ? parent->position : vec3(0);
		position = a_position + pos_parent;

		if (a_extent.y() <= 0)
		{
			float height_parent = parent ? parent->height : .0099f;
			height = height_parent + .0001f;
			extent = vec3(a_extent.x(), height, a_extent.z()) + position;
		}
		else
		{
			height = a_extent.y();
			extent = a_extent + position;
		}

		vec3 t_parent = parent ? parent->translation : vec3(0);
		translation = a_translation + t_parent;

		float pi = 2 * acos(0.0);
		float rad_x = angles.x() * 2 * pi / 360;
		float rad_y = angles.y() * 2 * pi / 360;
		float rad_z = angles.z() * 2 * pi / 360;
		quat quat_x, quat_y, quat_z;
		quat_x = quat(cos(rad_x), sin(rad_x), .0f, .0f);
		quat_y = quat(cos(rad_y), .0f, sin(rad_y), .0f);
		quat_z = quat(cos(rad_z), .0f, .0f, sin(rad_z));
		const quat quat_parent = parent ? parent->rotation : IDENTITY_QUAT;
		rotation = quat_parent * quat_x * quat_z.inverse() * quat_y;

		color = a_color;

		if (parent) parent->children.push_back(this);
	};

	set<int> check_containments(vector<vec3> vecs)
	{
		set<int> result;

		for (size_t i = 0; i < vecs.size(); i++)
		{
			if (contains(vecs[i]))
			{
				result.insert(i);
				on_touch();
			}
		}

		for (size_t i = 0; i < children.size(); i++)
		{
			set<int> child_result = children[i]->check_containments(vecs);
			result.insert(child_result.begin(), child_result.end());
		}

		has_been_touched = result.size() > 0;

		return result;
	}

	bool contains(vec3 v)
	{
		v -= translation;
		rotation.inverse().rotate(v);

		return min(position.x(), extent.x()) <= v.x() && v.x() <= max(position.x(), extent.x())
			&& min(position.y(), extent.y()) <= v.y() && v.y() <= max(position.y(), extent.y())
			&& min(position.z(), extent.z()) <= v.z() && v.z() <= max(position.z(), extent.z());
	}

	virtual void on_touch() {};

	vector<vec3> get_positions_rec()
	{
		vector<vec3> result{ position };
		for (size_t i = 0; i < children.size(); i++)
		{
			vector<vec3> child_result = children[i]->get_positions_rec();
			for (size_t j = 0; j < child_result.size(); j++)
			{
				result.push_back(child_result[j]);
			}
		}

		return result;
	}

	vector<vec3> get_extents_rec()
	{
		vector<vec3> result{ extent };
		for (size_t i = 0; i < children.size(); i++)
		{
			vector<vec3> child_result = children[i]->get_extents_rec();
			for (size_t j = 0; j < child_result.size(); j++)
			{
				result.push_back(child_result[j]);
			}
		}

		return result;
	}

	vector<vec3> get_translations_rec()
	{
		vector<vec3> result{ translation };
		for (size_t i = 0; i < children.size(); i++)
		{
			vector<vec3> child_result = children[i]->get_translations_rec();
			for (size_t j = 0; j < child_result.size(); j++)
			{
				result.push_back(child_result[j]);
			}
		}

		return result;
	}

	vector<quat> get_rotations_rec()
	{
		vector<quat> result{ rotation };
		for (size_t i = 0; i < children.size(); i++)
		{
			vector<quat> child_result = children[i]->get_rotations_rec();
			for (size_t j = 0; j < child_result.size(); j++)
			{
				result.push_back(child_result[j]);
			}
		}

		return result;
	}

	vector<rgb> get_colors_rec()
	{
		vector<rgb> result{ color };
		for (size_t i = 0; i < children.size(); i++)
		{
			vector<rgb> child_result = children[i]->get_colors_rec();
			for (size_t j = 0; j < child_result.size(); j++)
			{
				result.push_back(child_result[j]);
			}
		}

		return result;
	}

	rgb get_color() { return color; }
	void set_color(rgb a_color) { color = a_color; }
};

class color_switch_button : public panel_node
{
public:
	using panel_node::panel_node;

	void on_touch()
	{
		if (!has_been_touched)
		{
			rgb tmp = color;
			color = parent->get_color();
			parent->set_color(tmp);
			has_been_touched = true;
		}
	}
};