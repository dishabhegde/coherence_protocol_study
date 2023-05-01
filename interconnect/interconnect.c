#include <getopt.h>

#include <memory.h>
#include <interconnect.h>
#include <stdbool.h>

extern int traffic;
extern int if_shared;
extern bool redo;
extern stats_t stats;

typedef enum _bus_req_state
{
    NONE,
    QUEUED,
    TRANSFERING_CACHE,
    TRANSFERING_MEMORY,
    WAITING_CACHE,
    WAITING_MEMORY,
    WAITING_UPDATE
} bus_req_state;

typedef struct _bus_req {
    bus_req_type brt;
    bus_req_state currentState;
    uint64_t addr;
    int procNum;
    uint8_t shared;
    uint8_t data;
    struct _bus_req* next;
} bus_req;

bus_req* pendingRequest = NULL;
bus_req** queuedRequests;
interconn* self;
coher* coherComp;
memory* memComp;

int CADSS_VERBOSE = 0;
int processorCount = 1;

const int CACHE_DELAY = 10;
const int CACHE_TRANSFER = 10;

void registerCoher(coher* cc);
void busReq(bus_req_type brt, uint64_t addr, int procNum);
int busReqCacheTransfer(uint64_t addr, int procNum);

// Helper methods for per-processor request queues.
static void enqBusRequest(bus_req* pr, int procNum)
{
    bus_req* iter;

    // No items in the queue.
    if (!queuedRequests[procNum])
    {
        queuedRequests[procNum] = pr;
        return;
    }

    // Add request to the end of the queue.
    iter = queuedRequests[procNum];
    while (iter->next)
    {
        iter = iter->next;
    }

    pr->next = NULL;
    iter->next = pr;
}

static bus_req* deqBusRequest(int procNum)
{
    bus_req* ret;

    ret = queuedRequests[procNum];

    // Move the head to the next request (if there is one).
    if (ret)
    {
        queuedRequests[procNum] = ret->next;
    }

    return ret;
}

interconn* init(inter_sim_args* isa)
{
    int op;

    while ((op = getopt(isa->arg_count, isa->arg_list, "v")) != -1)
    {
        switch (op)
        {
            default:
                break;
        }
    }

    queuedRequests = malloc(sizeof(bus_req*) * processorCount);
    stats.cumulative_wait_time = malloc(sizeof(int) * processorCount);
    for (int i = 0; i < processorCount; i++)
    {
        queuedRequests[i] = NULL;
        stats.cumulative_wait_time[i] = 0;
    }

    self = malloc(sizeof(interconn));
    self->busReq = busReq;
    self->registerCoher = registerCoher;
    self->busReqCacheTransfer = busReqCacheTransfer;
    self->si.tick = tick;
    self->si.finish = finish;
    self->si.destroy = destroy;
    stats.mem_transfers = 0;
    stats.bus_reqs = 0;
    stats.per_req_stats.busrd = 0;
    stats.per_req_stats.buswr = 0;
    stats.per_req_stats.busupd = 0;
    stats.per_req_stats.shared = 0;
    stats.per_req_stats.data = 0;

    memComp = isa->memory;
    memComp->registerInterconnect(self);

    return self;
}

int countDown = 0;
int lastProc = 0; // for round robin arbitration

void registerCoher(coher* cc)
{
    coherComp = cc;
}

void busReq(bus_req_type brt, uint64_t addr, int procNum)
{
    // printf("This is interconnect bus request type %d, addr %lx and procNum %d\n", brt, addr, procNum);

    stats.bus_reqs++;
    switch (brt) {
        case BUSRD:
            stats.per_req_stats.busrd++;
            break;
        case BUSWR:
            stats.per_req_stats.buswr++;
            break;
        case BUSUPDATE:
            stats.per_req_stats.busupd++;
            break;
        case SHARED:
            stats.per_req_stats.shared++;
            break;
        case DATA:
            stats.per_req_stats.data++;
            break;
        default:
            break;
    }
    if (pendingRequest == NULL)
    {
        assert(brt != SHARED);
        assert(brt != DATA);

        bus_req* nextReq = calloc(1, sizeof(bus_req));
        nextReq->brt = brt;
        nextReq->currentState = QUEUED;
        // printf("proc %d pendingRequest addr 0x%lx current state -> WAITING_CACHE\n", procNum, addr);
        nextReq->addr = addr;
        nextReq->procNum = procNum;

        enqBusRequest(nextReq, procNum);

        // pendingRequest = nextReq;
        // countDown = CACHE_DELAY;
        // stats.cumulative_wait_time[pendingRequest->procNum] += CACHE_DELAY;

        return;
    } 
    else if (brt == SHARED && pendingRequest->addr == addr)
    {
        // printf("pending request state %d\n",pendingRequest->currentState);
        assert(pendingRequest->currentState == WAITING_MEMORY || pendingRequest->currentState == WAITING_UPDATE);
        pendingRequest->shared = 1;
        pendingRequest->currentState = TRANSFERING_CACHE;
        // printf("proc %d pendingRequest addr 0x%lx current state -> TRANSFERRING_CACHE for SHARED\n", procNum, addr);
        if (pendingRequest->brt != BUSUPDATE) {
            countDown = CACHE_TRANSFER;
            stats.cumulative_wait_time[pendingRequest->procNum] += CACHE_TRANSFER;
        }
        return;
    }
    else if (brt == DATA && pendingRequest->addr == addr)
    {
        // printf("pending request state %d\n",pendingRequest->currentState);
        assert(pendingRequest->currentState == WAITING_MEMORY || pendingRequest->currentState == WAITING_UPDATE);
        pendingRequest->data = 1;
        pendingRequest->currentState = TRANSFERING_CACHE;
        // printf("proc %d pendingRequest addr 0x%lx current state -> TRANSFERRING_CACHE for DATA\n", procNum, addr);
        countDown = CACHE_TRANSFER;
        stats.cumulative_wait_time[pendingRequest->procNum] += CACHE_TRANSFER;
        return;
    } 
    else
    {
        assert(brt != SHARED);

        bus_req* nextReq = calloc(1, sizeof(bus_req));
        nextReq->brt = brt;
        nextReq->currentState = QUEUED;
        // printf("proc %d pendingRequest addr 0x%lx current state -> QUEUED\n", procNum, addr);
        nextReq->addr = addr;
        nextReq->procNum = procNum;

        enqBusRequest(nextReq, procNum);
    }
}

int pendingRequestCount() {
    int c = 0;
    if (pendingRequest == NULL) {
        return c;
    }
    c++;
    while (pendingRequest->next != NULL) {
        c++;
    }
    return c;
}

int tick()
{
    memComp->si.tick();
    redo = false;
    // // printf("interconnect tick \n");
    if (countDown > 0)
    {
        assert(pendingRequest != NULL);    
        traffic = pendingRequestCount();

        countDown--;
        // If the count-down has elapsed (or there hasn't been a
        // cache-to-cache transfer, the memory will respond with
        // the data.
        if (memComp->dataAvail(pendingRequest->addr, pendingRequest->procNum))
        {
                pendingRequest->currentState = TRANSFERING_MEMORY;
                countDown = 0;
        }
        // printf("Coundown %d\n", countDown);
        if (countDown == 0)
        {
            if (pendingRequest->currentState == WAITING_CACHE)
            {
                // Make a request to memory.
                assert(pendingRequest->brt == BUSRD || pendingRequest->brt == BUSWR || pendingRequest->brt == BUSUPDATE);
                if(pendingRequest->brt != BUSUPDATE) {
                    // printf("proc %d pendingRequest addr 0x%lx current state -> WAITING_MEMORY\n", pendingRequest->procNum, pendingRequest->addr);
                    countDown = memComp->busReq(pendingRequest->addr,
                                                pendingRequest->procNum);
                    stats.cumulative_wait_time[pendingRequest->procNum] += countDown;

                    pendingRequest->currentState = WAITING_MEMORY;
                } else {
                    pendingRequest->currentState = WAITING_UPDATE;
                }

                // The processors will snoop for this request as well.
                for (int i = 0; i < processorCount; i++)
                {
                    if (pendingRequest->procNum != i)
                    {
                        // printf("proc %d\n", i);
                        coherComp->busReq(pendingRequest->brt,
                                          pendingRequest->addr, i, pendingRequest->procNum);
                    } else {
                        if_shared &= ~(1 << pendingRequest->procNum);
                    }
                }
                if (redo) {
                    for (int i = 0; i < processorCount; i++)
                    {
                        if (pendingRequest->procNum != i)
                        {
                            // printf("proc %d\n", i);
                            coherComp->busReq(REDO,
                                            pendingRequest->addr, i, pendingRequest->procNum);
                        }
                    }
                }

                if (pendingRequest->data == 1)
                {
		        // printf("changing brt to DATA for %x\n", pendingRequest->addr);
                    pendingRequest->brt = DATA;
                }
                if (pendingRequest->brt == BUSUPDATE) {
                    coherComp->busReq(pendingRequest->brt,
                                          pendingRequest->addr, pendingRequest->procNum, pendingRequest->procNum);

                    // printf("freeing request busupdate\n");

                    free(pendingRequest);
                    pendingRequest = NULL;
                }
            }
            else if (pendingRequest->currentState == TRANSFERING_MEMORY)
            {
		// printf("changing brt to %d for %x\n", (pendingRequest->shared == 1) ? SHARED : DATA, pendingRequest->addr);
                // printf("If_shared %d\n", if_shared);
                bus_req_type brt
                    = (pendingRequest->shared == 1) ? SHARED : DATA;
                brt
                    = (if_shared != 0) ? SHARED : brt;
                coherComp->busReq(brt, pendingRequest->addr,
                                  pendingRequest->procNum, pendingRequest->procNum);
                stats.mem_transfers++;
                    // printf("freeing request in transferring memory\n");
                free(pendingRequest);
                pendingRequest = NULL;
            }
            else if (pendingRequest->currentState == TRANSFERING_CACHE)
            {
                bus_req_type brt = pendingRequest->brt;
                if (pendingRequest->shared == 1) {
		            // printf("changing brt to SHARED for %x\n", pendingRequest->addr);
                    brt = SHARED;
		        }
                // printf("snooping own bus request brt %d addr %x procNum %d\n", brt, pendingRequest->addr, pendingRequest->procNum);
                coherComp->busReq(brt, pendingRequest->addr,
                                  pendingRequest->procNum, pendingRequest->procNum);

                    // printf("freeing request in transferring cache\n");
                free(pendingRequest);
                pendingRequest = NULL;
            }
        }
    }
    else if (countDown == 0)
    {
        for (int i = 0; i < processorCount; i++)
        {
            int pos = (i + lastProc) % processorCount;
            if (queuedRequests[pos] != NULL)
            {
                printf("inside pos %d\n", pos);
                pendingRequest = deqBusRequest(pos);
                countDown = CACHE_DELAY;
                stats.cumulative_wait_time[pendingRequest->procNum] += CACHE_DELAY;
                pendingRequest->currentState = WAITING_CACHE;
                lastProc = (pos + 1) % processorCount;
                break;
            }
        }
    }
    return 0;
}

// Return a non-zero value if the current request
// was satisfied by a cache-to-cache transfer.
int busReqCacheTransfer(uint64_t addr, int procNum)
{

    // printf("cache transfer pendingRequest %p\n",pendingRequest);
    assert(pendingRequest);

    if (addr == pendingRequest->addr && procNum == pendingRequest->procNum)
        return (pendingRequest->currentState == TRANSFERING_CACHE);

    return 0;
}

int finish(int outFd)
{
    memComp->si.finish(outFd);
    return 0;
}

int destroy(void)
{
    // TODO
    memComp->si.destroy();
    return 0;
}
