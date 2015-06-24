#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <mqueue.h>
#include <errno.h>
#include <string.h>
#include <math.h>

/* pc headings */
#define UP    1
#define DOWN  2
#define LEFT  3
#define RIGHT 4

#define MAX_STACK_SIZE 48

/* dimensions */
#define PRG_WIDTH  48
#define PRG_HEIGHT 48
#define WORLD_WIDTH  48*48*10
#define WORLD_HEIGHT 48*48*10

/* prg states */
#define DIED 0
#define BORN 1

#define MIN(x, y) (((x) < (y)) ? (x) : (y))

/* a point in shared memory */
typedef struct {
    int x, y;
} point;

/* fungal vm internals */
typedef struct {
    point pc;
    point stack;

    char uuid[16];
    int heading;
} fungal_vm;

/* linked list node */
typedef struct {
    fungal_vm vm;
    struct vm_node* next;
} vm_node;

/* shared RAM */
char shared[WORLD_HEIGHT][WORLD_WIDTH];

/* list of VMs */
vm_node* vms;

/* mqueue */
mqd_t msg_q;
mqd_t msg_i;

struct mq_attr attrs;

/* reads prg and metadata from a char[] */
void read_prg(char buffer[], fungal_vm* vm, char prg[]) {
    memcpy(vm->uuid, buffer, sizeof(char) * 16);
    memcpy(prg, buffer+16, sizeof(char) * PRG_WIDTH * PRG_HEIGHT);
}

/* listen for SIGINT */
void signal_handler(int signum) {
    vm_node* curr_vm;
    vm_node* next;

    mq_close(msg_q);

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
vm_node* init_interpreter(char shared[WORLD_HEIGHT][WORLD_WIDTH], char msg[], point at) {
    vm_node* node = malloc(sizeof(vm_node));
    char prg[PRG_WIDTH*PRG_HEIGHT];

    read_prg(msg, &node->vm, prg);

    /* init vm */
    node->vm.stack.x = at.x;
    node->vm.stack.y = at.y;

    node->vm.pc.x = at.x;
    node->vm.pc.y = at.y;

    node->vm.heading = RIGHT;
    node->next = NULL;

    /* copy prg into shared mem */
    int prg_x = 0,
        prg_y = 0;

    int x, y;
    for(y = at.y; y < at.y + PRG_HEIGHT; y++) {
        for(x = at.x; x < at.x + PRG_WIDTH; x++) {
            shared[x][y] = prg[(prg_y * PRG_WIDTH) + prg_x++];
        }
        prg_y++;
    }

    return node;
}

/* move an interpreter one step and return false if now dead */
bool step_interpreter(char shared[WORLD_HEIGHT][WORLD_WIDTH], fungal_vm* vm) {
    char instruction;
    bool is_alive = true;

    /* ensure relative memory access does not seg fault */
    int delta_x = vm->pc.x - vm->stack.x,
        delta_y = vm->pc.y - vm->stack.y;

    if(delta_x > PRG_WIDTH || delta_x < -PRG_WIDTH || delta_y > PRG_HEIGHT || delta_y < -PRG_HEIGHT) {
        return is_alive = false;
    }

    /* grab and execute next instruction */
    instruction = shared[vm->pc.x][vm->pc.y];

    switch(instruction) {
    case '>':
        vm->heading = RIGHT;
        break;
    case '<':
        vm->heading = LEFT;
        break;
    case '^':
        vm->heading = UP;
        break;
    case 'v':
        vm->heading = DOWN;
        break;
    case ' ':
        /* NOP */
        break;
    default:
        /* unknown instruction, kill it! */
        is_alive = false;
    }

    if(vm->heading == RIGHT) {
        vm->pc.x++;
    }
    else if(vm->heading == LEFT) {
        vm->pc.x--;
    }
    else if(vm->heading == UP) {
        vm->pc.y--;
    }
    else if(vm->heading == DOWN) {
        vm->pc.y++;
    }

    return is_alive;
}

/* searches shared RAM and finds an available location for a new prg */
void find_ram(char shared[WORLD_HEIGHT][WORLD_WIDTH], point* p) {
    // wrap arounds for pcs and init interpreter
    // map interface with real shared memeory  
    // finish instruction set
    int x = rand() % WORLD_WIDTH -  PRG_WIDTH;
    int y = rand() % WORLD_HEIGHT - PRG_HEIGHT;

    int count = 0;
    vm_node* curr_vm = vms;
    bool clean = false;

    p->x = -1;
    p->y = -1;
    while(count < 1000) {
      clean = true;
      while(curr_vm != NULL) {
        if( (x >= curr_vm->vm.stack.x && x <= curr_vm->vm.stack.x + PRG_WIDTH) && (y >= curr_vm->vm.stack.y && y<= curr_vm->vm.stack.y + PRG_HEIGHT)) {
          clean = false;
          break;
        }
        curr_vm = curr_vm->next;
      }

      if(clean) {
        p->x = x;
        p->y = y;
        printf("(%d, %d)\n", x, y);
        return;
      }
      
      x = rand() % WORLD_WIDTH -  PRG_WIDTH;
      y = rand() % WORLD_HEIGHT - PRG_HEIGHT;
    }
}

/* adds a prg given the buffer and a reference to the list of vms */
void add_prg(char msg_buffer[], vm_node* prev_vm) {
  vm_node* new_node;
  point new_point;

  int bytes_to_send = 0;

  new_point.x = -1;
  new_point.y = -1;

  find_ram(shared, &new_point);

  if(new_point.x == -1 || new_point.y == -1) {
    perror("Unable to allocate space for new prg");
  }
  else {
    new_node = (vm_node*) init_interpreter(shared, msg_buffer, new_point);
    
    if(prev_vm == NULL) {
      vms = prev_vm = new_node;
    }
    else {
      prev_vm->next = new_node;
      prev_vm = new_node;
    }

    bytes_to_send = prg_create_msg(&new_node->vm, msg_buffer);
    if(mq_send(msg_i, msg_buffer, bytes_to_send, 0) == -1) {
      perror("Unable to write to message queue responses");
    }
  }
}

/* creates a msg to send out on the responses queue when a prg is created */
int prg_create_msg(fungal_vm* vm, char buffer[]) {
    buffer[0] = BORN;
    memcpy(buffer+1, vm->uuid, sizeof(char)* 16);

    return 17;
}

/* creates a msg to send out on the responses queue when a prg has died */
int prg_died_msg(fungal_vm* vm, char buffer[]) {
    buffer[0] = DIED;
    memcpy(buffer+1, vm->uuid, sizeof(char)* 16);

    return 17;
}

/* creates a msg to send out on the responses queue in response to a prg location query */

int main() {
    /* variables for reading from msg queue */
    int i;
    int msg_priority;
    char msg_buffer[8192];
    int bytes_read;

    int bytes_to_send;

    /* vm variables */
    vms = NULL;
    vm_node* curr_vm  = NULL;
    vm_node* prev_vm  = NULL;
    vm_node* new_node = NULL;

    /* setup mq */
    msg_q = mq_open("/orbs-befungis-requests", O_RDWR | O_CREAT, 0664, NULL);
    if(msg_q == -1) {
        perror("Unable to create message queue for requests");
    }

    msg_i = mq_open("/orbs-befungis-responses", O_RDWR | O_CREAT, 0664, NULL);
    if(msg_i == -1) {
        perror("Unable to create message queue for responses");
    }

    signal(SIGINT, signal_handler);

    while(true) {
        /* step each interpreter */
        prev_vm = NULL;
        curr_vm = vms;

        while(curr_vm != NULL) {
            if(!step_interpreter(shared, &(curr_vm->vm))) {
                /* dealing with the head */
                if(prev_vm == NULL) {
                    vms = NULL;
                }
                else {
                    prev_vm->next = curr_vm->next;
                }

                bytes_to_send = prg_died_msg(&new_node->vm, msg_buffer);
                if(mq_send(msg_i, msg_buffer, bytes_to_send, 0) == -1) {
                    perror("Unable to write to message queue responses");
                }

                free(curr_vm);
                curr_vm = NULL;
            }
            else {
                prev_vm = curr_vm;
                curr_vm = (vm_node*) curr_vm->next;
            }
        }

        /* add new programs */
        mq_getattr(msg_q, &attrs);
        if(attrs.mq_curmsgs != 0) {
            for(i = 0; i < attrs.mq_curmsgs; i++) {
                bytes_read = mq_receive(msg_q, msg_buffer, attrs.mq_msgsize, &msg_priority);

                if(bytes_read > 0) {
                  add_prg(msg_buffer, prev_vm);
                }
            }
        }
    }

    return 0;
}
