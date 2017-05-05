#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "pthread.h"
#include "sys/time.h"
#define MATCH(s) (!strcmp(argv[ac], (s)))
#define PERIOD 3.0

FILE *logSummary;
FILE *logAll;

typedef struct _ticket {
    int seat;
    int tour;
    struct _ticket* next;
} ticket_t;

typedef struct _reserve_t {
    double time; // When ticket is reserved. 0 if reserved slot is available
    int tour;
    int seat;
} reserve_t;

typedef struct _pass_data_t {
    int thrid;
    reserve_t reserveds[2];
    int isRunning;
    ticket_t* tickets;
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

void addTicket(pass_data_t *p, int s, int t){
    ticket_t *newTicket = (ticket_t*) calloc(1,sizeof(ticket_t));
    newTicket->seat = s;
    newTicket->tour = t;
    newTicket->next = (p->tickets);
    p->tickets = newTicket;
}

ticket_t removeTicket(pass_data_t *p){
    ticket_t result;
    result.seat = -1;
    result.tour = -1;
    if(p->tickets != NULL){
        
        result.seat = p->tickets->seat;
        result.tour = p->tickets->tour;
        p->tickets = p->tickets->next;
        
        /*
        ticket_t *newT = NULL;
        newT->next = p->tickets->next;
        newT->seat = p->tickets->seat;
        newT->tour = p->tickets->tour;
        p->tickets = newT;
        */
    }
    return result;

}

int NUM_PASS, NUM_AGENTS;
int NUM_TOURS = 1;
int NUM_SEATS = 10;
int NUM_DAYS = 2;
int SEED = 1;
double START_TIME = 0.0;
int* LOGFLAGS;

pthread_t *PASSENGERS;
pthread_t *AGENTS;
pass_data_t *pass_data;
agent_data_t *agent_data;
pthread_mutex_t *passlocks;
pthread_mutex_t **seatlocks;
int **BUSES; // A seat is accesible via BUSES[tour_id][seat_id]

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
    
    int bus, seat;

    while((get_time() - START_TIME <= NUM_DAYS*PERIOD)){
        if(LOGFLAGS[0]){
            LOGFLAGS[1+thr_data->thrid] = 1;
            int action = rand() % 100;
            int isReserved = 0;
            if(action <= 40){ // RESERVE
                if((pthread_mutex_trylock(&passlocks[thr_data->thrid]) == 0)){
                    for(int slot=0; slot<2; slot++){
                        if(thr_data->reserveds[slot].time == 0.0){
                            bus = rand() % NUM_TOURS;
                            for(int i=0; i< NUM_SEATS && !isReserved; i++){
                                if(pthread_mutex_trylock(&(seatlocks[bus][i])) == 0){
                                    if(BUSES[bus][i] == 0){
                                        BUSES[bus][i] = thr_data->thrid+1;
                                        
                                        thr_data->reserveds[slot].time = get_time();
                                        thr_data->reserveds[slot].tour = bus;
                                        thr_data->reserveds[slot].seat = i;                                        
                                        // LOG RESERVATION ACTION HERE
                                        fprintf(logAll, "%f\t %d\t 0\t R\t %d\t %d\n",get_time()-START_TIME,thr_data->thrid,i,bus+1);
                                        isReserved = 1;
                                        slot = 2; // BREAK THE SLOT LOOP
                                    } 
                                    pthread_mutex_unlock(&(seatlocks[bus][i]));
                                }
                            }
                        }
                    }
                    pthread_mutex_unlock(&passlocks[thr_data->thrid]);
                }
            } else if(action <= 60){ // CANCEL
                int isCancelled = 0;
                for(int i =0; i<2; i++){
                    
                    if((thr_data->reserveds[i].time != 0.0)){ 
                        if(pthread_mutex_trylock(&passlocks[thr_data->thrid]) == 0){
                            thr_data->reserveds[i].time = 0;
                            BUSES[thr_data->reserveds[i].tour][thr_data->reserveds[i].seat] = 0;
                            pthread_mutex_unlock(&passlocks[thr_data->thrid]);
                            isCancelled = 1;
                            fprintf(logAll,"%f\t %d\t 0\t C\t %d\t %d\n",get_time()-START_TIME,thr_data->thrid,thr_data->reserveds[i].seat,thr_data->reserveds[i].tour+1);
                            break;
                        }
                    }
                }
                if(!isCancelled){
                    pthread_mutex_lock(&passlocks[thr_data->thrid]);
                    ticket_t cancelled = removeTicket(thr_data);
                    if(cancelled.tour != -1){
                        //pthread_mutex_lock(&(seatlocks[cancelled.tour][cancelled.seat]));
                        BUSES[cancelled.tour][cancelled.seat] = 0;
                        fprintf(logAll,"%f\t %d\t 0\t C\t %d\t %d\n",get_time()-START_TIME,thr_data->thrid,cancelled.seat,cancelled.tour+1);
                        //pthread_mutex_unlock(&(seatlocks[cancelled.tour][cancelled.seat]));
                    }
                    pthread_mutex_unlock(&passlocks[thr_data->thrid]);
                }

            } else if(action <= 80){ // VIEW
                
                // WHAT TO DO
                int s;
                int t;
                if(thr_data->tickets != NULL){
                    s = thr_data->tickets->seat;
                    t = thr_data->tickets->tour+1;
                }else if(thr_data->reserveds[1].time !=0.0){
                    s = thr_data->reserveds[1].seat;
                    t = thr_data->reserveds[1].tour+1;
                }else if(thr_data->reserveds[0].time !=0.0){
                    s = thr_data->reserveds[0].seat;
                    t = thr_data->reserveds[0].tour+1;
                }else{
                    s = 0;
                    t = 0;
                }
            fprintf(logAll,"%f\t %d\t 0\t V\t %d\t %d\n",get_time()-START_TIME,thr_data->thrid,s,t);
            } else { // BUY
                bus = rand() % NUM_TOURS;
                int isBought = 0;
                for(int i= 0; i< NUM_SEATS && !isBought; i++){
                    if((pthread_mutex_trylock(&seatlocks[bus][i]) == 0)){
                        if(BUSES[bus][i] == 0){
                            BUSES[bus][i] = thr_data->thrid+1;
                            pthread_mutex_lock(&passlocks[thr_data->thrid]);
                            addTicket(thr_data,i,bus);
                            pthread_mutex_unlock(&passlocks[thr_data->thrid]);
                            fprintf(logAll,"%f\t %d\t 0\t B\t %d\t %d\n",get_time()-START_TIME,thr_data->thrid,i,bus+1);
                            isBought = 1;
                        }

                        pthread_mutex_unlock(&seatlocks[bus][i]);
                    }
                }

                if(!isBought){
                    // wait on cond
                }
            }

            LOGFLAGS[1+thr_data->thrid] = 0;
        }
    }
    
    printf("Passenger %d has finished running\n", thr_data->thrid);
}

void *agent_func(void* arg){
    agent_data_t* thr_data = (agent_data_t*) arg;
    printf("Agent %d is running\n", thr_data->thrid);
    int bus, seat, passenger;
    
    while((get_time() - START_TIME <= NUM_DAYS*PERIOD)){
        if(LOGFLAGS[0]){
            passenger = rand() % NUM_PASS;
            LOGFLAGS[1+NUM_PASS+thr_data->thrid] = 1;
            int action = rand() % 100;
            int isReserved = 0;
            if(action <= 40){ // RESERVE
                if((pthread_mutex_trylock(&passlocks[passenger]) == 0)){
                    for(int slot=0; slot<2; slot++){
                        if(pass_data[passenger].reserveds[slot].time == 0.0){
                            bus = rand() % NUM_TOURS;
                            for(int i=0; i< NUM_SEATS && !isReserved; i++){
                                if(pthread_mutex_trylock(&(seatlocks[bus][i])) == 0){
                                    if(BUSES[bus][i] == 0){
                                        BUSES[bus][i] = passenger+1;
                                        
                                        pass_data[passenger].reserveds[slot].time = get_time();
                                        pass_data[passenger].reserveds[slot].tour = bus;
                                        pass_data[passenger].reserveds[slot].seat = i;                                        
                                        // LOG RESERVATION ACTION HERE
                                        fprintf(logAll, "%f\t %d\t %d\t R\t %d\t %d\n",get_time()-START_TIME,passenger+1,thr_data->thrid,i,bus+1);
                                        isReserved = 1;
                                        slot = 2; // BREAK THE SLOT LOOP
                                    } 
                                    pthread_mutex_unlock(&(seatlocks[bus][i]));
                                }
                            }
                        }
                    }
                    pthread_mutex_unlock(&passlocks[passenger]);
                }
            } else if(action <= 60){ // CANCEL
                int isCancelled = 0;
                for(int i =0; i<2; i++){
                    
                    if((pass_data[passenger].reserveds[i].time != 0.0)){ 
                        if(pthread_mutex_trylock(&passlocks[passenger]) == 0){
                            pass_data[passenger].reserveds[i].time = 0;
                            BUSES[pass_data[passenger].reserveds[i].tour][pass_data[passenger].reserveds[i].seat] = 0;
                            pthread_mutex_unlock(&passlocks[passenger]);
                            isCancelled = 1;
                            fprintf(logAll,"%f\t %d\t %d\t C\t %d\t %d\n",get_time()-START_TIME,passenger+1,thr_data->thrid,pass_data[passenger].reserveds[i].seat,pass_data[passenger].reserveds[i].tour+1);
                            break;
                        }
                    }
                }
                if(!isCancelled){
                    pthread_mutex_lock(&passlocks[passenger]);
                    ticket_t cancelled = removeTicket(&pass_data[passenger]);
                    if(cancelled.tour != -1){
                        //pthread_mutex_lock(&(seatlocks[cancelled.tour][cancelled.seat]));
                        BUSES[cancelled.tour][cancelled.seat] = 0;
                        fprintf(logAll,"%f\t %d\t %d\t C\t %d\t %d\n",get_time()-START_TIME,passenger+1,thr_data->thrid,cancelled.seat,cancelled.tour+1);
                        //pthread_mutex_unlock(&(seatlocks[cancelled.tour][cancelled.seat]));
                    }
                    pthread_mutex_unlock(&passlocks[passenger]);
                }

            } else if(action <= 80){ // VIEW
                
                // WHAT TO DO
                int s;
                int t;
                if(pass_data[passenger].tickets != NULL){
                    s = pass_data[passenger].tickets->seat;
                    t = pass_data[passenger].tickets->tour+1;
                }else if(pass_data[passenger].reserveds[1].time !=0.0){
                    s = pass_data[passenger].reserveds[1].seat;
                    t = pass_data[passenger].reserveds[1].tour+1;
                }else if(pass_data[passenger].reserveds[0].time !=0.0){
                    s = pass_data[passenger].reserveds[0].seat;
                    t = pass_data[passenger].reserveds[0].tour+1;
                }else{
                    s = 0;
                    t = 0;
                }
            fprintf(logAll,"%f\t %d\t %d\t V\t %d\t %d\n",get_time()-START_TIME,passenger+1,thr_data->thrid,s,t);
            } else { // BUY
                bus = rand() % NUM_TOURS;
                int isBought = 0;
                for(int i= 0; i< NUM_SEATS && !isBought; i++){
                    if((pthread_mutex_trylock(&seatlocks[bus][i]) == 0)){
                        if(BUSES[bus][i] == 0){
                            BUSES[bus][i] = passenger+1;
                            pthread_mutex_lock(&passlocks[passenger]);
                            addTicket(&pass_data[passenger],i,bus);
                            pthread_mutex_unlock(&passlocks[passenger]);
                            fprintf(logAll,"%f\t %d\t %d\t B\t %d\t %d\n",get_time()-START_TIME,passenger+1,thr_data->thrid,i,bus+1);
                            isBought = 1;
                        }

                        pthread_mutex_unlock(&seatlocks[bus][i]);
                    }
                }
            }

            LOGFLAGS[1+NUM_PASS+thr_data->thrid] = 0;
        }
    }
}

int main(int argc, char** argv){
    logSummary = fopen("summaryLog.txt", "w");
    logAll = fopen("allLog.txt","w");
    fprintf(logAll,"TIME\t P_ID\t A_ID\t Operation\t Seat No\t Tour No\n");
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

    logSummary = fopen("summaryLog.txt", "w");
    logAll = fopen("allLog.txt","w");

    LOGFLAGS = calloc(1+NUM_PASS+NUM_AGENTS, sizeof(int));
    for(int i=0; i< 1+NUM_PASS+NUM_AGENTS; i++){
        LOGFLAGS[i] = 1;
    }

    PASSENGERS = (pthread_t*) calloc(NUM_PASS,sizeof(pthread_t));
    AGENTS = (pthread_t*) calloc(NUM_AGENTS,sizeof(pthread_t));
    pass_data = (pass_data_t*) calloc(NUM_PASS,sizeof(pass_data_t));
    agent_data = (agent_data_t*) calloc(NUM_AGENTS,sizeof(agent_data_t));

    BUSES = (int**) calloc(NUM_TOURS,sizeof(int*));
    for(int i =0; i< NUM_TOURS; i++){
        BUSES[i] = (int*) calloc(NUM_SEATS,sizeof(int));
    }

    passlocks = (pthread_mutex_t*) calloc(NUM_PASS,sizeof(pthread_mutex_t));
    seatlocks = (pthread_mutex_t**) calloc(NUM_TOURS,sizeof(pthread_mutex_t*));
    for(int i =0; i< NUM_TOURS; i++){
        seatlocks[i] = (pthread_mutex_t*) calloc(NUM_SEATS,sizeof(pthread_mutex_t));
    }

    for(int i=0; i< NUM_PASS; i++){
        pthread_mutex_init(&(passlocks[i]), NULL);
    }

    for(int i=0; i< NUM_TOURS; i++){
        for(int j=0; j< NUM_SEATS; j++){
            pthread_mutex_init(&(seatlocks[i][j]), NULL);
        }
    }


    int current_day = 1;
    START_TIME = get_time();

    for(int i=0; i<NUM_PASS;i++){
        pass_data[i].thrid = i;
        pthread_create(&PASSENGERS[i], NULL, pass_func, &pass_data[i]);
        printf("The passenger %d is created.\n",i+1);
    }

    for(int i=0; i<NUM_AGENTS;i++){
        agent_data[i].thrid = i;
        pthread_create(&AGENTS[i], NULL, agent_func, &agent_data[i]);
        printf("The agent %d is created.\n",i+1);
    }

    // SIMULATION
    while(current_day <= NUM_DAYS){
        fprintf(logSummary,"----------DAY %d----------\n",current_day);
        while(get_time() - START_TIME <= current_day*PERIOD){
            // Cancel invalid reserved tickets
            for(int i = 0; i < NUM_PASS; i++){
                for(int j = 0; j < 2; j++){
                    if(pass_data[i].reserveds[j].time - get_time() > PERIOD){
                        pthread_mutex_lock(&(passlocks[i]));
                        reserve_t res = pass_data[i].reserveds[j];
                        pass_data[i].reserveds[j].time = 0;
                        BUSES[res.tour][res.seat] = 0;
                        pthread_mutex_unlock(&(passlocks[i]));

                        fprintf(logAll,"Reservation time is up. The ticket is cancelled. Passenger: %d Tour: %d, Seat: %d\n",pass_data[i].thrid,res.tour,res.seat);
                        fprintf(logAll,"The seat %d in tour %d is now empty.\n",res.seat,res.tour);
                    }
                }
            }
        }

        // LOG DAY SUMMARY
        LOGFLAGS[0] = 0;
        while(1){
            int stopped = 0;
            for(int i = 1; i< 1+NUM_PASS+NUM_AGENTS;i++){
                stopped = stopped || LOGFLAGS[i];
            }
            if(!stopped){
                break;
            }
        }
        
        fprintf(logAll,"-------END OF DAY %d------\n",current_day);
        fprintf(logSummary,"Reserved Seats:\n");
        for(int i=0; i<NUM_TOURS; i++){
            fprintf(logSummary,"Tour %d:",i+1);
            for(int j=0; j<NUM_PASS; j++){
                reserve_t res1 = pass_data[j].reserveds[0];
                if((res1.time != 0) && (res1.tour==i)){
                    fprintf(logSummary," (%d_%d)",j+1,res1.seat);
                }
                reserve_t res2 = pass_data[j].reserveds[1];
                if((res2.time != 0) && res2.tour==i){
                    fprintf(logSummary," (%d_%d)",j+1,res2.seat);
                }
            }
            fprintf(logSummary,"\n");
        }
        fprintf(logSummary,"Bought Seats:\n");
        for(int i=0; i<NUM_TOURS; i++){
            fprintf(logSummary,"Tour %d:",i+1);
            for(int j=0; j<NUM_PASS; j++){

                ticket_t* ticks = pass_data[j].tickets;
                while(ticks!=NULL){
                    if(ticks->tour==i){
                        fprintf(logSummary," (%d_%d)",j+1,ticks->seat);
                    }
                    ticks = ticks->next;
                }
                
            }
            fprintf(logSummary,"\n");
        }
        fprintf(logSummary,"Wait List (Passenger ID)\n");
        int flag = 1;
        fprintf(logSummary,"Passengers:");
        for(int i=0; i<NUM_PASS; i++){
            if(!(pass_data[i].isRunning)){
                fprintf(logSummary," %d",pass_data[i].thrid);
                flag = 0;
            }
        }
        if(flag){
            fprintf(logSummary," no passenger is waiting.");
        }
        fprintf(logSummary,"\n");
        fprintf(logSummary,"-------END OF DAY %d------\n",current_day);
        LOGFLAGS[0] = 1;
        current_day++;
    }


    for(int i=0; i<NUM_PASS;i++){
        pthread_join(PASSENGERS[i], NULL);
    }

    for(int i=0; i<NUM_AGENTS;i++){
        pthread_join(AGENTS[i], NULL);
    }

    fclose(logSummary);
    fclose(logAll);

    return 0;
}
