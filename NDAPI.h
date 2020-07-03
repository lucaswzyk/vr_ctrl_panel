/*---------------------------------------------------------------------------
| SDK																		|
| @Author: NeuroDigital Technologies, S.L.									|
| Copyright © NeuroDigital Technologies, S.L. 2012 - 2018           		|
---------------------------------------------------------------------------*/

#ifndef __NDAPI_H__
#define __NDAPI_H__


#ifdef NDAPI_EXPORTS
#define NDAPI_API __declspec(dllexport)
#else
#define NDAPI_API __declspec(dllimport)
#endif

#include <BaseTsd.h>  //necessary for extern "C"
#include <string>
#define MAX_DELAY 10000

namespace NDAPISpace
{

	/// <summary>To store the rotation values from the IMU of the device</summary>
	struct quaternion_s {
		/// <summary>
		/// X component of the quaternion structure
		/// </summary>
		float x;
		/// <summary>
		/// Y component of the quaternion structure
		/// </summary>
		float y;
		/// <summary>
		/// Z component of the quaternion structure
		/// </summary>
		float z;
		/// <summary>
		/// W component of the quaternion structure
		/// </summary>
		float w;
	};
	typedef struct quaternion_s quaternion_t;

	/// <summary>To store the acceleration values from the IMU of the device</summary>
	struct vector3d_s {
		/// <summary> X component of the vector3d structure</summary>
		float x;
		/// <summary> Y component of the vector3d structure</summary>
		float y;
		/// <summary> Z component of the vector3d structure</summary>
		float z;
	};
	typedef struct vector3d_s vector3d_t;




	/// <summary>Possible errors using methods of the class</summary>
	enum Error
	{
		/// <summary>Occurs when the communication with the service is not possible</summary>
		ND_ERROR_SERVICE_UNAVAILABLE = -1, 	/* Client not connected to server. */
		/// <summary>Occurs when the glove is not connected</summary>
		ND_ERROR_DEVICE_NOT_CONNECTED = -2, 	/* Device not connected or port in use. */
		/// <summary>Occurs when a invalid glove id is used</summary>
		ND_ERROR_INVALID_DEVICE = -3, 			/* Device not present in Server. */
		/// <summary>Occurs when a invalid actuator id  is used</summary>
		ND_ERROR_INVALID_ACTUATOR_ID = -4,
		/// <summary>Occurs when a invalid contact is used</summary>
		ND_ERROR_INVALID_CONTACT_ID = -5,
		/// <summary>Occurs when a invalid parameter id is used</summary>
		ND_ERROR_INVALID_PARAMETER_ID = -6,
		/// <summary>Occurs when a invalid value is used</summary>
		ND_ERROR_INVALID_VALUE = -7,
		/// <summary>License is required to use Avatar VR</summary>
		ND_ERROR_LICENSE_REQUIRED = -8,
		/// <summary>License can't be checked via Internet</summary>
		ND_ERROR_LICENSE_CANNOT_CHECK = -9,
		/// <sumary> Function doesn't work anymore
		ND_ERROR_DEPRECATED = -10
	};

	/// <summary>Params to get/set of the device</summary>
	enum DriverParam {
		/// <summary>Firmware current version of the glove</summary>
		PARAM_FIRMWARE_VERSION = 0,
		/// <summary>Hardware current revision of the glove</summary>
		PARAM_HARDWARE_REVISION = 1,
		/// <summary>Time to shutdown if the glove stands PARAM_SHUTDOWN_TIME seconds without any movement (Not take effect if the glove is USB connected)</summary>
		PARAM_SHUTDOWN_TIME = 2,
		/// <summary>Time to sleep (shutdown the bluetooth) if the glove stands PARAM_SLEEP_TIME seconds without any movement. (Not implemented)</summary>
		PARAM_SLEEP_TIME = 3,
		/// <summary> Parameter to setup the glove to pairable mode </summary>
		PARAM_PAIRABLE_MODE = 4,
		/// <summary>Total number of parameters </summary>
		NUM_PARAMS = 5
	};

	/// <summary>Location of the device, right hand of left hand</summary>
	enum Location
	{
		/// <summary>The glove is right-handed</summary>
		LOC_RIGHT_HAND = 0,
		/// <summary>The glove is left-handed</summary>
		LOC_LEFT_HAND = 1
	};
	/// <summary>Location of the imus of Avatar VR</summary>
	enum ImuLocation
	{
		/// <summary>It refers to the principal IMU</summary>
		IMULOC_PALM = 0,
		/// <summary>It refers to the IMU on Thumb's first phalanx</summary>
		IMULOC_THUMB0 = 1,
		/// <summary>It refers to the IMU on Thumb's second phalanx</summary>
		IMULOC_THUMB1 = 2,
		/// <summary>It refers to the IMU on the index</summary>
		IMULOC_INDEX = 3,
		/// <summary>It refers to the IMU on the middle</summary>
		IMULOC_MIDDLE = 4,
		/// <summary>It refers to the IMU on the ring</summary>
		IMULOC_RING = 5,
		/// <summary>It refers to the IMU on the pink</summary>
		IMULOC_PINKY = 6,
		/// <summary>It refers to the IMU on the chest</summary>
		IMULOC_CHEST = 7,
		/// <summary>It refers to the IMU on the arm</summary>
		IMULOC_ARM = 8,
		/// <summary>It refers to the IMU on the forearm</summary>
		IMULOC_FOREARM = 9
	};
	/// <summary>Type of debug messages</summary>
	enum DebugType
	{
		/// <summary>Messages come from Firmware</summary>
		DEBUG_FIRMWARE = 0,
		/// <summary>Messages come from Driver</summary>
		DEBUG_DRIVER = 1,
		/// <summary>Messages come from Server</summary>
		DEBUG_SERVER = 2
	};
	/// <summary>Level of importance of debug messages</summary>
	enum DebugInfoLevel {
		/// <summary>Importance of error</summary>
		LEVEL_ERROR = 1,
		/// <summary>Importance of warning</summary>
		LEVEL_WARNING = 2,
		/// <summary>Importance of info</summary>
		LEVEL_INFO = 3,
		/// <summary>Importance of debug</summary>
		LEVEL_DEBUG = 4,
		/// <summary>Importance of verbose</summary>
		LEVEL_VERBOSE = 5
	};
	/// <summary>To store IMUS values</summary>
	struct imu_sensor_s
	{
		/// <summary>Location of the IMU</summary>
		ImuLocation location;
		/// <summary>Struct to store rotation of the IMU</summary>
		quaternion_t rawRotation;
		/// <summary>It says if there is available rotation</summary>
		bool hasRotation;
	};
	typedef struct imu_sensor_s imu_sensor_t;

	/// <summary>Flex sensors of the device</summary>
	enum Flex {
		/// <summary>It refers to thumb flex</summary>
		FLEX_THUMB = 0,
		/// <summary>It refers to index flex</summary>
		FLEX_INDEX = 1,
		/// <summary>It refers to middle flex</summary>
		FLEX_MIDDLE = 2,
		/// <summary>It refers to ring flex</summary>
		FLEX_RING = 3,
		/// <summary>It refers to pinky flex</summary>
		FLEX_PINKY = 4,
	};
	/// <summary>Actuators of the device</summary>
	enum Actuator {
		/// <summary>It refers to thumb actuator</summary>
		ACT_THUMB = 0,
		/// <summary>It refers to index actuator</summary>
		ACT_INDEX = 1,
		/// <summary>It refers to middle actuator</summary>
		ACT_MIDDLE = 2,
		/// <summary>It refers to ring actuator</summary>
		ACT_RING = 3,
		/// <summary>It refers to pinky actuator</summary>
		ACT_PINKY = 4,
		/// <summary>It refers to palm index up actuator (see manual)</summary>
		ACT_PALM_INDEX_UP = 5,
		/// <summary>It refers to palm index down actuator (see manual)</summary>
		ACT_PALM_INDEX_DOWN = 6,
		/// <summary>It refers to palm pinky up actuator (see manual)</summary>
		ACT_PALM_PINKY_UP = 7,
		/// <summary>It refers to palm pinky down actuator (see manual)</summary>
		ACT_PALM_PINKY_DOWN = 8,
		/// <summary>It refers to palm middle up actuator (see manual)</summary>
		ACT_PALM_MIDDLE_UP = 9
	};
	/// <summary>Contacts of the device</summary>
	enum Contact {
		/// <summary>Only the palm contact is consider</summary>
		CONT_PALM = 1,
		/// <summary>Only the thumb contact is consider</summary>
		CONT_THUMB = 2,
		/// <summary>Only the index contact is consider</summary>
		CONT_INDEX = 4,
		/// <summary>Only the middle contact is consider</summary>
		CONT_MIDDLE = 8,
		/// <summary>Any contact is consider</summary>
		CONT_ANY = 15
	};

	/// <summary>Connection type from the device to computer</summary>
	enum ConnectionType {
		/// <summary>Glove disconnected</summary>
		CONN_DISCONNECTED = 0,
		/// <summary>Glove connected by USB</summary>
		CONN_USB = 1,
		/// <summary>Glove connected by bluetooth</summary>
		CONN_BLUETOOTH = 2,
		/// <summary>Virtual glove connected (Not implemented)</summary>
		CONN_VIRTUAL = 3
	};
	/// <summary>Info from Driver</summary>
	enum DriverInfo {
		/// <summary>Driver version</summary>
		INFO_DRIVER_VERSION = 0,
		/// <summary>IMU FPS</summary>
		INFO_IMU_FPS = 1
	};
	/// <summary>Profiles param types</summary>
	enum ProfileParam {
		/// <summary>Forearm measurement in mm</summary>
		PARAM_PROFILE_FOREARM = 0,
		/// <summary>Arm measurement in mm</summary>
		PARAM_PROFILE_ARM = 1,
		/// <summary>Shoulder width measurement in mm</summary>
		PARAM_PROFILE_SHOULDER_WIDTH = 2
	};

	class NDAPI_Private;

	/// <summary>Used to handle NDAPI devices with c++</summary>	
	class NDAPI_API NDAPI
	{
	public:


		NDAPI();
		~NDAPI();


		/// <summary>
		/// Determines if the contacts id1 and id2 are joined.
		/// </summary>
		/// <param name="id1">Contact identifier 1.</param>
		/// <param name="id2">Contact identifier 2.</param>
		/// <param name="deviceId">The device identifier.</param>
		/// <returns>1 if are joined, 0 if not or an Error enum.</returns>
		int areContactsJoined(Contact id1, Contact id2, int deviceId);
		/// <summary>
		/// Allows to know if a contact is pressed or not, and the group it belongs to.
		/// </summary>
		/// <param name="id">The contact identifier.</param>
		/// <param name="deviceId">The device identifier.</param>
		/// <returns>The group it belongs to, 0 if is not pressed or an Error enum.</returns>
		int getContactState(Contact id, int deviceId);
		/// <summary>
		/// Allows to know if all contacts are pressed or not, and the group they belong to
		/// </summary>
		/// <param name="values">A pointer to an int array in which the contacts state will be stored</param>
		/// <param name="numValues">The number of contacts in the device.</param>
		/// <param name="deviceId">The device identifier.</param>
		/// <returns>0 if succeeded, otherwise an Error enum.</returns>
		int getContactsState(int * values, int numValues, int deviceId);
		/// <summary>
		/// Gets the number of contacts of the specified device.
		/// </summary>
		/// <param name="deviceId">The device identifier.</param>
		/// <returns>Number of contacts or an Error enum.</returns>
		int getNumberOfContacts(int deviceId);
		/// <summary>
		/// Determines whether the specified device has a specific actuator.
		/// </summary>
		/// <param name="id">An Actuator enum.</param>
		/// <param name="deviceId">The device identifier.</param>
		/// <returns>1 if the device has the actuator, 0 if not or an Error enum.</returns>
		int hasActuator(Actuator id, int deviceId);
		/// <summary>
		/// Gets the number of actuators of the specified device.
		/// </summary>
		/// <param name="deviceId">The device identifier.</param>
		/// <returns>Number of actuators or an Error enum</returns>
		int getNumberOfActuators(int deviceId);
		/// <summary>
		/// Sets the state of a specific actuator.
		/// </summary>
		/// <param name="id">An Actuator enum.</param>
		/// <param name="level">The actuator level of power between 0.0 and 1.0</param>
		/// <param name="deviceId">The device identifier.</param>
		/// <returns>0 if succeeded, otherwise an Error enum.</returns>
		int setActuatorState(Actuator id, float level, int deviceId);
		/// <summary>
		/// Sets the state of all actuators.
		/// </summary>
		/// <param name="level">A pointer to a float array with the actuators level of power between 0.0 and 1.0</param>
		/// <param name="numValues">The number actuators.</param>
		/// <param name="deviceId">The device identifier.</param>
		/// <returns>0 if succeeded, otherwise an Error enum.</returns>
		int setActuatorsState(const float *level, int numValues, int deviceId);
		/// <summary>
		/// Sets the state of a specific actuator for a limited period of time, then it is stopped.
		/// </summary>
		/// <param name="id">An Actuator enum.</param>
		/// <param name="level">The actuator level of power between 0.0 and 1.0.</param>
		/// <param name="duration">The time in milliseconds between 0 and 1024.</param>
		/// <param name="deviceId">The device identifier.</param>
		/// <returns>0 if succeeded, otherwise an Error enum.</returns>
		int setActuatorPulse(Actuator id, float level, unsigned int duration, int deviceId);
		/// <summary>
		/// Sets a value to a specified driver param.
		/// </summary>
		/// <param name="paramId">A DriverParam enum.</param>
		/// <param name="value">The value of the param.</param>
		/// <param name="deviceId">The device identifier.</param>
		/// <returns>Duration or max-duration if succeeded, otherwise an Error enum.</returns>
		int setParameter(DriverParam paramId, float value, int deviceId);
		/// <summary>
		/// Gets the value of a specified driver param.
		/// </summary>
		/// <param name="paramId">A DriverParam enum.</param>
		/// <param name="outValue">A float where the value of the parameter will be stored.</param>
		/// <param name="deviceId">The device identifier.</param>
		/// <returns>0 if succeeded, otherwise an Error enum.</returns>
		int getParameter(DriverParam paramId, float &outValue, int deviceId);
		/// <summary>
		/// Gets the actuators level of power.
		/// </summary>
		/// <param name="values">A pointer to a float array in which the actuators state will be stored.</param>
		/// <param name="numValues">The number of actuators.</param>
		/// <param name="deviceId">The device identifier.</param>
		/// <returns>0 if succeeded, otherwise an Error enum.</returns>
		int getActuatorsState(float *values, int numValues, int deviceId);
		/// <summary>
		/// Gets the device location.
		/// </summary>
		/// <param name="deviceId">The device identifier.</param>
		/// <returns>A Location enum if succeeded, otherwise an Error enum.</returns>
		int getDeviceLocation(int deviceId);
		/// <summary>
		/// Determines whether the specified device has a specific contact.
		/// </summary>
		/// <param name="id">A Contact enum.</param>
		/// <param name="deviceId">The device identifier.</param>
		/// <returns>1 if the device has the contact, 0 if not or an Error enum.</returns>
		int hasContact(Contact id, int deviceId);
		/// <summary>
		/// Gets the rotation of the IMU.
		/// </summary>
		/// <param name="q">A quaternion where the values of rotation will be stored.</param>
		/// <param name="deviceId">The device identifier.</param>
		/// <returns>0 if succeeded, otherwise an Error enum.</returns>
		int getPalmRotation(quaternion_t &q, int deviceId);
		/// <summary>
		/// Gets the acceleration of the IMU.
		/// </summary>
		/// <param name="v">A vector3d_t where the values of acceleration will be stored.</param>		>
		/// <param name="deviceId">The device identifier.</param>
		/// <returns>0 if succeeded, otherwise an Error enum.</returns>
		int getPalmAcceleration(vector3d_t &v, int deviceId);
		/// <summary>
		/// Sets all actuators to a value of power of 0.0
		/// </summary>
		/// <param name="deviceId">The device identifier.</param>
		/// <returns>0 if succeeded, otherwise an Error enum.</returns>
		int setActuatorsStop(int deviceId);
		/// <summary>
		/// Determines whether the specified device identifier is connected.
		/// </summary>
		/// <param name="deviceId">The device identifier.</param>
		/// <returns>1 if is connected, 0 if not or an Error enum.</returns>
		int isConnected(int deviceId);
		/// <summary>
		/// Gets the connection type of the specified device.
		/// </summary>
		/// <param name="deviceId">The device identifier.</param>
		/// <returns>A ConnectionType enum if succeeded, otherwise an Error enum.</returns>
		int getConnectionType(int deviceId);
		/// <summary>
		/// Gets the serial key of the specified device.
		/// </summary>
		/// <param name="serial">Where the serial will be stored.</param>
		/// <param name="deviceId">The device identifier.</param>
		/// <returns>0 if succeeded, otherwise an Error enum.</returns>
		int getSerialKey(std::string &serial, int deviceId);
		/// <summary>
		/// Gets the battery level of the specified device.
		/// </summary>
		/// <param name="value">Where the battery level will be stored.</param>
		/// <param name="deviceId">The device identifier.</param>
		/// <returns>0 if succeeded, otherwise an Error enum.</returns>
		int getBatteryLevel(float &value, int deviceId);

		/// <summary>
		/// Connects to the server that is running inside of the NDAPI service.
		/// </summary>
		/// <returns>0 if succeeded, otherwise an Error enum.</returns>
		int connectToServer();
		/// <summary>
		/// Closes the connection with the server that is running inside of the NDAPI service.
		/// </summary>
		/// <returns>0 if succeeded, otherwise an Error enum.</returns>
		int closeConnection();
		/// <summary>
		/// Gets the number of devices.
		/// </summary>
		/// <returns>Number of devices or an Error enum</returns>
		int getNumberOfDevices();
		/// <summary>
		/// Gets the number of devices from a specific filter.
		/// </summary>
		/// <param name="filter">A Location enum.</param>
		/// <returns>Number of devices from a specific filter or an Error enum</returns>
		int getNumberOfDevices(Location filter);
		/// <summary>
		/// Gets all the device identifiers.
		/// </summary>
		/// <param name="ids">A pointer to an array of int where the identifiers will be stored.</param>
		/// <param name="numIds">Number of devices.</param>
		/// <returns>Number of devices if succeeded, otherwise an Error enum.</returns>
		int getDevicesId(int * ids, int numIds);
		/// <summary>
		/// Gets all the device identifiers from a specific filter.
		/// </summary>
		/// <param name="filter">A Location enum.</param>
		/// <param name="ids">A pointer to an array of int where the identifiers will be stored.</param>
		/// <param name="numIds">The number of devices from a specific filter.</param>
		/// <returns>>Number of devices from a specific filter if succeeded, otherwise an Error enum.</returns>
		int getDevicesId(Location filter, int * ids, int numIds);

		/// <summary>
		/// Gets the number of devices paired with the PC. Use only for ND Suite
		/// </summary>
		/// <returns>Number of devices paired or an Error enum </returns>
		int getNumPaired();
		/// <summary>
		/// Gets all the device identifiers paired with the PC. Use only for ND Suite
		/// </summary>
		/// <param name="numPorts">Number of devices</param>
		/// <param name="ports">A pointer to an array of int where the identifiers will be stored</param>
		/// <returns>0 if succeeded, otherwise an Error enum.</returns>
		int getPaired(int numPorts, int *ports);
		/// <summary>
		/// Gets the MAC address from a specific device
		/// </summary>
		/// <param name="mac">Where the MAC will be stored.</param>
		/// <param name="deviceId">The device identifier</param>
		/// <returns>0 if succeeded, otherwise an Error enum</returns>		
		int getPairedMac(std::string &mac, int deviceId);

		//virtual gloves
		/// <summary>
		/// Creates a virtual device. Use only for ND Suite.
		/// </summary>
		/// <param name="loc">A Location enum</param>
		/// <param name="numActuators">Number of Actuators</param>
		/// <param name="numContacts">Number of Contacts</param>
		/// <returns>0 if succeeded, otherwise an Error enum</returns>
		int createVirtualGlove(NDAPISpace::Location loc, int numActuators, int numContacts);
		/// <summary>
		/// Deletes a virtual device. Use only for ND Suite.
		/// </summary>		
		/// <param name="deviceId">The device identifier</param>
		/// <returns>0 if succeeded, otherwise an Error enum</returns>
		int deleteVirtualGlove(int deviceId);
		/// <summary>
		/// Sets the contacts state for a virtual device
		/// </summary>
		/// <param name="values">A pointer array with the contacts state</param>
		/// <param name="nValues">Number of values</param>
		/// <param name="deviceId">The virtual device identifier</param>
		/// <returns>0 if succeeded, otherwise an Error enum</returns>
		int setContactsState(int *values, int nValues, int deviceId);
		//functions
		/// <summary>
		/// Sets a Sensation for a specific device
		/// </summary>
		/// <param name="values">A pointer array with the values of the sensation</param>
		/// <param name="nValues">Number of values</param>
		/// <param name="deviceId">The device identifier</param>
		/// <returns>0 if succeeded, otherwise an Error enum</returns>
		int setSensation(float *values, int nValues, int deviceId);

		/// <summary>
		/// Gets an array of float values from a file representing a sensation. 
		/// </summary>
		/// <param name="path">Full path of the file</param>
		/// <param name="nValues">Number of values</param>
		/// <returns>Gets a pointer array with the values or NULL if an error ocurrs</returns>
		float* getSensationValues(std::string path, int &nValues);
		/// <summary>
		/// Gets num of values from a file representing a sensation. 
		/// </summary>
		/// <param name="path">Full path of the file</param>		
		/// <returns>Number of values</returns>
		int getNumSensationValues(std::string path);
		/// <summary>
		/// Sets a sensation from a file for a specific device 
		/// </summary>
		/// <param name="path">Full path of the file</param>
		/// <param name="deviceId">The device identifier</param>
		/// <returns>0 if succeeded, otherwise an Error enum</returns>
		int setSensation(std::string path, int deviceId);

		//same as before but with a delay of x milliseconds
		/// <summary>
		/// Sets a Sensation for a specific device with a delay
		/// </summary>
		/// <param name="values">A pointer array with the values of the sensation</param>
		/// <param name="nValues">Number of values</param>
		/// <param name="delay">Delay in ms. Maximum </param>
		/// <param name="deviceId">The device identifier</param>
		/// <returns>0 if succeeded, otherwise an Error enum</returns>
		int setSensation(float *values, int nValues, int delay, int deviceId);
		/// <summary>
		/// Sets a sensation from a file for a specific device  with a delay
		/// </summary>
		/// <param name="path">Full path of the file</param>
		/// <param name="delay">Delay in ms. Maximum </param>
		/// <param name="deviceId">The device identifier</param>
		/// <returns>0 if succeeded, otherwise an Error enum</returns>
		int setSensation(std::string path, int delay, int deviceId);

		//imus				
		/// <summary>
		/// Gets the number of Imus for a spcedific device
		/// </summary>
		/// <param name="deviceId">The device identifier</param>
		/// <returns>Number of Imus or an Error enum</returns>
		int getNumberOfImus(int deviceId);
		/// <summary>
		/// Gets the imus rotation values for a specific device
		/// </summary>
		/// <param name="imus">A pointer to an array of imu_sensor_t struct</param>
		/// <param name="num">Number of imus</param>		
		/// <param name="deviceId">The device identifier</param>
		/// <returns>0 if succeeded, otherwise an Error enum</returns>
		int getRotations(imu_sensor_t *imus, int num, int deviceId);
		/// <summary>
		/// Gets the debug info filtered by DebugType. Use only for ND Suite.
		/// </summary>
		/// <param name="type">A DebugType enum to filter the type of Info</param>
		/// <param name="debugInfo">A pointer to a char array where de info will be stored</param>
		/// <param name="size">The maximum number of char to store in debugInfo</param>
		/// <param name="deviceId">The device identifier</param>
		/// <returns>number of char stored in debugInfo, otherwise an Error enum</returns>
		int getDebugInfo(DebugType type, char *debugInfo, int size, int deviceId);
		/// <summary>
		/// Gets the value of a specific Driver parameter
		/// </summary>
		/// <param name="paramId">A DriverInfo enum parameter</param>
		/// <param name="outValue">Where the parameter's value will be stored</param>
		/// <param name="deviceId">The device identifier</param>
		/// <returns>0 if succeeded, otherwise an Error enum</returns>
		int getInfo(DriverInfo paramId, float &outValue, int deviceId);
		/// <summary>
		/// Gets the number of flex sensors of the specified device.
		/// </summary>
		/// <param name="deviceId">The device identifier.</param>
		/// <returns>Number of flex sensors or an Error enum</returns>
		int getNumberOfFlex(int deviceId);
		/// <summary>
		/// Gets the flex sensor levels.
		/// </summary>
		/// <param name="values">A pointer to a float array in which the flex sensor states will be stored.</param>
		/// <param name="nValues">The number of flex sensors.</param>
		/// <param name="deviceId">The device identifier.</param>
		/// <returns>0 if succeeded, otherwise an Error enum.</returns>
		int getFlexState(float *values, int nValues, int deviceId);
		/// <summary>
		/// Gets the MAC address of the specified device.
		/// </summary>
		/// <param name="mac">Where the MAC will be stored.</param>
		/// <param name="deviceId">The device identifier.</param>
		/// <returns>0 if succeeded, otherwise an Error enum.</returns>
		int getMACAddress(std::string &mac, int deviceId);
		/// <summary>
		/// Gets the friendly name from a specific paired device
		/// </summary>
		/// <param name="name">Where the name will be stored.</param>
		/// <param name="deviceId">The device identifier</param>
		/// <returns>0 if succeeded, otherwise an Error enum</returns>		
		int getPairedName(std::string &name, int deviceId);
		/// <summary>
		/// Calibrate flex sensors. Once it is called, with the glove put on, open and close your hand twice in order to reset flex values
		/// </summary>		
		/// <param name="deviceId">The device identifier</param>
		/// <returns>0 if succeeded, otherwise an Error enum</returns>	
		int calibrateFlexSensors(int deviceId);
		/// <summary>
		/// Gets the profiles in json format
		/// </summary>		
		/// <param name="list">List of profiles in jSon format</param>
		/// <returns>Number of profiles, otherwise an Error enum</returns>	
		int getProfileList(std::string &list);
		/// <summary>
		/// Deletes a profile
		/// </summary>		
		/// <param name="name">Name of the profile</param>
		/// <returns>0 if succeeded, otherwise an Error enum</returns>	
		int deleteProfile(std::string name);
		/// <summary>
		/// Adds or modifies a profile
		/// </summary>		
		/// <param name="name">Name of the profile</param>
		/// <param name="dataJson">Data in Json format</param>
		/// <returns>0 if succeeded, otherwise an Error enum</returns>
		int addModifyProfile(std::string name, std::string dataJson);
		/// <summary>
		/// Gets the param measurement in mm
		/// </summary>		
		/// <param name="name">Name of the profile</param>
		/// <param name="param">A ProfileParam enum parameter</param>
		/// <returns>measurement in mm if succeeded, otherwise an Error enum</returns>
		int getParamProfile(std::string name, NDAPISpace::ProfileParam param);
	private:

		NDAPI_Private *priv;
	};

}

extern "C" {

	NDAPI_API INT_PTR nd_init();
	NDAPI_API int nd_end(INT_PTR pointer);
	//NDAPIDriver	
	NDAPI_API int nd_areContactsJoined(INT_PTR pointer, NDAPISpace::Contact id1, NDAPISpace::Contact id2, int deviceId);
	NDAPI_API int nd_getContactState(INT_PTR pointer, NDAPISpace::Contact id, int deviceId);
	NDAPI_API int nd_getContactsState(INT_PTR pointer, int * values, int numValues, int deviceId);
	NDAPI_API int nd_getNumberOfContacts(INT_PTR pointer, int deviceId);
	NDAPI_API int nd_hasActuator(INT_PTR pointer, NDAPISpace::Actuator id, int deviceId);
	NDAPI_API int nd_getNumberOfActuators(INT_PTR pointer, int deviceId);
	NDAPI_API int nd_setActuatorState(INT_PTR pointer, NDAPISpace::Actuator id, float level, int deviceId);
	NDAPI_API int nd_setActuatorsState(INT_PTR pointer, const float *level, int numValues, int deviceId);
	NDAPI_API int nd_setActuatorPulse(INT_PTR pointer, NDAPISpace::Actuator id, float level, unsigned int duration, int deviceId);
	NDAPI_API int nd_setParameter(INT_PTR pointer, NDAPISpace::DriverParam paramId, float value, int deviceId);
	NDAPI_API int nd_getParameter(INT_PTR pointer, NDAPISpace::DriverParam paramId, float &outValue, int deviceId);
	NDAPI_API int nd_getActuatorsState(INT_PTR pointer, float *values, int numValues, int deviceId);
	NDAPI_API int nd_getDeviceLocation(INT_PTR pointer, int deviceId);
	NDAPI_API int nd_hasContact(INT_PTR pointer, NDAPISpace::Contact id, int deviceId);
	NDAPI_API int nd_getPalmRotation(INT_PTR pointer, NDAPISpace::quaternion_t &q, int deviceId);
	NDAPI_API int nd_getPalmAcceleration(INT_PTR pointer, NDAPISpace::vector3d_t &v, int deviceId);
	NDAPI_API int nd_setActuatorsStop(INT_PTR pointer, int deviceId);
	NDAPI_API int nd_isConnected(INT_PTR pointer, int deviceId);
	NDAPI_API int nd_getConnectionType(INT_PTR pointer, int deviceId);

	NDAPI_API int nd_getSerialKey(INT_PTR pointer, char * serial, int &bufferSize, int deviceId);


	NDAPI_API int nd_getBatteryLevel(INT_PTR pointer, float &value, int deviceId);

	//server
	NDAPI_API int nd_connectToServer(INT_PTR pointer);
	NDAPI_API int nd_closeConnection(INT_PTR pointer);
	NDAPI_API int nd_getNumberOfDevices(INT_PTR pointer);
	NDAPI_API int nd_getNumberOfDevicesFiltered(INT_PTR pointer, NDAPISpace::Location filter);
	NDAPI_API int nd_getDevicesId(INT_PTR pointer, int *ids, int numIds);
	NDAPI_API int nd_getDevicesIdFiltered(INT_PTR pointer, NDAPISpace::Location filter, int *ids, int numIds);

	NDAPI_API int nd_getNumPaired(INT_PTR pointer);
	NDAPI_API int nd_getPaired(INT_PTR pointer, int numPorts, int *ports);
	NDAPI_API int nd_getPairedMac(INT_PTR pointer, char * mac, int &bufferSize, int deviceId);
	//virtual
	NDAPI_API int nd_createVirtualGlove(INT_PTR pointer, NDAPISpace::Location loc, int numActuators, int numContacts);
	NDAPI_API int nd_deleteVirtualGlove(INT_PTR pointer, int deviceId);
	NDAPI_API int nd_setContactsState(INT_PTR pointer, int *values, int nValues, int deviceId);
	//function
	NDAPI_API int nd_setSensation(INT_PTR pointer, float *values, int nValues, int delay, int deviceId);
	NDAPI_API int nd_getNumberOfImus(INT_PTR pointer, int deviceId);
	NDAPI_API int nd_getRotations(INT_PTR pointer, NDAPISpace::imu_sensor_t *imus, int num, int deviceId);
	NDAPI_API int nd_getDebugInfo(INT_PTR pointer, NDAPISpace::DebugType type, char *debugInfo, int size, int deviceId);
	NDAPI_API int nd_getInfo(INT_PTR pointer, NDAPISpace::DriverInfo paramId, float &outValue, int deviceId);

	NDAPI_API int nd_getSensationValues(INT_PTR pointer, float *values, char *path, int numChar);
	NDAPI_API int nd_getNumValuesSensation(INT_PTR pointer, char *path, int numChar);
	NDAPI_API int nd_getNumberOfFlex(INT_PTR pointer, int deviceId);
	NDAPI_API int nd_getFlexState(INT_PTR pointer, float *values, int nValues, int deviceId);
	NDAPI_API int nd_getMACAddress(INT_PTR pointer, char * mac, int &bufferSize, int deviceId);

	NDAPI_API int nd_getPairedName(INT_PTR pointer, char * name, int &bufferSize, int deviceId);
	NDAPI_API int nd_calibrateFlexSensors(INT_PTR pointer, int deviceId);
	NDAPI_API int nd_getProfileList(INT_PTR pointer, char * list, int &bufferSize);
	NDAPI_API int nd_deleteProfile(INT_PTR pointer, char * name, int size);
	NDAPI_API int nd_addModifyProfile(INT_PTR pointer, char * name, int sizeName, char * dataJson, int sizeData);
	NDAPI_API int nd_getParamProfile(INT_PTR pointer, char * name, int sizeName, NDAPISpace::ProfileParam param);
}

#endif  // __NDAPI_H__ 



