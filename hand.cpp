#include "hand.h"

bool hand::handle(cgv::gui::event& e)
{
	if ((e.get_flags() & cgv::gui::EF_VR) == 0)
		return false;
	if (e.get_kind() == cgv::gui::EID_POSE)
	{
		cout << "pose" << endl;
	}

	return false;
}

void hand::stream_help(std::ostream& os)
{
}

void hand::create_gui()
{
}
