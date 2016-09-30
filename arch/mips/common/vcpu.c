/*
Copyright (c) 2016, prpl Foundation

Permission to use, copy, modify, and/or distribute this software for any purpose with or without 
fee is hereby granted, provided that the above copyright notice and this permission notice appear 
in all copies.

THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE
INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE 
FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM 
LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, 
ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.

This code was written by Carlos Moratelli at Embedded System Group (GSE) at PUCRS/Brazil.

*/

/**
 * @file vcpu.c
 * 
 * @section DESCRIPTION
 * 
 * VCPU related function calls (context save, restore and instruction emulation).
 */

#include <vcpu.h> 
#include <types.h>
#include <libc.h>
#include <rw_regs.h>
#include <globals.h>
#include <hal.h>
#include <mips_cp0.h>

/**
 * @brief Save the VCPU context. Saving only the necessary registers for 
 * the supported OSs. 
 */
void contextSave(){
	vcpu_t *vcputosave;
	
	if (!is_vcpu_executing){
		return;
	}
	
	vcputosave = vcpu_executing;
	
	if (vcputosave->init == 0){
		/* already Initialized VCPU - save the context */
		
		/* TODO: Guest timer is not being saved. The guests have
		 * absolute time view. This may not be the correct behavior
		 * for all type of guests. Timer virtualization should be implemented
		 * using the guest offset register.
		 *		
		 *		vcputosave->cp0_registers[9][0] = guestcount;
		 *		vcputosave->rootcount = counter;
		 */
		 
		 /* Save only necessary guest.cp0 registers. All CP0 registers
		  * write by the guests must be saved. 
		  */
		 vcputosave->cp0_registers[4][0] = mfgc0(4,0);	
		 vcputosave->cp0_registers[6][0] = mfgc0(6,0);
		 vcputosave->cp0_registers[8][0] = mfgc0(8,0);
		 vcputosave->cp0_registers[11][0] = mfgc0(11,0);
		 vcputosave->cp0_registers[12][0] = mfgc0(12,0);
		 vcputosave->cp0_registers[12][1] = mfgc0(12,1);	
		 vcputosave->cp0_registers[12][2] = mfgc0(12,2);
		 vcputosave->cp0_registers[12][3] = mfgc0(12,3);
		 vcputosave->cp0_registers[13][0] = mfgc0(13,0);
		 vcputosave->cp0_registers[14][0] = mfgc0(14,0);
		 vcputosave->cp0_registers[14][2] = mfgc0(14,2);
		 vcputosave->cp0_registers[15][0] = mfgc0(15,1);
		 vcputosave->cp0_registers[16][0] = mfgc0(16,0);
		 vcputosave->cp0_registers[17][0] = mfgc0(17,0);
		 vcputosave->cp0_registers[16][3] = mfgc0(16,3);
		 vcputosave->cp0_registers[30][0] = mfgc0(30,0);

		 vcputosave->guestclt2 |= getGuestCTL2();

		 vcputosave->pc = getEPC();
	}
}

/**
 * @brief Determines the Guest Time Offset. No being used in the current implementation.
 * Guests have absolute time view. 
 */
uint32_t calculateGTOffset(uint32_t savedcounter, uint32_t currentCount){
	uint32_t offset;	
	
	/* check for count overflow */
	if(savedcounter > currentCount){
		offset= (0xFFFFFFFF - savedcounter) + currentCount;
	}else{
		offset= currentCount - savedcounter;
	}
	
	/* gtoffset is the two's complement of the time between save and restore the context of a VCPU.*/
	return (~offset) + 1;
}


/**
 * @brief  Restore the VCPU context on context switch. 
 */
void contextRestore(){
	
	vcpu_t *vcpu = vcpu_executing;
	
	setPreviousShadowSet(vcpu->gprshadowset);
	
	setLowestGShadow(vcpu->gprshadowset);	
	setGuestID(vcpu->id);
	
	setGuestMode();
	
	/* Mark the VCPU as initialized. */
	if(vcpu_executing->init){
		vcpu_executing->init = 0;
	}
	
	mtgc0(12, 0, vcpu->cp0_registers[12][0]);
	mtgc0(4, 0, vcpu->cp0_registers[4][0]);
	mtgc0(6, 0, vcpu->cp0_registers[6][0]);
	mtgc0(5, 0, vcpu->cp0_registers[5][0]);
	mtgc0(8, 0, vcpu->cp0_registers[8][0]);
	mtgc0(13, 0, vcpu->cp0_registers[13][0]);
	mtgc0(12, 1, vcpu->cp0_registers[12][1]);
	mtgc0(12, 2, vcpu->cp0_registers[12][2]);
	mtgc0(12, 3, vcpu->cp0_registers[12][3]);
	mtgc0(14, 0, vcpu->cp0_registers[14][0]);
	mtgc0(14, 2, vcpu->cp0_registers[14][2]);	
	mtgc0(15, 1, vcpu->cp0_registers[15][0]);	
	mtgc0(16, 0, vcpu->cp0_registers[16][0]);
	mtgc0(17, 0, vcpu->cp0_registers[17][0]);
	mtgc0(16, 3, vcpu->cp0_registers[16][3]);
	mtgc0(30, 0, vcpu->cp0_registers[30][0]);
	
	setGuestCTL2(vcpu->guestclt2);
	
	setEPC(vcpu->pc);
}


/**
 * @brief  Implement instruction emulation. 
 */
uint32_t InstructionEmulation(uint32_t epc){
	uint32_t badinstr = mfc0(CP0_BADVADDR, 2);
	uint32_t regvalue;
	uint32_t currentValue;
	uint32_t rs, rt, rd;
	uint32_t opcode, flag = 0;
	uint32_t co, func, emulinstr;
	
	opcode = OPCODE(badinstr);	
	rs = RS(badinstr);
	rt = RT(badinstr);
	rd = RD(badinstr);
	co = CO(badinstr);
	func = FUNC(badinstr);
	
	switch(opcode){
		/* COP0 instructions */
		case CP0:
			switch (co){
				case	0x1: 
					switch (func){
						case WAIT: /* wait */
							WARNING("Wait emulation ignored.");
							break;
						default:
							WARNING("Instruction 0x%x at 0x%x not supported on VCPU 0x%x ", badinstr, epc, vcpu_executing->id);
							break;
					}
					break;
				case 	0:
					switch (rs){
						case MTC:
							regvalue = MoveFromPreviousGuestGPR(rt);
							switch (rd){
								case 0x0C:
									switch(SEL(badinstr)){
										case 0:
											currentValue = mfgc0(rd ,0);
											regvalue = (regvalue & STATUS_MASK) | (currentValue & !STATUS_MASK);
											mtgc0(rd, 0, regvalue);
											flag = 1;
											break;
										case 3: /* srsctl */
											WARNING("Write to CP0 SRSCTL ignored");
											break;
										default:
											WARNING("Instruction 0x%x at 0x%x not supported on VCPU 0x%x ", badinstr, epc, vcpu_executing->id);
											break;
									}
								break;
								case 0x09:
									switch(SEL(badinstr)){
										case 0:
											currentValue = mfgc0(0x9, 0);
											currentValue = (~currentValue) + 1;
											setGTOffset(currentValue);
											break;
										default:
											WARNING("Instruction 0x%x at 0x%x not supported on VCPU 0x%x ", badinstr, epc, vcpu_executing->id);
											break;
									}
								break;
								
								default:
									WARNING("Instruction 0x%x at 0x%x not supported on VCPU 0x%x ", badinstr, epc, vcpu_executing->id);
									break;
							}
							break;
						case MFC:
							switch (rd){
								case 0xf: /* read from COP0 15.*/
									regvalue = (mfc0(CP0_PRID,0) & ~0xff00) | 0x8000;
									MoveToPreviousGuestGPR(rt, regvalue);
									break;
								case 0xc: /* read from COP0 12.*/
									switch(SEL(badinstr)){
										case 2:
											regvalue = mfc0(CP0_SRSCTL, 2) & ~SRSCLT_HSS;
											MoveToPreviousGuestGPR(rt, regvalue);
											break;
										default:
											WARNING("Instruction 0x%x at 0x%x not supported on VCPU 0x%x ", badinstr, epc, vcpu_executing->id);
											break;
									}
									break;
								case 0x19: /* read from COP0 25.*/
									regvalue = mfc0(CP0_PERFCTL0, 0);
									MoveToPreviousGuestGPR(rt, regvalue);
									break;
						
								default:
									WARNING("Instruction 0x%x at 0x%x not supported on VCPU 0x%x ", badinstr, epc, vcpu_executing->id);
									break;
							}
							break;
						default:
							WARNING("Instruction 0x%x at 0x%x not supported on VCPU 0x%x ", badinstr, epc, vcpu_executing->id);
							break;
					}
					break;
				default:
					WARNING("Instruction 0x%x at 0x%x not supported on VCPU 0x%x ", badinstr, epc, vcpu_executing->id);
					break;
			}
			break;
			/* Cache instructions */
		case CACHE:
			WARNING("Cache instruction 0x%x at 0x%x not supported on VCPU 0x%x ", badinstr, epc, vcpu_executing->id);
			break;
		default:
			WARNING("Instruction 0x%x at 0x%x not supported on VCPU 0x%x", badinstr, epc, vcpu_executing->id);
			break;
	}

	return 0;
}

