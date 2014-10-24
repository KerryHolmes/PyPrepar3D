#include "dispatch_receiver.hpp"

namespace prepar3d {
namespace simconnect {

void DispatchReceiver::init() {
	MAP_TO_FUNC(OPEN);
	MAP_TO_FUNC(EVENT);
	MAP_TO_FUNC(EXCEPTION);
	MAP_TO_FUNC(QUIT);
	MAP_TO_FUNC(EVENT_OBJECT_ADDREMOVE);
	MAP_TO_FUNC(EVENT_FILENAME);
	MAP_TO_FUNC(EVENT_FRAME);
	MAP_TO_FUNC(SIMOBJECT_DATA);
	MAP_TO_FUNC(SIMOBJECT_DATA_BYTYPE);
	MAP_TO_FUNC(WEATHER_OBSERVATION);
	MAP_TO_FUNC(CLOUD_STATE);
	MAP_TO_FUNC(ASSIGNED_OBJECT_ID);
	MAP_TO_FUNC(RESERVED_KEY);
	MAP_TO_FUNC(CUSTOM_ACTION);
	MAP_TO_FUNC(SYSTEM_STATE);
	MAP_TO_FUNC(CLIENT_DATA);
	MAP_TO_FUNC(EVENT_WEATHER_MODE);
	MAP_TO_FUNC(AIRPORT_LIST);
	MAP_TO_FUNC(VOR_LIST);
	MAP_TO_FUNC(NDB_LIST);
	MAP_TO_FUNC(WAYPOINT_LIST);
	MAP_TO_FUNC(EVENT_MULTIPLAYER_SERVER_STARTED);
	MAP_TO_FUNC(EVENT_MULTIPLAYER_CLIENT_STARTED);
	MAP_TO_FUNC(EVENT_MULTIPLAYER_SESSION_ENDED);
	MAP_TO_FUNC(EVENT_RACE_END);
	MAP_TO_FUNC(EVENT_RACE_LAP);
	MAP_TO_FUNC(OBSERVER_DATA);

	MAP_TO_FUNC(GROUND_INFO);
	MAP_TO_FUNC(SYNCHRONOUS_BLOCK);
	MAP_TO_FUNC(EXTERNAL_SIM_CREATE);
	MAP_TO_FUNC(EXTERNAL_SIM_DESTROY);
	MAP_TO_FUNC(EXTERNAL_SIM_SIMULATE);
	MAP_TO_FUNC(EXTERNAL_SIM_LOCATION_CHANGED);
	MAP_TO_FUNC(EXTERNAL_SIM_EVENT);
	MAP_TO_FUNC(EVENT_WEAPON);
	MAP_TO_FUNC(EVENT_COUNTERMEASURE);
	MAP_TO_FUNC(EVENT_OBJECT_DAMAGED_BY_WEAPON);
	MAP_TO_FUNC(VERSION);
	MAP_TO_FUNC(SCENERY_COMPLEXITY);
	MAP_TO_FUNC(SHADOW_FLAGS);
	MAP_TO_FUNC(TACAN_LIST);
	MAP_TO_FUNC(CAMERA_6DOF);
	MAP_TO_FUNC(CAMERA_FOV);
	MAP_TO_FUNC(CAMERA_SENSOR_MODE);
	MAP_TO_FUNC(CAMERA_WINDOW_POSITION);
	MAP_TO_FUNC(CAMERA_WINDOW_SIZE);
	MAP_TO_FUNC(MISSION_OBJECT_COUNT);
	MAP_TO_FUNC(GOAL);
	MAP_TO_FUNC(MISSION_OBJECTIVE);
	MAP_TO_FUNC(FLIGHT_SEGMENT);
	MAP_TO_FUNC(PARAMETER_RANGE);
	MAP_TO_FUNC(FLIGHT_SEGMENT_READY_FOR_GRADING);
	MAP_TO_FUNC(GOAL_PAIR);
	MAP_TO_FUNC(EVENT_FLIGHT_ANALYSIS_DIAGRAMS);
	MAP_TO_FUNC(LANDING_TRIGGER_INFO);
	MAP_TO_FUNC(LANDING_INFO);
	MAP_TO_FUNC(SESSION_DURATION);
}

void DispatchReceiver::registerID(SIMCONNECT_RECV_ID id) {
	_registeredIdList[id] = _functionMap.at(id);
}

tuple DispatchReceiver::getNextDispatchForHandle(PyObject *handle) {
	if (!_registeredIdList.empty()) {
		SIMCONNECT_RECV* pData;
		DWORD cbData;
		HRESULT res = SimConnect_GetNextDispatch(PyCObject_AsVoidPtr(handle), &pData, &cbData);
		std::map<SIMCONNECT_RECV_ID, FunctionType>::iterator iter = _registeredIdList.find(static_cast<SIMCONNECT_RECV_ID>(pData->dwID));
		if (iter != _registeredIdList.end()) {
			return make_tuple(res, iter->second(pData), cbData);
		} else {
			return tuple();
		}
	} else {
		//TODO throw
		return tuple();
	}
}

} // end namespace simconnect
} // end namespace prepar3d
