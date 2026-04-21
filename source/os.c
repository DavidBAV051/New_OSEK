/*
 * os.c
 *
 *  Created on: Mar 23, 2026
 *      Author: david
 */

#include "os.h"
#include "board.h"
#include <string.h>

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

/* Queues */
Queue S_queues[MAX_QUEUES];
u8 queue_pool[MAX_QUEUES][QUEUE_BUFFER_SIZE];
u8 config_queues = 0;

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
/*==================================================================*/
/* Quees Section */
u8 init_queue(u32 permisos_mask, u8 ID, u32 max_elementos, u32 size_dato) {
    if(config_queues >= MAX_QUEUES) {
        return E_OS_LIMIT;
    }

    //Validar que los datos caben en nuestro buffer
    if((max_elementos * size_dato) > QUEUE_BUFFER_SIZE) {
        return E_OS_LIMIT;
    }

    //Tomar la siguiente queue disponible
    Queue* q = &S_queues[config_queues];

    q->ID = ID;
    q->Read = 0;
    q->Write = 0;
    q->Size = max_elementos;
    q->Current_count = 0;
    q->Item_size = size_dato;
    q->Task_permissions = permisos_mask;

    q->data = queue_pool[config_queues];

    config_queues++;

    return E_OK;
}

u8 write_queue(u8 ID, void* ptr_write_dato, u32 wait_ticks) {
	Queue* q = NULL;

	    //Buscar la queue por su ID
	    for(int i = 0; i < config_queues; i++){
	        if(S_queues[i].ID == ID){
	            q = &S_queues[i];
	            break;
	        }
	    }
	    if(q == NULL) return E_OS_ID;

	    //Permisos
	    if((q->Task_permissions & (1 << current_task_id)) == 0){
	        return E_OS_LIMIT;
	    }

	    Enter_Critical();//Deshabilitar interrupciones

	    //Espacio
	    if(q->Current_count >= q->Size){
	        Exit_Critical();
	        if(wait_ticks > 0){
	            task_delay(wait_ticks);
	        }
	        return E_OS_LIMIT; // Llena
	    }

	    //Calcular la dirección exacta donde escribir
	    u32 offset = q->Write * q->Item_size;
	    memcpy(&(q->data[offset]), ptr_write_dato, q->Item_size);

	    //Actualizar apuntadores
	    q->Write = (q->Write + 1) % q->Size;
	    q->Current_count++;

	    Exit_Critical();//Habilitar interrupciones

	    return E_OK;
}

u8 read_queue(u8 ID, void* ptr_read_dato, u32 wait_ticks) {
    Queue* q = NULL;

    //Buscar la queue por su ID
    for(int i = 0; i < config_queues; i++){
        if(S_queues[i].ID == ID){
            q = &S_queues[i];
            break;
        }
    }
    if(q == NULL) return E_OS_ID;//Queue no encontrada

    //Permisos
    if((q->Task_permissions & (1 << current_task_id)) == 0){
        return E_OS_LIMIT; // Task no tiene permiso
    }

    Enter_Critical();//Deshabilitar interrupciones

    //Queue vacía
    if(q->Current_count == 0){
        Exit_Critical();
        if(wait_ticks > 0){
            task_delay(wait_ticks);
        }
        return E_OS_LIMIT; //No había datos
    }

    //Calcular el offset y copiar el dato
    u32 offset = q->Read * q->Item_size;
    memcpy(ptr_read_dato, &(q->data[offset]), q->Item_size);

    //Actualizar apuntadores
    q->Read = (q->Read + 1) % q->Size;
    q->Current_count--;

    Exit_Critical();//Habilitar interrupciones

    return E_OK;
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

	task_arr[TASK_ISR_BTN_ID].Autostart = FALSE;
	task_arr[TASK_ISR_BTN_ID].Priority = TWO;
	task_arr[TASK_ISR_BTN_ID].DirTask = task_ISR_BTN;

	task_arr[TASK_2_ID].Autostart = TRUE;
	task_arr[TASK_2_ID].Priority = ONE;
	task_arr[TASK_2_ID].DirTask = task_PWM1;

	task_arr[TASK_3_ID].Autostart = TRUE;
	task_arr[TASK_3_ID].Priority = ONE;
	task_arr[TASK_3_ID].DirTask = task_PWM2;

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
