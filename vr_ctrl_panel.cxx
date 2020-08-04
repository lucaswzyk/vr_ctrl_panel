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
	hand_poses = vector<mat4>(hands.size());

	hd.set_text("Hello World");
	hd.set_visible();

	return res;
}

inline void vr_ctrl_panel::draw(cgv::render::context& ctx)
{
	ctx.push_modelview_matrix();
	ctx.mul_modelview_matrix(c.model_view_mat);

	//auto t0 = std::chrono::steady_clock::now();
	for (auto loc : existing_hand_locs)
	{
		hands[loc]->update_and_draw(ctx, panel, hand_poses[loc], hand_scale);
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
		void* dev_id = vrpe.get_device_id();
		mat4 pose_mat = pose4(vrpe.get_pose_matrix());

		if (dev_id == c.hmd_id)
		{
			hd.set_pose(pose_mat);
			return true;
		}
		else if (c.tr_assign.count(dev_id))
		{
			int tracker_loc = c.tr_assign[dev_id];
			hand_poses[tracker_loc] = c.world_to_model * pose_mat;
			update_calibration(vrpe.get_state(), dev_id, vrpe.get_trackable_index());
			return true;
		}
		else 
		{
			return false;
		}
	}

	return false;
}

// Inherited via provider
inline void vr_ctrl_panel::create_gui()
{
	add_member_control(this, "render hands", c.render_hands, "toggle");
	add_member_control(this, "render panel", c.render_panel, "toggle");
	add_member_control(this, "render bridge", c.render_bridge, "toggle");
	add_member_control(this, "load bridge mesh", c.load_bridge, "toggle");
	cgv::signal::connect_copy(add_button("reassign trackers")->click, rebind(this, &vr_ctrl_panel::reset_tracker_assigns));
	cgv::signal::connect_copy(add_button("export calibration")->click, rebind(this, &vr_ctrl_panel::export_calibration));

	for (auto loc : existing_hand_locs)
	{
		string loc_string = loc == NDAPISpace::LOC_RIGHT_HAND ? "right" : "left";
		add_member_control(this, loc_string, *(hands[loc]->get_man_ack()));
	}
}

void vr_ctrl_panel::update_calibration(vr::vr_kit_state state, void* t_id, int t_index)
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
			hd.set_text("Calibration in " + to_string(time_to_calibration / 1000) + "s...");
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
		if (state.hmd.status == vr::VRS_TRACKED)
		{
			c.user_position = math_conversion::position_from_pose(state.hmd.pose);
			next_calibration_stage(false, true);
		}
		else
		{
			hd.set_text("Tap index and thumb above your head");
			if (!c.is_signal_invalid && is_calibrating_hand_ack)
			{
				c.user_position = math_conversion::position_from_pose(state.controller[t_index].pose);
				next_calibration_stage();
			}
		}
		break;
	case GLOVES:
		time_to_calibration = c.hand_calibration_prep -
			chrono::duration_cast<chrono::milliseconds>(chrono::steady_clock::now() - c.last_tp).count();
		hd.set_text("Move your hands as shown, fingers together, tap thumb and index when ready\nCalibration in " + to_string(time_to_calibration / 1000) + "s...");
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
		hd.set_text("Acknowledge when done");
		calibrate_model_view(math_conversion::position_from_pose(state.controller[t_index].pose));
		if (!c.is_signal_invalid && is_calibrating_hand_ack)
		{
			next_calibration_stage();
		}
		break;
	case ABORT:
		hd.set_text("Restore last calibration (index) or go with this one (middle)?");
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
	auto controllers = state.controller;
	for (size_t i = 0; i < sizeof(controllers) / sizeof(controllers[0]); i++)
	{
		hands_ave += math_conversion::position_from_pose(controllers[i].pose);
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
	new_model_view_mat.set_col(3, math_conversion::hom_pos(panel_origin));
	vec4 center_translation = math_conversion::hom_pos(-panel_pos_on_bridge);
	new_model_view_mat *= cgv::math::translate4(math_conversion::inhom_pos(center_translation));

	c.model_view_mat = new_model_view_mat;
	c.world_to_model = cgv::math::inv(new_model_view_mat);
}

void vr_ctrl_panel::assign_trackers(cgv::gui::vr_pose_event& vrpe)
{
	void* t_id = vrpe.get_device_id();
	int t_index = vrpe.get_trackable_index();

	if (t_index = -1 || t_id == 0)
	{
		cout << "Invalid index or id handed in vr_pose_event for tracker assignment." << endl;
	}

	if (c.tr_assign.size() == existing_hand_locs.size())
	{
		reset_tracker_assigns();
	}
	
	if (c.tr_assign.size())
	{
		auto controllers = vrpe.get_state().controller;
		void* other_id = c.tr_assign.begin()->first;
		int other_index = -1;

		for (size_t i = 0; i < sizeof(controllers)/sizeof(controllers[0]); i++)
		{
			if (i != t_index && controllers[i].status == vr::VRS_TRACKED)
			{
				other_index = i;
			}
		}
		
		if (other_index == -1)
		{
			cout << "Could not find second tracker!" << endl;
			return;
		}

		vec3 t_pos = math_conversion::position_from_pose(controllers[t_index].pose) - c.user_position,
			 other_pos = math_conversion::position_from_pose(controllers[other_index].pose) - c.user_position;
		if (cross(t_pos, c.z_dir).y() > 0 && cross(other_pos, c.z_dir).y() < 0)
		{
			c.tr_assign[t_id] = NDAPISpace::LOC_LEFT_HAND;
			c.tr_assign[other_id] = NDAPISpace::LOC_RIGHT_HAND;
		} 
		else if (cross(t_pos, c.z_dir).y() > 0 && cross(other_pos, c.z_dir).y() < 0)
		{
			c.tr_assign[t_id] = NDAPISpace::LOC_RIGHT_HAND;
			c.tr_assign[other_id] = NDAPISpace::LOC_LEFT_HAND;
		}
		else
		{
			cout << "Could not assign trackers: inconclusive positions!" << endl;
		}
	} 
	else
	{
		c.tr_assign[t_id] = existing_hand_locs[0];
	}
}

void vr_ctrl_panel::reset_tracker_assigns() {
	cout << "Resetting tracker assignments" << endl;
	c.tr_assign = map<void*, int>();
}

void vr_ctrl_panel::export_calibration()
{
	ofstream cal_file("calibration.txt");
	cal_file << to_string(c.model_view_mat(0, 0)) + " " + to_string(c.model_view_mat(0, 1)) + " " + to_string(c.model_view_mat(0, 2)) + " " + to_string(c.model_view_mat(0, 3));
	cal_file << to_string(c.model_view_mat(1, 0)) + " " + to_string(c.model_view_mat(1, 1)) + " " + to_string(c.model_view_mat(1, 2)) + " " + to_string(c.model_view_mat(1, 3));
	cal_file << to_string(c.model_view_mat(2, 0)) + " " + to_string(c.model_view_mat(2, 1)) + " " + to_string(c.model_view_mat(2, 2)) + " " + to_string(c.model_view_mat(2, 3));
	cal_file << to_string(c.model_view_mat(3, 0)) + " " + to_string(c.model_view_mat(3, 1)) + " " + to_string(c.model_view_mat(3, 2)) + " " + to_string(c.model_view_mat(3, 3)) << endl;

	for (auto it = c.tr_assign.begin(); it != c.tr_assign.end(); ++it)
	{
		cal_file << it->first << " ";
	}
	cal_file << endl;

	cal_file.close();
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
