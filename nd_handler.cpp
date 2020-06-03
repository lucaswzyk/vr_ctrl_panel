#include "nd_handler.h"

bool nd_handler::connect()
{
	int res = nd.connectToServer();

	if (res == NDAPISpace::ND_ERROR_SERVICE_UNAVAILABLE)
	{
		cout << "Error: ND Service unavailable" << endl;
		return false;
	}

	int num_dev = nd.getNumberOfDevices();
	if (!num_dev)
	{
		app_mode = NO_DEVICE;
		cout << "No device connected!" << endl;
	}
	else
	{
		device_ids = new int[num_dev];
		nd.getDevicesId(device_ids, num_dev);

		if (num_dev == 1)
		{
			if (nd.getDeviceLocation(device_ids[0]) == NDAPISpace::LOC_LEFT_HAND)
			{
				app_mode = LEFT_ONLY;
				cout << "Left hand device connected." << endl;
			}
			else if (nd.getDeviceLocation(device_ids[0]) == NDAPISpace::LOC_RIGHT_HAND)
			{
				app_mode = RIGHT_ONLY;
				cout << "Right hand device connected." << endl;
			}
		}
		else
		{
			app_mode = BOTH_HANDS;

			// check if NDAPISpace::Locations do not match array indices
			// and switch in that case
			int supposed_left = device_ids[NDAPISpace::LOC_LEFT_HAND];
			if (nd.getDeviceLocation(supposed_left) == NDAPISpace::LOC_RIGHT_HAND)
			{
				int tmp = device_ids[0];
				device_ids[0] = device_ids[1];
				device_ids[1] = tmp;
			}
			cout << "Both hands connected.";
		}
	}

	return true;
}

int nd_handler::get_device_id(NDAPISpace::Location location)
{
	if (app_mode == BOTH_HANDS)
	{
		return device_ids[location];
	}
	else if (app_mode == LEFT_ONLY
		&& location == NDAPISpace::LOC_LEFT_HAND
		|| app_mode == RIGHT_ONLY
		&& location == NDAPISpace::LOC_RIGHT_HAND)
	{
		return device_ids[0];
	}

	return -1;
}
