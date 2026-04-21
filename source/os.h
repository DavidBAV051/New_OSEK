/*
 * os.h
 *
 *  Created on: Mar 23, 2026
 *      Author: david
 */

#ifndef OS_H_
#define OS_H_

#include <stdint.h>
#include <stdbool.h>

/*==================================================================*/
/* Definitions */
#define MAX_NUMBER_TASKS 5
#define CONFIGURED_TASKS 5

#define MAX_QUEUES 3
#define QUEUE_BUFFER_SIZE 128

#define E_OK       0
#define E_OS_LIMIT 1
#define E_OS_ID    2

#define TRUE 1
#define FALSE 0

#define TASK_IDLE_ID 0
#define TASK_ISR_BTN_ID 1
#define TASK_2_ID 2
#define TASK_3_ID 3
#define TASK_4_ID 4

#define ZERO 0
#define ONE 1
#define TWO 2
#define THREE 3
#define FOUR 4
#define FIVE 5
#define SIX 6
#define SEVEN 7
#define EIGHT 8
#define NINE 9

#define U8_SIZE 255
#define STACK_SIZE 128

/*==================================================================*/
/* Context Backup*/
#define Context_Backup() \
	__asm volatile ("ldr r2, =temp_sp\n" \
					"str r13, [r2]\n" \
					"ldr r2, =temp\n" \
					"str r14, [r2]");
#define Context_Restore() \
	__asm volatile ("ldr r2, =temp\n"\
					"ldr r2, [r2]\n"\
					"ldr r3, =temp_sp\n"\
					"ldr r3, [r3]\n"\
					"orr r2, r2, #1\n"\
					"mov r13, r3\n"\
					"mov r15, r2");

/* Context Backup for ISRs */
#define Context_Backup_ISR() \
    __asm volatile("push {r4-r11}\n" \
                   "ldr r0, =temp_sp  \n" \
                   "str r13, [r0]       ")
#define Context_Restore_ISR() \
    __asm volatile("ldr r0, =temp_sp  \n" \
                   "ldr r13, [r0]     \n" \
                   "pop {r4-r11}    ")

/*==================================================================*/
/* Type definitions */
typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;

typedef enum{
	SUSPENDED = 0,
	READY,
	RUNNING,
	WAIT
} TASK_STATES;

typedef struct{
	u8 Autostart;
	u8 Priority;
	u8 Pause;
	TASK_STATES Estado;
	void (*DirTask)(void);
	void (*DirTask_Pause)(void);
	unsigned long SP_Pause;
	u32 task_counter;
}Task_Control_Struct;

/*=== Queus ===*/
#define Enter_Critical()  __asm volatile("cpsid i") // Interrupt disabl
#define Exit_Critical()   __asm volatile("cpsie i") // Interupt enable

typedef struct {
    u8 ID;
    u32 Read;
    u32 Write;
    u32 Size;
    u32 Current_count;
    u32 Item_size;
    u32 Task_permissions;
    u8* data;
} Queue;
/*==================================================================*/
/* Global Variables */
extern Task_Control_Struct task_arr[MAX_NUMBER_TASKS];
extern unsigned long idle;
extern unsigned long temp;
extern unsigned long temp_sp;
extern u8 current_task_id;
extern volatile u8 interrupt_active;

extern volatile u8 tick_count;
extern volatile bool td_flag;

/*==================================================================*/
/* Function Protoypes */
void task_config(void);

void os_init(void);
u8 activate_task(u8 Task_ID);
u8 activate_task_ISR(u8 Task_ID);
void terminate_task(void);
void terminate_task_ISR(void);
void chain_task(u8 Task_ID);
void scheduler(void);

void SysTick_Handler(void);
void task_delay(u32 ticks);
void task_delay_savedctxt(u32 ticks);

/* Task Prototypes */
void task_idle(void);
void task_ISR_BTN(void);
void task_PWM1(void);
void task_PWM2(void);

/* Queues Protoype */
u8 init_queue(u32 permisos_mask, u8 ID, u32 max_elementos, u32 size_dato);
u8 write_queue(u8 ID, void* ptr_write_dato, u32 wait_ticks);
u8 read_queue(u8 ID, void* ptr_read_dato, u32 wait_ticks);

/*==================================================================*/
#endif /* OS_H_ */
