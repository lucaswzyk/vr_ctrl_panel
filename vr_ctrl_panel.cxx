#include "vr_ctrl_panel.h"

bool vr_ctrl_panel::init(context& ctx)
{
	load_calibration();
	bool res = true;

	cgv::gui::connect_vr_server(false);
	nd_handler& ndh = nd_handler::instance();
	res = res && nd_handler::instance().get_is_connected();

	cgv::render::ref_rounded_cone_renderer(ctx, 1);
	cgv::render::ref_box_renderer(ctx, 1);
	cgv::render::ref_sphere_renderer(ctx, 1);
	cgv::render::ref_rectangle_renderer(ctx, 1);
	
	switch (ndh.get_app_mode())
	{
	case nd_handler::BOTH_HANDS:
		hands.push_back(new hand(NDAPISpace::LOC_RIGHT_HAND, c.tracker_refs[0]));
		existing_hand_locs.push_back(NDAPISpace::LOC_RIGHT_HAND);
		hands.push_back(new hand(NDAPISpace::LOC_LEFT_HAND, c.tracker_refs[1]));
		existing_hand_locs.push_back(NDAPISpace::LOC_LEFT_HAND);
		break;
	case nd_handler::LEFT_ONLY:
		hands.push_back(nullptr);
		hands.push_back(new hand(NDAPISpace::LOC_LEFT_HAND, c.tracker_refs[1]));
		existing_hand_locs.push_back(NDAPISpace::LOC_LEFT_HAND);
		break;
	case nd_handler::RIGHT_ONLY:
		hands.push_back(new hand(NDAPISpace::LOC_RIGHT_HAND, c.tracker_refs[0]));
		existing_hand_locs.push_back(NDAPISpace::LOC_RIGHT_HAND);
		hands.push_back(nullptr);
		break;
	default:
		break;
	}
	hand_positions = vector<vec3>(hands.size(), vec3(0));
	hand_orientations = vector<mat3>(hands.size());

	hd.set_text("");

	return res;
}

inline void vr_ctrl_panel::draw(cgv::render::context& ctx)
{
	ctx.push_modelview_matrix();
	ctx.mul_modelview_matrix(c.model_view_mat);

	//auto t0 = std::chrono::steady_clock::now();
	for (auto loc : existing_hand_locs)
	{
		hands[loc]->update_and_draw(ctx, panel, hand_positions[loc], hand_orientations[loc]);
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

	/*auto t2 = std::chrono::steady_clock::now();
	cout << "hand: " << std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0).count() << endl;
	cout << "mesh: " << std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1).count() << endl << endl;*/
	ctx.pop_modelview_matrix();
	
	hd.draw(ctx, hd_position, hd_orientation);
}

inline void vr_ctrl_panel::destruct(context& ctx)
{
	ref_rounded_cone_renderer(ctx, -1);
	ref_sphere_renderer(ctx, -1);
	ref_box_renderer(ctx, -1);
	ref_rectangle_renderer(ctx, -1);
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
		mat3 ori_mat = vrpe.get_orientation();
		vec3 position = vrpe.get_position();
		
		if (t_id == -1)
		{
			hd_position = position;
			hd_orientation = ori_mat;
			return true;
		}
		else
		{
			if (c.tr_assign.count(t_id))
			{
				int tracker_loc = c.tr_assign[t_id];
				hand_positions[tracker_loc] = math_conversion::inhom_pos(c.world_to_model * math_conversion::hom_pos(position));
				hand_orientations[tracker_loc] = ori_mat;
				update_calibration(vrpe.get_state(), t_id);
				return true;
			}
			else 
			{
				assign_trackers(vrpe);
				return false;
			}
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
}

void vr_ctrl_panel::update_calibration(vr::vr_kit_state state, int t_id)
{
	if (c.stage > NOT_CALIBRATING && hands[c.tr_assign[t_id]] != c.cal_hand)
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
			c.cal_hand = hands[c.tr_assign[t_id]];
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
			hd.set_text("Calibration in " + to_string((int)time_to_calibration / 1000) + "s...");
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
		hd.set_text("Use HMD position (index + palm) \nor manual position (middle + palm)?");
		if (c.cal_hand->is_in_choice1_pose())
		{
			c.user_position = math_conversion::position_from_pose(state.hmd.pose);
			next_calibration_stage();
			next_calibration_stage(false, true);
		}
		else if (c.cal_hand->is_in_choice2_pose())
		{
			next_calibration_stage();
		}
		break;
	case USER_POS_MAN:
		hd.set_text("Tap index and thumb above your head.");
		if (!c.is_signal_invalid && is_calibrating_hand_ack)
		{
			c.user_position = math_conversion::position_from_pose(state.controller[t_id].pose);
			next_calibration_stage();
		}
		break;
	case GLOVES:
		time_to_calibration = c.hand_calibration_prep -
			chrono::duration_cast<chrono::milliseconds>(chrono::steady_clock::now() - c.last_tp).count();
		hd.set_text("Move your as shown, fingers together.\nCalibration in " + to_string((int)time_to_calibration / 1000) + "s...");
		calibrate_new_z(state);
		calibrate_model_view(math_conversion::ave_pos(state.controller));
		for (auto loc : existing_hand_locs)
		{
			hands[loc]->calibrate_to_quat(quat(hand_orientations[loc]).inverse());
			c.tracker_refs[loc] = cgv::math::inv(hand_orientations[loc]);
		}
		if (time_to_calibration <= 0)
		{
			set_boolean(c.render_panel, last_cal.render_panel);
			set_boolean(c.render_bridge, last_cal.render_bridge);
			next_calibration_stage();
		}
		break;
	case PANEL:
		hd.set_text("Adjust panel position and acknowledge \nwhen done (index + thumb).");
		calibrate_model_view(math_conversion::ave_pos(state.controller));
		if (!c.is_signal_invalid && is_calibrating_hand_ack)
		{
			next_calibration_stage();
		}
		break;
	case EXPORT:
		hd.set_text("Save this calibration? \nYes: index + palm      No: middle + palm");
		if (!c.is_signal_invalid && c.cal_hand->is_in_choice1_pose())
		{
			export_calibration();
			next_calibration_stage();
		}
		else if (!c.is_signal_invalid && c.cal_hand->is_in_choice2_pose())
		{
			next_calibration_stage();
		}
		break;
	case ABORT:
		hd.set_text("Restore last calibration (index + palm) \nor go with this one (middle + palm)?");
		if (!c.is_signal_invalid && c.cal_hand->is_in_choice1_pose())
		{
			abort_calibration(true);
		}
		else if (!c.is_signal_invalid && c.cal_hand->is_in_choice2_pose())
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
		reset_tracker_assigns();
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
			for (auto loc : existing_hand_locs)
			{
				hands[loc]->init_interactive_pulse(hand::ACK);
			}
		}
	}
}

void vr_ctrl_panel::reset_calibration_stage()
{
	set_boolean(c.render_hands, last_cal.render_hands);
	set_boolean(c.render_panel, last_cal.render_panel);
	set_boolean(c.render_bridge, last_cal.render_bridge);
	c.is_signal_invalid = true;
	c.stage = NOT_CALIBRATING;
	hd.set_text("");
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
	c.z_dir = c.user_position - math_conversion::ave_pos(state.controller);
	c.z_dir.y() = .0f;
	c.z_dir.normalize();
}

inline void vr_ctrl_panel::calibrate_model_view(vec3 panel_origin)
{
	/*panel_origin += vec3(0, c.hand_vs_panel_for_calibration.y(), 0)
		+ c.z_dir * c.hand_vs_panel_for_calibration.z();*/
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
	vec4 center_translation = math_conversion::hom_pos(c.hand_vs_panel_for_calibration - panel_pos_on_bridge);
	new_model_view_mat *= cgv::math::translate4(math_conversion::inhom_pos(center_translation));

	c.model_view_mat = new_model_view_mat;
	c.world_to_model = cgv::math::inv(new_model_view_mat);
}

void vr_ctrl_panel::assign_trackers(cgv::gui::vr_pose_event& vrpe)
{
	int t_id = vrpe.get_trackable_index();

	if (t_id == -1)
	{
		cout << "Invalid index handed in vr_pose_event for tracker assignment." << endl;
		return;
	}

	if (c.tr_assign.size() == existing_hand_locs.size())
	{
		reset_tracker_assigns();
	}
	
	if (c.tr_assign.size())
	{
		auto controllers = vrpe.get_state().controller;
		int other_id = -1;

		for (size_t i = 0; i < 4; i++)
		{
			if (i != t_id && controllers[i].status == vr::VRS_TRACKED)
			{
				other_id = i;
			}
		}
		
		if (other_id == -1)
		{
			cout << "Could not find second tracker!" << endl;
			return;
		}

		vec3 user_pos, z_dir;
		
		if (vrpe.get_state().hmd.status == vr::VRS_TRACKED)
		{
			user_pos = math_conversion::position_from_pose(vrpe.get_state().hmd.pose);
			z_dir = user_pos - math_conversion::ave_pos(controllers);
			z_dir.y() = 0;
			z_dir.normalize();
		}
		else
		{
			user_pos = c.user_position;
			z_dir = c.z_dir;
		}

		vec3 t_pos = math_conversion::position_from_pose(controllers[t_id].pose) - user_pos,
			 other_pos = math_conversion::position_from_pose(controllers[other_id].pose) - user_pos;
		if (cross(t_pos, z_dir).y() > 0 && cross(other_pos, z_dir).y() < 0)
		{
			c.tr_assign[t_id] = NDAPISpace::LOC_LEFT_HAND;
			c.tr_assign[other_id] = NDAPISpace::LOC_RIGHT_HAND;
		} 
		else if (cross(t_pos, z_dir).y() < 0 && cross(other_pos, z_dir).y() > 0)
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
	c.tr_assign = map<int, int>();
}

void vr_ctrl_panel::export_calibration()
{
	ofstream cal_file("calibration.txt");
	cal_file << to_string(c.model_view_mat(0, 0)) + " " + to_string(c.model_view_mat(0, 1)) + " " + to_string(c.model_view_mat(0, 2)) + " " + to_string(c.model_view_mat(0, 3)) + " ";
	cal_file << to_string(c.model_view_mat(1, 0)) + " " + to_string(c.model_view_mat(1, 1)) + " " + to_string(c.model_view_mat(1, 2)) + " " + to_string(c.model_view_mat(1, 3)) + " ";
	cal_file << to_string(c.model_view_mat(2, 0)) + " " + to_string(c.model_view_mat(2, 1)) + " " + to_string(c.model_view_mat(2, 2)) + " " + to_string(c.model_view_mat(2, 3)) + " ";
	cal_file << to_string(c.model_view_mat(3, 0)) + " " + to_string(c.model_view_mat(3, 1)) + " " + to_string(c.model_view_mat(3, 2)) + " " + to_string(c.model_view_mat(3, 3)) << endl;
	
	cal_file << to_string(c.world_to_model(0, 0)) + " " + to_string(c.world_to_model(0, 1)) + " " + to_string(c.world_to_model(0, 2)) + " " + to_string(c.world_to_model(0, 3)) + " ";
	cal_file << to_string(c.world_to_model(1, 0)) + " " + to_string(c.world_to_model(1, 1)) + " " + to_string(c.world_to_model(1, 2)) + " " + to_string(c.world_to_model(1, 3)) + " ";
	cal_file << to_string(c.world_to_model(2, 0)) + " " + to_string(c.world_to_model(2, 1)) + " " + to_string(c.world_to_model(2, 2)) + " " + to_string(c.world_to_model(2, 3)) + " ";
	cal_file << to_string(c.world_to_model(3, 0)) + " " + to_string(c.world_to_model(3, 1)) + " " + to_string(c.world_to_model(3, 2)) + " " + to_string(c.world_to_model(3, 3)) << endl;

	cal_file << to_string(c.user_position(0)) + " " + to_string(c.user_position(1)) + " " + to_string(c.user_position(2)) << endl;
	cal_file << to_string(c.z_dir(0)) + " " + to_string(c.z_dir(1)) + " " + to_string(c.z_dir(2)) << endl;

	cal_file << to_string(c.tracker_refs[0](0, 0)) + " " + to_string(c.tracker_refs[0](0, 1)) + " " + to_string(c.tracker_refs[0](0, 2)) + " ";
	cal_file << to_string(c.tracker_refs[0](1, 0)) + " " + to_string(c.tracker_refs[0](1, 1)) + " " + to_string(c.tracker_refs[0](1, 2)) + " ";
	cal_file << to_string(c.tracker_refs[0](2, 0)) + " " + to_string(c.tracker_refs[0](2, 1)) + " " + to_string(c.tracker_refs[0](2, 2)) << endl;
	
	cal_file << to_string(c.tracker_refs[1](0, 0)) + " " + to_string(c.tracker_refs[1](0, 1)) + " " + to_string(c.tracker_refs[1](0, 2)) + " ";
	cal_file << to_string(c.tracker_refs[1](1, 0)) + " " + to_string(c.tracker_refs[1](1, 1)) + " " + to_string(c.tracker_refs[1](1, 2)) + " ";
	cal_file << to_string(c.tracker_refs[1](2, 0)) + " " + to_string(c.tracker_refs[1](2, 1)) + " " + to_string(c.tracker_refs[1](2, 2)) << endl;
	
	cal_file.close();
}

void vr_ctrl_panel::load_calibration()
{
	try
	{
		ifstream cal_file("calibration.txt");
		if (!cal_file.good())
		{
			throw exception();
		}
		for (size_t i = 0; i < 4; i++)
		{
			for (size_t j = 0; j < 4; j++)
			{
				cal_file >> c.model_view_mat(i, j);
			}
		}
		for (size_t i = 0; i < 4; i++)
		{
			for (size_t j = 0; j < 4; j++)
			{
				cal_file >> c.world_to_model(i, j);
			}
		}
		for (size_t i = 0; i < 3; i++)
		{
			cal_file >> c.user_position(i);
		}
		for (size_t i = 0; i < 3; i++)
		{
			cal_file >> c.z_dir(i);
		}
		for (size_t i = 0; i < 3; i++)
		{
			for (size_t j = 0; j < 3; j++)
			{
				cal_file >> c.tracker_refs[0](i, j);
			}
		}
		for (size_t i = 0; i < 3; i++)
		{
			float foo;
			for (size_t j = 0; j < 3; j++)
			{
				cal_file >> foo;
				cal_file >> c.tracker_refs[1](i, j);
			}
		}
	}
	catch (const std::exception&)
	{
		cout << "Could not find or read calibration file. Writing trivial values." << endl;
		export_calibration();
	}
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
