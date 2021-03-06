/*
 * hw_timer.c
 *
 *  Created on: 16.03.2015
 *      Author: Marko Petrovic
 */


/*************************************************************************\
 * Registers Definition
\*************************************************************************/

// base addresses

#define DMTIMER0 				(0x44E05000)
#define DMTIMER1_MS				(0x44E31000)
#define DMTIMER2				(0x48040000)
#define DMTIMER3				(0x48042000)
#define DMTIMER4				(0x48044000)
#define DMTIMER5				(0x48046000)
#define DMTIMER6				(0x48048000)
#define DMTIMER7				(0x4804A000)


// register offsets for DMTIMER0, DMTIMER2-7

#define TIDR 					(0x00)						// Identification Register
#define	TIOCP_CFG				(0x10)						// Timer OCP Configuration Register
#define	IRQ_EOI					(0x20)						// Timer IRQ End-of-Interrupt Register
#define IRQSTATUS_RAW			(0x24)						// Timer Status Raw Register
#define IRQSTATUS				(0x28)						// Timer Status Register
#define IRQENABLE_SET			(0x2C)						// Timer Interrupt Enable Set Register
#define IRQENABLE_CLR			(0x30)						// Timer Interrupt Enable Clear Register
#define IRQWAKEEN				(0x34)						// Timer IRQ Wakeup Enable Register
#define TCLR					(0x38)						// Timer Control Register
#define TCRR					(0x3C)						// Timer Counter Register
#define TLDR					(0x40)						// Timer Load Register
#define TTGR					(0x44)						// Timer Trigger Register
#define TWPS					(0x48)						// Timer Write Posting Bits Register
#define TMAR					(0x4C)						// Timer Match Register
#define TCAR1					(0x50)						// Timer Capture Register
#define TSICR					(0x54)						// Timer Synchronous Interface Control Register
#define TCAR2					(0x58)						// Timer Capture Register

// TCLR
#define ENABLE_COMPARE 						0x20
#define DISABLE_COMPARE 					0x00
#define NO_TRIGGER 							0x0
#define OVERFLOW_TRIGGER 					0x400
#define OVERFLOW_MATCH_TRIGGER 				0x800
#define SINGLE_CAPTURE 						0x0
#define CAPTURE_ON_SECOND_EVENT 			0x4000
#define ONE_SHOT_TIMER 						0x0
#define AUTO_RELOAD 						0x02
#define NO_CAPTURE 							0x0
#define CAPTURE_ON_TRANSITION_LOW_TO_HIGH 	0x100
#define CAPTURE_ON_TRANSITION_HIGH_TO_LOW 	0x200
#define CAPTURE_ON_BOTH_EDGE_TRANSITION 	0x300
#define PINMODE_PULSE 						0x0
#define PINMODE_TOGGLE 						0x1000
#define PRESCALER_DISABLE   				~(0x00000020u)
#define PRESCALER_ENABLE   					(0x00000020u)
#define PRESCALER_VALUE_RANGE				(0x1Cu)

// TSICR
#define DMTIMER_TSICR_POSTED   		(0x00000004u)

// TWPS
#define WRITE_PEND_TCLR   	(0x1u)				// When equal to 1, a write is pending to the TCLR register
#define WRITE_PEND_TCRR		(0x2u)				// When equal to 1, a write is pending to the TCRR register
#define WRITE_PEND_TLDR		(0x4u)				// When equal to 1, a write is pending to the TLDR register
#define WRITE_PEND_TTGR		(0x8u)				// When equal to 1, a write is pending to the TTGR register
#define WRITE_PEND_TMAR 	(0x16u)				// When equal to 1, a write is pending to the TMAR register


// TIOCP_CFG
#define TIOCP_CFG_SOFTRESET_DONE   (0x0u)
#define TIOCP_CFG_SOFTRESET   (0x00000001u)

// IRQENABLE_SET
#define IRQ_CAPTURE_ENABLE 			(0x4)
#define IRQ_OVERFLOW_ENABLE			(0x2)
#define IRQ_MATCH_ENABLE			(0x1)

// register offsets for DMTIMER1 (1MS)

#define TIDR_1MS 				(0x00)						// Identification Register
#define	TIOCP_CFG_1MS			(0x10)						// Timer OCP Configuration Register
#define TISTAT_1MS				(0x14)						// status information, excluding interrupt status information
#define	TISR_1MS				(0x18)						// Timer Status Register, determines which timer event req an interrupt
#define	TIER_1MS				(0x1C)						// enable/disable interrupt events
#define TWER_1MS				(0x20)						// enable/disable wakeup feature on spec. interrupts
#define TCLR_1MS				(0x24)						// Timer Control Register
#define TCRR_1MS 				(0x28)						// Timer Counter Register
#define	TLDR_1MS				(0x2C)						// Timer Load Value
#define TTGR_1MS				(0x30)						// Timer Trigger Register
#define	TWPS_1MS				(0x34)						// Timer Write Posting Bits Register
#define	TMAR_1MS				(0x38)						// match value to be compared with counter value
#define TCAR1_1MS				(0x3C)						// value of the first counter capture
#define TSICR_1MS				(0x40)						// imer Synchronous Interface Control Register
#define TCAR2_1MS				(0x44)						// holds the value of the second counter capture
#define	TPIR_1MS				(0x48)						// user for 1ms tick generation. holds positive increment
															// value of this register is added with value of TCVR to define
															// the next value loaded in TCRR will be the sub-period value or the over-period value
#define	TNIR_1MS				(0x4C)						// used for 1ms tick generation.
															// The TNIR register holds the value of the negative increment.
															// The value of this register is added with the value of the
															// TCVR to define whether next value loaded in TCRR will be the
															// sub-period value or the over-period value.
#define TCVR_1MS				(0x50)						// used for 1ms tick generation.
															// The TCVR register defines whether next value loaded in
															// TCRR will be the sub-period value or the over-period value.
#define TSICR_1MS				(0x40)						// imer Synchronous Interface Control Register
#define TOCR_1MS				(0x54)						// used to mask the tick interrupt for a selected number of ticks.
#define TOWR_1MS				(0x58)						// This register holds the number of masked overflow interrupts.






