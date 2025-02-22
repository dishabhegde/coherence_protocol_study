#include <coherence.h>
#include <trace.h>
#include <getopt.h>
#include "coher_internal.h"

#include "stree.h"

typedef void (*cacheCallbackFunc)(int, int, int64_t);

tree_t** coherStates = NULL;
int processorCount = 1;
int CADSS_VERBOSE = 0;
coherence_scheme cs = MI;
coher* self = NULL;
interconn* inter_sim = NULL;
cacheCallbackFunc cacheCallback = NULL;

uint8_t busReq(bus_req_type reqType, uint64_t addr, int reqProc, int processorNum);
uint8_t permReq(uint8_t is_read, uint64_t addr, int processorNum);
uint8_t invlReq(uint64_t addr, int processorNum);
void registerCacheInterface(void (*callback)(int, int, int64_t));


coher* init(coher_sim_args* csa)
{
    int op;

    while ((op = getopt(csa->arg_count, csa->arg_list, "s:")) != -1)
    {
        switch (op)
        {
            case 's':
                // cs = atoi(optarg);
                // break;
                if (strcmp(optarg, "MI") == 0)
                    cs = MI;
                else if (strcmp(optarg, "MSI") == 0)
                    cs = MSI;
                else if (strcmp(optarg, "MESI") == 0)
                    cs = MESI;
                else if (strcmp(optarg, "MOESI") == 0)
                    cs = MOESI;
                else if (strcmp(optarg, "MESIF") == 0)
                    cs = MESIF;
                else if (strcmp(optarg, "Dragon") == 0)
                    cs = DRAGON;
                else if (strcmp(optarg, "HybridDragon") == 0)
                    cs = HYBRID_DRAGON;
                else {
                    fprintf(stderr, "Invalid coherence scheme: %s\n", optarg);
                    exit(EXIT_FAILURE);
                }
            break;
        }
    }

    if (processorCount < 1 || processorCount > 256)
    {
        fprintf(stderr,
                "Error: processorCount outside valid range - %d specified\n",
                processorCount);
        return NULL;
    }

    coherStates = malloc(sizeof(tree_t*) * processorCount);
    for (int i = 0; i < processorCount; i++)
    {
        coherStates[i] = tree_new();
    }

    inter_sim = csa->inter;

    self = malloc(sizeof(coher));
    self->si.tick = tick;
    self->si.finish = finish;
    self->si.destroy = destroy;
    self->permReq = permReq;
    self->busReq = busReq;
    self->invlReq = invlReq;
    self->registerCacheInterface = registerCacheInterface;

    inter_sim->registerCoher(self);

    return self;
}

void registerCacheInterface(void (*callback)(int, int, int64_t))
{
    cacheCallback = callback;
}

coherence_states getState(uint64_t addr, int processorNum)
{
    coherence_states lookState
        = (coherence_states)tree_find(coherStates[processorNum], addr);
    if (lookState == UNDEF)
        return INVALID;

    return lookState;
}

void setState(uint64_t addr, int processorNum, coherence_states nextState)
{
    tree_insert(coherStates[processorNum], addr, (void*)nextState);
}

uint8_t busReq(bus_req_type reqType, uint64_t addr, int processorNum, int reqProc)
{
    if (processorNum < 0 || processorNum >= processorCount)
    {
        // ERROR
    }

    coherence_states currentState = getState(addr, processorNum);
    coherence_states nextState;
    cache_action ca;


    switch (cs)
    {
        case MI:
            nextState
                = snoopMI(reqType, &ca, currentState, addr, processorNum, reqProc);
            break;
        case MSI:
            nextState
                = snoopMSI(reqType, &ca, currentState, addr, processorNum, reqProc);
            break;
        case MESI:
            nextState
                = snoopMESI(reqType, &ca, currentState, addr, processorNum, reqProc);
            break;
        case MOESI:
            nextState
                = snoopMOESI(reqType, &ca, currentState, addr, processorNum, reqProc);
            break;
        case MESIF:
            break;
        case DRAGON:
            nextState = snoopDragon(reqType, &ca, currentState, addr, processorNum, reqProc);
            break;
        case HYBRID_DRAGON:
            nextState = snoopHybridDragon(reqType, &ca, currentState, addr, processorNum, reqProc);
            break;
        default:
            fprintf(stderr, "Undefined coherence scheme - %d\n", cs);
            break;
    }

    switch (ca)
    {
        case DATA_RECV:
            cacheCallback(ca, processorNum, addr);
            break;
        case INVALIDATE:
        case NO_ACTION:
        case SHARE:
        case UPDATE:
            // cacheCallback(ca, processorNum, addr);
            break;

        default:
            printf("%d\n", ca);
            assert(0);
    }

    // If the destination state is invalid, that is an implicit
    // state and does not need to be stored in the tree.
    if (nextState == INVALID)
    {
        if (currentState != INVALID)
        {
            tree_remove(coherStates[processorNum], addr);
        }
    }
    else
    {
        setState(addr, processorNum, nextState);
    }

    return 0;
}

uint8_t permReq(uint8_t is_read, uint64_t addr, int processorNum)
{
    if (processorNum < 0 || processorNum >= processorCount)
    {
        // ERROR
    }

    coherence_states currentState = getState(addr, processorNum);
    coherence_states nextState;
    uint8_t permAvail = 0;

    switch (cs)
    {
        case MI:
            nextState = cacheMI(is_read, &permAvail, currentState, addr,
                                processorNum);
            break;

        case MSI:
            nextState = cacheMSI(is_read, &permAvail, currentState, addr,
                                processorNum);
            break;

        case MESI:
            nextState = cacheMESI(is_read, &permAvail, currentState, addr,
                                processorNum);
            break;

        case MOESI:
            nextState = cacheMOESI(is_read, &permAvail, currentState, addr,
                                processorNum);
            break;

        case MESIF:
            // TODO: Implement this.
            break;
        
        case DRAGON:
            nextState = cacheDragon(is_read, &permAvail, currentState, addr,
                                processorNum);
            break;
        
        case HYBRID_DRAGON:
            nextState = cacheHybridDragon(is_read, &permAvail, currentState, addr,
                                processorNum);
            break;

        default:
            fprintf(stderr, "Undefined coherence scheme - %d\n", cs);
            break;
    }

    setState(addr, processorNum, nextState);
    return permAvail;
}

uint8_t invlReq(uint64_t addr, int processorNum)
{
    coherence_states currentState, nextState = INVALID;
    cache_action ca;
    uint8_t flush;
    if (processorNum < 0 || processorNum >= processorCount)
    {
        // ERROR
    }

    currentState = getState(addr, processorNum);

    flush = 0;
    switch (cs)
    {
        case MI:
            nextState = INVALID;
            if (currentState != INVALID)
            {
                inter_sim->busReq(DATA, addr, processorNum);
                flush = 1;
            }
            break;

        case MSI:
            // TODO: Implement this.
            break;
        case MESI:
            // TODO: Implement this.
            break;

        case MOESI:
            // TODO: Implement this.
            break;

        case MESIF:
            // TODO: Implement this.
            break;

        default:
            fprintf(stderr, "Undefined coherence scheme - %d\n", cs);
            break;
    }

    tree_remove(coherStates[processorNum], addr);

    // Notify about "permReqOnFlush".
    return flush;
}

int tick()
{
    return inter_sim->si.tick();
}

int finish(int outFd)
{
    return inter_sim->si.finish(outFd);
}

int destroy(void)
{
    // TODO

    return inter_sim->si.destroy();
}
