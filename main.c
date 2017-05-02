#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "pthread.h"
#include "sys/time.h"
#define MATCH(s) (!strcmp(argv[ac], (s)))
#define PERIOD 5.0

int NUM_PASS, NUM_AGENTS;
int NUM_TOURS = 1;
int NUM_SEATS = 10;
int NUM_DAYS = 4;
int SEED = 1;
double START_TIME = 0.0;

typedef struct _ticket {
    int seat;
    int tour;
    struct _ticket* next;
} ticket_t;

void addTicket(pass_data_t *p, int s, int t){
    ticket_t *newTicket = (ticket_t*) malloc(sizeof(ticket_t));
    newTicket->seat = s;
    newTicket->tour = t;
    newTicket->next = p->tickets;
    p->tickets = newTicket;
}

void removeTicket(pass_data_t *p){
    if(p->tickets != NULL){
        ticket_t *newT = NULL;
        newT->next = p->tickets->next;
        newT->seat = p->tickets->seat;
        newT->tour = p->tickets->tour;
        p->tickets = newT;
    }
}

typedef struct _reserve_t {
    double time; // When ticket is reserved. 0 if reserved slot is available
    int tour;
    int seat;
} reserve_t;

typedef struct _pass_data_t {
    int thrid;
    reserve_t reserveds[2];
    bool isRunning;
    ticket_t *tickets;
    /*
    * Add important data of threads
    */
} pass_data_t;

typedef struct _agent_data_t {
    int thrid;
    
    /*
    * Add important data of threads
    */
} agent_data_t;

static const double kMicro = 1.0e-6;
double get_time() {
    struct timeval TV;
    struct timezone TZ;
    const int RC = gettimeofday(&TV, &TZ);
    if(RC == -1) {
        printf("ERROR: Bad call to gettimeofday\n");
        return(-1);
    }
    return( ((double)TV.tv_sec) + kMicro * ((double)TV.tv_usec) );
}

void *pass_func(void* arg){
    pass_data_t* thr_data = (pass_data_t*) arg;
    int action = rand() % 100;
    int bus = rand() % NUM_TOURS;

    while(get_time() - START_TIME <= NUM_DAYS*PERIOD){
        if(action <= 40){ // RESERVE
            
        } else if(action <= 60){ // CANCEL
            for(int i =0; i<2; i++){
                if(!(thr_data->reserveds[i])){
                    thr_data->reserveds[i].time = 0;
                    BUSES[thr_data->reserveds[i].tour][thr_data->reserveds[i].seat] = 0;
                    break;
                }
            }
        } else if(action <= 80){ // VIEW
            // WHAT TO DO
        } else { // BUY

        }
    }
    
    printf("Passenger %d has finished running\n", thr_data->thrid);
    exit(0);
}

void *agent_func(void* arg){
    agent_data_t* thr_data = (agent_data_t*) arg;

    if(action <= 40){

    } else if(action <= 60){

    } else if(action <= 80){
        
    } else {
        
    }

    printf("Agent %d is running\n", thr_data->thrid);
    exit(0);
}

int main(int argc, char** argv){

    if(argc<3) {
	  printf("Usage: %s [-p < Passengers >] [-a < Agents >] [-t < Tours default=1 >] [-s < Seats default=10 >] [-d < Days default=4 >] [-r < Seed default=1>]\n",argv[0]);
	  return(-1);
	}

    // -r is needed
	for(int ac=1;ac<argc;ac++) {
		if(MATCH("-p")) {
			NUM_PASS = atoi(argv[++ac]);
		} else if(MATCH("-a")) {
			NUM_AGENTS = atoi(argv[++ac]);
		} else if(MATCH("-t")) {
			NUM_TOURS = atoi(argv[++ac]);
		} else if(MATCH("-s")) {
			NUM_SEATS = atoi(argv[++ac]);
		} else if(MATCH("-d")) {
			NUM_DAYS = atoi(argv[++ac]);
		} else if(MATCH("-r")) {
			SEED = atoi(argv[++ac]);
		} else {
		printf("Usage: %s [-p < Passengers >] [-a < Agents >] [-t < Tours default=1 >] [-s < Seats default=10 >] [-d < Days default=4 >] [-r < Seed default=1>]\n",argv[0]);
	    return(-1);
		}
	}

    srand((unsigned) SEED);

    pthread_t PASSENGERS[NUM_PASS];
    pthread_t AGENTS[NUM_AGENTS];
    pass_data_t pass_data[NUM_PASS];
    agent_data_t agent_data[NUM_AGENTS];

    // A seat is accesible via BUSES[tour_id][seat_id]
    int BUSES[NUM_TOURS][NUM_SEATS];
    int current_day = 1;
    START_TIME = get_time();

    for(int i=1; i<=NUM_PASS;i++){
        pass_data[i].thrid = i;
        pthread_create(&PASSENGERS[i], NULL, pass_func, &pass_data[i]);
    }

    for(int i=1; i<=NUM_AGENTS;i++){
        agent_data[i].thrid = i;
        pthread_create(&AGENTS[i], NULL, agent_func, &agent_data[i]);
    }

    // SIMULATION
    while(current_day <= NUM_DAYS){
        while(get_time() - START_TIME <= current_day*PERIOD){
            // Cancle invalid reserved tickets
            for(int i = 0; i < NUM_PASSENGERS){
                for(int j = 0; j < 2; j++){
                    if(pass_data[i].reserveds[j].time - get_time() > PERIOD){
                        reserved_t res = pass_data[i].reserveds[j];
                        res.time = 0;
                        BUSES[res.tour][res.seat] = 0;
                        /*
                        res.tour = 0;
                        res.ticket = 0;
                        */
                    }
                }
            }
        }

        // LOG DAY SUMMARY






        current_day++;
    }


    for(int i=0; i<NUM_PASS;i++){
        pthread_join(PASSENGERS[i], NULL);
    }

    for(int i=0; i<NUM_AGENTS;i++){
        pthread_join(AGENTS[i], NULL);
    }

    return 0;
}
