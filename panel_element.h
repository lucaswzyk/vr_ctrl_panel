#pragma once

#include <cgv/render/drawable.h>

#include "space.h"

typedef cgv::render::render_types::vec3 vec3;
typedef cgv::render::render_types::quat quat;
typedef cgv::render::render_types::rgb rgb;

using namespace std;

// for a single box's geometry
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

// for multiple boxes' geometry
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

// for containment check
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
		vec3 a_angles, rgb a_color);

	void set_geometry(geometry g);

	// returns geometry of this element and its children
	virtual group_geometry get_geometry_rec();

	// returns true if geometry has changed 
	// since last get_geometry_rec() call
	bool geometry_changed();

	// returns a map of indices of contained ci.positions vs. vibration strength
	// saves ci as one of cis
	map<int, float> check_containments(containment_info ci, int hand_loc);

	virtual bool contains(vec3 v, float tolerance);

	virtual void on_touch(int hand_loc) {};
	virtual void on_no_touch() {};
	virtual float get_vibration_strength() { return .05f; }

	// sets is_responsive = true if this element should be responsive to touch
	virtual void calc_responsiveness(containment_info ci);

	// transforms v to this element's space
	vec3 to_local(vec3 v);

	quat get_rotation() { return geo.rotation; }

	void set_color(rgb a_color) { 
		geo.color = a_color; 
		geo.has_changed = true;
	}
};

// button that reacts to each touch exactly once
class button : public panel_node
{
protected:
	bool is_active;
	rgb base_color, active_color;
	space* s;
	void (*callback)(space*);

public:
	button(vec3 a_position, vec3 a_extent, vec3 a_translation,
		vec3 angles, rgb a_base_color, rgb a_active_color,
		space* a_space, void (*a_callback)(space*),
		panel_node* parent_ptr);

	virtual void on_touch(int hand_loc) override;
};

// button that is active as long as it is touched
class hold_button : public button
{
	using button::button;

	void calc_responsiveness(containment_info ci) override { is_responsive = true; }

	void on_touch(int hand_loc) override;

	void on_no_touch() override;
};

// one-directional slider
class slider : public panel_node
{
protected:
	float value, value_tolerance;
	space* s;
	void (*callback)(space*, float);
	rgb active_color;

	int NUM_INDICATOR_FIELDS = 7;
	float BORDER_FRAC = .1f, TOLERANCE_NUMERATOR = .1f;
	
public:
	slider(vec3 a_position, vec3 a_extent, vec3 a_translation,
		   vec3 angles, rgb base_color, rgb val_color,
		   space* a_space, void (*a_callback)(space*, float),
		panel_node* parent_ptr);

	void on_touch(int hand_loc) override;
	
	float vec_to_val(vec3 v);
	
	void set_indicator_colors();
};

// bi-directional slider
class pos_neg_slider : public panel_node
{
protected:
	float value, value_tolerance;
	space* sphere;
	void (*callback)(space*, float);
	rgb active_color;

	float z_frac;
	int NUM_INDICATOR_FIELDS = 7;
	float BORDER_FRAC = .05f, TOLERANCE_NUMERATOR = .1f;
	
public:
	pos_neg_slider(vec3 a_position, vec3 a_extent, vec3 a_translation,
		   vec3 angles, rgb base_color, rgb val_color,
		   space* a_sphere, void (*a_callback)(space*, float),
		panel_node* parent_ptr);

	void on_touch(int hand_loc) override;
	
	float vec_to_val(vec3 v);
	
	void set_indicator_colors();
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
		panel_node* parent_ptr);

	// responsive on grab (closed hand)
	void calc_responsiveness(containment_info ci) override { is_responsive = ci.contacts[2] && ci.contacts[3]; }

	void on_touch(int hand_loc) override;

	void update_children();
};