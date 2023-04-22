#include <getopt.h>

#include <memory.h>
#include <interconnect.h>

extern traffic;
extern if_shared;

typedef enum _bus_req_state
{
    NONE,
    QUEUED,
    TRANSFERING_CACHE,
    TRANSFERING_MEMORY,
    WAITING_CACHE,
    WAITING_MEMORY
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
    for (int i = 0; i < processorCount; i++)
    {
        queuedRequests[i] = NULL;
    }

    self = malloc(sizeof(interconn));
    self->busReq = busReq;
    self->registerCoher = registerCoher;
    self->busReqCacheTransfer = busReqCacheTransfer;
    self->si.tick = tick;
    self->si.finish = finish;
    self->si.destroy = destroy;

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
    printf("This is interconnect bus request type %d, addr %lx and procNum %d\n", brt, addr, procNum);
    if (pendingRequest == NULL)
    {
        assert(brt != SHARED);

        bus_req* nextReq = calloc(1, sizeof(bus_req));
        nextReq->brt = brt;
        nextReq->currentState = WAITING_CACHE;
        // printf("proc %d pendingRequest addr 0x%lx current state -> WAITING_CACHE\n", procNum, addr);
        nextReq->addr = addr;
        nextReq->procNum = procNum;

        pendingRequest = nextReq;
        countDown = CACHE_DELAY;

        return;
    } else if (brt == SHARED && pendingRequest->addr == addr)
    {
        assert(pendingRequest->currentState == WAITING_MEMORY);
        pendingRequest->shared = 1;
        pendingRequest->currentState = TRANSFERING_CACHE;
        // printf("proc %d pendingRequest addr 0x%lx current state -> TRANSFERRING_CACHE for SHARED\n", procNum, addr);
        countDown = CACHE_TRANSFER;
        return;
    }
    else if (brt == DATA && pendingRequest->addr == addr)
    {
        assert(pendingRequest->currentState == WAITING_MEMORY);
        pendingRequest->data = 1;
        pendingRequest->currentState = TRANSFERING_CACHE;
        // printf("proc %d pendingRequest addr 0x%lx current state -> TRANSFERRING_CACHE for DATA\n", procNum, addr);
        countDown = CACHE_TRANSFER;
        return;
    } else if (brt == BUSUPDATE && pendingRequest->addr == addr)
    {
        assert(pendingRequest->currentState == WAITING_MEMORY || pendingRequest->currentState == WAITING_CACHE );
        pendingRequest->shared = 1;
        pendingRequest->currentState = TRANSFERING_CACHE;
        // printf("proc %d pendingRequest addr 0x%lx current state -> TRANSFERRING_CACHE for BUSUPDATE\n", procNum, addr);
        countDown = CACHE_TRANSFER;
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
    // // printf("interconnect tick \n");
    if (countDown > 0)
    {
        assert(pendingRequest != NULL);    
        traffic = pendingRequestCount();

        if (traffic > 0) {
            // printf("traffic count: %d\n", traffic);
        }
        countDown--;
        // printf("countdown %d\n", countDown);
        // If the count-down has elapsed (or there hasn't been a
        // cache-to-cache transfer, the memory will respond with
        // the data.
        if (memComp->dataAvail(pendingRequest->addr, pendingRequest->procNum))
        {
            // if(pendingRequest->brt != BUSUPDATE) {
                // printf("Changing to TRANSFERRING_MEMORY\n");
                pendingRequest->currentState = TRANSFERING_MEMORY;
                // printf("proc %d pendingRequest addr 0x%lx current state -> TRANSFERRING_MEMORY\n", pendingRequest->procNum, pendingRequest->addr);
                countDown = 0;
            // }
            // countDown = 0;
        }

        if (countDown == 0)
        {
            if (pendingRequest->currentState == WAITING_CACHE)
            {
                // Make a request to memory.
                if(pendingRequest->brt != BUSUPDATE) {
                    countDown = memComp->busReq(pendingRequest->addr,
                                                pendingRequest->procNum);

                    pendingRequest->currentState = WAITING_MEMORY;
                    // printf("proc %d pendingRequest addr 0x%lx current state -> WAITING_MEMORY\n", pendingRequest->procNum, pendingRequest->addr);
                }
                else {
                    coherComp->busReq(pendingRequest->brt,
                                          pendingRequest->addr, pendingRequest->procNum);
                }
		// printf("All snoop reqType %d\n", pendingRequest->brt);

                // The processors will snoop for this request as well.
                for (int i = 0; i < processorCount; i++)
                {
                    if (pendingRequest->procNum != i)
                    {
                        // printf("proc %d\n", i);
                        coherComp->busReq(pendingRequest->brt,
                                          pendingRequest->addr, i);
                    } else {
                        if_shared &= ~(1 << pendingRequest->procNum);
                    }
                }

                if (pendingRequest->data == 1)
                {
		    // printf("changing brt to DATA for %x\n", pendingRequest->addr);
                    pendingRequest->brt = DATA;
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
                                  pendingRequest->procNum);

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

                coherComp->busReq(brt, pendingRequest->addr,
                                  pendingRequest->procNum);

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
                pendingRequest = deqBusRequest(pos);
                countDown = CACHE_DELAY;
                pendingRequest->currentState = WAITING_CACHE;
        // printf("proc %d pendingRequest addr 0x%lx current state -> WAITING_CACHE\n", pendingRequest->procNum, pendingRequest->addr);

                lastProc = (pos + 1) % processorCount;
                break;
            }
        }
    }
    // if_shared = 0;
    return 0;
}

// Return a non-zero value if the current request
// was satisfied by a cache-to-cache transfer.
int busReqCacheTransfer(uint64_t addr, int procNum)
{
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
