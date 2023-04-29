#include "coher_internal.h"
#include <stdbool.h>

extern int sentrequests;
extern uint32_t if_shared;
extern bool redo;
extern uint32_t sharing_threshold;

uint32_t countSetBits(uint32_t if_shared) {
    uint32_t cnt =0;
    while(if_shared) {
        cnt += if_shared & 1;
        if_shared >>= 1;
    }
    return cnt;
}

void sendBusRd(uint64_t addr, int procNum)
{
    sentrequests++;
    inter_sim->busReq(BUSRD, addr, procNum);
}

void sendBusWr(uint64_t addr, int procNum)
{
    sentrequests++;
    inter_sim->busReq(BUSWR, addr, procNum);
}

void sendData(uint64_t addr, int procNum)
{
    sentrequests++;
    inter_sim->busReq(DATA, addr, procNum);
}

void indicateShared(uint64_t addr, int procNum)
{
    sentrequests++;
    inter_sim->busReq(SHARED, addr, procNum);
}

void sendBusUpd(uint64_t addr, int procNum)
{
    sentrequests++;
    inter_sim->busReq(BUSUPDATE, addr, procNum);
}

coherence_states
cacheMI(uint8_t is_read, uint8_t* permAvail, coherence_states currentState,
        uint64_t addr, int procNum)
{
    switch (currentState)
    {
        case INVALID:
            *permAvail = 0;
            sendBusWr(addr, procNum);
            // printf("Cache - Case Invalid : Sending BusWr - is_read %d, permAvail %d, current_state %d, addr %lx, procNum %d\n", is_read, *permAvail, currentState, addr, procNum);
            return INVALID_MODIFIED;
        case MODIFIED:
            *permAvail = 1;
            // printf("Cache - Case Modified : is_read %d, permAvail %d, current_state %d, addr %lx, procNum %d\n", is_read, *permAvail, currentState, addr, procNum);
            return MODIFIED;
        case INVALID_MODIFIED:
            fprintf(stderr, "IM state on %lx, but request %d\n", addr,
                    is_read);
            *permAvail = 0;
            // printf("Cache - Case Modified : is_read %d, permAvail %d, current_state %d, addr %lx, procNum %d\n", is_read, *permAvail, currentState, addr, procNum);
            return INVALID_MODIFIED;
        default:
            fprintf(stderr, "State %d not supported, found on %lx\n",
                    currentState, addr);
            break;
    }

    return INVALID;
}

coherence_states
cacheMSI(uint8_t is_read, uint8_t* permAvail, coherence_states currentState,
        uint64_t addr, int procNum)
{
    switch (currentState)
    {
        case INVALID:
            *permAvail = 0;
            if (is_read) {
                sendBusRd(addr, procNum);
                // printf("Cache - Case Invalid -> Invalid Shared : Sending BusRd - is_read %d, permAvail %d, current_state %d, addr %lx, procNum %d\n", is_read, *permAvail, currentState, addr, procNum);
                return INVALID_SHARED;
            } else {
                sendBusWr(addr, procNum);
                // printf("Cache - Case Invalid -> Invalid Modified : Sending BusWr - is_read %d, permAvail %d, current_state %d, addr %lx, procNum %d\n", is_read, *permAvail, currentState, addr, procNum);
                return INVALID_MODIFIED;
            }
        case MODIFIED:
            *permAvail = 1;
            // printf("Cache - Case Modified - is_read %d, permAvail %d, current_state %d, addr %lx, procNum %d\n", is_read, *permAvail, currentState, addr, procNum);
            return MODIFIED;
        case SHAREDST:
            if (is_read) {
                *permAvail = 1;
                // printf("Cache - Case Shared - is_read %d, permAvail %d, current_state %d, addr %lx, procNum %d\n", is_read, *permAvail, currentState, addr, procNum);
                return SHAREDST;
            } else {
                *permAvail = 0;
                sendBusWr(addr, procNum);
                // printf("Cache - Case Shared : Sending BusWr - is_read %d, permAvail %d, current_state %d, addr %lx, procNum %d\n", is_read, *permAvail, currentState, addr, procNum);
                return SHARED_MODIFIED;
            }
        case INVALID_MODIFIED:
            fprintf(stderr, "IM state on %lx, but request %d\n", addr,
                    is_read);
            *permAvail = 0;
            // printf("Cache - Case Invalid_modified - is_read %d, permAvail %d, current_state %d, addr %lx, procNum %d\n", is_read, *permAvail, currentState, addr, procNum);
            return INVALID_MODIFIED;
        case INVALID_SHARED:
            fprintf(stderr, "IS state on %lx, but request %d\n", addr,
                    is_read);
            *permAvail = 0;
            // printf("Cache - Case Invalid_shared - is_read %d, permAvail %d, current_state %d, addr %lx, procNum %d\n", is_read, *permAvail, currentState, addr, procNum);
            return INVALID_SHARED;
        case SHARED_MODIFIED:
            fprintf(stderr, "SM state on %lx, but request %d\n", addr,
                    is_read);
            *permAvail = 0;
            // printf("Cache - Case Shared_modified - is_read %d, permAvail %d, current_state %d, addr %lx, procNum %d\n", is_read, *permAvail, currentState, addr, procNum);
            return SHARED_MODIFIED;
        default:
            fprintf(stderr, "State %d not supported, found on %lx\n",
                    currentState, addr);
            break;
    }

    return INVALID;
}

coherence_states
snoopMI(bus_req_type reqType, cache_action* ca, coherence_states currentState,
        uint64_t addr, int procNum, int reqProc)
{
    *ca = NO_ACTION;
    switch (currentState)
    {
        case INVALID:
        // printf("Snoop - Case Invalid : reqType %d, cache action %d, current_state %d, addr %lx, procNum %d\n", reqType, *ca, currentState, addr, procNum);
            return INVALID;
        case MODIFIED:
            sendData(addr, procNum);
            // indicateShared(addr, procNum); // Needed for E state
            *ca = INVALIDATE;
            // printf("Snoop - Case Modified - Send data : reqType %d, cache action %d, current_state %d, addr %lx, procNum %d\n", reqType, *ca, currentState, addr, procNum);
            return INVALID;
        case INVALID_MODIFIED:
            assert(reqType != SHARED);
            if (reqType == DATA || reqType == SHARED)
            {
                *ca = DATA_RECV;
                // printf("Snoop - Case Invalid_modified to Modified : reqType %d, cache action %d, current_state %d, addr %lx, procNum %d\n", reqType, *ca, currentState, addr, procNum);
                return MODIFIED;
            }
            // printf("Snoop - Case Invalid_modified : reqType %d, cache action %d, current_state %d, addr %lx, procNum %d\n", reqType, *ca, currentState, addr, procNum);
            return INVALID_MODIFIED;
        default:
            fprintf(stderr, "State %d not supported, found on %lx\n",
                    currentState, addr);
            break;
    }

    return INVALID;
}

coherence_states
snoopMSI(bus_req_type reqType, cache_action* ca, coherence_states currentState,
        uint64_t addr, int procNum, int reqProc)
{
    *ca = NO_ACTION;
    switch (currentState)
    {
        case INVALID:
        // printf("Snoop - Case Invalid : reqType %d, cache action %d, current_state %d, addr %lx, procNum %d\n", reqType, *ca, currentState, addr, procNum);
            return INVALID;
        case MODIFIED:
            sendData(addr, procNum);
            if (reqType == SHARED || reqType == BUSRD) {
                *ca = SHARE;
                // printf("Snoop - Case Modified -> shared - Send data : reqType %d, cache action %d, current_state %d, addr %lx, procNum %d\n", reqType, *ca, currentState, addr, procNum);
                return SHAREDST;
            } else if (reqType == BUSWR) {
                *ca = INVALIDATE;
                // printf("Snoop - Case Modified -> invalid - Send data : reqType %d, cache action %d, current_state %d, addr %lx, procNum %d\n", reqType, *ca, currentState, addr, procNum);
                return INVALID;
            } else {
                // printf("Snoop - Case Modified -> modified - Send data : reqType %d, cache action %d, current_state %d, addr %lx, procNum %d\n", reqType, *ca, currentState, addr, procNum);
                return MODIFIED;
            }
            // indicateShared(addr, procNum); // Needed for E state
        case SHAREDST:
            if (reqType == BUSWR) {
                *ca = INVALIDATE;
                // printf("Snoop - Case shared -> invalid  : reqType %d, cache action %d, current_state %d, addr %lx, procNum %d\n", reqType, *ca, currentState, addr, procNum);
                return INVALID;
            } else {
                // printf("Snoop - Case shared -> shared : reqType %d, cache action %d, current_state %d, addr %lx, procNum %d\n", reqType, *ca, currentState, addr, procNum);
                return SHAREDST;
            }
        case INVALID_MODIFIED:
            if (reqType == DATA)
            {
                *ca = DATA_RECV;
                // printf("Snoop - Case invalid_Modified -> modified  : reqType %d, cache action %d, current_state %d, addr %lx, procNum %d\n", reqType, *ca, currentState, addr, procNum);
                return MODIFIED;
            }
            // printf("Snoop - Case invalid_Modified -> invalid_modified  : reqType %d, cache action %d, current_state %d, addr %lx, procNum %d\n", reqType, *ca, currentState, addr, procNum);
            return INVALID_MODIFIED;
        case INVALID_SHARED:
            if (reqType == DATA || reqType == SHARED)
            {
                *ca = DATA_RECV;
                // printf("Snoop - Case invalid_shared -> shared  : reqType %d, cache action %d, current_state %d, addr %lx, procNum %d\n", reqType, *ca, currentState, addr, procNum);
                return SHAREDST;
            }
            // printf("Snoop - Case invalid_shared -> invalid_shared  : reqType %d, cache action %d, current_state %d, addr %lx, procNum %d\n", reqType, *ca, currentState, addr, procNum);
            return INVALID_SHARED;
        case SHARED_MODIFIED:
            if (reqType == DATA)
            {
                *ca = DATA_RECV;
                // printf("Snoop - Case shared_modified -> modified  : reqType %d, cache action %d, current_state %d, addr %lx, procNum %d\n", reqType, *ca, currentState, addr, procNum);
                return MODIFIED;
            } else if (reqType == NO_REQ) {
                // printf("Snoop - Case shared_modified -> modified : reqType %d, cache action %d, current_state %d, addr %lx, procNum %d\n", reqType, *ca, currentState, addr, procNum);
                return MODIFIED;
            }
            // printf("Snoop - Case shared_modified -> shared_modified : reqType %d, cache action %d, current_state %d, addr %lx, procNum %d\n", reqType, *ca, currentState, addr, procNum);
            return SHARED_MODIFIED;
        default:
            fprintf(stderr, "State %d not supported, found on %lx\n",
                    currentState, addr);
            break;
    }

    return INVALID;
}

coherence_states
cacheMESI(uint8_t is_read, uint8_t* permAvail, coherence_states currentState,
        uint64_t addr, int procNum)
{
    switch (currentState)
    {
        case INVALID:
            *permAvail = 0;
            if (is_read) {
                sendBusRd(addr, procNum);
                // printf("Cache - Case Invalid -> Invalid Shared : Sending BusRd - is_read %d, permAvail %d, current_state %d, addr %lx, procNum %d\n", is_read, *permAvail, currentState, addr, procNum);
                return INVALID_SHARED;
            } else {
                sendBusWr(addr, procNum);
                // printf("Cache - Case Invalid -> Invalid Modified : Sending BusWr - is_read %d, permAvail %d, current_state %d, addr %lx, procNum %d\n", is_read, *permAvail, currentState, addr, procNum);
                return INVALID_MODIFIED;
            }
        case MODIFIED:
            *permAvail = 1;
            // printf("Cache - Case Modified - is_read %d, permAvail %d, current_state %d, addr %lx, procNum %d\n", is_read, *permAvail, currentState, addr, procNum);
            return MODIFIED;
        case SHAREDST:
            if (is_read) {
                *permAvail = 1;
                // printf("Cache - Case Shared - is_read %d, permAvail %d, current_state %d, addr %lx, procNum %d\n", is_read, *permAvail, currentState, addr, procNum);
                return SHAREDST;
            } else {
                *permAvail = 0;
                sendBusWr(addr, procNum);
                // printf("Cache - Case Shared -> shared modified : Sending BusWr - is_read %d, permAvail %d, current_state %d, addr %lx, procNum %d\n", is_read, *permAvail, currentState, addr, procNum);
                return SHARED_MODIFIED;
            }
        case EXCLUSIVE:
            if (is_read) {
                *permAvail = 1;
                // printf("Cache - Case Exclusive - is_read %d, permAvail %d, current_state %d, addr %lx, procNum %d\n", is_read, *permAvail, currentState, addr, procNum);
                return EXCLUSIVE;
            } else {
                *permAvail = 1;
                // printf("Cache - Case Exclusive -> modified: is_read %d, permAvail %d, current_state %d, addr %lx, procNum %d\n", is_read, *permAvail, currentState, addr, procNum);
                return MODIFIED;
            }
        case INVALID_MODIFIED:
            fprintf(stderr, "IM state on %lx, but request %d\n", addr,
                    is_read);
            *permAvail = 0;
            // printf("Cache - Case Invalid_modified - is_read %d, permAvail %d, current_state %d, addr %lx, procNum %d\n", is_read, *permAvail, currentState, addr, procNum);
            return INVALID_MODIFIED;
        case INVALID_SHARED:
            fprintf(stderr, "IS state on %lx, but request %d\n", addr,
                    is_read);
            *permAvail = 0;
            // printf("Cache - Case Invalid_shared - is_read %d, permAvail %d, current_state %d, addr %lx, procNum %d\n", is_read, *permAvail, currentState, addr, procNum);
            return INVALID_SHARED;
        case SHARED_MODIFIED:
            fprintf(stderr, "SM state on %lx, but request %d\n", addr,
                    is_read);
            *permAvail = 0;
            // printf("Cache - Case Shared_modified - is_read %d, permAvail %d, current_state %d, addr %lx, procNum %d\n", is_read, *permAvail, currentState, addr, procNum);
            return SHARED_MODIFIED;
        default:
            fprintf(stderr, "State %d not supported, found on %lx\n",
                    currentState, addr);
            break;
    }

    return INVALID;
}

coherence_states
snoopMESI(bus_req_type reqType, cache_action* ca, coherence_states currentState,
        uint64_t addr, int procNum, int reqProc)
{
    *ca = NO_ACTION;
    switch (currentState)
    {
        case INVALID:
        // printf("Snoop - Case Invalid : reqType %d, cache action %d, current_state %d, addr %lx, procNum %d\n", reqType, *ca, currentState, addr, procNum);
            return INVALID;
        case MODIFIED:
            if (reqType == SHARED || reqType == BUSRD) {
                indicateShared(addr, procNum);
                *ca = SHARE;
                // printf("Snoop - Case Modified -> shared - indicateShared : reqType %d, cache action %d, current_state %d, addr %lx, procNum %d\n", reqType, *ca, currentState, addr, procNum);
                return SHAREDST;
            } else if (reqType == BUSWR) {
                sendData(addr, procNum);
                *ca = INVALIDATE;
                // printf("Snoop - Case Modified -> invalid - Send data : reqType %d, cache action %d, current_state %d, addr %lx, procNum %d\n", reqType, *ca, currentState, addr, procNum);
                return INVALID;
            } else {
                // printf("Snoop - Case Modified -> modified: reqType %d, cache action %d, current_state %d, addr %lx, procNum %d\n", reqType, *ca, currentState, addr, procNum);
                return MODIFIED;
            }
            // indicateShared(addr, procNum); // Needed for E state
        case SHAREDST:
            if (reqType == BUSWR) {
                *ca = INVALIDATE;
                // printf("Snoop - Case shared -> invalid  : reqType %d, cache action %d, current_state %d, addr %lx, procNum %d\n", reqType, *ca, currentState, addr, procNum);
                return INVALID;
            } else {
                // printf("Snoop - Case shared -> shared : reqType %d, cache action %d, current_state %d, addr %lx, procNum %d\n", reqType, *ca, currentState, addr, procNum);
                return SHAREDST;
            }
        case EXCLUSIVE:
            if (reqType == SHARED || reqType == BUSRD) {
                indicateShared(addr, procNum);
                *ca = SHARE;
                // printf("Snoop - Case Exclusive -> shared - indicateShared : reqType %d, cache action %d, current_state %d, addr %lx, procNum %d\n", reqType, *ca, currentState, addr, procNum);
                return SHAREDST;
            } else if (reqType == BUSWR) {
                sendData(addr, procNum);
                *ca = INVALIDATE;
                // printf("Snoop - Case exclusive -> invalid  : send data - reqType %d, cache action %d, current_state %d, addr %lx, procNum %d\n", reqType, *ca, currentState, addr, procNum);
                return INVALID;
            } else {
                // printf("Snoop - Case exclusive: reqType %d, cache action %d, current_state %d, addr %lx, procNum %d\n", reqType, *ca, currentState, addr, procNum);
                return EXCLUSIVE;
            }
        case INVALID_MODIFIED:
            if (reqType == DATA)
            {
                *ca = DATA_RECV;
                // printf("Snoop - Case invalid_Modified -> modified  : reqType %d, cache action %d, current_state %d, addr %lx, procNum %d\n", reqType, *ca, currentState, addr, procNum);
                return MODIFIED;
            }
            // printf("Snoop - Case invalid_Modified -> invalid_modified  : reqType %d, cache action %d, current_state %d, addr %lx, procNum %d\n", reqType, *ca, currentState, addr, procNum);
            return INVALID_MODIFIED;
        case INVALID_SHARED:
            if (reqType == SHARED)
            {
                *ca = DATA_RECV;
                // printf("Snoop - Case invalid_shared -> shared  : reqType %d, cache action %d, current_state %d, addr %lx, procNum %d\n", reqType, *ca, currentState, addr, procNum);
                return SHAREDST;
            } else if (reqType == DATA) {
                *ca = DATA_RECV;
                // printf("Snoop - Case invalid_shared -> exclusive  : reqType %d, cache action %d, current_state %d, addr %lx, procNum %d\n", reqType, *ca, currentState, addr, procNum);
                return EXCLUSIVE;
            }
            // printf("Snoop - Case invalid_shared -> invalid_shared  : reqType %d, cache action %d, current_state %d, addr %lx, procNum %d\n", reqType, *ca, currentState, addr, procNum);
            return INVALID_SHARED;
        case SHARED_MODIFIED:
            if (reqType == DATA)
            {
                *ca = DATA_RECV;
                // printf("Snoop - Case shared_modified -> modified  : reqType %d, cache action %d, current_state %d, addr %lx, procNum %d\n", reqType, *ca, currentState, addr, procNum);
                return MODIFIED;
            }
            // printf("Snoop - Case shared_modified -> shared_modified : reqType %d, cache action %d, current_state %d, addr %lx, procNum %d\n", reqType, *ca, currentState, addr, procNum);
            return SHARED_MODIFIED;
        default:
            fprintf(stderr, "State %d not supported, found on %lx\n",
                    currentState, addr);
            break;
    }

    return INVALID;
}


coherence_states
cacheMOESI(uint8_t is_read, uint8_t* permAvail, coherence_states currentState,
        uint64_t addr, int procNum)
{
    switch (currentState)
    {
        case INVALID:
            *permAvail = 0;
            if (is_read) {
                sendBusRd(addr, procNum);
                // printf("Cache - Case Invalid -> Invalid Shared : Sending BusRd - is_read %d, permAvail %d, current_state %d, addr %lx, procNum %d\n", is_read, *permAvail, currentState, addr, procNum);
                return INVALID_SHARED;
            } else {
                sendBusWr(addr, procNum);
                // printf("Cache - Case Invalid -> Invalid Modified : Sending BusWr - is_read %d, permAvail %d, current_state %d, addr %lx, procNum %d\n", is_read, *permAvail, currentState, addr, procNum);
                return INVALID_MODIFIED;
            }
        case MODIFIED:
            *permAvail = 1;
            // printf("Cache - Case Modified - is_read %d, permAvail %d, current_state %d, addr %lx, procNum %d\n", is_read, *permAvail, currentState, addr, procNum);
            return MODIFIED;
        case SHAREDST:
            if (is_read) {
                *permAvail = 1;
                // printf("Cache - Case Shared - is_read %d, permAvail %d, current_state %d, addr %lx, procNum %d\n", is_read, *permAvail, currentState, addr, procNum);
                return SHAREDST;
            } else {
                *permAvail = 0;
                sendBusWr(addr, procNum);
                // printf("Cache - Case Shared -> shared modified : Sending BusWr - is_read %d, permAvail %d, current_state %d, addr %lx, procNum %d\n", is_read, *permAvail, currentState, addr, procNum);
                return SHARED_MODIFIED;
            }
        case EXCLUSIVE:
            if (is_read) {
                *permAvail = 1;
                // printf("Cache - Case Exclusive - is_read %d, permAvail %d, current_state %d, addr %lx, procNum %d\n", is_read, *permAvail, currentState, addr, procNum);
                return EXCLUSIVE;
            } else {
                *permAvail = 1;
                // printf("Cache - Case Exclusive -> modified: is_read %d, permAvail %d, current_state %d, addr %lx, procNum %d\n", is_read, *permAvail, currentState, addr, procNum);
                return MODIFIED;
            }
        case OWNED:
            if (is_read) {
                *permAvail = 1;
                // printf("Cache - Case owned - is_read %d, permAvail %d, current_state %d, addr %lx, procNum %d\n", is_read, *permAvail, currentState, addr, procNum);
                return OWNED;
            } else {
                *permAvail = 0;
                sendBusWr(addr, procNum);
                // printf("Cache - Case owned -> owned_modified: is_read %d, permAvail %d, current_state %d, addr %lx, procNum %d\n", is_read, *permAvail, currentState, addr, procNum);
                return OWNED_MODIFIED;
            }
        case INVALID_MODIFIED:
            fprintf(stderr, "IM state on %lx, but request %d\n", addr,
                    is_read);
            *permAvail = 0;
            // printf("Cache - Case Invalid_modified - is_read %d, permAvail %d, current_state %d, addr %lx, procNum %d\n", is_read, *permAvail, currentState, addr, procNum);
            return INVALID_MODIFIED;
        case INVALID_SHARED:
            fprintf(stderr, "IS state on %lx, but request %d\n", addr,
                    is_read);
            *permAvail = 0;
            // printf("Cache - Case Invalid_shared - is_read %d, permAvail %d, current_state %d, addr %lx, procNum %d\n", is_read, *permAvail, currentState, addr, procNum);
            return INVALID_SHARED;
        case SHARED_MODIFIED:
            fprintf(stderr, "SM state on %lx, but request %d\n", addr,
                    is_read);
            *permAvail = 0;
            // printf("Cache - Case Shared_modified - is_read %d, permAvail %d, current_state %d, addr %lx, procNum %d\n", is_read, *permAvail, currentState, addr, procNum);
            return SHARED_MODIFIED;
        case OWNED_MODIFIED:
            fprintf(stderr, "OM state on %lx, but request %d\n", addr,
                    is_read);
            *permAvail = 0;
            // printf("Cache - Case owned_modified - is_read %d, permAvail %d, current_state %d, addr %lx, procNum %d\n", is_read, *permAvail, currentState, addr, procNum);
            return OWNED_MODIFIED;
        default:
            fprintf(stderr, "State %d not supported, found on %lx\n",
                    currentState, addr);
            break;
    }

    return INVALID;
}

coherence_states
snoopMOESI(bus_req_type reqType, cache_action* ca, coherence_states currentState,
        uint64_t addr, int procNum, int reqProc)
{
    *ca = NO_ACTION;
    switch (currentState)
    {
        case INVALID:
        // printf("Snoop - Case Invalid : reqType %d, cache action %d, current_state %d, addr %lx, procNum %d\n", reqType, *ca, currentState, addr, procNum);
            return INVALID;
        case MODIFIED:
            if (reqType == SHARED || reqType == BUSRD) {
                indicateShared(addr, procNum);
                *ca = SHARE;
                // printf("Snoop - Case Modified -> owned - indicateShared : reqType %d, cache action %d, current_state %d, addr %lx, procNum %d\n", reqType, *ca, currentState, addr, procNum);
                return OWNED;
            } else if (reqType == BUSWR) {
                sendData(addr, procNum);
                *ca = INVALIDATE;
                // printf("Snoop - Case Modified -> invalid - Send data : reqType %d, cache action %d, current_state %d, addr %lx, procNum %d\n", reqType, *ca, currentState, addr, procNum);
                return INVALID;
            } else {
                // printf("Snoop - Case Modified -> modified: reqType %d, cache action %d, current_state %d, addr %lx, procNum %d\n", reqType, *ca, currentState, addr, procNum);
                return MODIFIED;
            }
            // indicateShared(addr, procNum); // Needed for E state
        case SHAREDST:
            if (reqType == BUSWR) {
                *ca = INVALIDATE;
                // printf("Snoop - Case shared -> invalid  : reqType %d, cache action %d, current_state %d, addr %lx, procNum %d\n", reqType, *ca, currentState, addr, procNum);
                return INVALID;
            } else {
                // printf("Snoop - Case shared -> shared : reqType %d, cache action %d, current_state %d, addr %lx, procNum %d\n", reqType, *ca, currentState, addr, procNum);
                return SHAREDST;
            }
        case EXCLUSIVE:
            if (reqType == SHARED || reqType == BUSRD) {
                indicateShared(addr, procNum);
                *ca = SHARE;
                // printf("Snoop - Case Exclusive -> shared - indicateShared : reqType %d, cache action %d, current_state %d, addr %lx, procNum %d\n", reqType, *ca, currentState, addr, procNum);
                return SHAREDST;
            } else if (reqType == BUSWR) {
                sendData(addr, procNum);
                *ca = INVALIDATE;
                // printf("Snoop - Case exclusive -> invalid  : send data - reqType %d, cache action %d, current_state %d, addr %lx, procNum %d\n", reqType, *ca, currentState, addr, procNum);
                return INVALID;
            } else {
                // printf("Snoop - Case exclusive: reqType %d, cache action %d, current_state %d, addr %lx, procNum %d\n", reqType, *ca, currentState, addr, procNum);
                return EXCLUSIVE;
            }
        case OWNED:
            if (reqType == SHARED || reqType == BUSRD) {
                indicateShared(addr, procNum);
                *ca = SHARE;
                // printf("Snoop - Case Owned - indicateShared : reqType %d, cache action %d, current_state %d, addr %lx, procNum %d\n", reqType, *ca, currentState, addr, procNum);
                return OWNED;
            } else if (reqType == BUSWR) {
                sendData(addr, procNum);
                *ca = INVALIDATE;
                // printf("Snoop - Case Owned -> invalid  : send data - reqType %d, cache action %d, current_state %d, addr %lx, procNum %d\n", reqType, *ca, currentState, addr, procNum);
                return INVALID;
            } else {
                // printf("Snoop - Case Owned: reqType %d, cache action %d, current_state %d, addr %lx, procNum %d\n", reqType, *ca, currentState, addr, procNum);
                return OWNED;
            }
        case INVALID_MODIFIED:
            if (reqType == DATA)
            {
                *ca = DATA_RECV;
                // printf("Snoop - Case invalid_Modified -> modified  : reqType %d, cache action %d, current_state %d, addr %lx, procNum %d\n", reqType, *ca, currentState, addr, procNum);
                return MODIFIED;
            }
            // printf("Snoop - Case invalid_Modified -> invalid_modified  : reqType %d, cache action %d, current_state %d, addr %lx, procNum %d\n", reqType, *ca, currentState, addr, procNum);
            return INVALID_MODIFIED;
        case INVALID_SHARED:
            if (reqType == SHARED)
            {
                *ca = DATA_RECV;
                // printf("Snoop - Case invalid_shared -> shared  : reqType %d, cache action %d, current_state %d, addr %lx, procNum %d\n", reqType, *ca, currentState, addr, procNum);
                return SHAREDST;
            } else if (reqType == DATA) {
                *ca = DATA_RECV;
                // printf("Snoop - Case invalid_shared -> exclusive  : reqType %d, cache action %d, current_state %d, addr %lx, procNum %d\n", reqType, *ca, currentState, addr, procNum);
                return EXCLUSIVE;
            }
            // printf("Snoop - Case invalid_shared -> invalid_shared  : reqType %d, cache action %d, current_state %d, addr %lx, procNum %d\n", reqType, *ca, currentState, addr, procNum);
            return INVALID_SHARED;
        case SHARED_MODIFIED:
            if (reqType == DATA)
            {
                *ca = DATA_RECV;
                // printf("Snoop - Case shared_modified -> modified  : reqType %d, cache action %d, current_state %d, addr %lx, procNum %d\n", reqType, *ca, currentState, addr, procNum);
                return MODIFIED;
            }
            // printf("Snoop - Case shared_modified -> shared_modified : reqType %d, cache action %d, current_state %d, addr %lx, procNum %d\n", reqType, *ca, currentState, addr, procNum);
            return SHARED_MODIFIED;
        case OWNED_MODIFIED:
            if (reqType == DATA) {
                *ca = DATA_RECV;
                // printf("Snoop - Case owned_modified -> modified  : reqType %d, cache action %d, current_state %d, addr %lx, procNum %d\n", reqType, *ca, currentState, addr, procNum);
                return MODIFIED;
            }
            // printf("Snoop - Case owned_modified -> owned_modified : reqType %d, cache action %d, current_state %d, addr %lx, procNum %d\n", reqType, *ca, currentState, addr, procNum);
            return OWNED_MODIFIED;
        default:
            fprintf(stderr, "State %d not supported, found on %lx\n",
                    currentState, addr);
            break;
    }

    return INVALID;
}

coherence_states
cacheDragon(uint8_t is_read, uint8_t* permAvail, coherence_states currentState,
        uint64_t addr, int procNum)
{
    switch (currentState)
    {
        case INVALID:
            *permAvail = 0;
            if (is_read) {
                sendBusRd(addr, procNum);
                // printf("Cache - Case Invalid -> Invalid Shared : Sending BusRd - is_read %d, permAvail %d, current_state %d, addr %lx, procNum %d\n", is_read, *permAvail, currentState, addr, procNum);
                return INVALID_SHARED;
            } else {
                sendBusWr(addr, procNum);
                // printf("Cache - Case Invalid -> Invalid Modified : Sending BusWr - is_read %d, permAvail %d, current_state %d, addr %lx, procNum %d\n", is_read, *permAvail, currentState, addr, procNum);
                return INVALID_MODIFIED;
            }
        case MODIFIED:
            *permAvail = 1;
            // printf("Cache - Case Modified :- is_read %d, permAvail %d, current_state %d, addr %lx, procNum %d\n", is_read, *permAvail, currentState, addr, procNum);
            return MODIFIED;
        case EXCLUSIVE_CLEAN:
            *permAvail = 1;
            if (is_read) {
                // printf("Cache - Case Exclusive Clean - is_read %d, permAvail %d, current_state %d, addr %lx, procNum %d\n", is_read, *permAvail, currentState, addr, procNum);
                return EXCLUSIVE_CLEAN;
            } else {
                // printf("Cache - Case Exclusive Clean -> Modified: is_read %d, permAvail %d, current_state %d, addr %lx, procNum %d\n", is_read, *permAvail, currentState, addr, procNum);
                return MODIFIED;
            }
        case SHARED_CLEAN:
            if (is_read) {
                *permAvail = 1;
                // printf("Cache - Case Shared_Clean - is_read %d, permAvail %d, current_state %d, addr %lx, procNum %d\n", is_read, *permAvail, currentState, addr, procNum);
                return SHARED_CLEAN;
            } else {
                *permAvail = 0;
                sendBusUpd(addr, procNum);
                // printf("Cache - Case Shared_clean -> Shared_clean_modified : Sending BusWr - is_read %d, permAvail %d, current_state %d, addr %lx, procNum %d\n", is_read, *permAvail, currentState, addr, procNum);
                return SHARED_CLEAN_MODIFIED;
            }
        case DRAGON_SHARED_MODIFIED:
            if (is_read) {
                *permAvail = 1;
                // printf("Cache - Case dragon_Shared_modified - is_read %d, permAvail %d, current_state %d, addr %lx, procNum %d\n", is_read, *permAvail, currentState, addr, procNum);
                return DRAGON_SHARED_MODIFIED;
            } else {
                *permAvail = 0;
                sendBusUpd(addr, procNum);
                // printf("Cache - Case dragon_Shared_modified -> dragon_shared_modified_INT: Sending BusUpdate - is_read %d, permAvail %d, current_state %d, addr %lx, procNum %d\n", is_read, *permAvail, currentState, addr, procNum);
                return DRAGON_SHARED_MODIFIED_INT;
            }
        case INVALID_MODIFIED:
            fprintf(stderr, "IM state on %lx, but request %d\n", addr,
                    is_read);
            *permAvail = 0;
            // printf("Cache - Case Invalid_modified - is_read %d, permAvail %d, current_state %d, addr %lx, procNum %d\n", is_read, *permAvail, currentState, addr, procNum);
            return INVALID_MODIFIED;
        case INVALID_SHARED:
            fprintf(stderr, "IS state on %lx, but request %d\n", addr,
                    is_read);
            *permAvail = 0;
            // printf("Cache - Case Invalid_shared - is_read %d, permAvail %d, current_state %d, addr %lx, procNum %d\n", is_read, *permAvail, currentState, addr, procNum);
            return INVALID_SHARED;
        case SHARED_CLEAN_MODIFIED:
            fprintf(stderr, "SCM state on %lx, but request %d\n", addr,
                    is_read);
            *permAvail = 0;
            // printf("Cache - Case shared_clean_modified - is_read %d, permAvail %d, current_state %d, addr %lx, procNum %d\n", is_read, *permAvail, currentState, addr, procNum);
            return SHARED_CLEAN_MODIFIED;
        case DRAGON_SHARED_MODIFIED_INT:
            fprintf(stderr, "SMI state on %lx, but request %d\n", addr,
                    is_read);
            *permAvail = 0;
            // printf("Cache - Case dragon_shared_modified_int - is_read %d, permAvail %d, current_state %d, addr %lx, procNum %d\n", is_read, *permAvail, currentState, addr, procNum);
            return DRAGON_SHARED_MODIFIED_INT;
        default:
            fprintf(stderr, "State %d not supported, found on %lx\n",
                    currentState, addr);
            break;
    }

    return INVALID;
}

coherence_states
snoopDragon(bus_req_type reqType, cache_action* ca, coherence_states currentState,
        uint64_t addr, int procNum, int reqProc)
{
    *ca = NO_ACTION;
    switch (currentState)
    {
        case INVALID:
        if_shared &= ~(1 << procNum);
        // printf("Snoop - Case Invalid : reqType %d, cache action %d, current_state %d, addr %lx, procNum %d\n", reqType, *ca, currentState, addr, procNum);
            return INVALID;
        case MODIFIED:
            assert(reqType == BUSRD || reqType == BUSWR);
            if (reqType == BUSRD) {
                indicateShared(addr, procNum);
                *ca = SHARE;
                if_shared |= 1 << procNum;
                // printf("Snoop - Case Modified -> dragon_shared_modified - indicateShared : reqType %d, cache action %d, current_state %d, addr %lx, procNum %d\n", reqType, *ca, currentState, addr, procNum);
                return DRAGON_SHARED_MODIFIED;
            } else if (reqType == BUSWR) {
                indicateShared(addr, procNum);
                *ca = UPDATE;
                if_shared |= 1 << procNum;
                // printf("Snoop - Case Modified -> Shared clean - Send data : reqType %d, cache action %d, current_state %d, addr %lx, procNum %d\n", reqType, *ca, currentState, addr, procNum);
                return SHARED_CLEAN;
            } 
            // else {
            //     // assert(reqType != BUSUPDATE && reqType != DATA && reqType !=SHARED);
            //     if_shared &= ~(1 << procNum);
            //     // printf("Snoop - Case Modified -> modified: reqType %d, cache action %d, current_state %d, addr %lx, procNum %d\n", reqType, *ca, currentState, addr, procNum);
            //     return MODIFIED;
            // }
        case EXCLUSIVE_CLEAN:
            assert(reqType == BUSRD || reqType == BUSWR);
            if (reqType == BUSRD || reqType == BUSWR) {
                *ca = SHARE;
                if_shared |= 1 << procNum;
                // printf("Snoop - Case exclusive clean -> shared clean  : send data - reqType %d, cache action %d, current_state %d, addr %lx, procNum %d\n", reqType, *ca, currentState, addr, procNum);
                return SHARED_CLEAN;
            } else {
                if_shared &= ~(1 << procNum);
                // printf("Snoop - Case exclusive: reqType %d, cache action %d, current_state %d, addr %lx, procNum %d\n", reqType, *ca, currentState, addr, procNum);
                return EXCLUSIVE;
            }
        case SHARED_CLEAN:
            assert(!(reqType == BUSUPDATE && reqProc == procNum));
            assert(reqType == BUSRD || reqType == BUSWR || reqType == BUSUPDATE);
            if (reqType == BUSWR || (reqType == BUSUPDATE && reqProc != procNum)) {
                *ca = UPDATE;
                if_shared |= 1 << procNum;
                // printf("Snoop - Case shared clean -> shared clean  : reqType %d, cache action %d, current_state %d, addr %lx, procNum %d\n", reqType, *ca, currentState, addr, procNum);
                return SHARED_CLEAN;
            } 
            // else if (reqType == BUSUPDATE && reqProc == procNum) {
            //     if_shared |= 1 << procNum;
            //     *ca = DATA_RECV;
            //     // printf("Snoop - Case shared clean -> dragon shared modified  : reqType %d, cache action %d, current_state %d, addr %lx, procNum %d\n", reqType, *ca, currentState, addr, procNum);
            //     return DRAGON_SHARED_MODIFIED;
            // }
            else {
                if_shared |= 1 << procNum;
                // printf("Snoop - Case shared clean -> shared clean : reqType %d, cache action %d, current_state %d, addr %lx, procNum %d\n", reqType, *ca, currentState, addr, procNum);
                return SHARED_CLEAN;
            }
        case DRAGON_SHARED_MODIFIED:
            assert(!(reqType == BUSUPDATE && reqProc == procNum));
            assert(!(reqType == SHARED && reqProc == procNum));
            assert(reqType != DATA);
            if (reqType == BUSRD) {
                indicateShared(addr, procNum);
                *ca = SHARE;
                if_shared |= 1 << procNum;
                // printf("Snoop - Case dragon shared modified -> dragon shared modified - indicateShared : reqType %d, cache action %d, current_state %d, addr %lx, procNum %d\n", reqType, *ca, currentState, addr, procNum);
                return DRAGON_SHARED_MODIFIED;
            } else if (reqType == BUSWR || (reqType == BUSUPDATE && reqProc != procNum)) {
                indicateShared(addr, procNum);
                *ca = UPDATE;
                if_shared |= 1 << procNum;
                // printf("Snoop - Case dragon shared modified -> shared clean  : send data - reqType %d, cache action %d, current_state %d, addr %lx, procNum %d\n", reqType, *ca, currentState, addr, procNum);
                return SHARED_CLEAN;
            } else {
                if_shared |= 1 << procNum;
                // printf("Snoop - Case dragon shared modified : reqType %d, cache action %d, current_state %d, addr %lx, procNum %d\n", reqType, *ca, currentState, addr, procNum);
                return DRAGON_SHARED_MODIFIED;
            }
        case INVALID_MODIFIED:
            if (reqType == DATA)
            {
                *ca = DATA_RECV;
                if_shared &= ~(1 << procNum);
                // printf("Snoop - Case invalid_Modified -> modified  : reqType %d, cache action %d, current_state %d, addr %lx, procNum %d\n", reqType, *ca, currentState, addr, procNum);
                return MODIFIED;
            } else if (reqType == SHARED && reqProc == procNum) {
                *ca = DATA_RECV;
                if_shared |= 1 << procNum;
                // printf("Snoop - Case invalid_Modified -> dragon shared modified  : reqType %d, cache action %d, current_state %d, addr %lx, procNum %d\n", reqType, *ca, currentState, addr, procNum);
                return DRAGON_SHARED_MODIFIED;
            }
            if_shared &= ~(1 << procNum);
            // printf("Snoop - Case invalid_Modified -> invalid_modified  : reqType %d, cache action %d, current_state %d, addr %lx, procNum %d\n", reqType, *ca, currentState, addr, procNum);
            return INVALID_MODIFIED;
        case INVALID_SHARED:
            if (reqType == SHARED)
            {
                *ca = DATA_RECV;
                if_shared |= 1 << procNum;
                // printf("Snoop - Case invalid_shared -> shared clean : reqType %d, cache action %d, current_state %d, addr %lx, procNum %d\n", reqType, *ca, currentState, addr, procNum);
                return SHARED_CLEAN;
            } else if (reqType == DATA) {
                *ca = DATA_RECV;
                if_shared &= ~(1 << procNum);
                // printf("Snoop - Case invalid_shared -> exclusive clean  : reqType %d, cache action %d, current_state %d, addr %lx, procNum %d\n", reqType, *ca, currentState, addr, procNum);
                return EXCLUSIVE_CLEAN;
            }
            if_shared &= ~(1 << procNum);
            // printf("Snoop - Case invalid_shared -> invalid_shared  : reqType %d, cache action %d, current_state %d, addr %lx, procNum %d\n", reqType, *ca, currentState, addr, procNum);
            return INVALID_SHARED;
        case SHARED_CLEAN_MODIFIED:
            if (reqType == BUSUPDATE && reqProc == procNum) {
                *ca = DATA_RECV;
                if_shared |= 1 << procNum;
                // printf("Snoop - Case shared_clean_modified -> dragon_shared_modified  : reqType %d, cache action %d, current_state %d, addr %x, procNum %d\n", reqType, *ca, currentState, addr, procNum);
                return DRAGON_SHARED_MODIFIED;
            }
            if_shared |= 1 << procNum;
            return SHARED_CLEAN_MODIFIED;
        case DRAGON_SHARED_MODIFIED_INT:
            if ((reqType == BUSUPDATE) && reqProc == procNum) {
                *ca = DATA_RECV;
                if_shared |= 1 << procNum;
                // printf("Snoop - Case Dragon Shared modified int -> dragon shared modified  : reqType %d, cache action %d, current_state %d, addr %x, procNum %d reqProc %d\n", reqType, *ca, currentState, addr, procNum, reqProc);
                return DRAGON_SHARED_MODIFIED;
            }
            // printf("Snoop - Case Dragon Shared modified int : reqType %d, cache action %d, current_state %d, addr %x, procNum %d reqProc %d\n", reqType, *ca, currentState, addr, procNum, reqProc);
            if_shared |= 1 << procNum;
            return DRAGON_SHARED_MODIFIED_INT;
        default:
            fprintf(stderr, "State %d not supported, found on %lx\n",
                    currentState, addr);
            break;
    }

    return INVALID;
}

coherence_states
cacheHybridDragon(uint8_t is_read, uint8_t* permAvail, coherence_states currentState,
        uint64_t addr, int procNum)
{
    switch (currentState)
    {
        case INVALID:
            *permAvail = 0;
            if (is_read) {
                sendBusRd(addr, procNum);
                // printf("Cache - Case Invalid -> Invalid Shared : Sending BusRd - is_read %d, permAvail %d, current_state %d, addr %lx, procNum %d\n", is_read, *permAvail, currentState, addr, procNum);
                return INVALID_SHARED;
            } else {
                sendBusWr(addr, procNum);
                // printf("Cache - Case Invalid -> Invalid Modified : Sending BusWr - is_read %d, permAvail %d, current_state %d, addr %lx, procNum %d\n", is_read, *permAvail, currentState, addr, procNum);
                return INVALID_MODIFIED;
            }
        case MODIFIED:
            *permAvail = 1;
            // printf("Cache - Case Modified :- is_read %d, permAvail %d, current_state %d, addr %lx, procNum %d\n", is_read, *permAvail, currentState, addr, procNum);
            return MODIFIED;
        case EXCLUSIVE_CLEAN:
            *permAvail = 1;
            if (is_read) {
                // printf("Cache - Case Exclusive Clean - is_read %d, permAvail %d, current_state %d, addr %lx, procNum %d\n", is_read, *permAvail, currentState, addr, procNum);
                return EXCLUSIVE_CLEAN;
            } else {
                // printf("Cache - Case Exclusive Clean -> Modified: is_read %d, permAvail %d, current_state %d, addr %lx, procNum %d\n", is_read, *permAvail, currentState, addr, procNum);
                return MODIFIED;
            }
        case SHARED_CLEAN:
            if (is_read) {
                *permAvail = 1;
                // printf("Cache - Case Shared_Clean - is_read %d, permAvail %d, current_state %d, addr %lx, procNum %d\n", is_read, *permAvail, currentState, addr, procNum);
                return SHARED_CLEAN;
            } else {
                *permAvail = 0;
                sendBusUpd(addr, procNum);
                // printf("Cache - Case Shared_clean -> Shared_clean_modified : Sending BusWr - is_read %d, permAvail %d, current_state %d, addr %lx, procNum %d\n", is_read, *permAvail, currentState, addr, procNum);
                return SHARED_CLEAN_MODIFIED;
            }
        case DRAGON_SHARED_MODIFIED:
            if (is_read) {
                *permAvail = 1;
                // printf("Cache - Case dragon_Shared_modified - is_read %d, permAvail %d, current_state %d, addr %lx, procNum %d\n", is_read, *permAvail, currentState, addr, procNum);
                return DRAGON_SHARED_MODIFIED;
            } else {
                *permAvail = 0;
                sendBusUpd(addr, procNum);
                // printf("Cache - Case dragon_Shared_modified -> dragon_shared_modified_INT: Sending BusUpdate - is_read %d, permAvail %d, current_state %d, addr %lx, procNum %d\n", is_read, *permAvail, currentState, addr, procNum);
                return DRAGON_SHARED_MODIFIED_INT;
            }
        case INVALID_MODIFIED:
            fprintf(stderr, "IM state on %lx, but request %d\n", addr,
                    is_read);
            *permAvail = 0;
            // printf("Cache - Case Invalid_modified - is_read %d, permAvail %d, current_state %d, addr %lx, procNum %d\n", is_read, *permAvail, currentState, addr, procNum);
            return INVALID_MODIFIED;
        case INVALID_SHARED:
            fprintf(stderr, "IS state on %lx, but request %d\n", addr,
                    is_read);
            *permAvail = 0;
            // printf("Cache - Case Invalid_shared - is_read %d, permAvail %d, current_state %d, addr %lx, procNum %d\n", is_read, *permAvail, currentState, addr, procNum);
            return INVALID_SHARED;
        case SHARED_CLEAN_MODIFIED:
            fprintf(stderr, "SCM state on %lx, but request %d\n", addr,
                    is_read);
            *permAvail = 0;
            // printf("Cache - Case shared_clean_modified - is_read %d, permAvail %d, current_state %d, addr %lx, procNum %d\n", is_read, *permAvail, currentState, addr, procNum);
            return SHARED_CLEAN_MODIFIED;
        case DRAGON_SHARED_MODIFIED_INT:
            fprintf(stderr, "SMI state on %lx, but request %d\n", addr,
                    is_read);
            *permAvail = 0;
            // printf("Cache - Case dragon_shared_modified_int - is_read %d, permAvail %d, current_state %d, addr %lx, procNum %d\n", is_read, *permAvail, currentState, addr, procNum);
            return DRAGON_SHARED_MODIFIED_INT;
        default:
            fprintf(stderr, "State %d not supported, found on %lx\n",
                    currentState, addr);
            break;
    }

    return INVALID;
}

coherence_states
snoopHybridDragon(bus_req_type reqType, cache_action* ca, coherence_states currentState,
        uint64_t addr, int procNum, int reqProc)
{
    *ca = NO_ACTION;
    switch (currentState)
    {
        case INVALID:
        if_shared &= ~(1 << procNum);
        // printf("Snoop - Case Invalid : reqType %d, cache action %d, current_state %d, addr %lx, procNum %d\n", reqType, *ca, currentState, addr, procNum);
            return INVALID;
        case MODIFIED:
            assert(reqType == BUSRD || reqType == BUSWR);
            if (reqType == BUSRD) {
                indicateShared(addr, procNum);
                *ca = SHARE;
                if_shared |= 1 << procNum;
                // printf("Snoop - Case Modified -> dragon_shared_modified - indicateShared : reqType %d, cache action %d, current_state %d, addr %lx, procNum %d\n", reqType, *ca, currentState, addr, procNum);
                return DRAGON_SHARED_MODIFIED;
            } else if (reqType == BUSWR) {
                indicateShared(addr, procNum);
                *ca = UPDATE;
                if_shared |= 1 << procNum;
                // printf("Snoop - Case Modified -> Shared clean - Send data : reqType %d, cache action %d, current_state %d, addr %lx, procNum %d\n", reqType, *ca, currentState, addr, procNum);
                return SHARED_CLEAN;
            }
        case EXCLUSIVE_CLEAN:
            assert(reqType == BUSRD || reqType == BUSWR);
            if (reqType == BUSRD || reqType == BUSWR) {
                *ca = SHARE;
                if_shared |= 1 << procNum;
                // printf("Snoop - Case exclusive clean -> shared clean  : send data - reqType %d, cache action %d, current_state %d, addr %lx, procNum %d\n", reqType, *ca, currentState, addr, procNum);
                return SHARED_CLEAN;
            } else {
                if_shared &= ~(1 << procNum);
                // printf("Snoop - Case exclusive: reqType %d, cache action %d, current_state %d, addr %lx, procNum %d\n", reqType, *ca, currentState, addr, procNum);
                return EXCLUSIVE;
            }
        case SHARED_CLEAN:
            assert(!(reqType == BUSUPDATE && reqProc == procNum));
            assert(reqType == BUSRD || reqType == BUSWR || reqType == BUSUPDATE);
            if (reqType == BUSWR) {
                *ca = UPDATE;
                if_shared |= 1 << procNum;
                // printf("Snoop - Case shared clean -> shared clean  : reqType %d, cache action %d, current_state %d, addr %lx, procNum %d\n", reqType, *ca, currentState, addr, procNum);
                return SHARED_CLEAN;
            } else if (reqType == BUSUPDATE && reqProc != procNum) {
                redo = true; 
                return SHARED_CLEAN;
            } else if (reqType == REDO) {
                if (countSetBits(if_shared) > sharing_threshold) {
                    return SHARED_CLEAN;
                } else {
                    return INVALID;
                }
            }
            else {
                if_shared |= 1 << procNum;
                // printf("Snoop - Case shared clean -> shared clean : reqType %d, cache action %d, current_state %d, addr %lx, procNum %d\n", reqType, *ca, currentState, addr, procNum);
                return SHARED_CLEAN;
            }
        case DRAGON_SHARED_MODIFIED:
            assert(!(reqType == BUSUPDATE && reqProc == procNum));
            assert(!(reqType == SHARED && reqProc == procNum));
            assert(reqType != DATA);
            if (reqType == BUSRD) {
                indicateShared(addr, procNum);
                *ca = SHARE;
                if_shared |= 1 << procNum;
                // printf("Snoop - Case dragon shared modified -> dragon shared modified - indicateShared : reqType %d, cache action %d, current_state %d, addr %lx, procNum %d\n", reqType, *ca, currentState, addr, procNum);
                return DRAGON_SHARED_MODIFIED;
            } else if (reqType == BUSWR || (reqType == BUSUPDATE && reqProc != procNum)) {
                indicateShared(addr, procNum);
                *ca = UPDATE;
                if_shared |= 1 << procNum;
                // printf("Snoop - Case dragon shared modified -> shared clean  : send data - reqType %d, cache action %d, current_state %d, addr %lx, procNum %d\n", reqType, *ca, currentState, addr, procNum);
                return SHARED_CLEAN;
            } else {
                if_shared |= 1 << procNum;
                // printf("Snoop - Case dragon shared modified : reqType %d, cache action %d, current_state %d, addr %lx, procNum %d\n", reqType, *ca, currentState, addr, procNum);
                return DRAGON_SHARED_MODIFIED;
            }
        case INVALID_MODIFIED:
            if (reqType == DATA)
            {
                *ca = DATA_RECV;
                if_shared &= ~(1 << procNum);
                // printf("Snoop - Case invalid_Modified -> modified  : reqType %d, cache action %d, current_state %d, addr %lx, procNum %d\n", reqType, *ca, currentState, addr, procNum);
                return MODIFIED;
            } else if (reqType == SHARED && reqProc == procNum) {
                *ca = DATA_RECV;
                if_shared |= 1 << procNum;
                // printf("Snoop - Case invalid_Modified -> dragon shared modified  : reqType %d, cache action %d, current_state %d, addr %lx, procNum %d\n", reqType, *ca, currentState, addr, procNum);
                return DRAGON_SHARED_MODIFIED;
            }
            if_shared &= ~(1 << procNum);
            // printf("Snoop - Case invalid_Modified -> invalid_modified  : reqType %d, cache action %d, current_state %d, addr %lx, procNum %d\n", reqType, *ca, currentState, addr, procNum);
            return INVALID_MODIFIED;
        case INVALID_SHARED:
            if (reqType == SHARED)
            {
                *ca = DATA_RECV;
                if_shared |= 1 << procNum;
                // printf("Snoop - Case invalid_shared -> shared clean : reqType %d, cache action %d, current_state %d, addr %lx, procNum %d\n", reqType, *ca, currentState, addr, procNum);
                return SHARED_CLEAN;
            } else if (reqType == DATA) {
                *ca = DATA_RECV;
                if_shared &= ~(1 << procNum);
                // printf("Snoop - Case invalid_shared -> exclusive clean  : reqType %d, cache action %d, current_state %d, addr %lx, procNum %d\n", reqType, *ca, currentState, addr, procNum);
                return EXCLUSIVE_CLEAN;
            }
            if_shared &= ~(1 << procNum);
            // printf("Snoop - Case invalid_shared -> invalid_shared  : reqType %d, cache action %d, current_state %d, addr %lx, procNum %d\n", reqType, *ca, currentState, addr, procNum);
            return INVALID_SHARED;
        case SHARED_CLEAN_MODIFIED:
            if (reqType == BUSUPDATE && reqProc == procNum) {
                if (countSetBits(if_shared) > sharing_threshold) {
                    *ca = DATA_RECV;
                    return DRAGON_SHARED_MODIFIED;
                } else {
                    *ca = DATA_RECV;
                    return MODIFIED;
                }
            }
            if_shared |= 1 << procNum;
            return SHARED_CLEAN_MODIFIED;
        case DRAGON_SHARED_MODIFIED_INT:
            if ((reqType == BUSUPDATE) && reqProc == procNum) {
                if (countSetBits(if_shared) > sharing_threshold) {
                    *ca = DATA_RECV;
                    return DRAGON_SHARED_MODIFIED;
                } else {
                    *ca = DATA_RECV;
                    return MODIFIED;
                }
            }
            // printf("Snoop - Case Dragon Shared modified int : reqType %d, cache action %d, current_state %d, addr %x, procNum %d reqProc %d\n", reqType, *ca, currentState, addr, procNum, reqProc);
            if_shared |= 1 << procNum;
            return DRAGON_SHARED_MODIFIED_INT;
        default:
            fprintf(stderr, "State %d not supported, found on %lx\n",
                    currentState, addr);
            break;
    }

    return INVALID;
}