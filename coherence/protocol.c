#include "coher_internal.h"

void sendBusRd(uint64_t addr, int procNum)
{
    inter_sim->busReq(BUSRD, addr, procNum);
}

void sendBusWr(uint64_t addr, int procNum)
{
    inter_sim->busReq(BUSWR, addr, procNum);
}

void sendData(uint64_t addr, int procNum)
{
    inter_sim->busReq(DATA, addr, procNum);
}

void indicateShared(uint64_t addr, int procNum)
{
    inter_sim->busReq(SHARED, addr, procNum);
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
            // printf("Cache - Case Invalid : Sending BusWr - is_read %d, permAvail %d, current_state %d, addr %x, procNum %d\n", is_read, *permAvail, currentState, addr, procNum);
            return INVALID_MODIFIED;
        case MODIFIED:
            *permAvail = 1;
            // printf("Cache - Case Modified : is_read %d, permAvail %d, current_state %d, addr %x, procNum %d\n", is_read, *permAvail, currentState, addr, procNum);
            return MODIFIED;
        case INVALID_MODIFIED:
            fprintf(stderr, "IM state on %lx, but request %d\n", addr,
                    is_read);
            *permAvail = 0;
            // printf("Cache - Case Modified : is_read %d, permAvail %d, current_state %d, addr %x, procNum %d\n", is_read, *permAvail, currentState, addr, procNum);
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
                // printf("Cache - Case Invalid -> Invalid Shared : Sending BusRd - is_read %d, permAvail %d, current_state %d, addr %x, procNum %d\n", is_read, *permAvail, currentState, addr, procNum);
                return INVALID_SHARED;
            } else {
                sendBusWr(addr, procNum);
                // printf("Cache - Case Invalid -> Invalid Modified : Sending BusWr - is_read %d, permAvail %d, current_state %d, addr %x, procNum %d\n", is_read, *permAvail, currentState, addr, procNum);
                return INVALID_MODIFIED;
            }
        case MODIFIED:
            *permAvail = 1;
            // printf("Cache - Case Modified - is_read %d, permAvail %d, current_state %d, addr %x, procNum %d\n", is_read, *permAvail, currentState, addr, procNum);
            return MODIFIED;
        case SHAREDST:
            if (is_read) {
                *permAvail = 1;
                // printf("Cache - Case Shared - is_read %d, permAvail %d, current_state %d, addr %x, procNum %d\n", is_read, *permAvail, currentState, addr, procNum);
                return SHAREDST;
            } else {
                *permAvail = 0;
                sendBusWr(addr, procNum);
                // printf("Cache - Case Shared : Sending BusWr - is_read %d, permAvail %d, current_state %d, addr %x, procNum %d\n", is_read, *permAvail, currentState, addr, procNum);
                return SHARED_MODIFIED;
            }
        case INVALID_MODIFIED:
            fprintf(stderr, "IM state on %lx, but request %d\n", addr,
                    is_read);
            *permAvail = 0;
            // printf("Cache - Case Invalid_modified - is_read %d, permAvail %d, current_state %d, addr %x, procNum %d\n", is_read, *permAvail, currentState, addr, procNum);
            return INVALID_MODIFIED;
        case INVALID_SHARED:
            fprintf(stderr, "IS state on %lx, but request %d\n", addr,
                    is_read);
            *permAvail = 0;
            // printf("Cache - Case Invalid_shared - is_read %d, permAvail %d, current_state %d, addr %x, procNum %d\n", is_read, *permAvail, currentState, addr, procNum);
            return INVALID_SHARED;
        case SHARED_MODIFIED:
            fprintf(stderr, "SM state on %lx, but request %d\n", addr,
                    is_read);
            *permAvail = 0;
            // printf("Cache - Case Shared_modified - is_read %d, permAvail %d, current_state %d, addr %x, procNum %d\n", is_read, *permAvail, currentState, addr, procNum);
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
        uint64_t addr, int procNum)
{
    *ca = NO_ACTION;
    switch (currentState)
    {
        case INVALID:
        // printf("Snoop - Case Invalid : reqType %d, cache action %d, current_state %d, addr %x, procNum %d\n", reqType, *ca, currentState, addr, procNum);
            return INVALID;
        case MODIFIED:
            sendData(addr, procNum);
            // indicateShared(addr, procNum); // Needed for E state
            *ca = INVALIDATE;
            // printf("Snoop - Case Modified - Send data : reqType %d, cache action %d, current_state %d, addr %x, procNum %d\n", reqType, *ca, currentState, addr, procNum);
            return INVALID;
        case INVALID_MODIFIED:
            if (reqType == DATA || reqType == SHARED)
            {
                *ca = DATA_RECV;
                // printf("Snoop - Case Invalid_modified to Modified : reqType %d, cache action %d, current_state %d, addr %x, procNum %d\n", reqType, *ca, currentState, addr, procNum);
                return MODIFIED;
            }
            // printf("Snoop - Case Invalid_modified : reqType %d, cache action %d, current_state %d, addr %x, procNum %d\n", reqType, *ca, currentState, addr, procNum);
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
        uint64_t addr, int procNum)
{
    *ca = NO_ACTION;
    switch (currentState)
    {
        case INVALID:
        // printf("Snoop - Case Invalid : reqType %d, cache action %d, current_state %d, addr %x, procNum %d\n", reqType, *ca, currentState, addr, procNum);
            return INVALID;
        case MODIFIED:
            sendData(addr, procNum);
            if (reqType == SHARED || reqType == BUSRD) {
                *ca = SHARE;
                // printf("Snoop - Case Modified -> shared - Send data : reqType %d, cache action %d, current_state %d, addr %x, procNum %d\n", reqType, *ca, currentState, addr, procNum);
                return SHAREDST;
            } else if (reqType == BUSWR) {
                *ca = INVALIDATE;
                // printf("Snoop - Case Modified -> invalid - Send data : reqType %d, cache action %d, current_state %d, addr %x, procNum %d\n", reqType, *ca, currentState, addr, procNum);
                return INVALID;
            } else {
                // printf("Snoop - Case Modified -> modified - Send data : reqType %d, cache action %d, current_state %d, addr %x, procNum %d\n", reqType, *ca, currentState, addr, procNum);
                return MODIFIED;
            }
            // indicateShared(addr, procNum); // Needed for E state
        case SHAREDST:
            if (reqType == BUSWR) {
                *ca = INVALIDATE;
                // printf("Snoop - Case shared -> invalid  : reqType %d, cache action %d, current_state %d, addr %x, procNum %d\n", reqType, *ca, currentState, addr, procNum);
                return INVALID;
            } else {
                // printf("Snoop - Case shared -> shared : reqType %d, cache action %d, current_state %d, addr %x, procNum %d\n", reqType, *ca, currentState, addr, procNum);
                return SHAREDST;
            }
        case INVALID_MODIFIED:
            if (reqType == DATA || reqType == SHARED)
            {
                *ca = DATA_RECV;
                // printf("Snoop - Case invalid_Modified -> modified  : reqType %d, cache action %d, current_state %d, addr %x, procNum %d\n", reqType, *ca, currentState, addr, procNum);
                return MODIFIED;
            }
            // printf("Snoop - Case invalid_Modified -> invalid_modified  : reqType %d, cache action %d, current_state %d, addr %x, procNum %d\n", reqType, *ca, currentState, addr, procNum);
            return INVALID_MODIFIED;
        case INVALID_SHARED:
            if (reqType == DATA || reqType == SHARED)
            {
                *ca = DATA_RECV;
                // printf("Snoop - Case invalid_shared -> shared  : reqType %d, cache action %d, current_state %d, addr %x, procNum %d\n", reqType, *ca, currentState, addr, procNum);
                return SHAREDST;
            }
            // printf("Snoop - Case invalid_shared -> invalid_shared  : reqType %d, cache action %d, current_state %d, addr %x, procNum %d\n", reqType, *ca, currentState, addr, procNum);
            return INVALID_SHARED;
        case SHARED_MODIFIED:
            if (reqType == DATA || reqType == SHARED)
            {
                *ca = DATA_RECV;
                // printf("Snoop - Case shared_modified -> modified  : reqType %d, cache action %d, current_state %d, addr %x, procNum %d\n", reqType, *ca, currentState, addr, procNum);
                return MODIFIED;
            }
            // printf("Snoop - Case shared_modified -> shared_modified : reqType %d, cache action %d, current_state %d, addr %x, procNum %d\n", reqType, *ca, currentState, addr, procNum);
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
                // // printf("Cache - Case Invalid -> Invalid Shared : Sending BusRd - is_read %d, permAvail %d, current_state %d, addr %x, procNum %d\n", is_read, *permAvail, currentState, addr, procNum);
                return INVALID_SHARED;
            } else {
                sendBusWr(addr, procNum);
                // // printf("Cache - Case Invalid -> Invalid Modified : Sending BusWr - is_read %d, permAvail %d, current_state %d, addr %x, procNum %d\n", is_read, *permAvail, currentState, addr, procNum);
                return INVALID_MODIFIED;
            }
        case MODIFIED:
            *permAvail = 1;
            // // printf("Cache - Case Modified - is_read %d, permAvail %d, current_state %d, addr %x, procNum %d\n", is_read, *permAvail, currentState, addr, procNum);
            return MODIFIED;
        case SHAREDST:
            if (is_read) {
                *permAvail = 1;
                // // printf("Cache - Case Shared - is_read %d, permAvail %d, current_state %d, addr %x, procNum %d\n", is_read, *permAvail, currentState, addr, procNum);
                return SHAREDST;
            } else {
                *permAvail = 0;
                sendBusWr(addr, procNum);
                // // printf("Cache - Case Shared -> shared modified : Sending BusWr - is_read %d, permAvail %d, current_state %d, addr %x, procNum %d\n", is_read, *permAvail, currentState, addr, procNum);
                return SHARED_MODIFIED;
            }
        case EXCLUSIVE:
            if (is_read) {
                *permAvail = 1;
                // // printf("Cache - Case Exclusive - is_read %d, permAvail %d, current_state %d, addr %x, procNum %d\n", is_read, *permAvail, currentState, addr, procNum);
                return EXCLUSIVE;
            } else {
                *permAvail = 1;
                // // printf("Cache - Case Exclusive -> modified: is_read %d, permAvail %d, current_state %d, addr %x, procNum %d\n", is_read, *permAvail, currentState, addr, procNum);
                return MODIFIED;
            }
        case INVALID_MODIFIED:
            fprintf(stderr, "IM state on %lx, but request %d\n", addr,
                    is_read);
            *permAvail = 0;
            // // printf("Cache - Case Invalid_modified - is_read %d, permAvail %d, current_state %d, addr %x, procNum %d\n", is_read, *permAvail, currentState, addr, procNum);
            return INVALID_MODIFIED;
        case INVALID_SHARED:
            fprintf(stderr, "IS state on %lx, but request %d\n", addr,
                    is_read);
            *permAvail = 0;
            // // printf("Cache - Case Invalid_shared - is_read %d, permAvail %d, current_state %d, addr %x, procNum %d\n", is_read, *permAvail, currentState, addr, procNum);
            return INVALID_SHARED;
        case SHARED_MODIFIED:
            fprintf(stderr, "SM state on %lx, but request %d\n", addr,
                    is_read);
            *permAvail = 0;
            // // printf("Cache - Case Shared_modified - is_read %d, permAvail %d, current_state %d, addr %x, procNum %d\n", is_read, *permAvail, currentState, addr, procNum);
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
        uint64_t addr, int procNum)
{
    *ca = NO_ACTION;
    switch (currentState)
    {
        case INVALID:
        // // printf("Snoop - Case Invalid : reqType %d, cache action %d, current_state %d, addr %x, procNum %d\n", reqType, *ca, currentState, addr, procNum);
            return INVALID;
        case MODIFIED:
            if (reqType == SHARED || reqType == BUSRD) {
                indicateShared(addr, procNum);
                *ca = SHARE;
                // // printf("Snoop - Case Modified -> shared - indicateShared : reqType %d, cache action %d, current_state %d, addr %x, procNum %d\n", reqType, *ca, currentState, addr, procNum);
                return SHAREDST;
            } else if (reqType == BUSWR) {
                sendData(addr, procNum);
                *ca = INVALIDATE;
                // // printf("Snoop - Case Modified -> invalid - Send data : reqType %d, cache action %d, current_state %d, addr %x, procNum %d\n", reqType, *ca, currentState, addr, procNum);
                return INVALID;
            } else {
                // // printf("Snoop - Case Modified -> modified: reqType %d, cache action %d, current_state %d, addr %x, procNum %d\n", reqType, *ca, currentState, addr, procNum);
                return MODIFIED;
            }
            // indicateShared(addr, procNum); // Needed for E state
        case SHAREDST:
            if (reqType == BUSWR) {
                *ca = INVALIDATE;
                // // printf("Snoop - Case shared -> invalid  : reqType %d, cache action %d, current_state %d, addr %x, procNum %d\n", reqType, *ca, currentState, addr, procNum);
                return INVALID;
            } else {
                // // printf("Snoop - Case shared -> shared : reqType %d, cache action %d, current_state %d, addr %x, procNum %d\n", reqType, *ca, currentState, addr, procNum);
                return SHAREDST;
            }
        case EXCLUSIVE:
            if (reqType == SHARED || reqType == BUSRD) {
                indicateShared(addr, procNum);
                *ca = SHARE;
                // // printf("Snoop - Case Exclusive -> shared - indicateShared : reqType %d, cache action %d, current_state %d, addr %x, procNum %d\n", reqType, *ca, currentState, addr, procNum);
                return SHAREDST;
            } else if (reqType == BUSWR) {
                sendData(addr, procNum);
                *ca = INVALIDATE;
                // // printf("Snoop - Case exclusive -> invalid  : send data - reqType %d, cache action %d, current_state %d, addr %x, procNum %d\n", reqType, *ca, currentState, addr, procNum);
                return INVALID;
            } else {
                // // printf("Snoop - Case exclusive: reqType %d, cache action %d, current_state %d, addr %x, procNum %d\n", reqType, *ca, currentState, addr, procNum);
                return EXCLUSIVE;
            }
        case INVALID_MODIFIED:
            if (reqType == DATA || reqType == SHARED)
            {
                *ca = DATA_RECV;
                // // printf("Snoop - Case invalid_Modified -> modified  : reqType %d, cache action %d, current_state %d, addr %x, procNum %d\n", reqType, *ca, currentState, addr, procNum);
                return MODIFIED;
            }
            // // printf("Snoop - Case invalid_Modified -> invalid_modified  : reqType %d, cache action %d, current_state %d, addr %x, procNum %d\n", reqType, *ca, currentState, addr, procNum);
            return INVALID_MODIFIED;
        case INVALID_SHARED:
            if (reqType == SHARED)
            {
                *ca = DATA_RECV;
                // // printf("Snoop - Case invalid_shared -> shared  : reqType %d, cache action %d, current_state %d, addr %x, procNum %d\n", reqType, *ca, currentState, addr, procNum);
                return SHAREDST;
            } else if (reqType == DATA) {
                *ca = DATA_RECV;
                // // printf("Snoop - Case invalid_shared -> exclusive  : reqType %d, cache action %d, current_state %d, addr %x, procNum %d\n", reqType, *ca, currentState, addr, procNum);
                return EXCLUSIVE;
            }
            // // printf("Snoop - Case invalid_shared -> invalid_shared  : reqType %d, cache action %d, current_state %d, addr %x, procNum %d\n", reqType, *ca, currentState, addr, procNum);
            return INVALID_SHARED;
        case SHARED_MODIFIED:
            if (reqType == DATA || reqType == SHARED)
            {
                *ca = DATA_RECV;
                // // printf("Snoop - Case shared_modified -> modified  : reqType %d, cache action %d, current_state %d, addr %x, procNum %d\n", reqType, *ca, currentState, addr, procNum);
                return MODIFIED;
            }
            // // printf("Snoop - Case shared_modified -> shared_modified : reqType %d, cache action %d, current_state %d, addr %x, procNum %d\n", reqType, *ca, currentState, addr, procNum);
            return SHARED_MODIFIED;
        default:
            fprintf(stderr, "State %d not supported, found on %lx\n",
                    currentState, addr);
            break;
    }

    return INVALID;
}