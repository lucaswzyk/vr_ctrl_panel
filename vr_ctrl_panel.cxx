#include "vr_ctrl_panel.h"

bool vr_ctrl_panel::init(context& ctx)
{
	bool res = true;

	cgv::gui::connect_vr_server(false);
	nd_handler& ndh = nd_handler::instance();
	res = res && nd_handler::instance().get_is_connected();

	//cgv::render::ref_rounded_cone_renderer(ctx, 1);
	cgv::render::ref_box_renderer(ctx, 1);
	cgv::render::ref_sphere_renderer(ctx, 1);

	switch (ndh.get_app_mode())
	{
	case nd_handler::BOTH_HANDS:
		hands.push_back(new hand(NDAPISpace::LOC_RIGHT_HAND));
		existing_hand_locs.push_back(NDAPISpace::LOC_RIGHT_HAND);
		hands.push_back(new hand(NDAPISpace::LOC_LEFT_HAND));
		existing_hand_locs.push_back(NDAPISpace::LOC_LEFT_HAND);
		break;
	case nd_handler::LEFT_ONLY:
		hands.push_back(nullptr);
		hands.push_back(new hand(NDAPISpace::LOC_LEFT_HAND));
		existing_hand_locs.push_back(NDAPISpace::LOC_LEFT_HAND);
		break;
	case nd_handler::RIGHT_ONLY:
		hands.push_back(new hand(NDAPISpace::LOC_RIGHT_HAND));
		existing_hand_locs.push_back(NDAPISpace::LOC_RIGHT_HAND);
		hands.push_back(nullptr);
		break;
	default:
		break;
	}
	hand_translations = vector<vec3>(hands.size(), vec3(0));
	c.model_view_mat.identity();
	c.world_to_model.identity();
	c.bridge_view_mat = rotate4(vec3(0, 180, 0));

	c.stage = NOT_CALIBRATING;

	return res;
}

inline void vr_ctrl_panel::draw(cgv::render::context& ctx)
{
	ctx.push_modelview_matrix();
	ctx.mul_modelview_matrix(c.model_view_mat);

	//auto t0 = std::chrono::steady_clock::now();
	for (auto loc : existing_hand_locs)
	{
		hands[loc]->update_and_draw(ctx, panel, hand_translations[loc], hand_scale);
	}
	//auto t1 = std::chrono::steady_clock::now();

	if (c.render_bridge)
	{
		bridge.draw(ctx);
	}

	if (c.render_panel)
	{
		panel.draw(ctx);
	}
	auto t2 = std::chrono::steady_clock::now();
	/*cout << "hand: " << std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0).count() << endl;
	cout << "mesh: " << std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1).count() << endl << endl;*/
	ctx.pop_modelview_matrix();
}

inline void vr_ctrl_panel::destruct(context& ctx)
{
	cgv::render::ref_rounded_cone_renderer(ctx, -1);
	cgv::render::ref_sphere_renderer(ctx, -1);
	cgv::render::ref_box_renderer(ctx, -1);
	bridge.destruct(ctx);
}

// Inherited via event_handler

inline bool vr_ctrl_panel::handle(cgv::gui::event& e)
{
	if ((e.get_flags() & cgv::gui::EF_VR) == 0)
	{
		return false;
	}

	if (e.get_kind() == cgv::gui::EID_POSE)
	{
		cgv::gui::vr_pose_event& vrpe = static_cast<cgv::gui::vr_pose_event&>(e);
		int t_id = vrpe.get_trackable_index();
		if (t_id == -1) return false;

		if (!c.tr_assign.size())
		{
			assign_trackers(vrpe.get_state());
		}
		else if (!c.tr_assign.count(t_id))
		{
			reset_tracker_assigns();
			assign_trackers(vrpe.get_state());
		}

		vec3 position = inhom_pos(c.world_to_model * hom(vrpe.get_position()));
		if (c.tr_assign.size() && c.tr_assign.count(t_id))
		{
			int tracker_loc = c.tr_assign[t_id];
			hand_translations[tracker_loc] = position;
			update_calibration(vrpe.get_state(), t_id);
		}

		return true;
	}

	return false;
}

inline vec3 vr_ctrl_panel::position_from_pose(const float pose[12])
{
	return vec3(pose[9], pose[10], pose[11]);
}

inline vr_ctrl_panel::vec4 vr_ctrl_panel::hom_pos(vec3 v) { return vec4(v.x(), v.y(), v.z(), 1.0f); }

inline vr_ctrl_panel::vec4 vr_ctrl_panel::hom_pos(const float pose[12]) { return hom_pos(position_from_pose(pose)); }

inline vec3 vr_ctrl_panel::inhom_pos(vec4 v) { return vec3(v.x(), v.y(), v.z()) / v.w(); }

inline vr_ctrl_panel::vec4 vr_ctrl_panel::hom_dir(vec3 dir) { return vec4(dir.x(), dir.y(), dir.z(), .0f); }

inline vec3 vr_ctrl_panel::inhom_dir(vec4 dir) { return vec3(dir.x(), dir.y(), dir.z()); }

// Inherited via provider

inline void vr_ctrl_panel::create_gui()
{
	add_member_control(this, "render hands", c.render_hands, "toggle");
	add_member_control(this, "render panel", c.render_panel, "toggle");
	add_member_control(this, "render bridge", c.render_bridge, "toggle");
	add_member_control(this, "load bridge mesh", c.load_bridge, "toggle");
	cgv::signal::connect_copy(add_button("reassign trackers")->click, rebind(this, &vr_ctrl_panel::reset_tracker_assigns));

	for (auto loc : existing_hand_locs)
	{
		string loc_string = loc == NDAPISpace::LOC_RIGHT_HAND ? "right" : "left";
		add_member_control(this, loc_string, *(hands[loc]->get_man_ack()));
	}
}

void vr_ctrl_panel::update_calibration(vr::vr_kit_state state, int t_id)
{
	if (c.stage > NOT_CALIBRATING && t_id != c.t_id)
	{
		return;
	}

	if (c.is_signal_invalid)
	{
		c.is_signal_invalid = false;
		for (auto loc : existing_hand_locs)
		{
			c.is_signal_invalid |= hands[loc]->is_in_ack_pose();
			c.is_signal_invalid |= hands[loc]->is_in_decl_pose();
		}
	}

	bool is_calibrating_hand_ack = hands[c.tr_assign[t_id]]->is_in_ack_pose(),
		is_calibrating_hand_decl = hands[c.tr_assign[t_id]]->is_in_decl_pose();
	float time_to_calibration;

	if (c.stage > NOT_CALIBRATING && is_calibrating_hand_decl)
	{
		c.is_signal_invalid = true;
		c.stage = ABORT;
	}

	switch (c.stage)
	{
	case NOT_CALIBRATING:
		if (!c.is_signal_invalid && is_calibrating_hand_ack)
		{
			c.t_id = t_id;
			next_calibration_stage(false, false);
		}
		break;
	case REQUESTED:
		if (!is_calibrating_hand_ack)
		{
			reset_calibration_stage();
		}
		else
		{
			time_to_calibration = c.request_dur -
				chrono::duration_cast<chrono::milliseconds>(chrono::steady_clock::now() - c.last_tp).count();
			cout << "Calibration in " << to_string(time_to_calibration / 1000) << "s..." << endl;
			if (time_to_calibration <= 0)
			{
				last_cal = c;
				set_boolean(c.render_panel, false);
				set_boolean(c.render_bridge, false);
				next_calibration_stage();
			}
		}
		break;
	case USER_POS:
		if (c.use_hmd)
		{
			c.user_position = position_from_pose(state.hmd.pose);
			next_calibration_stage(false, true);
		}
		else
		{
			cout << "Tap index and thumb above your head" << endl;
			if (!c.is_signal_invalid && is_calibrating_hand_ack)
			{
				c.user_position = position_from_pose(state.controller[t_id].pose);
				next_calibration_stage();
			}
		}
		break;
	case GLOVES:
		time_to_calibration = c.hand_calibration_prep -
			chrono::duration_cast<chrono::milliseconds>(chrono::steady_clock::now() - c.last_tp).count();
		cout << "Move your hands as shown, fingers together, tap thumb and index when ready" << endl
			<< "Calibration in " << to_string(time_to_calibration / 1000) << "s..." << endl;
		calibrate_new_z(state);
		for (auto loc : existing_hand_locs)
		{
			hands[loc]->calibrate_to_quat(quat(c.z_dir.z(), 0, -c.z_dir.x(), 0));
		}
		if (time_to_calibration <= 0)
		{
			set_boolean(c.render_panel, last_cal.render_panel);
			set_boolean(c.render_bridge, last_cal.render_bridge);
			next_calibration_stage();
		}
		break;
	case PANEL:
		cout << "Acknowledge when done" << endl;
		calibrate_model_view(position_from_pose(state.controller[c.t_id].pose));
		if (!c.is_signal_invalid && is_calibrating_hand_ack)
		{
			next_calibration_stage();
		}
		break;
	case ABORT:
		cout << "Restore last calibration (index) or go with this one (middle)?" << endl;
		if (!c.is_signal_invalid && is_calibrating_hand_ack)
		{
			abort_calibration(true);
		}
		else if (!c.is_signal_invalid && is_calibrating_hand_decl)
		{
			abort_calibration(false);
		}
		break;
	default:
		break;
	}
}

void vr_ctrl_panel::next_calibration_stage(bool set_interactive_pulse, bool invalidate_ack)
{
	c.stage++;
	if (c.stage == NUM_CALIBRATION_STEPS)
	{
		reset_calibration_stage();
		for (auto loc : existing_hand_locs)
		{
			hands[loc]->init_interactive_pulse(hand::DONE);
		}
	}
	else
	{
		c.last_tp = chrono::steady_clock::now();
		c.is_signal_invalid = invalidate_ack ? true : c.is_signal_invalid;
		if (set_interactive_pulse)
		{
			hands[c.tr_assign[c.t_id]]->init_interactive_pulse(hand::ACK);
		}
	}
}

void vr_ctrl_panel::reset_calibration_stage()
{
	c.is_signal_invalid = true;
	c.stage = NOT_CALIBRATING;
}

void vr_ctrl_panel::abort_calibration(bool restore)
{
	for (auto loc : existing_hand_locs)
	{
		hands[loc]->init_interactive_pulse(hand::ABORT);
		if (restore)
		{
			hands[loc]->restore_last_calibration();
		}
	}
	c = last_cal;
	update_all_members();
	reset_calibration_stage();
}

void vr_ctrl_panel::calibrate_new_z(const vr::vr_kit_state& state)
{
	vec3 hands_ave(0);
	for (auto assign : c.tr_assign)
	{
		hands_ave += position_from_pose(state.controller[assign.first].pose);
	}
	hands_ave /= c.tr_assign.size();
	c.z_dir = c.user_position - hands_ave;
	c.z_dir.y() = .0f;
	c.z_dir.normalize();
}

inline void vr_ctrl_panel::calibrate_model_view(vec3 panel_origin)
{
	mat4 new_model_view_mat;
	new_model_view_mat.identity();
	float rad_to_deg = 45.0f / atan(1.0f);
	float angle_y = acos(c.z_dir.z()) * rad_to_deg;
	if (c.z_dir.x() < 0)
	{
		angle_y = -angle_y;
	}
	new_model_view_mat *= cgv::math::rotate4(vec3(0, angle_y, 0));
	new_model_view_mat.set_col(3, hom_pos(panel_origin));
	vec4 center_translation = hom_pos(-panel_pos_on_bridge);
	new_model_view_mat *= cgv::math::translate4(inhom_pos(center_translation));

	c.model_view_mat = new_model_view_mat;
	c.world_to_model = cgv::math::inv(new_model_view_mat);
}


void vr_ctrl_panel::assign_trackers(const vr::vr_kit_state& state)
{
	if (c.tr_assign.size())
	{
		cout << "Trackers already assigned" << endl;
		return;
	}

	if (!existing_hand_locs.size())
	{
		cout << "No hands to assign trackers to" << endl;
	}
	else
	{
		cout << "Assigning trackers" << endl;

		map<float, int, greater<float>> prio_map1, prio_map2;
		for (size_t i = 0; i < 4; i++)
		{
			prio_map1[state.controller[i].pose[10]] = i;
		}

		if (existing_hand_locs.size() == 1)
		{
			c.tr_assign[prio_map1.begin()->second] = existing_hand_locs[0];
		}
		else
		{
			auto it = prio_map1.begin();
			c.tr_assign[it++->second] = NDAPISpace::LOC_RIGHT_HAND;
			c.tr_assign[it->second] = NDAPISpace::LOC_LEFT_HAND;
		}
	}
}

void vr_ctrl_panel::reset_tracker_assigns() {
	cout << "Resetting tracker assignments" << endl;
	c.tr_assign = map<int, int>();
}

inline void vr_ctrl_panel::set_boolean(bool& b, bool new_val)
{
	b = new_val;
	on_set(&b);
}

inline void vr_ctrl_panel::init_frame(context& ctx)
{
	if (c.load_bridge)
	{
		set_boolean(c.load_bridge, !bridge.init(ctx));
	}
}
