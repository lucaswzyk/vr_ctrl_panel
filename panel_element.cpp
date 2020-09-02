#include "panel_element.h"

void panel_node::set_geometry(vec3 a_position, vec3 a_extent, vec3 a_translation, vec3 a_angles, rgb a_color)
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

void panel_node::set_geometry(geometry g)
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

// returns geometry of this element and its children

group_geometry panel_node::get_geometry_rec()
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

// returns true if geometry has changed 
// since last get_geometry_rec() call

bool panel_node::geometry_changed()
{
	bool result = geo.has_changed;
	for (auto child : children)
	{
		result |= child->geometry_changed();
	}

	return result;
}

// returns a map of indices of contained ci.positions vs. vibration strength
// saves ci as one of cis

map<int, float> panel_node::check_containments(containment_info ci, int hand_loc)
{
	cis[hand_loc] = ci;
	map<int, float> ind_map;
	for (size_t i = 0; i < ci.positions.size(); i++)
	{
		if (contains(ci.positions[i], ci.tolerance))
		{
			ind_map[i] = get_vibration_strength();
		}
	}

	for (auto child : children)
	{
		map<int, float> child_map = child->check_containments(ci, hand_loc);
		for (auto p : child_map)
		{
			ind_map[p.first] = ind_map.count(p.first)
				? max(ind_map[p.first], get_vibration_strength())
				: get_vibration_strength();
		}
	}

	cis[hand_loc].ind_map = ind_map;
	calc_responsiveness(cis[hand_loc]);
	if (ind_map.size())
	{
		on_touch(hand_loc);
	}
	else
	{
		on_no_touch();
	}

	return ind_map;
}

bool panel_node::contains(vec3 v, float tolerance)
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

// sets is_responsive = true if this element should be responsive to touch

void panel_node::calc_responsiveness(containment_info ci)
{
	is_responsive = is_responsive && cis[0].ind_map.size() + cis[1].ind_map.size() == 1
		|| cis[0].ind_map.size() + cis[1].ind_map.size() == 0;
}

// transforms v to this element's space

vec3 panel_node::to_local(vec3 v)
{
	v -= geo.position + geo.translation;
	geo.rotation.inverse().rotate(v);
	return v;
}

button::button(vec3 a_position, vec3 a_extent, vec3 a_translation, vec3 angles, rgb a_base_color, rgb a_active_color, space* a_space, void(*a_callback)(space*), panel_node* parent_ptr)
{
	add_to_tree(parent_ptr);
	set_geometry(a_position, a_extent, a_translation, angles, a_base_color);

	s = a_space;
	callback = a_callback;
	base_color = a_base_color;
	active_color = a_active_color;
	is_active = false;
}

void button::on_touch(int hand_loc)
{
	if (is_responsive)
	{
		callback(s);
		is_active = !is_active;
		set_color(is_active ? active_color : base_color);
	}
	is_responsive = false;
}

void hold_button::on_touch(int hand_loc)
{
	if (is_responsive)
	{
		callback(s);
	}
	set_color(active_color);
}

void hold_button::on_no_touch()
{
	if (!cis[0].ind_map.size() && !cis[1].ind_map.size())
	{
		set_color(base_color);
	}
}

 slider::slider(vec3 a_position, vec3 a_extent, vec3 a_translation, vec3 angles, rgb base_color, rgb val_color, space* a_space, void(*a_callback)(space*, float), panel_node* parent_ptr)
{
	add_to_tree(parent_ptr);
	set_geometry(a_position, a_extent, a_translation, angles, base_color);

	value = 0;
	s = a_space;
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

 void slider::on_touch(int hand_loc)
 {
	 if (is_responsive)
	 {
		 int touch_ind = cis[hand_loc].ind_map.begin()->first;
		 float new_value = vec_to_val(cis[hand_loc].positions[touch_ind]);
		 if (abs(new_value - value) < value_tolerance)
		 {
			 value = new_value;
			 callback(s, value);
			 set_indicator_colors();
		 }
	 }
 }

 float slider::vec_to_val(vec3 v)
 {
	 float fraction = ((geo.position.z() + .5f * geo.extent.z()) - v.z()) / geo.extent.z();

	 return min(1.0f, max(.0f, fraction));
 }

 void slider::set_indicator_colors()
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

 pos_neg_slider::pos_neg_slider(vec3 a_position, vec3 a_extent, vec3 a_translation, vec3 angles, rgb base_color, rgb val_color, space* a_sphere, void(*a_callback)(space*, float), panel_node* parent_ptr)
 {
	 add_to_tree(parent_ptr);
	 set_geometry(a_position, a_extent, a_translation, angles, base_color);

	 value = 0;
	 sphere = a_sphere;
	 callback = a_callback;
	 value_tolerance = abs(TOLERANCE_NUMERATOR / (a_extent.z() - a_position.z()));
	 active_color = val_color;

	 float border = BORDER_FRAC * a_extent.x();
	 z_frac = a_extent.z() / (2 * NUM_INDICATOR_FIELDS + 3);
	 vec3 indicator_extent(a_extent.x() - border, 0, z_frac - border);

	 for (size_t i = 0; i < NUM_INDICATOR_FIELDS; i++)
	 {
		 vec3 new_pos(0, 0, -(2.0f + i) * z_frac);
		 new panel_node(
			 new_pos,
			 indicator_extent,
			 vec3(0), vec3(0), base_color, this
		 );
	 }
	 for (size_t i = 0; i < NUM_INDICATOR_FIELDS; i++)
	 {
		 new panel_node(
			 vec3(0, 0, (2.0f + i) * z_frac),
			 indicator_extent,
			 vec3(0), vec3(0), base_color, this
		 );
	 }
	 vec3 zero_button_extent(a_extent.x() - border, 0, 2 * z_frac - border);
	 new panel_node(
		 vec3(0),
		 zero_button_extent,
		 vec3(0), vec3(0), active_color, this
	 );
 }

 void pos_neg_slider::on_touch(int hand_loc)
 {
	 if (is_responsive)
	 {
		 int touch_ind = cis[hand_loc].ind_map.begin()->first;
		 value = vec_to_val(cis[hand_loc].positions[touch_ind]);
		 callback(sphere, value);
		 set_indicator_colors();
	 }
 }

 float pos_neg_slider::vec_to_val(vec3 v)
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

 void pos_neg_slider::set_indicator_colors()
 {
	 float val = value;
	 float indicator_fields_frac = 1.0f / NUM_INDICATOR_FIELDS;
	 for (size_t i = 0; i < 2 * NUM_INDICATOR_FIELDS; i++)
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

 // a_position - midpoint between lever arms
 // a_extent - lever length, handle width, thickness
 // angles - max rot. around own x in each direction, rot. around parent y, rot. around own z

 lever::lever(vec3 position, vec3 extent, vec3 translation, vec3 angles, rgb color, space* a_sphere, void(*a_callback)(space*, float), panel_node* parent_ptr)
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

 void lever::on_touch(int hand_loc)
 {
	 if (!is_responsive)
	 {
		 return;
	 }
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

 void lever::update_children()
 {
	 for (size_t i = 0; i < children.size(); i++)
	 {
		 children[i]->set_geometry(child_geos[i]);
	 }
 }
