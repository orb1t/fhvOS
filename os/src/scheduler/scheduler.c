/*
 * scheduler.c
 *
 *  Created on: 01.04.2015
 *      Author: Kevin
 */
#include "scheduler.h"
#include "../hal/cpu/hal_cpu.h"
#include "../driver/timer/driver_timer.h"
#include "../memmanager/mmu.h"
#include "../kernel/kernel.h"
#include "../systemapi/includes/system.h"

/*
 * Defines for the process stack start and size
 */
#define STACK_START		(0x8000FFFC)
// 3840 Byte
#define STACK_SIZE		(10000)

#define TIME_SLICE		(10)

/*
 * Register defines
 */
#define R13			(13)

/*
 * Global variables
 */
static processId_t runningProcess;
static process_t processes[PROCESSES_MAX];
static device_t timer;
static boolean_t schedulingEnabled;

/*
 * Internal functions
 */
static processId_t getNextProcessIdByState(processState_t state, int startId);
static processId_t getNextFreeProcessId(void);
static processId_t getNextReadyProcessId(void);

static void timerISR(address_t* context);

void dummyEnd() {
	exit(0);
}

/*
 * Functions from the .h file
 */
int SchedulerInit(void) {
	memset((void*)STACK_START, 0, STACK_SIZE * 1);

	int i;
	for (i = 0; i < PROCESSES_MAX; i++) {
		processes[i].state = FREE;
		processes[i].context = (context_t*) malloc(sizeof(context_t));
		memset(processes[i].context, 0, sizeof(context_t));
	}

	schedulingEnabled = true;

	// No process is running
	runningProcess = INVALID_PROCESS_ID;
	return SCHEDULER_OK;
}

int SchedulerStart(device_t initializedTimerDevice)
{
	timer = initializedTimerDevice;

	uint16_t timeInMilis = TIME_SLICE;
	uint16_t interruptMode = 0x02; // overflow
	uint16_t priority = 0x1;

	DeviceManagerIoctl(timer, timeInMilis, interruptMode, (char*) timerISR, priority);
	DeviceManagerOpen(timer);

	//SchedulerRunNextProcess(NULL);

	return 0;
}

process_t* SchedulerStartProcess(processFunc func) {
	CPUAtomicStart();

	processId_t freeProcess = getNextFreeProcessId();
	if (freeProcess == INVALID_PROCESS_ID) {
		CPUAtomicEnd();
		return NULL;
	}

	processes[freeProcess].func = func;
	processes[freeProcess].id = freeProcess;
	processes[freeProcess].state = BLOCKED;

	// IMPORTANT: when a task which was interrupted by SWI is scheduled by IRQ the
	// PC must be incremented because PC will be changed by return through SUBS thus prevent to repeat last SWI instruction
	// IMPORTANT: when a task which was interrupted by IRQ is scheduled by SWI the
	// PC must be subtracted because PC was incremented but SWI will not repeat instruction thus decrement PC to repeat
	// CONCLUSION: increment for SWI here and decrement according to systemstate in scheduleNextReady
	processes[freeProcess].context->pc = (register_t*)func;
	processes[freeProcess].context->lr = (address_t*)&dummyEnd;
	//processes[freeProcess].context->sp = (address_t*) (STACK_START + (freeProcess * STACK_SIZE));
	processes[freeProcess].context->sp = (address_t*) (0x10002000);

	// CPSR
	// N|Z|C|V|Q|IT|J| DNM| GE | IT   |E|A|I|F|T|  M  |
	// Code | Size | Description
	// N 	| [1]  | Negative/Less than
	// Z 	| [1]  | Zero
	// C 	| [1]  | Carry/Borrow/Extend
	// V 	| [1]  | Overflow
	// Q 	| [1]  | Sticky Overflow
	// IT   | [2]  | IT [1:0]
	// J	| [1]  | Java state bit
	// DNM  | [4]  | Do not modify
	// GE	| [4]  | Greater than or equal to
	// IT	| [6]  | IT [7:2]
	// E	| [1]  | Data endianness bit
	// A	| [1]  | Imprecise abort disable bit
	// I	| [1]  | IRQ disable
	// F	| [1]  | FIQ disable
	// T	| [1]  | Thumb state bit
	// M	| [1]  | M[4:0] Mode bits
	uint32_t userMode = 0b10000;
	processes[freeProcess].context->cpsr = (address_t*) userMode;
	// Let R13 point to the PCB of the running process
	// processes[freeProcess].context->registers[R13] = (void*) (STACK_START + STACK_SIZE);
	// processes[freeProcess].context->registers[R13] = (void*) (STACK_START + freeProcess * STACK_SIZE);
	// TODO: check this atomic end needed

	if (MMUInitProcess(&processes[freeProcess]) == MMU_NOT_OK)
	{
		processes[freeProcess].state = FREE;
		return NULL;
	}

	CPUAtomicEnd();
	return &processes[freeProcess];
}

int SchedulerRunNextProcess(context_t* context) {
	CPUAtomicStart();

	processId_t nextProcess = getNextReadyProcessId();
	if (nextProcess == INVALID_PROCESS_ID) {
		CPUAtomicEnd();
		return SCHEDULER_ERROR;
	}

	// TODO: Decide what should be done with a running process
	// which is finished or blocked
	if (processes[runningProcess].state == RUNNING) {
		// Set the running process to ready
		processes[runningProcess].state = READY;
		// Save the current context to the running process
		// and then change the running process
		memcpy(processes[runningProcess].context, context, sizeof(context_t));
	}

	// Set the next processes to running
	runningProcess = nextProcess;
	processes[runningProcess].state = RUNNING;

	// Update the context for the next running process
	memcpy(context, processes[runningProcess].context, sizeof(context_t));
	MMUSwitchToProcess(&processes[runningProcess]);
	CPUAtomicEnd();
	return SCHEDULER_OK;
}

int SchedulerKillProcess(processId_t id) {
	CPUAtomicStart();
	if (id < 0 || id >= PROCESSES_MAX) {
		CPUAtomicEnd();
		return SCHEDULER_ERROR;
	}

	KernelDebug("Scheduler is killing process with pid=%i\n", id);


	processes[id].state = FREE;
	processes[id].func = NULL;

	MMUFreeAllPageFramesOfProcess(&processes[id]);
	CPUAtomicEnd();
	return SCHEDULER_OK;
}

process_t* SchedulerGetRunningProcess(void) {
	if (runningProcess == INVALID_PROCESS_ID) {
		// This error should normally not appear
		return NULL;
	}

	return &processes[runningProcess];
}

void SchedulerBlockProcess(processId_t process)
{
	processes[process].state = BLOCKED;

	// TODO This works but decreases performance of the os
	// We should immediately run the next thread by a software interrupt
	while (processes[process].state == BLOCKED)
	{
		;
	}
}

void SchedulerUnblockProcess(processId_t process)
{
	processes[process].state = READY;
}

/*
 * Helper methods
 */
processId_t getNextFreeProcessId(void) {
	return getNextProcessIdByState(FREE, 0);
}

// Round Robin
processId_t getNextReadyProcessId(void) {
	return getNextProcessIdByState(READY, runningProcess + 1);
}

processId_t getNextProcessIdByState(processState_t state, int startId) {
	int i;

	if(startId < 0 || startId >= PROCESSES_MAX)
	{
		return INVALID_PROCESS_ID;
	}

	for (i = 0; i < PROCESSES_MAX; i++) {
		if (processes[(i + startId) % PROCESSES_MAX].state == state) {
			return (i + startId) % PROCESSES_MAX;
		}
	}

	return INVALID_PROCESS_ID;
}

void SchedulerDisableScheduling(void)
{
	schedulingEnabled = false;
}

void SchedulerEnableScheduling(void)
{
	schedulingEnabled = true;
}

void timerISR(address_t* context)
{
	KernelTick(TIME_SLICE); // TODO See Bug #66 (This should be extracted to another timer with 1ms)

	context_t* procContext = (context_t*) context;

	DeviceManagerWrite(timer, DISABLE_INTERRUPTS, TIMER_IRQ_OVERFLOW);
	DeviceManagerWrite(timer, CLEAR_INTERRUPT_STATUS, TIMER_IRQ_OVERFLOW);

	if (schedulingEnabled)
	{
		SchedulerRunNextProcess(procContext);
	}

	DeviceManagerWrite(timer, ENABLE_INTERRUPTS, TIMER_IRQ_OVERFLOW);
	DeviceManagerOpen(timer);
}
