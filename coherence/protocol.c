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
            
            return INVALID_MODIFIED;
        case MODIFIED:
            *permAvail = 1;
            
            return MODIFIED;
        case INVALID_MODIFIED:
            fprintf(stderr, "IM state on %lx, but request %d\n", addr,
                    is_read);
            *permAvail = 0;
            
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
                
                return INVALID_SHARED;
            } else {
                sendBusWr(addr, procNum);
                
                return INVALID_MODIFIED;
            }
        case MODIFIED:
            *permAvail = 1;
            
            return MODIFIED;
        case SHAREDST:
            if (is_read) {
                *permAvail = 1;
                
                return SHAREDST;
            } else {
                *permAvail = 0;
                sendBusWr(addr, procNum);
                
                return SHARED_MODIFIED;
            }
        case INVALID_MODIFIED:
            fprintf(stderr, "IM state on %lx, but request %d\n", addr,
                    is_read);
            *permAvail = 0;
            
            return INVALID_MODIFIED;
        case INVALID_SHARED:
            fprintf(stderr, "IS state on %lx, but request %d\n", addr,
                    is_read);
            *permAvail = 0;
            
            return INVALID_SHARED;
        case SHARED_MODIFIED:
            fprintf(stderr, "SM state on %lx, but request %d\n", addr,
                    is_read);
            *permAvail = 0;
            
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
        
            return INVALID;
        case MODIFIED:
            sendData(addr, procNum);
            *ca = INVALIDATE;
            
            return INVALID;
        case INVALID_MODIFIED:
            assert(reqType != SHARED);
            if (reqType == DATA || reqType == SHARED)
            {
                *ca = DATA_RECV;
                
                return MODIFIED;
            }
            
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
        
            return INVALID;
        case MODIFIED:
            sendData(addr, procNum);
            if (reqType == SHARED || reqType == BUSRD) {
                *ca = SHARE;
                
                return SHAREDST;
            } else if (reqType == BUSWR) {
                *ca = INVALIDATE;
                
                return INVALID;
            } else {
                
                return MODIFIED;
            }
        case SHAREDST:
            if (reqType == BUSWR) {
                *ca = INVALIDATE;
                
                return INVALID;
            } else {
                
                return SHAREDST;
            }
        case INVALID_MODIFIED:
            if (reqType == DATA)
            {
                *ca = DATA_RECV;
                
                return MODIFIED;
            }
            
            return INVALID_MODIFIED;
        case INVALID_SHARED:
            if (reqType == DATA || reqType == SHARED)
            {
                *ca = DATA_RECV;
                
                return SHAREDST;
            }
            
            return INVALID_SHARED;
        case SHARED_MODIFIED:
            if (reqType == DATA)
            {
                *ca = DATA_RECV;
                
                return MODIFIED;
            } else if (reqType == NO_REQ) {
                
                return MODIFIED;
            }
            
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
                
                return INVALID_SHARED;
            } else {
                sendBusWr(addr, procNum);
                
                return INVALID_MODIFIED;
            }
        case MODIFIED:
            *permAvail = 1;
            
            return MODIFIED;
        case SHAREDST:
            if (is_read) {
                *permAvail = 1;
                
                return SHAREDST;
            } else {
                *permAvail = 0;
                sendBusWr(addr, procNum);
                
                return SHARED_MODIFIED;
            }
        case EXCLUSIVE:
            if (is_read) {
                *permAvail = 1;
                
                return EXCLUSIVE;
            } else {
                *permAvail = 1;
                
                return MODIFIED;
            }
        case INVALID_MODIFIED:
            fprintf(stderr, "IM state on %lx, but request %d\n", addr,
                    is_read);
            *permAvail = 0;
            
            return INVALID_MODIFIED;
        case INVALID_SHARED:
            fprintf(stderr, "IS state on %lx, but request %d\n", addr,
                    is_read);
            *permAvail = 0;
            
            return INVALID_SHARED;
        case SHARED_MODIFIED:
            fprintf(stderr, "SM state on %lx, but request %d\n", addr,
                    is_read);
            *permAvail = 0;
            
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
        
            return INVALID;
        case MODIFIED:
            if (reqType == SHARED || reqType == BUSRD) {
                indicateShared(addr, procNum);
                *ca = SHARE;
                
                return SHAREDST;
            } else if (reqType == BUSWR) {
                sendData(addr, procNum);
                *ca = INVALIDATE;
                
                return INVALID;
            } else {
                
                return MODIFIED;
            }
        case SHAREDST:
            if (reqType == BUSWR) {
                *ca = INVALIDATE;
                
                return INVALID;
            } else {
                
                return SHAREDST;
            }
        case EXCLUSIVE:
            if (reqType == SHARED || reqType == BUSRD) {
                indicateShared(addr, procNum);
                *ca = SHARE;
                
                return SHAREDST;
            } else if (reqType == BUSWR) {
                sendData(addr, procNum);
                *ca = INVALIDATE;
                
                return INVALID;
            } else {
                
                return EXCLUSIVE;
            }
        case INVALID_MODIFIED:
            if (reqType == DATA)
            {
                *ca = DATA_RECV;
                
                return MODIFIED;
            }
            
            return INVALID_MODIFIED;
        case INVALID_SHARED:
            if (reqType == SHARED)
            {
                *ca = DATA_RECV;
                
                return SHAREDST;
            } else if (reqType == DATA) {
                *ca = DATA_RECV;
                
                return EXCLUSIVE;
            }
            
            return INVALID_SHARED;
        case SHARED_MODIFIED:
            if (reqType == DATA)
            {
                *ca = DATA_RECV;
                
                return MODIFIED;
            }
            
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
                
                return INVALID_SHARED;
            } else {
                sendBusWr(addr, procNum);
                
                return INVALID_MODIFIED;
            }
        case MODIFIED:
            *permAvail = 1;
            
            return MODIFIED;
        case SHAREDST:
            if (is_read) {
                *permAvail = 1;
                
                return SHAREDST;
            } else {
                *permAvail = 0;
                sendBusWr(addr, procNum);
                
                return SHARED_MODIFIED;
            }
        case EXCLUSIVE:
            if (is_read) {
                *permAvail = 1;
                
                return EXCLUSIVE;
            } else {
                *permAvail = 1;
                
                return MODIFIED;
            }
        case OWNED:
            if (is_read) {
                *permAvail = 1;
                
                return OWNED;
            } else {
                *permAvail = 0;
                sendBusWr(addr, procNum);
                
                return OWNED_MODIFIED;
            }
        case INVALID_MODIFIED:
            fprintf(stderr, "IM state on %lx, but request %d\n", addr,
                    is_read);
            *permAvail = 0;
            
            return INVALID_MODIFIED;
        case INVALID_SHARED:
            fprintf(stderr, "IS state on %lx, but request %d\n", addr,
                    is_read);
            *permAvail = 0;
            
            return INVALID_SHARED;
        case SHARED_MODIFIED:
            fprintf(stderr, "SM state on %lx, but request %d\n", addr,
                    is_read);
            *permAvail = 0;
            
            return SHARED_MODIFIED;
        case OWNED_MODIFIED:
            fprintf(stderr, "OM state on %lx, but request %d\n", addr,
                    is_read);
            *permAvail = 0;
            
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
        
            return INVALID;
        case MODIFIED:
            if (reqType == SHARED || reqType == BUSRD) {
                indicateShared(addr, procNum);
                *ca = SHARE;
                
                return OWNED;
            } else if (reqType == BUSWR) {
                sendData(addr, procNum);
                *ca = INVALIDATE;
                
                return INVALID;
            } else {
                
                return MODIFIED;
            }
        case SHAREDST:
            if (reqType == BUSWR) {
                *ca = INVALIDATE;
                
                return INVALID;
            } else {
                
                return SHAREDST;
            }
        case EXCLUSIVE:
            if (reqType == SHARED || reqType == BUSRD) {
                indicateShared(addr, procNum);
                *ca = SHARE;
                
                return SHAREDST;
            } else if (reqType == BUSWR) {
                sendData(addr, procNum);
                *ca = INVALIDATE;
                
                return INVALID;
            } else {
                
                return EXCLUSIVE;
            }
        case OWNED:
            if (reqType == SHARED || reqType == BUSRD) {
                indicateShared(addr, procNum);
                *ca = SHARE;
                
                return OWNED;
            } else if (reqType == BUSWR) {
                sendData(addr, procNum);
                *ca = INVALIDATE;
                
                return INVALID;
            } else {
                
                return OWNED;
            }
        case INVALID_MODIFIED:
            if (reqType == DATA)
            {
                *ca = DATA_RECV;
                
                return MODIFIED;
            }
            
            return INVALID_MODIFIED;
        case INVALID_SHARED:
            if (reqType == SHARED)
            {
                *ca = DATA_RECV;
                
                return SHAREDST;
            } else if (reqType == DATA) {
                *ca = DATA_RECV;
                
                return EXCLUSIVE;
            }
            
            return INVALID_SHARED;
        case SHARED_MODIFIED:
            if (reqType == DATA)
            {
                *ca = DATA_RECV;
                
                return MODIFIED;
            }
            
            return SHARED_MODIFIED;
        case OWNED_MODIFIED:
            if (reqType == DATA) {
                *ca = DATA_RECV;
                
                return MODIFIED;
            }
            
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
                
                return INVALID_SHARED;
            } else {
                sendBusWr(addr, procNum);
                
                return INVALID_MODIFIED;
            }
        case MODIFIED:
            *permAvail = 1;
            
            return MODIFIED;
        case EXCLUSIVE_CLEAN:
            *permAvail = 1;
            if (is_read) {
                
                return EXCLUSIVE_CLEAN;
            } else {
                
                return MODIFIED;
            }
        case SHARED_CLEAN:
            if (is_read) {
                *permAvail = 1;
                
                return SHARED_CLEAN;
            } else {
                *permAvail = 0;
                sendBusUpd(addr, procNum);
                
                return SHARED_CLEAN_MODIFIED;
            }
        case DRAGON_SHARED_MODIFIED:
            if (is_read) {
                *permAvail = 1;
                
                return DRAGON_SHARED_MODIFIED;
            } else {
                *permAvail = 0;
                sendBusUpd(addr, procNum);
                
                return DRAGON_SHARED_MODIFIED_INT;
            }
        case INVALID_MODIFIED:
            fprintf(stderr, "IM state on %lx, but request %d\n", addr,
                    is_read);
            *permAvail = 0;
            
            return INVALID_MODIFIED;
        case INVALID_SHARED:
            fprintf(stderr, "IS state on %lx, but request %d\n", addr,
                    is_read);
            *permAvail = 0;
            
            return INVALID_SHARED;
        case SHARED_CLEAN_MODIFIED:
            fprintf(stderr, "SCM state on %lx, but request %d\n", addr,
                    is_read);
            *permAvail = 0;
            
            return SHARED_CLEAN_MODIFIED;
        case DRAGON_SHARED_MODIFIED_INT:
            fprintf(stderr, "SMI state on %lx, but request %d\n", addr,
                    is_read);
            *permAvail = 0;
            
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
        
            return INVALID;
        case MODIFIED:
            assert(reqType == BUSRD || reqType == BUSWR);
            if (reqType == BUSRD) {
                indicateShared(addr, procNum);
                *ca = SHARE;
                if_shared |= 1 << procNum;
                
                return DRAGON_SHARED_MODIFIED;
            } else if (reqType == BUSWR) {
                indicateShared(addr, procNum);
                *ca = UPDATE;
                if_shared |= 1 << procNum;
                
                return SHARED_CLEAN;
            } 
        case EXCLUSIVE_CLEAN:
            assert(reqType == BUSRD || reqType == BUSWR);
            if (reqType == BUSRD || reqType == BUSWR) {
                *ca = SHARE;
                if_shared |= 1 << procNum;
                
                return SHARED_CLEAN;
            } else {
                if_shared &= ~(1 << procNum);
                
                return EXCLUSIVE;
            }
        case SHARED_CLEAN:
            assert(!(reqType == BUSUPDATE && reqProc == procNum));
            assert(reqType == BUSRD || reqType == BUSWR || reqType == BUSUPDATE);
            if (reqType == BUSWR || (reqType == BUSUPDATE && reqProc != procNum)) {
                *ca = UPDATE;
                if_shared |= 1 << procNum;
                
                return SHARED_CLEAN;
            } 
            else {
                if_shared |= 1 << procNum;
                
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
                
                return DRAGON_SHARED_MODIFIED;
            } else if (reqType == BUSWR || (reqType == BUSUPDATE && reqProc != procNum)) {
                indicateShared(addr, procNum);
                *ca = UPDATE;
                if_shared |= 1 << procNum;
                
                return SHARED_CLEAN;
            } else {
                if_shared |= 1 << procNum;
                
                return DRAGON_SHARED_MODIFIED;
            }
        case INVALID_MODIFIED:
            if (reqType == DATA)
            {
                *ca = DATA_RECV;
                if_shared &= ~(1 << procNum);
                
                return MODIFIED;
            } else if (reqType == SHARED && reqProc == procNum) {
                *ca = DATA_RECV;
                if_shared |= 1 << procNum;
                
                return DRAGON_SHARED_MODIFIED;
            }
            if_shared &= ~(1 << procNum);
            
            return INVALID_MODIFIED;
        case INVALID_SHARED:
            if (reqType == SHARED)
            {
                *ca = DATA_RECV;
                if_shared |= 1 << procNum;
                
                return SHARED_CLEAN;
            } else if (reqType == DATA) {
                *ca = DATA_RECV;
                if_shared &= ~(1 << procNum);
                
                return EXCLUSIVE_CLEAN;
            }
            if_shared &= ~(1 << procNum);
            
            return INVALID_SHARED;
        case SHARED_CLEAN_MODIFIED:
            if (reqType == BUSUPDATE && reqProc == procNum) {
                *ca = DATA_RECV;
                if_shared |= 1 << procNum;
                
                return DRAGON_SHARED_MODIFIED;
            }
            if_shared |= 1 << procNum;
            return SHARED_CLEAN_MODIFIED;
        case DRAGON_SHARED_MODIFIED_INT:
            if ((reqType == BUSUPDATE) && reqProc == procNum) {
                *ca = DATA_RECV;
                if_shared |= 1 << procNum;
                
                return DRAGON_SHARED_MODIFIED;
            }
            
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
                
                return INVALID_SHARED;
            } else {
                sendBusWr(addr, procNum);
                
                return INVALID_MODIFIED;
            }
        case MODIFIED:
            *permAvail = 1;
            
            return MODIFIED;
        case EXCLUSIVE_CLEAN:
            *permAvail = 1;
            if (is_read) {
                
                return EXCLUSIVE_CLEAN;
            } else {
                
                return MODIFIED;
            }
        case SHARED_CLEAN:
            if (is_read) {
                *permAvail = 1;
                
                return SHARED_CLEAN;
            } else {
                *permAvail = 0;
                sendBusUpd(addr, procNum);
                
                return SHARED_CLEAN_MODIFIED;
            }
        case DRAGON_SHARED_MODIFIED:
            if (is_read) {
                *permAvail = 1;
                
                return DRAGON_SHARED_MODIFIED;
            } else {
                *permAvail = 0;
                sendBusUpd(addr, procNum);
                
                return DRAGON_SHARED_MODIFIED_INT;
            }
        case INVALID_MODIFIED:
            fprintf(stderr, "IM state on %lx, but request %d\n", addr,
                    is_read);
            *permAvail = 0;
            
            return INVALID_MODIFIED;
        case INVALID_SHARED:
            fprintf(stderr, "IS state on %lx, but request %d\n", addr,
                    is_read);
            *permAvail = 0;
            
            return INVALID_SHARED;
        case SHARED_CLEAN_MODIFIED:
            fprintf(stderr, "SCM state on %lx, but request %d\n", addr,
                    is_read);
            *permAvail = 0;
            
            return SHARED_CLEAN_MODIFIED;
        case DRAGON_SHARED_MODIFIED_INT:
            fprintf(stderr, "SMI state on %lx, but request %d\n", addr,
                    is_read);
            *permAvail = 0;
            
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
        
            return INVALID;
        case MODIFIED:
            
            assert(reqType == BUSRD || reqType == BUSWR || reqType == BUSUPDATE);
            if (reqType == BUSRD) {
                indicateShared(addr, procNum);
                *ca = SHARE;
                if_shared |= 1 << procNum;
                
                return DRAGON_SHARED_MODIFIED;
            } else if (reqType == BUSWR || reqType == BUSUPDATE) {
                indicateShared(addr, procNum);
                *ca = UPDATE;
                if_shared |= 1 << procNum;
                
                return SHARED_CLEAN;
            }
        case EXCLUSIVE_CLEAN:
            assert(reqType == BUSRD || reqType == BUSWR);
            if (reqType == BUSRD || reqType == BUSWR) {
                *ca = SHARE;
                if_shared |= 1 << procNum;
                
                return SHARED_CLEAN;
            } else {
                if_shared &= ~(1 << procNum);
                
                return EXCLUSIVE;
            }
        case SHARED_CLEAN:
            assert(!(reqType == BUSUPDATE && reqProc == procNum));
            assert(reqType == BUSRD || reqType == BUSWR || reqType == BUSUPDATE || reqType == REDO);
            if (reqType == BUSWR) {
                *ca = UPDATE;
                if_shared |= 1 << procNum;
                
                return SHARED_CLEAN;
            } else if (reqType == BUSUPDATE && reqProc != procNum) {
                redo = true; 
                
                return SHARED_CLEAN;
            } else if (reqType == REDO) {
                
                if (countSetBits(if_shared) >= sharing_threshold) {
                    
                    return SHARED_CLEAN;
                } else {
                    
                    return INVALID;
                }
            }
            else {
                if_shared |= 1 << procNum;
                
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
                
                return DRAGON_SHARED_MODIFIED;
            } else if (reqType == BUSWR || (reqType == BUSUPDATE && reqProc != procNum)) {
                indicateShared(addr, procNum);
                *ca = UPDATE;
                if_shared |= 1 << procNum;
                
                return SHARED_CLEAN;
            } else {
                if_shared |= 1 << procNum;
                
                return DRAGON_SHARED_MODIFIED;
            }
        case INVALID_MODIFIED:
            if (reqType == DATA)
            {
                *ca = DATA_RECV;
                if_shared &= ~(1 << procNum);
                
                return MODIFIED;
            } else if (reqType == SHARED && reqProc == procNum) {
                *ca = DATA_RECV;
                if_shared |= 1 << procNum;
                
                return DRAGON_SHARED_MODIFIED;
            }
            if_shared &= ~(1 << procNum);
            
            return INVALID_MODIFIED;
        case INVALID_SHARED:
            if (reqType == SHARED)
            {
                *ca = DATA_RECV;
                if_shared |= 1 << procNum;
                
                return SHARED_CLEAN;
            } else if (reqType == DATA) {
                *ca = DATA_RECV;
                if_shared &= ~(1 << procNum);
                
                return EXCLUSIVE_CLEAN;
            }
            if_shared &= ~(1 << procNum);
            
            return INVALID_SHARED;
        case SHARED_CLEAN_MODIFIED:
            if (reqType == BUSUPDATE && reqProc == procNum) {
                
                if (countSetBits(if_shared) >= sharing_threshold) {
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
                
                if (countSetBits(if_shared) >= sharing_threshold) {
                    *ca = DATA_RECV;
                    
                    return DRAGON_SHARED_MODIFIED;
                } else {
                    *ca = DATA_RECV;
                    
                    return MODIFIED;
                }
            }
            
            if_shared |= 1 << procNum;
            return DRAGON_SHARED_MODIFIED_INT;
        default:
            fprintf(stderr, "State %d not supported, found on %lx\n",
                    currentState, addr);
            break;
    }

    return INVALID;
}