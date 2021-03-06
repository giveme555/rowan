/***********************************
 *	Copyright (c) 2015
 * 	microtrust
 * 	All rights reserved
 * 	Author: Steven Meng
 **********************************/
#include <assert.h>
#include <debug.h>
#include <string.h>

#include <platform.h>
#include "plat_private.h"
#include <arch_helpers.h>

#include <bl_common.h>
#include <teei_private.h>
#include <gic_v2.h>
#include <gic_v3.h>
#include <stdio.h>
#include "plat_teei.h"
#include <mt_gic_v3.h>
void gicd_v3_do_wait_for_rwp(unsigned int gicd_base);
// Initilization parameters for teei.
static tee_arg_t_ptr teeiBootCfg;
static tee_keys_t_ptr teeiKeys;

extern uint32_t uart_apc_num;
extern uint32_t spi_apc_num;
extern uint32_t TEEI_STATE;

unsigned int sec_exc[8];
unsigned int nsec_exc[8];


unsigned int SEC_EXC_CNT = 0;
unsigned int NSEC_EXC_CNT = 0;

unsigned int NSEC_TIMER; 	//	     30
unsigned int NSEC_UART;      //          124

unsigned int SEC_TIMER;      //          205
//~ unsigned int SEC_TP; 		      //           189
unsigned int SEC_SPI=154;

uint64_t gic_mpidr_to_affinity(uint64_t mpidr);
int gic_populate_rdist(unsigned int *rdist_base);
void aee_wdt_dump();
int gic_populate_rdist(unsigned int *rdist_base);
void irq_raise_softirq(unsigned int map, unsigned int irq);

void disable_group(unsigned int grp)
{
	unsigned int  pre;
	pre = gicd_v3_read_ctlr(BASE_GICD_BASE);
	pre =pre &(~(1<<grp));
	gicd_v3_write_ctlr(BASE_GICD_BASE, pre);
	gicd_v3_do_wait_for_rwp(BASE_GICD_BASE);
}

void enable_group(unsigned int grp)
{
	unsigned int  pre;;
	pre = gicd_v3_read_ctlr(BASE_GICD_BASE);
	pre =pre |(1<<grp);
	gicd_v3_write_ctlr(BASE_GICD_BASE, pre);
	gicd_v3_do_wait_for_rwp(BASE_GICD_BASE);
}


void  disable_ns_exc_intr( unsigned int gicd_base)
{
    unsigned int index;
    for (index =0;  index < NSEC_EXC_CNT; index++) {
			gicd_set_icenabler(gicd_base, nsec_exc[index]);
			gicd_v3_do_wait_for_rwp(gicd_base);
		}
}

void  enable_ns_exc_intr(unsigned int gicd_base)
{
	unsigned int index;
    for (index =0;  index < NSEC_EXC_CNT; index++) {
			gicd_set_isenabler(gicd_base,  nsec_exc[index]);
			gicd_v3_do_wait_for_rwp(gicd_base);
		}
}

void  disable_s_exc_intr(unsigned int gicd_base)
{
	unsigned int index;
    for (index =0;  index < SEC_EXC_CNT; index++) {
			 gicd_set_icenabler(gicd_base, sec_exc[index]);
			 gicd_v3_do_wait_for_rwp(gicd_base);
		}
}

void  enable_s_exc_intr(unsigned int gicd_base)
{
   unsigned int index;
    for ( index =0;  index < SEC_EXC_CNT; index++){
			gicd_set_isenabler(gicd_base,  sec_exc[index]);
			gicd_v3_set_irouter(gicd_base, sec_exc[index],\
					gic_mpidr_to_affinity(read_mpidr()));
			gicd_v3_do_wait_for_rwp(gicd_base);

		}
}

void prepare_gic_for_nsec_boot()
{
		unsigned int gicd_base;
		gicd_base = BASE_GICD_BASE;
		disable_s_exc_intr(gicd_base);
}
unsigned int get_irq_target(unsigned int irq)
{
		unsigned int gicd_base;
		gicd_base = BASE_GICD_BASE;
		unsigned int val =   gicd_read_itargetsr(gicd_base, irq);
		return val;


}

void prepare_gic_for_sec_boot()
{
		unsigned int gicd_base, index;
		gicd_base = BASE_GICD_BASE;
	    for ( index =0;  index < SEC_EXC_CNT; index++) {
				 gicd_set_icenabler(gicd_base,  sec_exc[index]);
			}
}


void migrate_gic_context(uint32_t secure_state)
{
	unsigned int gicd_base, index;
	//unsigned int val;
	gicd_base = BASE_GICD_BASE;

	if (secure_state == SECURE) {

		disable_ns_exc_intr(gicd_base);
		//enable_s_exc_intr(gicd_base);
		if (TEEI_STATE < TEEI_BUF_READY)
		{
			disable_group(1);
		}
		for ( index =0;  index < SEC_EXC_CNT; index++) {
	//	 val = gicd_read_itargetsr(gicd_base, sec_exc[index]);
		gicd_v3_set_irouter(gicd_base, sec_exc[index],\
									gic_mpidr_to_affinity(read_mpidr()));
		gicd_set_isenabler(gicd_base,  sec_exc[index]);
		}
	} else {
		enable_ns_exc_intr(gicd_base);
		disable_s_exc_intr(gicd_base);
		if (TEEI_STATE < TEEI_BUF_READY)
			enable_group(1);
	}
}

void trigger_soft_intr(unsigned int id)
{
#if 0
	unsigned int val ;
	val = gicd_read_itargetsr(BASE_GICD_BASE,id);
#endif
	gicd_v3_set_irouter(BASE_GICD_BASE, id,gic_mpidr_to_affinity(read_mpidr()));
	gicd_set_ispendr(BASE_GICD_BASE, id);
}
/*
void dump_gic_all_setting()
{
	unsigned int gicd_base, gicc_base;

	gicd_base = BASE_GICD_BASE;
	gicc_base = BASE_GICC_BASE;

    gic_print_all_setting(gicd_base, gicc_base);
}
*/

void sec_exc_add(unsigned int intr_num)
{
	sec_exc[SEC_EXC_CNT] =  intr_num;
	SEC_EXC_CNT ++;
}

void nsec_exc_add(unsigned int intr_num)
{
	nsec_exc[NSEC_EXC_CNT] =  intr_num;
	NSEC_EXC_CNT ++;
}

void teei_init_interrupt()
{
    int i;

	SEC_EXC_CNT = 0;
	NSEC_EXC_CNT = 0;

	teeiBootCfg = (tee_arg_t_ptr)(uintptr_t)TEEI_BOOT_PARAMS;
	teeiKeys =  (tee_keys_t_ptr)(uintptr_t)TEEI_SECURE_PARAMS;

	for (i = 0; i < 5; i++) {
		if (teeiBootCfg->tee_dev[i].dev_type == MT_UART16550){
			uart_apc_num = teeiBootCfg->tee_dev[i].apc_num;
			NSEC_UART =   teeiBootCfg->tee_dev[i].intr_num;
	   }
	   if (teeiBootCfg->tee_dev[i].dev_type == MT_SEC_GPT) {
			SEC_TIMER = teeiBootCfg->tee_dev[i].intr_num;
		}
	}

	//FIXME: Make it can be configured
	NSEC_TIMER =  30;

    /*init secure exlusive array*/
    sec_exc_add(SEC_TIMER);
    sec_exc_add(SEC_APP_INTR);
	sec_exc_add(SEC_DRV_INTR);
	sec_exc_add(SEC_RDRV_INTR);
	//sec_exc_add(SEC_SPI);
    //spec_intr_add(SEC_TP);

    /*init non secure exlusive array*/
    nsec_exc_add(NSEC_TIMER);
    nsec_exc_add(NSEC_UART);

    //DBG_PRINTF("[microtrust]:  uart_apc_num  is %d \n", uart_apc_num);
    //DBG_PRINTF("[microtrust]:  spi_apc_num  is %d \n", spi_apc_num);

}

void teei_gic_setup()
{
	unsigned int gicd_base;

	gicd_base = BASE_GICD_BASE;

	teei_init_interrupt();

    /* Configure  secure interrupts now */
    for ( unsigned int index = 0;  index < SEC_EXC_CNT; index++){
		gicd_clr_igroupr(gicd_base, sec_exc[index]);                                                                        /*  set this interrupt to group 0*/
		gicd_set_ipriorityr(gicd_base,  sec_exc[index], GIC_HIGHEST_SEC_PRIORITY);        /*set this interrupt GIC_HIGHEST_SEC_PRIORITY*/
		/*set itarget to current cpu */
		/* use 1-N model, means as long as one of N core can handle, this will be handled */
		gicd_v3_set_irouter(gicd_base, sec_exc[index], gic_mpidr_to_affinity(read_mpidr()));
		mt_irq_set_sens(gicd_base,  sec_exc[index], MT_EDGE_SENSITIVE);                          /*set gic edge sensitive via GICD_ICFG*/
		mt_irq_set_polarity( sec_exc[index], MT_POLARITY_LOW);                                           /*set low polarity*/
		gicd_set_icenabler(gicd_base,  sec_exc[index]);                                                                 /*disable  this interrupt in gicd*/
    }
}
