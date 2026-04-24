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
u8 num_tasks_configured = CONFIGURED_TASKS;
u8 volatile interrupt_active = ZERO;
volatile bool td_flag = FALSE;

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
			temp_sp = (unsigned long)task_arr[current_task_id].SP_Pause;

			Context_Restore();
		}
		if(task_arr[current_task_id].DirTask != ZERO && idle != ONE){
			idle = ZERO;
			(*task_arr[current_task_id].DirTask)();
		}
		if(interrupt_active > ZERO){
			interrupt_active --;
			idle = ZERO;
			Context_Restore_ISR();
		}
	}
}

/*==================================================================*/
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

/*==================================================================*/
/* Task Delay Section */
__attribute__((naked)) void task_delay(u32 ticks){
    __asm volatile (
        "ldr r2, =temp_sp   \n"
        "str r13, [r2]      \n"
        "ldr r2, =temp      \n"
        "str r14, [r2]      \n"
        "bl task_delay_savedctxt \n"
        "bx lr              \n"
    );
}
void task_delay_savedctxt(u32 ticks){
    task_arr[current_task_id].task_counter = ticks;
    task_arr[current_task_id].Estado = WAIT;
    task_arr[current_task_id].Pause = ONE;
    task_arr[current_task_id].DirTask_Pause = (void (*)(void))temp;
    task_arr[current_task_id].SP_Pause = temp_sp;
    scheduler();
}
void SysTick_Handler(void) {
	u8 i;
	for(i = ZERO; i < MAX_NUMBER_TASKS; i++){
		if(task_arr[i].Estado == WAIT){
			if(task_arr[i].task_counter > ZERO){
				task_arr[i].task_counter--;

				if(task_arr[i].task_counter == ZERO){
					task_arr[i].Estado = READY;
					td_flag = TRUE;
				}
			}
		}
	}
}

/*==================================================================*/

void sem_init(Semaphore_t *s, u8 initial_count){
    s->count           = initial_count;
    s->waiting_task_id = U8_SIZE;
}

void sem_wait(Semaphore_t *s){
    Context_Backup();

    if(s->count > ZERO){
        s->count--;
    } else {
        s->waiting_task_id = current_task_id;

        task_arr[current_task_id].Estado = WAIT;
        task_arr[current_task_id].Pause = TRUE;
        task_arr[current_task_id].DirTask_Pause = (void (*)(void))temp;
        task_arr[current_task_id].SP_Pause = temp_sp;

        scheduler();
    }
}

void sem_signal(Semaphore_t *s){
    if(s->waiting_task_id != U8_SIZE){
        u8 waiting         = s->waiting_task_id;
        s->waiting_task_id = U8_SIZE;
        task_arr[waiting].Estado = READY;
    } else {
        s->count++;
    }
}

/*==================================================================*/
u8 activate_task(u8 Task_ID){
	Context_Backup();
	if(Task_ID >= MAX_NUMBER_TASKS){
		return E_OS_LIMIT;
	}
	task_arr[Task_ID].Estado = READY;
	task_arr[current_task_id].Estado = READY;
	task_arr[current_task_id].Pause = TRUE;

	task_arr[current_task_id].DirTask_Pause = (void (*)(void))temp;
	task_arr[current_task_id].SP_Pause = temp_sp;

	scheduler();
	return E_OK;
}

u8 activate_task_ISR(u8 Task_ID){
    Context_Backup_ISR();

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

/*==================================================================*/
void task_config(void){
	task_arr[TASK_IDLE_ID].Autostart = TRUE;
	task_arr[TASK_IDLE_ID].Priority = ZERO;
	task_arr[TASK_IDLE_ID].DirTask = task_idle;

	task_arr[TASK_ISR_TMR_ID].Autostart = FALSE;
	task_arr[TASK_ISR_TMR_ID].Priority = FIVE;
	task_arr[TASK_ISR_TMR_ID].DirTask = task_ISR_TMR;

	// Para los Pwms
	task_arr[TASK_2_ID].Autostart = TRUE;
	task_arr[TASK_2_ID].Priority = ONE;
	task_arr[TASK_2_ID].DirTask = task_PWM1;

	task_arr[TASK_3_ID].Autostart = TRUE;
	task_arr[TASK_3_ID].Priority = ONE;
	task_arr[TASK_3_ID].DirTask = task_PWM2;

	task_arr[TASK_4_ID].Autostart = TRUE;
	task_arr[TASK_4_ID].Priority = ONE;
	task_arr[TASK_4_ID].DirTask = task_PWM3;

	//Para las uarts
	task_arr[TASK_5_ID].Autostart = FALSE;
	task_arr[TASK_5_ID].Priority = ONE;
	task_arr[TASK_5_ID].DirTask = task_UART1;

	task_arr[TASK_6_ID].Autostart = FALSE;
	task_arr[TASK_6_ID].Priority = ONE;
	task_arr[TASK_6_ID].DirTask = task_UART2;

	task_arr[TASK_7_ID].Autostart = FALSE;
	task_arr[TASK_7_ID].Priority = ONE;
	task_arr[TASK_7_ID].DirTask = task_UART3;

}

/*==================================================================*/
void task_idle(void){
	while(ONE){
		if(td_flag == TRUE){
			td_flag = FALSE;
			task_arr[current_task_id].Estado = READY;
			scheduler();
		}
	}
}
