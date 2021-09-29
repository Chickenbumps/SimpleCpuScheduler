// 2020/OS(운영체제)/< CPU Scheduling Simulator >
// CPU Schedule Simulator Homework

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <limits.h>

#define SEED 10

// process states
#define S_IDLE 0
#define S_READY 1
#define S_BLOCKED 2
#define S_RUNNING 3
#define S_TERMINATE 4

int NPROC, NIOREQ, QUANTUM;

struct ioDoneEvent {
    int procid;
    int doneTime;
    int len;
    struct ioDoneEvent *prev;
    struct ioDoneEvent *next;
} ioDoneEventQueue, *ioDoneEvent;

struct process {
    int id;
    int len;        // for queue
    int targetServiceTime;
    int serviceTime;
    int startTime;
    int endTime;
    char state;
    int priority;
    int saveReg0, saveReg1;
    struct process *prev;
    struct process *next;
} *procTable;

struct process idleProc;
struct process readyQueue;
struct process blockedQueue;
struct process *runningProc;

int cpuReg0, cpuReg1;
int currentTime = 0;
int *procIntArrTime, *procServTime, *ioReqIntArrTime, *ioServTime;

void compute() {
    // DO NOT CHANGE THIS FUNCTION
    cpuReg0 = cpuReg0 + runningProc->id;
    cpuReg1 = cpuReg1 + runningProc->id;
    //printf("In computer proc %d cpuReg0 %d\n",runningProc->id,cpuReg0);
}

void initProcTable() {
    int i;
    for(i=0; i < NPROC; i++) {
        procTable[i].id = i;
        procTable[i].len = 0;
        procTable[i].targetServiceTime = procServTime[i];
        procTable[i].serviceTime = 0;
        procTable[i].startTime = 0;
        procTable[i].endTime = 0;
        procTable[i].state = S_IDLE;
        procTable[i].priority = 0;
        procTable[i].saveReg0 = 0;
        procTable[i].saveReg1 = 0;
        procTable[i].prev = NULL;
        procTable[i].next = NULL;
    }
}
void initIoDoneEvent(){
    int i;
    for (i = 0; i < NIOREQ; i++) {
        ioDoneEvent[i].doneTime =0;
        ioDoneEvent[i].len = 0;
        ioDoneEvent[i].next = NULL;
        ioDoneEvent[i].prev = NULL;
        ioDoneEvent[i].procid = -1;
    }
}

void procExecSim(struct process *(*scheduler)()) {
    int pid, qTime=0, cpuUseTime = 0, nproc=0, termProc = 0, nioreq=0;
    char schedule = 0, nextState = S_IDLE;
    int nextForkTime, nextIOReqTime;

    nextForkTime = procIntArrTime[nproc];
    nextIOReqTime = ioReqIntArrTime[nioreq];
    idleProc.id = -1;
    runningProc = &idleProc;
    int nioFlag = 0;
    while(1) {
        currentTime++;
        qTime++;
        runningProc->serviceTime++;
        if (runningProc != &idleProc ) cpuUseTime++;

        // MUST CALL compute() Inside While loop
        compute();

        int iFlag = 0;
        struct ioDoneEvent* f = ioDoneEventQueue.next;
        while(f != NULL){
            if(f == &ioDoneEventQueue){
                break;
            }
            if (f->doneTime == currentTime) {
                iFlag = 1;
                break;
            }
            f = f->next;
        }


        if (currentTime == nextForkTime && nproc < NPROC) { /* CASE 2 : a new process created */
            printf("nproc :%d\n",nproc);
            struct process* forkP = NULL;
            procTable[nproc].state = S_READY;
            procTable[nproc].startTime = currentTime;
            forkP = &procTable[nproc];
            if(readyQueue.len == 0){
                readyQueue.next = forkP;
            }
            else{
                readyQueue.prev->next = forkP;
            }
            readyQueue.prev = forkP;
            readyQueue.len++;

            if(runningProc->state == S_RUNNING && iFlag != 1 &&cpuUseTime != nextIOReqTime &&runningProc->serviceTime != runningProc->targetServiceTime){
                if(qTime == QUANTUM){
                    runningProc->priority -= 1;
                }
                runningProc->state = S_READY;
                procTable[runningProc->id] = *runningProc;
                procTable[runningProc->id].saveReg0 = cpuReg0;
                procTable[runningProc->id].saveReg1 = cpuReg1;
                struct process* frP = &procTable[runningProc->id];
                readyQueue.prev->next = frP;
                readyQueue.prev = frP;
                readyQueue.len++;
            }
            if(cpuUseTime != nextIOReqTime && iFlag != 1 && runningProc->serviceTime != runningProc->targetServiceTime){

                runningProc = scheduler();
                qTime = 0;
            }
            if(NIOREQ == 0 && nioFlag == 0){
                runningProc = scheduler();
                nioFlag= 1;
            }
            nproc++;
            if(nproc == NPROC){
                nextForkTime = -1;
            }else{
                nextForkTime = procIntArrTime[nproc] + currentTime;
            }
        }

        if (qTime == QUANTUM ) { /* CASE 1 : The quantum expires */
            runningProc->priority -= 1;
            if(runningProc->state == S_RUNNING && runningProc->serviceTime != runningProc->targetServiceTime && nextIOReqTime != cpuUseTime && iFlag != 1){
                runningProc->state = S_READY;
                procTable[runningProc->id] = *runningProc;
                procTable[runningProc->id].saveReg0 = cpuReg0;
                procTable[runningProc->id].saveReg1 = cpuReg1;
                struct process* qP = &procTable[runningProc->id];
                if(readyQueue.len == 0){
                    readyQueue.next = qP;
                }else{
                    readyQueue.prev->next = qP;
                }
                readyQueue.prev = qP;
                readyQueue.len++;
                runningProc = scheduler();
            }
            qTime = 0;
        }

        while (1) { /* CASE 3 : IO Done Event */
            struct ioDoneEvent* temp = ioDoneEventQueue.next;
            int ioid= -2;
            if(ioDoneEventQueue.next == &ioDoneEventQueue){
                break;
            }
            else if(ioDoneEventQueue.next->doneTime == currentTime){
                ioid = ioDoneEventQueue.next->procid;
                ioDoneEventQueue.next = ioDoneEventQueue.next->next;
                ioDoneEventQueue.len -= 1;
            }
            else{
                while(temp->next != NULL){
                    if (temp->next->doneTime == currentTime) {
                        ioid = temp->next->procid;
                        temp->next = temp->next->next;
                        if(temp->next == NULL){
                            ioDoneEventQueue.prev = temp;
                        }
                        ioDoneEventQueue.len -= 1;
                        break;
                    }
                    temp = temp->next;
                }
            }
            if(ioDoneEventQueue.len == 0){
                ioDoneEventQueue.prev = ioDoneEventQueue.next = &ioDoneEventQueue;
            }

            if(ioid == -2){
                break;
            }
            iFlag = 0;
            struct process* bTemp = NULL;
            if(procTable[ioid].state != S_TERMINATE){
                procTable[ioid].state = S_READY;
                struct process* t = blockedQueue.next;
                if(blockedQueue.next->id == ioid){
                    bTemp = blockedQueue.next;
                    blockedQueue.next = blockedQueue.next->next;
                    blockedQueue.len -= 1;
                    bTemp->next = NULL;
                }
                else{
                    while(t->next != NULL){
                        if(t->next->id == ioid){
                            bTemp = t->next;
                            t->next = t->next->next;
                            if(t->next == NULL){
                                blockedQueue.prev = t;
                            }
                            blockedQueue.len -= 1;
                            break;
                        }
                        t = t->next;
                    }
                }
                bTemp->next = NULL;
                if(blockedQueue.len == 0){
                    blockedQueue.prev = blockedQueue.next = &blockedQueue;
                }
                if(readyQueue.len == 0){
                    readyQueue.next = bTemp;
                }else{
                    readyQueue.prev->next = bTemp;
                }
                readyQueue.prev = bTemp;
                readyQueue.len++;
            }

            struct ioDoneEvent* ff = ioDoneEventQueue.next;
            while(ff != NULL){
                if(ff == &ioDoneEventQueue){
                    break;
                }
                if (ff->doneTime == currentTime) {
                    iFlag = 1;
                    break;
                }
                ff = ff->next;
            }
            if(runningProc->state == S_RUNNING && iFlag != 1 &&runningProc->serviceTime != runningProc->targetServiceTime && cpuUseTime != nextIOReqTime){


                runningProc->state = S_READY;
                procTable[runningProc->id] = *runningProc;
                procTable[runningProc->id].saveReg0 = cpuReg0;
                procTable[runningProc->id].saveReg1 = cpuReg1;
                struct process* idrP = &procTable[runningProc->id];
                if(readyQueue.len == 0){
                    readyQueue.next = idrP;
                }else{
                    readyQueue.prev->next = idrP;
                }
                readyQueue.prev = idrP;
                readyQueue.len++;
            }

            if( iFlag != 1 && runningProc->serviceTime != runningProc->targetServiceTime && cpuUseTime != nextIOReqTime){
                runningProc = scheduler();
                qTime = 0;

            }

        }

        if (cpuUseTime == nextIOReqTime && nioreq < NIOREQ) { /* CASE 5: reqest IO operations (only when the process does not terminate) */
            if(runningProc->state == S_RUNNING){
                pid = runningProc->id;
                if(runningProc->serviceTime != runningProc->targetServiceTime)
                {
                    if(qTime < QUANTUM && qTime != 0){
                        runningProc->priority += 1;
                    }
                    runningProc->state = S_BLOCKED;
                    procTable[pid] = *runningProc;
                    procTable[pid].saveReg0 = cpuReg0;
                    procTable[pid].saveReg1 = cpuReg1;
                    struct process* rqP = &procTable[pid];
                    if(blockedQueue.len == 0){
                        blockedQueue.next = rqP;
                    }else{
                        blockedQueue.prev->next = rqP;
                    }
                    blockedQueue.prev = rqP;
                    blockedQueue.len += 1;
                    runningProc = scheduler();
                    qTime = 0;
                }
                ioDoneEvent[nioreq].procid = pid;
                ioDoneEvent[nioreq].doneTime = ioServTime[nioreq] + currentTime;
                if(ioDoneEventQueue.len == 0){
                    ioDoneEventQueue.next = &ioDoneEvent[nioreq];
                }else{
                    ioDoneEventQueue.prev->next = &ioDoneEvent[nioreq];
                }
                ioDoneEventQueue.prev = &ioDoneEvent[nioreq];
                ioDoneEventQueue.len += 1;

            }
            nioreq++;
            if(nioreq == NIOREQ){
                nextIOReqTime = -1;
            }
            else{
                nextIOReqTime = ioReqIntArrTime[nioreq] + cpuUseTime;
            }
        }
        if (runningProc->serviceTime == runningProc->targetServiceTime) { /* CASE 4 : the process job done and terminates */
            runningProc->state = S_TERMINATE;
            runningProc->endTime = currentTime;
            procTable[runningProc->id] = *runningProc;
            procTable[runningProc->id].saveReg0 = cpuReg0;
            procTable[runningProc->id].saveReg1 = cpuReg1;
            termProc++;
            runningProc = &idleProc;

            runningProc = scheduler();
            qTime = 0;
        }

        if(termProc == NPROC){
            break;
        }
        // call scheduler() if needed

    } // while loop
}

//RR,SJF(Modified),SRTN,Guaranteed Scheduling(modified),Simple Feed Back Scheduling
struct process* RRschedule() {

    if(runningProc->serviceTime == runningProc->targetServiceTime){
        return runningProc;
    }
    if(readyQueue.len == 0){
        return &idleProc;
    }
    struct process* newP = readyQueue.next;
    readyQueue.next = newP->next;
    newP->next = NULL;
    readyQueue.len--;
    if(readyQueue.len == 0){
        readyQueue.prev = readyQueue.next = NULL;
    }
    newP->state = S_RUNNING;
    cpuReg0 = newP->saveReg0;
    cpuReg1 = newP->saveReg1;
    return newP;

}
struct process* SJFschedule() {
    if(runningProc->serviceTime == runningProc->targetServiceTime){
        return runningProc;
    }
    if(readyQueue.len == 0){
        return &idleProc;
    }
    struct process* rt = readyQueue.next;
    int target= readyQueue.next->targetServiceTime;
    while(rt->next != NULL){
        if (rt->next->targetServiceTime < target) {
            target = rt->next->targetServiceTime;
        }
        rt = rt->next;
    }
    struct process* temp = readyQueue.next;
    struct process* newP = readyQueue.next;
    if(readyQueue.next->targetServiceTime == target){
        readyQueue.next = readyQueue.next->next;
        readyQueue.len -= 1;
    }else{
        while(temp->next != NULL){
            if(temp->next->targetServiceTime == target){
                newP = temp->next;
                temp->next = temp->next->next;
                if(temp->next == NULL){
                    readyQueue.prev = temp;
                }
                readyQueue.len -= 1;
                break;
            }
            temp = temp->next;
        }
    }
    if(readyQueue.len == 0){
        readyQueue.next = readyQueue.prev = &readyQueue;
    }
    newP->state = S_RUNNING;
    newP->next = NULL;
    cpuReg0 = newP->saveReg0;
    cpuReg1 = newP->saveReg1;

    return newP;
}
struct process* SRTNschedule() {
    if(runningProc->serviceTime == runningProc->targetServiceTime){
        return runningProc;
    }
    if(readyQueue.len == 0){
        return &idleProc;
    }
    struct process* rt = readyQueue.next;
    int time= readyQueue.next->targetServiceTime - readyQueue.next->serviceTime;
    while(rt->next != NULL){
        if ((rt->next->targetServiceTime - rt->next->serviceTime) < time) {
            time = rt->next->targetServiceTime- rt->next->serviceTime;
        }
        rt = rt->next;
    }
    struct process* temp = readyQueue.next;
    struct process* newP = readyQueue.next;
    if((readyQueue.next->targetServiceTime - readyQueue.next->serviceTime) == time){
        readyQueue.next = readyQueue.next->next;
        readyQueue.len -= 1;
    }else{
        while(temp->next != NULL){
            if((temp->next->targetServiceTime - temp->next->serviceTime) == time){
                newP = temp->next;
                temp->next = temp->next->next;
                if(temp->next == NULL){
                    readyQueue.prev = temp;
                }
                readyQueue.len -= 1;
                break;
            }
            temp = temp->next;
        }
    }
    if(readyQueue.len == 0){
        readyQueue.next = readyQueue.prev = &readyQueue;
    }
    newP->state = S_RUNNING;
    newP->next = NULL;
    cpuReg0 = newP->saveReg0;
    cpuReg1 = newP->saveReg1;

    return newP;
}
struct process* GSschedule() {
    if(runningProc->serviceTime == runningProc->targetServiceTime){
        return runningProc;
    }
    if(readyQueue.len == 0){
        return &idleProc;
    }
    struct process* rt = readyQueue.next;
    double cpuT= (double)readyQueue.next->serviceTime/(double)readyQueue.next->targetServiceTime;
    while(rt->next != NULL){
        if (((double)rt->next->serviceTime/(double)rt->next->targetServiceTime) < cpuT) {
            cpuT = (double)rt->next->serviceTime/(double)rt->next->targetServiceTime;
        }
        rt = rt->next;
    }
    struct process* temp = readyQueue.next;
    struct process* newP = readyQueue.next;
    if(((double)readyQueue.next->serviceTime/(double)readyQueue.next->targetServiceTime) == cpuT){
        readyQueue.next = readyQueue.next->next;
        readyQueue.len -= 1;
    }else{
        while(temp->next != NULL){
            if(((double)temp->next->serviceTime/(double)temp->next->targetServiceTime) == cpuT){
                newP = temp->next;
                temp->next = temp->next->next;
                if(temp->next == NULL){
                    readyQueue.prev = temp;
                }
                readyQueue.len -= 1;
                break;
            }
            temp = temp->next;
        }
    }
    if(readyQueue.len == 0){
        readyQueue.next = readyQueue.prev = &readyQueue;
    }
    newP->state = S_RUNNING;
    newP->next = NULL;
    cpuReg0 = newP->saveReg0;
    cpuReg1 = newP->saveReg1;

    return newP;
}
struct process* SFSschedule() {
    if(runningProc->serviceTime == runningProc->targetServiceTime){
        return runningProc;
    }
    if(readyQueue.len == 0){
        return &idleProc;
    }
    struct process* rt = readyQueue.next;
    int priority= readyQueue.next->priority;

    while(rt->next != NULL){
        if (rt->next->priority > priority) {
            priority = rt->next->priority;
        }
        rt = rt->next;
    }
    struct process* temp = readyQueue.next;
    struct process* newP = readyQueue.next;
    if(readyQueue.next->priority == priority){
        readyQueue.next = readyQueue.next->next;
        readyQueue.len -= 1;
    }else{
        while(temp->next != NULL){
            if(temp->next->priority == priority){
                newP = temp->next;
                temp->next = temp->next->next;
                if(temp->next == NULL){
                    readyQueue.prev = temp;
                }
                readyQueue.len -= 1;
                break;
            }
            temp = temp->next;
        }
    }
    if(readyQueue.len == 0){
        readyQueue.next = readyQueue.prev = &readyQueue;
    }
    newP->state = S_RUNNING;
    newP->next = NULL;
    cpuReg0 = newP->saveReg0;
    cpuReg1 = newP->saveReg1;

    return newP;
}

void printResult() {
    // DO NOT CHANGE THIS FUNCTION
    int i;
    long totalProcIntArrTime=0,totalProcServTime=0,totalIOReqIntArrTime=0,totalIOServTime=0;
    long totalWallTime=0, totalRegValue=0;
    for(i=0; i < NPROC; i++) {
        totalWallTime += procTable[i].endTime - procTable[i].startTime;
        /*
        printf("proc %d serviceTime %d targetServiceTime %d saveReg0 %d\n",
            i,procTable[i].serviceTime,procTable[i].targetServiceTime, procTable[i].saveReg0);
        */
        totalRegValue += procTable[i].saveReg0+procTable[i].saveReg1;
        /* printf("reg0 %d reg1 %d totalRegValue %d\n",procTable[i].saveReg0,procTable[i].saveReg1,totalRegValue);*/
    }
    for(i = 0; i < NPROC; i++ ) {
        totalProcIntArrTime += procIntArrTime[i];
        totalProcServTime += procServTime[i];
    }
    for(i = 0; i < NIOREQ; i++ ) {
        totalIOReqIntArrTime += ioReqIntArrTime[i];
        totalIOServTime += ioServTime[i];
    }

    printf("Avg Proc Inter Arrival Time : %g \tAverage Proc Service Time : %g\n", (float) totalProcIntArrTime/NPROC, (float) totalProcServTime/NPROC);
    printf("Avg IOReq Inter Arrival Time : %g \tAverage IOReq Service Time : %g\n", (float) totalIOReqIntArrTime/NIOREQ, (float) totalIOServTime/NIOREQ);
    printf("%d Process processed with %d IO requests\n", NPROC,NIOREQ);
    printf("Average Wall Clock Service Time : %g \tAverage Two Register Sum Value %g\n", (float) totalWallTime/NPROC, (float) totalRegValue/NPROC);

}

int main(int argc, char *argv[]) {
    // DO NOT CHANGE THIS FUNCTION
    int i;
    int totalProcServTime = 0, ioReqAvgIntArrTime;
    int SCHEDULING_METHOD, MIN_INT_ARRTIME, MAX_INT_ARRTIME, MIN_SERVTIME, MAX_SERVTIME, MIN_IO_SERVTIME, MAX_IO_SERVTIME, MIN_IOREQ_INT_ARRTIME;

    if (argc < 12) {
        printf("%s: SCHEDULING_METHOD NPROC NIOREQ QUANTUM MIN_INT_ARRTIME MAX_INT_ARRTIME MIN_SERVTIME MAX_SERVTIME MIN_IO_SERVTIME MAX_IO_SERVTIME MIN_IOREQ_INT_ARRTIME\n",argv[0]);
        exit(1);
    }

    SCHEDULING_METHOD = atoi(argv[1]);
    NPROC = atoi(argv[2]);
    NIOREQ = atoi(argv[3]);
    QUANTUM = atoi(argv[4]);
    MIN_INT_ARRTIME = atoi(argv[5]);
    MAX_INT_ARRTIME = atoi(argv[6]);
    MIN_SERVTIME = atoi(argv[7]);
    MAX_SERVTIME = atoi(argv[8]);
    MIN_IO_SERVTIME = atoi(argv[9]);
    MAX_IO_SERVTIME = atoi(argv[10]);
    MIN_IOREQ_INT_ARRTIME = atoi(argv[11]);

    printf("SIMULATION PARAMETERS : SCHEDULING_METHOD %d NPROC %d NIOREQ %d QUANTUM %d \n", SCHEDULING_METHOD, NPROC, NIOREQ, QUANTUM);
    printf("MIN_INT_ARRTIME %d MAX_INT_ARRTIME %d MIN_SERVTIME %d MAX_SERVTIME %d\n", MIN_INT_ARRTIME, MAX_INT_ARRTIME, MIN_SERVTIME, MAX_SERVTIME);
    printf("MIN_IO_SERVTIME %d MAX_IO_SERVTIME %d MIN_IOREQ_INT_ARRTIME %d\n", MIN_IO_SERVTIME, MAX_IO_SERVTIME, MIN_IOREQ_INT_ARRTIME);

    srandom(SEED);

    // allocate array structures
    procTable = (struct process *) malloc(sizeof(struct process) * NPROC);
    ioDoneEvent = (struct ioDoneEvent *) malloc(sizeof(struct ioDoneEvent) * NIOREQ);
    procIntArrTime = (int *) malloc(sizeof(int) * NPROC);
    procServTime = (int *) malloc(sizeof(int) * NPROC);
    ioReqIntArrTime = (int *) malloc(sizeof(int) * NIOREQ);
    ioServTime = (int *) malloc(sizeof(int) * NIOREQ);

    // initialize queues
    readyQueue.next = readyQueue.prev = &readyQueue;

    blockedQueue.next = blockedQueue.prev = &blockedQueue;
    ioDoneEventQueue.next = ioDoneEventQueue.prev = &ioDoneEventQueue;
    ioDoneEventQueue.doneTime = INT_MAX;
    ioDoneEventQueue.procid = -1;
    ioDoneEventQueue.len  = readyQueue.len = blockedQueue.len = 0;

    // generate process interarrival times
    for(i = 0; i < NPROC; i++ ) {
        procIntArrTime[i] = random()%(MAX_INT_ARRTIME - MIN_INT_ARRTIME+1) + MIN_INT_ARRTIME;
    }

    // assign service time for each process
    for(i=0; i < NPROC; i++) {
        procServTime[i] = random()% (MAX_SERVTIME - MIN_SERVTIME + 1) + MIN_SERVTIME;
        totalProcServTime += procServTime[i];
    }

    ioReqAvgIntArrTime = totalProcServTime/(NIOREQ+1);
    assert(ioReqAvgIntArrTime > MIN_IOREQ_INT_ARRTIME);

    // generate io request interarrival time
    for(i = 0; i < NIOREQ; i++ ) {
        ioReqIntArrTime[i] = random()%((ioReqAvgIntArrTime - MIN_IOREQ_INT_ARRTIME)*2+1) + MIN_IOREQ_INT_ARRTIME;
    }

    // generate io request service time
    for(i = 0; i < NIOREQ; i++ ) {
        ioServTime[i] = random()%(MAX_IO_SERVTIME - MIN_IO_SERVTIME+1) + MIN_IO_SERVTIME;
    }

#ifdef DEBUG
    // printing process interarrival time and service time
    printf("Process Interarrival Time :\n");
    for(i = 0; i < NPROC; i++ ) {
        printf("%d ",procIntArrTime[i]);
    }
    printf("\n");
    printf("Process Target Service Time :\n");
    for(i = 0; i < NPROC; i++ ) {
        printf("%d ",procTable[i].targetServiceTime);
    }
    printf("\n");
#endif

    // printing io request interarrival time and io request service time
    printf("IO Req Average InterArrival Time %d\n", ioReqAvgIntArrTime);
    printf("IO Req InterArrival Time range : %d ~ %d\n",MIN_IOREQ_INT_ARRTIME,
           (ioReqAvgIntArrTime - MIN_IOREQ_INT_ARRTIME)*2+ MIN_IOREQ_INT_ARRTIME);

#ifdef DEBUG
    printf("IO Req Interarrival Time :\n");
    for(i = 0; i < NIOREQ; i++ ) {
        printf("%d ",ioReqIntArrTime[i]);
    }
    printf("\n");
    printf("IO Req Service Time :\n");
    for(i = 0; i < NIOREQ; i++ ) {
        printf("%d ",ioServTime[i]);
    }
    printf("\n");
#endif

    struct process* (*schFunc)();
    switch(SCHEDULING_METHOD) {
        case 1 : schFunc = RRschedule; break;
        case 2 : schFunc = SJFschedule; break;
        case 3 : schFunc = SRTNschedule; break;
        case 4 : schFunc = GSschedule; break;
        case 5 : schFunc = SFSschedule; break;
        default : printf("ERROR : Unknown Scheduling Method\n"); exit(1);
    }
    initIoDoneEvent();
    initProcTable();
    procExecSim(schFunc);
    printResult();

}
