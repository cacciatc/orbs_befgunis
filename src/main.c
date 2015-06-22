#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>

/* pc headings */
#define UP    1
#define DOWN  2
#define LEFT  3
#define RIGHT 4

#define MAX_STACK_SIZE 80

/* dimensions */
#define PRG_WIDTH  80
#define PRG_HEIGHT 25
#define WORLD_WIDTH  2048
#define WORLD_HEIGHT 2048

/* a point in shared memory */
typedef struct {
	int x, y;
} point;

/* fungal vm internals */
typedef struct {
	point pc;
	point stack;
	
	int heading;
} fungal_vm;

/* linked list node */
typedef struct {
	fungal_vm vm;
	struct vm_node* next;
} vm_node;

char** shared;
vm_node* vms;

/* listen for SIGINT */
void signal_handler(int signum) {
	vm_node* curr_vm;
	vm_node* next;
	
	free(shared);
	
	/* free all vms */
	curr_vm = vms;
	while(curr_vm != NULL) {
		next = (vm_node*) curr_vm->next;
		free(curr_vm);
		
		curr_vm = next;
	}

	exit(signum);
}

/* initializes a new fungal vm */
vm_node* init_interpreter(char** shared, char** prg, point at) {
	vm_node* node = malloc(sizeof(vm_node));

	/* init vm */
	node->vm.stack.x = at.x;
	node->vm.stack.y = at.y;

	node->vm.pc.x = at.x;
	node->vm.pc.y = at.y + 1;
	
	node->vm.heading = RIGHT;

	/* copy prg into shared mem */
	int prg_x = 0, 
			prg_y = 0;

	for(int x = at.x; x < at.x + PRG_WIDTH; x++) {
		for(int y = at.y; y < at.y + PRG_HEIGHT; y++) {
			shared[y][x] = prg[prg_y++][prg_x];
		}
		prg_x++;
	}

	return node;
}

/* move an interpreter one step and return false if now dead */ 
bool step_interpreter(char** shared, fungal_vm vm) {
	char instruction;
	bool is_dead = false;

	/* ensure relative memory access does not seg fault */
	int delta_x = vm.pc.x - vm.stack.x, 
			delta_y = vm.pc.y - vm.stack.y;
	
	if(delta_x > PRG_WIDTH || delta_x < -PRG_WIDTH || delta_y > PRG_HEIGHT || delta_y < -PRG_HEIGHT) {
		return is_dead;
	}

	/* grab and execute next instruction */
	instruction = shared[vm.pc.y][vm.pc.x];

	switch(instruction) {
		case '>':
			vm.heading = RIGHT;
			break;
		case '<':
			vm.heading = LEFT;
			break;
		case '^':
			vm.heading = UP;
			break;
		case 'v':
			vm.heading = DOWN;
			break;
		case ' ':
			/* NOP */
			break;
		default:
			/* unknown instruction, kill it! */
			is_dead = true;
	}
	return is_dead;
}

int main() {
	shared = malloc(sizeof(char)*WORLD_WIDTH*WORLD_HEIGHT);
	
	/* vm variables */	
	vms = NULL;
	vm_node* curr_vm;
	vm_node* prev_vm;

	signal(SIGINT, signal_handler);

	while(true) {
		/* step each interpreter */
		prev_vm = vms;
		curr_vm = vms;

		while(curr_vm != NULL) {
			if(!step_interpreter(shared, curr_vm->vm)) {
				prev_vm->next = curr_vm->next;
				free(curr_vm);
			}
		
			prev_vm = curr_vm;
			curr_vm = (vm_node*) vms->next;
		}
 
		/* add new programs */
		
		/* parse (remove newlines) */
	}

	return 0;
}
