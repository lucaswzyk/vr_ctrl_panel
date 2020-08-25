#pragma once

#include <cgv/render/drawable.h>

#include "space.h"

typedef cgv::render::render_types::vec3 vec3;
typedef cgv::render::render_types::quat quat;
typedef cgv::render::render_types::rgb rgb;

using namespace std;

struct geometry
{
	vec3 position, extent, translation;
	quat rotation;
	rgb color;
	bool has_changed;

	geometry()
		: position(0), extent(0), translation(0), rotation(vec3(1, 0, 0), 0), color(0), has_changed(true)
	{};

	geometry(vec3 a_position, vec3 a_extent, vec3 a_translation,
		vec3 a_angles, rgb a_color)
		: position(a_position), extent(a_extent), translation(a_translation), color(a_color)
	{
		quat quat_x, quat_y, quat_z;
		quat_x = quat(vec3(1, 0, 0), cgv::math::deg2rad(a_angles.x()));
		quat_y = quat(vec3(0, 1, 0), cgv::math::deg2rad(a_angles.y()));
		quat_z = quat(vec3(0, 0, 1), cgv::math::deg2rad(a_angles.z()));
		rotation = quat_z * quat_y * quat_x;
	};
};

struct group_geometry
{
	vector<vec3> positions, extents, translations;
	vector<quat> rotations;
	vector<rgb> colors;

	void push_back(geometry g)
	{
		positions.push_back(g.position);
		extents.push_back(g.extent);
		translations.push_back(g.translation);
		rotations.push_back(g.rotation);
		colors.push_back(g.color);
	}

	void append(group_geometry gg)
	{
		positions.insert(positions.end(), gg.positions.begin(), gg.positions.end());
		extents.insert(extents.end(), gg.extents.begin(), gg.extents.end());
		translations.insert(translations.end(), gg.translations.begin(), gg.translations.end());
		rotations.insert(rotations.end(), gg.rotations.begin(), gg.rotations.end());
		colors.insert(colors.end(), gg.colors.begin(), gg.colors.end());
	}
};

struct containment_info
{
	// positions of joints
	vector<vec3> positions;
	// indices of contained joints
	map<int, float> ind_map;
	// are joined: thumb+index, thumb+middle, palm+index, palm+middle
	bool contacts[4];
	// tolerance for containment check
	float tolerance;
};

class panel_node
{
protected:
	panel_node* parent;
	vector<panel_node*> children;

	geometry geo;
	group_geometry last_group_geo;

	vector<containment_info> cis;
	bool is_responsive;

public:
	panel_node() : panel_node(vec3(0), vec3(0), vec3(0), vec3(0), rgb(0), nullptr) {};

	// if a_extent.y() <= 0, the box height will be chosen slightly bigger than the parent's
	panel_node(vec3 a_position, vec3 a_extent, vec3 a_translation,
		vec3 a_angles, rgb a_color,
		panel_node* parent_ptr)
		: panel_node(
			geometry(a_position, a_extent, a_translation, a_angles, a_color), 
			parent_ptr
		)
	{};

	panel_node(geometry local_geo, panel_node* parent_ptr)
		: cis(2), is_responsive(true)
	{
		add_to_tree(parent_ptr);
		set_geometry(local_geo);
	};
	
	void add_to_tree(panel_node* parent_ptr)
	{
		parent = parent_ptr;
		if (parent) parent->children.push_back(this);
	}

	void set_geometry(vec3 a_position, vec3 a_extent, vec3 a_translation,
		vec3 a_angles, rgb a_color)
	{
		geometry tmp;
		tmp.position = a_position;
		tmp.extent = a_extent;
		tmp.translation = a_translation;

		quat quat_x, quat_y, quat_z;
		quat_x = quat(vec3(1, 0, 0), cgv::math::deg2rad(a_angles.x()));
		quat_y = quat(vec3(0, 1, 0), cgv::math::deg2rad(a_angles.y()));
		quat_z = quat(vec3(0, 0, 1), cgv::math::deg2rad(a_angles.z()));
		tmp.rotation = quat_z * quat_y * quat_x;

		tmp.color = a_color;

		set_geometry(tmp);
	}

	void set_geometry(geometry g)
	{
		geometry pgeo = parent ? parent->geo : geometry();

		pgeo.rotation.rotate(g.position);
		geo.position = g.position + pgeo.position + pgeo.translation;
		geo.extent = g.extent;

		geo.rotation = pgeo.rotation * g.rotation;

		geo.rotation.rotate(g.translation);
		geo.translation = g.translation;

		geo.extent = abs(g.extent);
		if (geo.extent.y() == 0)
		{
			geo.extent.y() = pgeo.extent.y() + .0001f;
		}
		geo.color = g.color;

		geo.has_changed = true;
	}

	group_geometry get_geometry_rec()
	{
		if (geometry_changed())
		{
			group_geometry group_geo;
			group_geo.push_back(geo);
			for (auto child : children)
			{
				group_geo.append(child->get_geometry_rec());
			}
			geo.has_changed = false;
			last_group_geo = group_geo;
		}

		return last_group_geo;
	}

	bool geometry_changed()
	{
		bool result = geo.has_changed;
		for (auto child : children)
		{
			result |= child->geometry_changed();
		}

		return result;
	}

	map<int, float> check_containments(containment_info ci, int hand_loc)
	{
		map<int, float> ind_map_cp = ci.ind_map;
		ci.ind_map = map<int, float>();
		for (size_t i = 0; i < ci.positions.size(); i++)
		{
			if (contains(ci.positions[i], ci.tolerance))
			{
				ci.ind_map[i] = get_vibration_strength();
				ind_map_cp[i] = ci.ind_map.count(i)
					? max(ci.ind_map[i], get_vibration_strength())
					: get_vibration_strength();
			}
		}

		for (auto child : children)
		{
			ci.ind_map = child->check_containments(ci, hand_loc);
		}

		cis[hand_loc] = ci;
		calc_responsiveness(ci);
		if (is_responsive && ci.ind_map.size())
		{
			on_touch(hand_loc);
		}
		return ind_map_cp;
	}

	virtual bool contains(vec3 v, float tolerance)
	{
		v = to_local(v);

		bool is_contained = -.5 * geo.extent.x() - tolerance <= v.x()
			&& -.5 * geo.extent.y() - tolerance <= v.y()
			&& -.5 * geo.extent.z() - tolerance <= v.z()
			&& v.x() <= .5 * geo.extent.x() + tolerance
			&& v.y() <= .5 * geo.extent.y() + tolerance
			&& v.z() <= .5 * geo.extent.z() + tolerance;

		return is_contained;
	}

	virtual void on_touch(int hand_loc) {};
	
	virtual float get_vibration_strength() { return .1f; }

	virtual void calc_responsiveness(containment_info ci) 
	{
		is_responsive = is_responsive && cis[0].ind_map.size() + cis[0].ind_map.size() == 1
			|| cis[0].ind_map.size() + cis[0].ind_map.size() == 0;
	}

	vec3 to_local(vec3 v)
	{
		v -= geo.position + geo.translation;
		geo.rotation.inverse().rotate(v);
		return v;
	}

	quat get_rotation() { return geo.rotation; }
	void set_color(rgb a_color) { 
		geo.color = a_color; 
		geo.has_changed = true;
	}
};

class slider : public panel_node
{
protected:
	float value, value_tolerance;
	space* sphere;
	void (*callback)(space*, float);
	rgb active_color;

	int NUM_INDICATOR_FIELDS = 7;
	float BORDER_FRAC = .1f, TOLERANCE_NUMERATOR = .1f;
	
public:
	slider(vec3 a_position, vec3 a_extent, vec3 a_translation,
		   vec3 angles, rgb base_color, rgb val_color,
		   space* a_sphere, void (*a_callback)(space*, float),
		   panel_node* parent_ptr)
	{
		add_to_tree(parent_ptr);
		set_geometry(a_position, a_extent, a_translation, angles, base_color);

		value = 0;
		sphere = a_sphere;
		callback = a_callback;
		value_tolerance = abs(TOLERANCE_NUMERATOR / (a_extent.z() - a_position.z()));
		active_color = val_color;

		float border = BORDER_FRAC * a_extent.x();
		float z_frac = a_extent.z() / NUM_INDICATOR_FIELDS;
		vec3 indicator_extent(a_extent.x() - border, 0, z_frac - border);

		float first_z = a_position.z() + .5f * (a_extent.z() - z_frac);
		for (size_t i = 0; i < NUM_INDICATOR_FIELDS; i++)
		{
			new panel_node(
				vec3(0, 0, first_z - i * z_frac),
				indicator_extent,
				vec3(0), vec3(0), base_color, this
			);
		}
	}

	void on_touch(int hand_loc) override
	{
		int touch_ind = cis[hand_loc].ind_map.begin()->first;
		float new_value = vec_to_val(cis[hand_loc].positions[touch_ind]);
		if (abs(new_value - value) < value_tolerance)
		{
			value = new_value;
			callback(sphere, value);
			set_indicator_colors();
		}
	}
	
	float vec_to_val(vec3 v)
	{
		float fraction = ((geo.position.z() + .5f * geo.extent.z()) - v.z()) / geo.extent.z();
	
		return min(1.0f, max(.0f, fraction));
	}
	
	void set_indicator_colors()
	{
		float val = value;
		float indicator_fields_frac = 1.0f / NUM_INDICATOR_FIELDS;
		for (size_t i = 0; i < children.size(); i++)
		{
			if (val > indicator_fields_frac)
			{
				children[i]->set_color(active_color);
				val -= indicator_fields_frac;
			}
			else if (value > 0)
			{
				val = val * NUM_INDICATOR_FIELDS;
				children[i]->set_color(val * active_color + (1 - val) * geo.color);
				val = .0f;
			}
			else
			{
				children[i]->set_color(geo.color);
			}
		}
	}
};

class pos_neg_slider : public panel_node
{
protected:
	float value, value_tolerance;
	space* sphere;
	void (*callback)(space*, float);
	rgb active_color;

	float z_frac;
	int NUM_INDICATOR_FIELDS = 7;
	float BORDER_FRAC = .1f, TOLERANCE_NUMERATOR = .1f;
	
public:
	pos_neg_slider(vec3 a_position, vec3 a_extent, vec3 a_translation,
		   vec3 angles, rgb base_color, rgb val_color,
		   space* a_sphere, void (*a_callback)(space*, float),
		   panel_node* parent_ptr)
	{
		add_to_tree(parent_ptr);
		set_geometry(a_position, a_extent, a_translation, angles, base_color);

		value = 0;
		sphere = a_sphere;
		callback = a_callback;
		value_tolerance = abs(TOLERANCE_NUMERATOR / (a_extent.z() - a_position.z()));
		active_color = val_color;

		float border = BORDER_FRAC * a_extent.x();
		z_frac = a_extent.z() / (2 * NUM_INDICATOR_FIELDS + 1);
		vec3 indicator_extent(a_extent.x() - border, 0, z_frac - border);

		for (size_t i = 0; i < NUM_INDICATOR_FIELDS; i++)
		{
			vec3 new_pos(0, 0, -(1.0f + i) * z_frac);
			new panel_node(
				new_pos,
				indicator_extent,
				vec3(0), vec3(0), base_color, this
			);
		}
		for (size_t i = 0; i < NUM_INDICATOR_FIELDS; i++)
		{
			new panel_node(
				vec3(0, 0, (1 + i) * z_frac),
				indicator_extent,
				vec3(0), vec3(0), base_color, this
			);
		}
	}

	void on_touch(int hand_loc) override
	{
		int touch_ind = cis[hand_loc].ind_map.begin()->first;
		float new_value = vec_to_val(cis[hand_loc].positions[touch_ind]);
		if (abs(new_value - value) < value_tolerance)
		{
			value = new_value;
			callback(sphere, value);
			set_indicator_colors();
		}
	}
	
	float vec_to_val(vec3 v)
	{
		v = to_local(v);
		float fraction = 0,
			num = .5f * (geo.extent.z() - z_frac);
		if (v.z() > .5f * z_frac)
		{
			fraction = (v.z() - .5f * z_frac) / num;
		}
		else if (v.z() < -.5f * z_frac)
		{
			fraction = (v.z() + .5f * z_frac) / num;
		}

		return -min(1.0f, max(-1.0f, fraction));
	}
	
	void set_indicator_colors()
	{
		float val = value;
		float indicator_fields_frac = 1.0f / NUM_INDICATOR_FIELDS;
		for (size_t i = 0; i < children.size(); i++)
		{
			children[i]->set_color(geo.color);
		}

		size_t start, stop;
		if (val > 0)
		{
			start = 0;
			stop = NUM_INDICATOR_FIELDS;
		}
		else 
		{
			start = NUM_INDICATOR_FIELDS;
			stop = 2 * NUM_INDICATOR_FIELDS;
			val = -val;
		}
		for (size_t i = start; i < stop; i++)
		{
			if (val > indicator_fields_frac)
			{
				children[i]->set_color(active_color);
				val -= indicator_fields_frac;
			}
			else if (value > 0)
			{
				val = val * NUM_INDICATOR_FIELDS;
				children[i]->set_color(val * active_color + (1 - val) * geo.color);
				val = .0f;
			}
			else
			{
				return;
			}
		}
	}
};

class lever : public panel_node
{
protected:
	float max_deflection, length, value;
	space* sphere;
	void (*callback)(space*, float);

	quat quat_yz;
	vector<geometry> child_geos;

public:
	// a_position - midpoint between lever arms
	// a_extent - lever length, handle width, thickness
	// angles - max rot. around own x in each direction, rot. around parent y, rot. around own z
	lever(vec3 position, vec3 extent, vec3 translation,
		vec3 angles, rgb color, 
		space* a_sphere, void (*a_callback)(space*, float),
		panel_node* parent_ptr)
	{
		max_deflection = cgv::math::deg2rad(angles.x());
		value = 0;
		sphere = a_sphere;
		callback = a_callback;

		quat_yz = quat(vec3(0, 0, 1), cgv::math::deg2rad(angles.z()))
			* quat(vec3(0, 1, 0), cgv::math::deg2rad(angles.y()));
		length = extent.y();
		extent.y() = extent.z();

		add_to_tree(parent_ptr);
		set_geometry(position, extent, translation, angles, color);

		child_geos = vector<geometry>{
			geometry(vec3(0), extent, vec3(.0f, length, .0f), vec3(0), color),
			geometry(vec3(-.5f * extent.x() + .5f * extent.y(), .0f, .0f), 
					 vec3(extent.y(), length, extent.y()), 
					 vec3(.0f, .5f * length, .0f), vec3(0), color),
			geometry(vec3(.5f * extent.x() - .5f * extent.y(), .0f, .0f),
					 vec3(extent.y(), length, extent.y()),
					 vec3(.0f, .5f * length, .0f), vec3(0), color)
		};
		new panel_node(child_geos[0], this);
		new panel_node(child_geos[1], this);
		new panel_node(child_geos[2], this);
	}

	void calc_responsiveness(containment_info ci) override
	{
		is_responsive = ci.contacts[2] && ci.contacts[3];
	}

	void on_touch(int hand_loc) override
	{
		geo.rotation = parent->get_rotation() * quat_yz;

		vec3 touch_loc = to_local(cis[hand_loc].positions[0]);
		touch_loc.x() = 0;
		touch_loc.normalize();
		vec3 cr = cross(vec3(0, 1, 0), touch_loc);

		float angle_x = min(max_deflection, asin(cr.length()));
		angle_x = cr.x() >= 0 ? angle_x : -angle_x;
		callback(sphere, .5f * (1 - angle_x / max_deflection));

		geo.rotation = parent->get_rotation() * quat_yz * quat(vec3(1, 0, 0), angle_x);
		update_children();
		geo.has_changed = true;
	}

	void update_children()
	{
		for (size_t i = 0; i < children.size(); i++)
		{
			children[i]->set_geometry(child_geos[i]);
		}
	}
};