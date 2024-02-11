/** WFPDriver.h

The WFPDriver is intended to demonstrate how to set up a
driver that utilizes the Windows Filtering Platform to register
a Callout and Filter to the Base Filtering Engine.

Author: Jared Wright - 2015
*/

#include "WFPDriver.h"
#include "Callout.h"

/************************************
    Private Data and Prototypes
************************************/
// Global handle to the WFP Base Filter Engine
HANDLE filter_engine_handle = NULL;

#define DEVICE_STRING L"\\Device\\WFPDriver"
#define DOS_DEVICE_STRING L"\\DosDevices\\WFPDriver"

// Data and constants for the Callout
#define CALLOUT_NAME        L"Callout"
#define CALLOUT_DESCRIPTION L"A callout used for demonstration purposes"
UINT32 callout_id;    // This id gets assigned by the filter engine, must preserve to unregister
DEFINE_GUID(CALLOUT_GUID,    // 191551e5-1601-4931-b49e-b3d85b3eaf40
	0x191551e5, 0x1601, 0x4931, 0xb4, 0x9e, 0xb3, 0xd8, 0x5b, 0x3e, 0xaf, 0x40);

// Data and constants for the Sublayer
#define SUBLAYER_NAME L"Sublayer"
#define SUBLAYER_DESCRIPTION L"A sublayer used to hold filters in a callout driver"
DEFINE_GUID(SUBLAYER_GUID,    // b5766767-532a-4f3c-80ec-899ea1cf2c93
	0xb5766767, 0x532a, 0x4f3c, 0x80, 0xec, 0x89, 0x9e, 0xa1, 0xcf, 0x2c, 0x93);

// Data and constants for the Filter
#define FILTER_NAME L"Filter"
#define FILTER_DESCRIPTION L"A filter that uses the callout"
UINT64 filter_id = 0;

// Driver entry and exit points
DRIVER_INITIALIZE DriverEntry;
DRIVER_UNLOAD DriverUnload;
EVT_WDF_DRIVER_UNLOAD empty_evt_unload;

// Initializes required WDFDriver and WDFDevice objects
NTSTATUS init_driver_objects(DRIVER_OBJECT* driver_obj, UNICODE_STRING* registry_path,
    WDFDRIVER* driver, WDFDEVICE* device);

// Demonstrates how to register/unregister a callout, sublayer, and filter to the Base Filtering Engine
NTSTATUS register_example_callout(DEVICE_OBJECT* wdm_device);
NTSTATUS unregister_example_callout();
NTSTATUS register_example_sublayer();
NTSTATUS register_example_filter();
NTSTATUS unregister_example_filter();

/************************************
			Functions
************************************/
NTSTATUS DriverEntry(IN PDRIVER_OBJECT driver_obj, IN PUNICODE_STRING registry_path)
{
	NTSTATUS status = STATUS_SUCCESS;
	WDFDRIVER driver = { 0 };
	WDFDEVICE device = { 0 };
	DEVICE_OBJECT* wdm_device = NULL;
	FWPM_SESSION wdf_session = { 0 };
	BOOLEAN in_transaction = FALSE;
	BOOLEAN callout_registered = FALSE;

	status = init_driver_objects(driver_obj, registry_path, &driver, &device);
	if (!NT_SUCCESS(status)) goto Exit;

	// Begin a transaction to the FilterEngine. You must register objects (filter, callouts, sublayers)
	//to the filter engine in the context of a 'transaction'
	wdf_session.flags = FWPM_SESSION_FLAG_DYNAMIC;	// <-- Automatically destroys all filters and callouts after this wdf_session ends
	status = FwpmEngineOpen(NULL, RPC_C_AUTHN_WINNT, NULL, &wdf_session, &filter_engine_handle);
	if (!NT_SUCCESS(status)) goto Exit;
	status = FwpmTransactionBegin(filter_engine_handle, 0);
	if (!NT_SUCCESS(status)) goto Exit;
	in_transaction = TRUE;

	// Register the example Callout to the filter engine
	wdm_device = WdfDeviceWdmGetDeviceObject(device);
	status = register_example_callout(wdm_device);
	if (!NT_SUCCESS(status)) goto Exit;
	callout_registered = TRUE;

	// Register a new sublayer to the filter engine
	status = register_example_sublayer();
	if (!NT_SUCCESS(status)) goto Exit;

	// Register a Filter that uses the example callout
	status = register_example_filter();
	if (!NT_SUCCESS(status)) goto Exit;

	// Commit transaction to the Filter Engine
	status = FwpmTransactionCommit(filter_engine_handle);
	if (!NT_SUCCESS(status)) goto Exit;
	in_transaction = FALSE;

	// Define this driver's unload function
	driver_obj->DriverUnload = DriverUnload;

	// Cleanup and handle any errors
Exit:
	if (!NT_SUCCESS(status)) {
		DbgPrint("WFPDriver example driver failed to load, status 0x%08x", status);
		if (in_transaction == TRUE) {
			FwpmTransactionAbort(filter_engine_handle);
			_Analysis_assume_lock_not_held_(filter_engine_handle); // Potential leak if "FwpmTransactionAbort" fails
		}
		if (callout_registered == TRUE) {
			unregister_example_callout();
		}
		status = STATUS_FAILED_DRIVER_ENTRY;
	}
	else {
		DbgPrint("--- WFPDriver example driver loaded successfully ---");
	}

	return status;
}

NTSTATUS init_driver_objects(DRIVER_OBJECT* driver_obj, UNICODE_STRING* registry_path,
	WDFDRIVER* driver, WDFDEVICE* device)
{
	NTSTATUS status = STATUS_SUCCESS;
	WDF_DRIVER_CONFIG config = { 0 };
	UNICODE_STRING device_name = { 0 };
	UNICODE_STRING device_symlink = { 0 };
	PWDFDEVICE_INIT device_init = NULL;

	RtlInitUnicodeString(&device_name, DEVICE_STRING);
	RtlInitUnicodeString(&device_symlink, DOS_DEVICE_STRING);

	// Create a WDFDRIVER for this driver
	WDF_DRIVER_CONFIG_INIT(&config, WDF_NO_EVENT_CALLBACK);
	config.DriverInitFlags = WdfDriverInitNonPnpDriver;
	config.EvtDriverUnload = empty_evt_unload; // <-- Necessary for this driver to unload correctly
	status = WdfDriverCreate(driver_obj, registry_path, WDF_NO_OBJECT_ATTRIBUTES, &config, driver);
	if (!NT_SUCCESS(status)) goto Exit;

	// Create a WDFDEVICE for this driver
	device_init = WdfControlDeviceInitAllocate(*driver, &SDDL_DEVOBJ_SYS_ALL_ADM_ALL);	// only admins and kernel can access device
	if (!device_init) {
		status = STATUS_INSUFFICIENT_RESOURCES;
		goto Exit;
	}

	// Configure the WDFDEVICE_INIT with a name to allow for access from user mode
	WdfDeviceInitSetDeviceType(device_init, FILE_DEVICE_NETWORK);
	WdfDeviceInitSetCharacteristics(device_init, FILE_DEVICE_SECURE_OPEN, FALSE);
	WdfDeviceInitAssignName(device_init, &device_name);
	WdfPdoInitAssignRawDevice(device_init, &GUID_DEVCLASS_NET);
	WdfDeviceInitSetDeviceClass(device_init, &GUID_DEVCLASS_NET);

	status = WdfDeviceCreate(&device_init, WDF_NO_OBJECT_ATTRIBUTES, device);
	if (!NT_SUCCESS(status)) {
		WdfDeviceInitFree(device_init);
		goto Exit;
	}

	WdfControlFinishInitializing(*device);

Exit:
	return status;
}

NTSTATUS register_example_callout(DEVICE_OBJECT* wdm_device)
{
	NTSTATUS status = STATUS_SUCCESS;
	FWPS_CALLOUT s_callout = { 0 };
	FWPM_CALLOUT m_callout = { 0 };
	FWPM_DISPLAY_DATA display_data = { 0 };

	if (filter_engine_handle == NULL)
		return STATUS_INVALID_HANDLE;

	display_data.name = CALLOUT_NAME;
	display_data.description = CALLOUT_DESCRIPTION;

	// Register a new Callout with the Filter Engine using the provided callout functions
	s_callout.calloutKey = CALLOUT_GUID;
	s_callout.classifyFn = classify;
	s_callout.notifyFn = notify;
	s_callout.flowDeleteFn = flow_delete;
	status = FwpsCalloutRegister((void*)wdm_device, &s_callout, &callout_id);
	if (!NT_SUCCESS(status)) {
		DbgPrint("Failed to register callout functions for example callout, status 0x%08x", status);
		goto Exit;
	}

	// Setup a FWPM_CALLOUT structure to store/track the state associated with the FWPS_CALLOUT
	m_callout.calloutKey = CALLOUT_GUID;
	m_callout.displayData = display_data;
	m_callout.applicableLayer = FWPM_LAYER_OUTBOUND_TRANSPORT_V4;
	m_callout.flags = 0;
	status = FwpmCalloutAdd(filter_engine_handle, &m_callout, NULL, NULL);
	if (!NT_SUCCESS(status)) {
		DbgPrint("Failed to register example callout, status 0x%08x", status);
	}
	else {
		DbgPrint("Example Callout Registered");
	}

Exit:
	return status;
}

NTSTATUS unregister_example_callout()
{
	return FwpsCalloutUnregisterById(callout_id);
}

NTSTATUS register_example_sublayer()
{
	NTSTATUS status = STATUS_SUCCESS;
	FWPM_SUBLAYER sublayer = { 0 };

	sublayer.subLayerKey = SUBLAYER_GUID;
	sublayer.displayData.name = SUBLAYER_NAME;
	sublayer.displayData.description = SUBLAYER_DESCRIPTION;
	sublayer.flags = 0;
	sublayer.weight = 0x0f;
	status = FwpmSubLayerAdd(filter_engine_handle, &sublayer, NULL);
	if (!NT_SUCCESS(status)) {
		DbgPrint("Failed to register example sublayer, status 0x%08x", status);
	}
	else {
		DbgPrint("Example sublayer registered");
	}
	return status;
}

NTSTATUS register_example_filter()
{
	NTSTATUS status = STATUS_SUCCESS;
	FWPM_FILTER filter = { 0 };

	filter.displayData.name = FILTER_NAME;
	filter.displayData.description = FILTER_DESCRIPTION;
	filter.action.type = FWP_ACTION_CALLOUT_TERMINATING;	// Says this filter's callout MUST make a block/permit decission
	filter.subLayerKey = SUBLAYER_GUID;
	filter.weight.type = FWP_UINT8;
	filter.weight.uint8 = 0xf;		// The weight of this filter within its sublayer
	filter.numFilterConditions = 0;	// If you specify 0, this filter invokes its callout for all traffic in its layer
	filter.layerKey = FWPM_LAYER_OUTBOUND_TRANSPORT_V4;	// This layer must match the layer that ExampleCallout is registered to
	filter.action.calloutKey = CALLOUT_GUID;
	status = FwpmFilterAdd(filter_engine_handle, &filter, NULL, &(filter_id));
	if (status != STATUS_SUCCESS) {
		DbgPrint("Failed to register example filter, status 0x%08x", status);
	}
	else {
		DbgPrint("Example filter registered");
	}

	return status;
}

NTSTATUS unregister_example_filter()
{
	return FwpmFilterDeleteById(filter_engine_handle, filter_id);
}

VOID DriverUnload(PDRIVER_OBJECT driver_obj)
{
	NTSTATUS status = STATUS_SUCCESS;
	UNICODE_STRING symlink = { 0 };
	UNREFERENCED_PARAMETER(driver_obj);

	status = unregister_example_filter();
	if (!NT_SUCCESS(status)) DbgPrint("Failed to unregister filters, status: 0x%08x", status);
	status = unregister_example_callout();
	if (!NT_SUCCESS(status)) DbgPrint("Failed to unregister callout, status: 0x%08x", status);

	// Close handle to the WFP Filter Engine
	if (filter_engine_handle) {
		FwpmEngineClose(filter_engine_handle);
		filter_engine_handle = NULL;
	}

	RtlInitUnicodeString(&symlink, DOS_DEVICE_STRING);
	IoDeleteSymbolicLink(&symlink);

	DbgPrint("--- WFPDriver example driver unloaded ---");
	return;
}

VOID empty_evt_unload(WDFDRIVER Driver)
{
	UNREFERENCED_PARAMETER(Driver);
	return;
}