/** Callout.c

This implementation defines a Callout for inspecting outbound TCP traffic at the FWPM_LAYER_OUTBOUND_TRANSPORT_V4 layer. 
The classify function within this Callout examines the destination IP address of outbound packets. 
If a packet is destined for the specific IP address 168.63.129.16, it blocks the packet. 
For all other packets, it logs their TCP 4-tuple (source IP, source port, destination IP, destination port) information. 
The notify function provides log messages upon adding or deleting filters that utilize this Callout, aiding in debugging and monitoring. 
Additionally, a stub flow_delete function is included, though it currently performs no operation.

Based on the code of Jared Wright, only changed ClassifyFn Function to perform IP block instead of port block. 
Dmitry Porotnikov
*/

#include "Callout.h"

#define FORMAT_ADDR(x) (x>>24)&0xFF, (x>>16)&0xFF, (x>>8)&0xFF, x&0xFF

/*************************
    ClassifyFn Function
**************************/
void classify(
    const FWPS_INCOMING_VALUES* inFixedValues,
    const FWPS_INCOMING_METADATA_VALUES* inMetaValues,
    void* layerData,
    const void* classifyContext,
    const FWPS_FILTER* filter,
    UINT64 flowContext,
    FWPS_CLASSIFY_OUT* classifyOut)
{
    UINT32 local_address = inFixedValues->incomingValue[FWPS_FIELD_OUTBOUND_TRANSPORT_V4_IP_LOCAL_ADDRESS].value.uint32;
    UINT32 remote_address = inFixedValues->incomingValue[FWPS_FIELD_OUTBOUND_TRANSPORT_V4_IP_REMOTE_ADDRESS].value.uint32;
    UINT16 local_port = inFixedValues->incomingValue[FWPS_FIELD_OUTBOUND_TRANSPORT_V4_IP_LOCAL_PORT].value.uint16;
    UINT16 remote_port = inFixedValues->incomingValue[FWPS_FIELD_OUTBOUND_TRANSPORT_V4_IP_REMOTE_PORT].value.uint16;

    UNREFERENCED_PARAMETER(inMetaValues);
    UNREFERENCED_PARAMETER(layerData);
    UNREFERENCED_PARAMETER(classifyContext);
    UNREFERENCED_PARAMETER(flowContext);
    UNREFERENCED_PARAMETER(filter);

    // Directly calculate the IP address "168.63.129.16" in network byte order with explicit casting
    UINT32 blockIP = ((UINT32)168 << 24) | ((UINT32)63 << 16) | ((UINT32)129 << 8) | (UINT32)16;

    // If the packet is destined for IP address 168.63.129.16, block the packet
    if (remote_address == blockIP) {
        DbgPrint("Blocking Packet to IP 168.63.129.16");
        classifyOut->actionType = FWP_ACTION_BLOCK;
        return;
    }
    // Otherwise, print its TCP 4-tuple
    else {
        DbgPrint("Classify found a packet: %d.%d.%d.%d:%hu --> %d.%d.%d.%d:%hu",
            FORMAT_ADDR(local_address), local_port, FORMAT_ADDR(remote_address), remote_port);
    }

    classifyOut->actionType = FWP_ACTION_PERMIT;
    return;
}

/*************************
    NotifyFn Function
**************************/
NTSTATUS notify(
    FWPS_CALLOUT_NOTIFY_TYPE notifyType,
    const GUID* filterKey,
    const FWPS_FILTER* filter)
{
    NTSTATUS status = STATUS_SUCCESS;
    UNREFERENCED_PARAMETER(filterKey);
    UNREFERENCED_PARAMETER(filter);

    switch (notifyType) {
    case FWPS_CALLOUT_NOTIFY_ADD_FILTER:
        DbgPrint("A new filter has registered the Callout as its action");
        break;
    case FWPS_CALLOUT_NOTIFY_DELETE_FILTER:
        DbgPrint("A filter that uses the Callout has just been deleted");
        break;
    }
    return status;
}

/***************************
    FlowDeleteFn Function
****************************/
NTSTATUS flow_delete(UINT16 layerId, UINT32 calloutId, UINT64 flowContext)
{
    UNREFERENCED_PARAMETER(layerId);
    UNREFERENCED_PARAMETER(calloutId);
    UNREFERENCED_PARAMETER(flowContext);
    return STATUS_SUCCESS;
}
