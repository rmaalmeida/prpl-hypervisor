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

This code was written by Sergio Johann at Embedded System Group (GSE) at PUCRS/Brazil.

*/

#include <os.h>

#define N 13

sem_t empty, full;
sem_t m;

int32_t in=0, out=0, buffer[N];

void producer(void){
	int32_t item;

	while(1){
		item=random();
		os_semwait(&empty);
		os_semwait(&m);
			buffer[in]=item;
			printf("\nproducer %d putting at %d", os_selfid(), in);
			in=(in+1)%N;
		os_sempost(&m);
		os_sempost(&full);
	}
}

void consumer(void){
	int32_t item;

	while(1){
		os_semwait(&full);
		os_semwait(&m);
			item=buffer[out];
			printf("\nconsumer %d getting from %d", os_selfid(), out);
			out=(out+1)%N;
		os_sempost(&m);
		os_sempost(&empty);
		item=0;
	}
}

void app_main(void){
	os_seminit(&empty,N);
	os_seminit(&full,0);
	os_seminit(&m,1);

	os_spawn(producer, "Producer A", 1024);
	os_spawn(producer, "Producer B", 1024);
	os_spawn(producer, "Producer C", 1024);
	os_spawn(consumer, "Consumer A", 1024);
	os_spawn(consumer, "Consumer B", 1024);
	os_spawn(consumer, "Consumer C", 1024);
}