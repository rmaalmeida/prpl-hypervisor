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

/*************************************************************
 * Ping-Pong application - Inter-VM communication.
 * 
 * To execute the Ping-Pong set the CFG_FILE on the main 
 * Makefile to the sample-2VMs.cfg configuration file.  
 * 
 */

#include <arch.h>
#include <libc.h>
#include <network.h>
#include <guest_interrupts.h>
#include <hypercalls.h>
#include <platform.h>


volatile int32_t t2 = 0;

void irq_timer(){
	t2++;     
}


char message_buffer[128];


int main() {
	int32_t ret;
	uint32_t source;
    
	interrupt_register(irq_timer, GUEST_TIMER_INT);
    
	serial_select(UART2);
	
	printf("\nping VM ID %d", get_guestid());
	while (1){
		sprintf(message_buffer, "%s %d", "ping?", t2);
		ret = SendMessage(2, message_buffer, strlen(message_buffer)+1);
		if (ret<0){
			print_net_error(ret);
		}else{
			ret = ReceiveMessage(&source, message_buffer, sizeof(message_buffer), 1);
			if (ret<0){
				print_net_error(ret);
			}else{
				if(ret)
					printf("\nping VM: message from VM ID %d: \"%s\" (%d bytes)", source, message_buffer, ret);
			}
		}
	}
	
	return 0;
}

