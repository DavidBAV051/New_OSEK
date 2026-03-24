/*
 * os.c
 *
 *  Created on: Mar 23, 2026
 *      Author: david
 */

#include "os.h"
#include "board.h"

/*==================================================================*/
/* Variables */
Task_Control_Struct task_arr[MAX_NUMBER_TASKS];
unsigned long idle;
unsigned long temp;
unsigned long temp_sp;
u8 current_task_id = ZERO;
u8 num_tasks_configured = FIVE;
u8 volatile interrupt_active = ZERO;

/*==================================================================*/
void scheduler(void){
	u8 i;
	u8 max_prioridad_actual = ZERO;
	u8 task_to_run = U8_SIZE;
	u8 task_found = FALSE;

	for(i = ZERO; i < num_tasks_configured; i++){
		if(task_arr[i].Estado == READY){
			if(task_arr[i].Priority >= max_prioridad_actual){
				max_prioridad_actual = task_arr[i].Priority;
				task_to_run = i;
				task_found = TRUE;
			}
		}
	}
	if(task_found == TRUE){
		current_task_id = task_to_run;
		task_arr[current_task_id].Estado = RUNNING;

		if(task_arr[current_task_id].Pause == TRUE){
			task_arr[current_task_id].Pause = FALSE;

			temp = (unsigned long)task_arr[current_task_id].DirTask_Pause;

			__asm volatile ("ldr r2, =temp");
			__asm volatile ("ldr r2, [r2]");
			__asm volatile ("orr r2, r2, #1");
			__asm volatile ("mov r15, r2");
		}
		if(task_arr[current_task_id].DirTask != ZERO && idle != ONE){
			idle = ZERO;
			(*task_arr[current_task_id].DirTask)();
		}
		if(interrupt_active > ZERO){
			interrupt_active --;
			idle = ZERO;
			Context_Restore();
		}
	}
}

void os_init(void){
	u8 i;
	for(i = ZERO; i < num_tasks_configured; i++){
		if(task_arr[i].Autostart == TRUE){
			current_task_id = i;
			task_arr[i].Estado = READY;
		} else{
			task_arr[i].Estado = SUSPENDED;
		}
	}

	/* Start System */
	scheduler();
}

u8 activate_task(u8 Task_ID){
	if(Task_ID >= MAX_NUMBER_TASKS){
		return E_OS_LIMIT;
	}
	task_arr[Task_ID].Estado = READY;
	task_arr[current_task_id].Estado = READY;
	task_arr[current_task_id].Pause = TRUE;

	__asm volatile ("ldr r2, =temp");
	__asm volatile ("str r14, [r2]");

	task_arr[current_task_id].DirTask_Pause = (void (*)(void))temp;

	scheduler();
	return E_OK;
}

u8 activate_task_ISR(u8 Task_ID){
    Context_Backup();

	if(Task_ID >= MAX_NUMBER_TASKS){
		return E_OS_LIMIT;
	}

	task_arr[Task_ID].Estado = READY;
	task_arr[current_task_id].Estado = READY;
	interrupt_active += ONE;
	scheduler();
	return E_OK;
}
void terminate_task(void){
	task_arr[current_task_id].Estado = SUSPENDED;
	scheduler();
}
void terminate_task_ISR(void){
	task_arr[current_task_id].Estado = SUSPENDED;
	idle = ONE;
	scheduler();
}
void chain_task(u8 Task_ID){
	if(Task_ID < MAX_NUMBER_TASKS){
		task_arr[Task_ID].Estado = READY;
	}
	task_arr[current_task_id].Estado = SUSPENDED;
	scheduler();
}

void task_config(void){
	task_arr[TASK_IDLE_ID].Autostart = TRUE;
	task_arr[TASK_IDLE_ID].Priority = ZERO;
	task_arr[TASK_IDLE_ID].DirTask = task_idle;

	task_arr[TASK_1_ID].Autostart = TRUE;
	task_arr[TASK_1_ID].Priority = ONE;
	task_arr[TASK_1_ID].DirTask = task_1;

	task_arr[TASK_2_ID].Autostart = FALSE;
	task_arr[TASK_2_ID].Priority = TWO;
	task_arr[TASK_2_ID].DirTask = task_2;

	task_arr[TASK_3_ID].Autostart = FALSE;
	task_arr[TASK_3_ID].Priority = THREE;
	task_arr[TASK_3_ID].DirTask = task_3;

	task_arr[TASK_4_ID].Autostart = FALSE;
	task_arr[TASK_4_ID].Priority = FOUR;
	task_arr[TASK_4_ID].DirTask = task_4;
}
