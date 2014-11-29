/*
 * dispatch_listener.cpp
 *
 *  Created on: Oct 24, 2014
 *      Author: Erik
 */

#include "dispatch_handler.hpp"

#include "recv_type_converter.hpp"
#include "data_type_converter.hpp"

#include <boost/python/stl_iterator.hpp>
#include <boost/foreach.hpp>

namespace prepar3d
{
namespace simconnect
{
namespace _internal
{
struct DummyObject
{
};

DispatchHandler *__dispatchHandler__;

void CALLBACK __dispatchCallback__(SIMCONNECT_RECV* pData, DWORD cbData, void *pContext)
{
	if (pData->dwID == SIMCONNECT_RECV_ID_SIMOBJECT_DATA_BYTYPE)
	{

	}
	else if (pData->dwID == SIMCONNECT_RECV_ID_SIMOBJECT_DATA)
	{

		const SIMCONNECT_RECV_SIMOBJECT_DATA *pObjData = (const SIMCONNECT_RECV_SIMOBJECT_DATA *) pData;

		// find our request id
		const DispatchHandler::DataEventObjectStructureInfoType &callbackDataTypeList = __dispatchHandler__->dataEventMap.at(
				pObjData->dwRequestID);
		size_t pos = 0;
		boost::python::dict *dataStructure = callbackDataTypeList.get<2>().get();

		BOOST_FOREACH( const DispatchHandler::DataEventStructureElemInfoType &ref, callbackDataTypeList.get<1>())
		{
			const DataTypeConverter::SizeFunctionType &sizeConverter = ref.second;
			dataStructure->operator [](ref.first.c_str()) = sizeConverter.second((void*) &((&pObjData->dwData)[pos]));
			pos += sizeConverter.first;

		}
		callbackDataTypeList.get<0>()(*dataStructure);
	}
	else
	{

		// handle all system events and input events
		DispatchHandler::EventMapType::const_iterator iter = __dispatchHandler__->eventMap.find(static_cast<DWORD>(pData->dwID));
		if (iter != __dispatchHandler__->eventMap.end())
		{
			const DispatchHandler::EventCallbackConverterType &callbackConverter = iter->second.at(
					((SIMCONNECT_RECV_EVENT_BASE*) pData)->uEventID);
			callbackConverter.first(callbackConverter.second(pData), cbData/*, handle<>(PyCapsule_New(pContext, NULL, NULL))*/);
		}

		// handle all the RecvID events such as Exception Event
		DispatchHandler::EventIDCallbackType::const_iterator cb = __dispatchHandler__->recvIdMap.find(
				static_cast<SIMCONNECT_RECV_ID>(pData->dwID));
		if (cb != __dispatchHandler__->recvIdMap.end())
		{
			const DispatchHandler::EventCallbackConverterType &callbackConverter = cb->second;
			callbackConverter.first(callbackConverter.second(pData), cbData/*, handle<>(PyCapsule_New(pContext, NULL, NULL))*/);
		}
	}

}

} // end namespace _internal

DispatchHandler::DispatchHandler(PyObject *handle) :
		_handle(boost::shared_ptr<PyObject>(handle))
{
	_internal::__dispatchHandler__ = this;
}

HRESULT DispatchHandler::subscribeSystemEvent(const char *eventName, const DWORD &recvID, object callable, const int &id,
		const SIMCONNECT_STATE &state)
{
	EventIDCallbackType &callbackMap = eventMap[recvID];
	HANDLE handle = PyCapsule_GetPointer(_handle.get(), NULL);
	HRESULT res = SimConnect_SubscribeToSystemEvent(handle, id, eventName);
	SimConnect_SetSystemEventState(handle, id, state);
	if (res == S_OK)
	{
		callbackMap[id] = std::make_pair(callable,
				util::Singletons::get<RecvTypeConverter, 1>().getConverterForID(static_cast<SIMCONNECT_RECV_ID>(recvID)));
	}
	return res;
}

HRESULT DispatchHandler::subscribeInputEvent(const char *inputTrigger, object callable, const int &id, const SIMCONNECT_STATE &state,
		const DWORD &priority, const char *simEvent)
{
	assert(id > 0);
	EventIDCallbackType &callbackMap = eventMap[SIMCONNECT_RECV_ID_EVENT];
	HANDLE handle = PyCapsule_GetPointer(_handle.get(), NULL);
	const HRESULT hr1 = SimConnect_MapClientEventToSimEvent(handle, id, simEvent);

// we are using 0 for the init group
	const HRESULT hr2 = SimConnect_AddClientEventToNotificationGroup(handle, 0, id);
	const HRESULT hr3 = SimConnect_SetNotificationGroupPriority(handle, 0, priority);

	const HRESULT hr4 = SimConnect_MapInputEventToClientEvent(handle, id, inputTrigger, id);
	const HRESULT hr5 = SimConnect_SetInputGroupState(handle, id, state);

	if ((hr1 || hr2 || hr3 || hr4 || hr5) == 0)
	{
		callbackMap[id] = std::make_pair(callable,
				util::Singletons::get<RecvTypeConverter, 1>().getConverterForID(SIMCONNECT_RECV_ID_EVENT));
		return S_OK;
	}
	else
	{
		return E_FAIL;
	}
}

void DispatchHandler::subscribeRecvIDEvent(const DWORD &recvID, object callable)
{
	recvIdMap[recvID] = std::make_pair(callable,
			util::Singletons::get<RecvTypeConverter, 1>().getConverterForID(static_cast<SIMCONNECT_RECV_ID>(recvID)));
}

HRESULT DispatchHandler::subscribeDataEvent(const object &event)
{
	// retrieve data
	const list simulation_variables = extract<list>(event.attr("_variables"));
	const SIMCONNECT_DATA_REQUEST_ID id = extract<SIMCONNECT_DATA_REQUEST_ID>(event.attr("_id"));
	const SIMCONNECT_DATA_DEFINITION_ID dataDefinitionID = extract<SIMCONNECT_DATA_DEFINITION_ID>(event.attr("_data_definition_id"));
	const SIMCONNECT_PERIOD period = extract<SIMCONNECT_PERIOD>(event.attr("_period"));
	const DWORD flags = extract<DWORD>(event.attr("_flags"));
	object callback = extract<object>(event.attr("_callback"));

	const boost::python::ssize_t n = boost::python::len(simulation_variables);
	const HANDLE handle = PyCapsule_GetPointer(_handle.get(), NULL);

	// register all data fields associated with this DataEvent
	const DataEventStructureInfoType &dataTypeList = dataToDefinition(handle, dataDefinitionID, simulation_variables);

	HRESULT ret = SimConnect_RequestDataOnSimObject(handle, id, dataDefinitionID, extract<SIMCONNECT_OBJECT_ID>(event.attr("_object_id")),
			period, flags);
	if (ret == S_OK)
	{
		dataEventMap[id] = boost::make_tuple(callback, dataTypeList, boost::shared_ptr<boost::python::dict>(new boost::python::dict()));
	}
	return ret;
}

void DispatchHandler::subscribeRadiusData(const object &radiusData)
{
//	{
//		std::cout << "huhu5" << std::endl;
//		SimConnect_RequestDataOnSimObjectType(handle, id, dataDefinitionID, extract<DWORD>(event.attr("_radius")),
//				extract<SIMCONNECT_SIMOBJECT_TYPE>(event.attr("_object_type")));
//		radiusDataEventList.push_back(
//				RadiusDataEventInfoType(id, dataDefinitionID, extract<DWORD>(event.attr("_radius")),
//						extract<SIMCONNECT_SIMOBJECT_TYPE>(event.attr("_object_type"))));
//	}

}

DispatchHandler::DataEventStructureInfoType DispatchHandler::dataToDefinition(HANDLE handle,
		const SIMCONNECT_DATA_DEFINITION_ID &dataDefinitionID, const boost::python::list &dataList)
{
	DataEventStructureInfoType dataTypeList;
	const boost::python::ssize_t n = boost::python::len(dataList);
	for (boost::python::ssize_t i = 0; i < n; ++i)
	{
		boost::python::object simulation_variable = extract<object>(dataList[i]);

		const char * dataName = extract<const char *>(simulation_variable.attr("_name"));
		const char * dataUnit = extract<const char *>(simulation_variable.attr("_unit"));
		const char * attribute = extract<const char *>(simulation_variable.attr("_key"));
		const SIMCONNECT_DATATYPE &dataType = extract<SIMCONNECT_DATATYPE>(simulation_variable.attr("_data_type"));

		const DataTypeConverter::SizeFunctionType &sizeFunction = util::Singletons::get<DataTypeConverter, 1>().getConverter(dataType);
		dataTypeList.push_back(std::make_pair(std::string(attribute), sizeFunction));
		SimConnect_AddToDataDefinition(handle, dataDefinitionID, dataName, strlen(dataUnit) > 0 ? dataUnit : NULL, dataType,
				extract<float>(simulation_variable.attr("_epsilon")), extract<int>(simulation_variable.attr("_id")));
	}

	return dataTypeList;
}

void DispatchHandler::listen(const DWORD &sleepTime)
{
	HRESULT res = S_OK;
	const HANDLE handle = PyCapsule_GetPointer(_handle.get(), NULL);
	while (res == S_OK)
	{
		res = SimConnect_CallDispatch(handle, _internal::__dispatchCallback__, NULL);
		Sleep(sleepTime);
	}
}

} // end namespace simconnect
} // end namespace prepar3d

