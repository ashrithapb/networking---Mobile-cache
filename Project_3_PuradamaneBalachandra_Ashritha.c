/*Project #3; Puradamane Balachandra, Ashritha */

#include "csim.h"
#include <stdio.h>

#define NUM_CLIENTS 100L
#define NUM_SERVER 1L
#define T_DELAY 0.8// (data_size/bandwidth) = 1024*8/10000
#define REQUEST 1L
#define REPLY 2L
#define Query_dealy 10 // output is taken when value is 10 or 20 or 25 or 30 or 50
#define CACHE_SIZE 10
#define SERVER_NODE 100
#define L 20.0

typedef struct msg *msg_t;

struct msg
{

        int flag;
        long from;
        long to;
        long type;
        long data_id;
        float start_time;
        long updated_time;
        msg_t link;
};

typedef struct invalidationReport *ir;

struct invalidationReport{
        long current_timestamp;
        long data_item_id[1000];
        long last_update_time[1000];
        long from;
        long to;
        long start_time;
        ir link;
};

msg_t msg_queue;
ir ir_queue;

struct nde
{
    FACILITY cpu;
    MBOX input;
};

struct cache
{
        long valid_bit[CACHE_SIZE];
        long data_item_id[CACHE_SIZE];
        long last_update_time[CACHE_SIZE];
        long last_accessed_time[CACHE_SIZE];
};

struct nde node[NUM_CLIENTS+NUM_SERVER];
struct cache c[NUM_CLIENTS];
TABLE resp_tm;
FILE *fp;
EVENT ir_event;
EVENT server_sent_msg;
EVENT server_receive;

msg_t new_msg();
void init();
void server();
void client();
void update();
void Recv();
void IR();
void query();
void RecvIR();
void form_reply();
void broadcast();
ir generate_IR();
void send_IR();
void send_msg();
void send_query_msg();
void RecvMsg();
void results();

long mean_update_value = 0;
long mean_query_generate_time = 0;
long simulationTime = 0.0;
long a[1000];
long data_id;

long query_generated_count= 0;
long cache_hit_count = 0;
float cache_hit_ratio = 0;
float query_delay = 0;
long number_of_queries_served = 0;

void sim()
{
        long i;
        create("sim");
        ir_event = event("ir_event");
        server_sent_msg = event("server_sent_msg");
        server_receive = event("server_receive");
        init();
        printf("\n Enter the mean update value(50 to 300): ");
        scanf("%ld", &mean_update_value);
        printf("\n Enter the mean query generate time(25 to 300): ");
        scanf("%ld", &mean_query_generate_time);
        simulationTime = (CACHE_SIZE * Query_dealy * 10);
        for(i = 0; i< 1000; i ++){
                a[i]= 0;
        }

        client();
        server();
        hold(simulationTime);
        results();
}

void results(){
        float avg_query_delay;
        long avg_number_of_queries_served;
        printf("\n query_generated_count: %ld, cache_hit_count: %ld\n",query_generated_count,cache_hit_count);
        cache_hit_ratio = (float)cache_hit_count/query_generated_count;
        printf("\n cache hit ratio is %6.3f\n",cache_hit_ratio);
        avg_query_delay = (float)(query_delay/(query_generated_count * NUM_CLIENTS));
        printf("\n average query delay is %6.3f\n",avg_query_delay);
        int total_intervals = simulationTime/L;
        avg_number_of_queries_served = number_of_queries_served/(total_intervals);
        printf("\n Average number of queries per IR interval is %ld\n",number_of_queries_served);
}


void init()
{
        long i, j;
        char str[24];
        long NUM_NODES;
        long x;
        NUM_NODES = NUM_CLIENTS+NUM_SERVER;
        max_facilities(NUM_NODES * NUM_NODES + NUM_NODES);
        max_servers(NUM_NODES * NUM_NODES + NUM_NODES);
        max_mailboxes(NUM_NODES) ;
        max_events(2 * NUM_NODES * NUM_NODES);
        resp_tm = table("message response time");
        ir_queue = NIL;
        for (i = 0; i < NUM_NODES; i++)
        {
                sprintf(str, "cpu.%ld", i);
                node[i].cpu = facility(str);
                sprintf(str, "input.%ld", i);
                node[i].input = mailbox(str);
        }

        for (i = 0; i < NUM_NODES; i++)
                for (j = 0; j < NUM_NODES; j++)
                {
                        sprintf(str, "nt%ld.%ld", i, j);
 //                     network[i][j] = facility(str);
                }
}


void server(){
        create("serverprocess");
        update();
        IR();
        Recv();
        while (clock < simulationTime){
                hold(10.0);
        }
}

void update(){
        long x;
        double data_update_prob;
        long server_node = SERVER_NODE; //change to 100
        create("updateproc");
        while (clock < simulationTime){
                hold(mean_update_value);
                data_update_prob = uniform(0.0, 1.0);
                printf("\n data update probability at server is %lf\n",data_update_prob);
                if(data_update_prob < 0.33){
                        data_id = uniform(0,49);
                        printf("\n Sever updated the hot data item with id %ld at time: %6.3f \n",data_id,clock);
                }else{
                        data_id =  uniform(50,999);
                        printf("\n Sever updated the cold data item with id %ld at time: %6.3f \n",data_id,clock);
                }
               x =(long) clock;
               a[data_id] = x;
     }

}

void IR(){
        create("irproc");
        while (clock < simulationTime){
                hold(L);
                broadcast();
 //               printf("IR broadcasted from server to clients at time: %6.3f\n",clock);
      }
}
void broadcast(){
        long i;
        ir new_ir;
        long j = SERVER_NODE; // change it to 100
        new_ir = generate_IR(j);
        for(i=0; i < NUM_CLIENTS;i++){
                send_IR(new_ir,i);
        }
        printf("\n Broadcasted IR from server node to all clients at %6.3f seconds\n",clock);
}


ir generate_IR(from)
long from;
{
        ir iReport;
        long i;
        long j;
        long clock_time;
        long z;
        if (ir_queue == NIL)
        {
                iReport = (ir)do_malloc(sizeof(struct invalidationReport));
        }
        else
        {
                iReport = ir_queue;
                ir_queue = ir_queue->link;
        }
        iReport->from = from;
        iReport->start_time = clock;
        iReport->current_timestamp = simtime();
        for(j = 0; j<1000;j++){
                 if((a[j] != 0 )){
                        printf("\n Generated IR contains: the following id(s) sending at %6.3f seconds :\n",clock);
                        break;
                }
        }
        for(j = 0; j<1000;j++){
                iReport->data_item_id[j] = 0;
              iReport->last_update_time[j] = 0.0;
                if((a[j] != 0 )){
                        if(iReport->current_timestamp < a[j]+200){   //w*L= 200
        //                      printf("\n current time:%ld, update time com:%ld \n",iReport->current_timestamp,z );
                                iReport->data_item_id[j] = j;
                                iReport->last_update_time[j] =a[j];
                                printf(" %ld,",j);
                        }
                }
        }
        printf("\n");
        return (iReport);
}
void send_IR(new_ir,to)
        ir new_ir;
        long to;
{
        long  j;
        new_ir->to = to;
        long from ;
        long r;
        from = new_ir->from;
        to = new_ir->to;
//  reserve(network[from][to]);
//      hold(TRANS_TIME);
        send(node[to].input,(long)new_ir);
     // printf("sent IR  from %ld to %ld at %6.3f seconds\n",new_ir->from,new_ir->to,clock);
//status_mailboxes();
//  release(network[from][to]);

}
void Recv(){
        msg_t m;
        float s = 0, t = 0;
        long i;
        long lbcast[20];
        long j;
        create("serverrecvproc");
        while (clock < simulationTime){
                if((s-t) < mean_query_generate_time && (s-t) >0){
                        hold(mean_query_generate_time - (s-t));
                }else{
                        hold(mean_query_generate_time);
                }
                receive(node[SERVER_NODE].input, (long *)&m); // change to 100
                printf("\n Server recieved the requested query id:%ld at %6.3fseconds\n",m->data_id,clock);
                if(m->type == 1){
                        t = (float)simtime();
                        wait(ir_event);
                        s = (float)simtime();
                               form_reply(m);
                               printf("\n Server broadcasted the requested data item to clients\n",m->data_id);
                                for(i=0; i < NUM_CLIENTS;i++){
                                send_msg(m,i);
                                }
                        set(server_sent_msg);
                }
        }

}

void form_reply(m)
    msg_t m;
{
        long from, to;
        to = m->to;
        from = m->from;
        m->from = to;
//      m->to = from;
        m->type = REPLY;
//      printf("node.%ld recieved message from node.%ld at %3.3f seconds \n", m->from, m->to, clock);
        use(node[from].cpu, 0);
}

void client(){
        create("clientprocess");
        RecvIR();
        query();
        RecvMsg();
        while (clock < simulationTime){

               hold(10.0);
        }
}

void RecvIR(){
        ir new_ir;
        long l;
        long j;
        long x;
        long i, index;
        long updated_data_present;
        create("clientrecvirproc");
        hold(T_DELAY);
        while (clock < simulationTime){
                hold(L);
                printf("\n Broadcasted IR is received by clients at %6.3f seconds\n",clock);
                for(l = 0; l< NUM_CLIENTS; l++){
        //                printf("\n IR recieved at node :%ld at %6.3f seconds\n",l,clock);
                        receive(node[l].input, (long *)&new_ir);
                        set(ir_event);
                       for(j = 0; j<1000;j++){
                                if(new_ir->data_item_id[j] != 0 && (new_ir->last_update_time[j] != 0) ){
                                        updated_data_present = 0;
                                        for(i = 0 ; i < CACHE_SIZE ; i++){
                                                if(c[l].data_item_id[i] == new_ir->data_item_id[j]){
                                                         updated_data_present = 1;
                                                        index = i;
                                                        break;
                                                }else{
                                                         updated_data_present = 0;
                                                }
                                        }
                                        if(updated_data_present == 1){
                                        if(new_ir->last_update_time[j] > c[l].last_update_time[index]){
                                                if(c[l].data_item_id[index] == new_ir->data_item_id[j]){
                                                        c[l].valid_bit[index] = 0;
                              //                          printf("\n Existing cache is invalid for data item with id %ld: %ld at %6.3f seconds\n",c[l].data_item_id[i],new_ir->data_item_id[j],clock);
                                                        break;
                                                }
                                        } }
                                }
                        }
                }
        clear(ir_event);
        }
}

void query(){
        long k;
        float hot_data_access_probability;
        msg_t m;
        float s = 0,tm= 0;
        long t;
        long i;
        long y = SERVER_NODE;  // change it to 100
        long cache_flag;
        create("clientqueryproc");
        while (clock < simulationTime){
                if((s-tm) < mean_query_generate_time && (s-tm) >0 && (s-tm) < 20){
                        hold(mean_query_generate_time - (s-tm));
                }else{
                        hold(mean_query_generate_time);
                }
                s= 0; tm= 0;
                k = random(0,(NUM_CLIENTS -1));                                //change to 0,99
                hot_data_access_probability = uniform(0,1);
                if(hot_data_access_probability < 0.8){
                        data_id = random(0,49);
                }else{
                        data_id = random(50,999);
                }
                query_generated_count++;
                printf("\n Query generated at client side for the id:%ld at %6.3f seconds\n",data_id,clock);
                m = new_msg(k,data_id);
                for(i = 0; i < CACHE_SIZE ; i++){
                        if(c[k].data_item_id[i] == data_id){
                                cache_flag = 1;
                                break;
                        }else{
                                cache_flag = 0;
                        }
                }
                if(cache_flag == 0){
                        printf("\n data item is not cached and hence query request is sending to server at %6.3f seconds\n",clock);
                        send_msg(m,y);
                }else{
                        cache_hit_count++;
                //      printf("\n cache hit count is %ld\n",cache_hit_count);
                        tm = (float)simtime();
                        wait(ir_event);
                        s = (float)simtime();
                        for(i = 0; i < CACHE_SIZE ; i++){
                                if(c[k].data_item_id[i] == data_id){
                                        if(c[k].valid_bit[i] == 0){
                               //                 printf("\n client cache for data item id:%ld is invalid and hence request query is sending to server at %6.3f seconds\n",data_id,clock);   
                                send_msg(m,y);
                                        }else{
                                        //        send_msg(m,y);
                                                t = simtime();
                                                number_of_queries_served++;
                                                float cur_time = (float)simtime();
                                                query_delay =  query_delay + cur_time - m->start_time;
                                //              printf("\n aaaaaaaaaaaaaaaa query delay is %6.3f\n",query_delay);
                                                c[k].last_accessed_time[i] = t;
                                                printf("\n data item id:%ld is valid in cache and hence it is accessed by the client node %ld at %6.3f seconds\n",data_id,k,clock);
                                        }
                                }

                        }
                }
        }
}

msg_t new_msg(from, data_id)
long from; long data_id;
{
    msg_t m;
    if (msg_queue == NIL)
    {
        m = (msg_t)do_malloc(sizeof(struct msg));
    }
    else
    {
        m = msg_queue;
        msg_queue = msg_queue->link;
   }
  //  m->to = 100;
//    m->flag = 0;
    m->from = from;
        m->data_id = data_id;
        m->type = REQUEST;
    m->start_time = (float)simtime();
//printf("new msg generated\n");
   return(m);
}

void send_msg(m,i)
 msg_t m; long i;
{
        long from, to;
        long r;
        from = m->from;
        m->to = i;
        to = m->to;
  //    reserve(network[from][to]);
 //     hold(TRANS_TIME);
send(node[to].input, (long)m);
//        printf("msg sent id:%ld, from %ld to %ld at %6.3f seconds\n",m->data_id,m->from,m->to,clock);
//      release(network[from][to]);
}
void RecvMsg(){
        long l;
        msg_t m;
        long y;
        long i;
        long cache_has;
        long oldest;
        long index;
        long is_cache_full;
        long all_valid_bits;
        long oldest_valid;
        float s, t;
        long reset = 0;
        create("clientRecvMsgproc");
        while (clock < simulationTime){
                 if((s-t) < mean_query_generate_time && (s-t) >0){
                        hold(mean_query_generate_time - (s-t));
                }else{
                        hold(mean_query_generate_time);
                }
                s= 0;t = 0;
        //      hold(mean_query_generate_time);
//              wait(ir_event);
                t = (float)simtime();
                wait(server_sent_msg);
                s = (float)simtime();
                number_of_queries_served++;
                long print_sts = 0;
                for(l = 0; l< NUM_CLIENTS; l++){
                        cache_has = 0;
                        receive(node[l].input, (long *)&m);
                        if(print_sts == 0){
                                printf("\n Requested data item id:%ld is received by the all clients at %6.3f seconds\n",m->data_id,clock);
                                print_sts = 1;
                        }
                        data_id = m->data_id;
                        float cur_time = (float)simtime();
                        query_delay = query_delay + cur_time - m->start_time;
                        for(i = 0; i < CACHE_SIZE ; i++){
                                if(c[l].data_item_id[i] == data_id){
                                        cache_has = 1;
                                        index = i;
                                        break;
                                }else{
                                        cache_has = 0;
                                }
                        }
                        if(cache_has == 1){
                                if(c[l].valid_bit[index] == 0){
                                        //c[l].data_item_id[i] = m->data_id;
                                        y =(long) simtime();
                                        c[l].last_update_time[index] = y;
                                        c[l].last_accessed_time[index] = y;
                                        c[l].valid_bit[index] = 1;
                                  //      printf("\n client %ld recieved the requested id:%ld from the server and cache is updated at %6.3f seconds\n", l,m->data_id,clock);
                                }else{ //this case may not occur
                                //      printf("\n client %ld recieved the requested id:%ld from the server and existing cache is valid at %6.3f seconds.\n", l,m->data_id,clock);
                                }

                        }else{
                                //implement LRU: data id is not previously cached here. so replace by LRU if cache is full
                                for(i = 0; i < CACHE_SIZE ; i++){
                                        if(c[l].last_update_time[i] == 0){  //there is an empty block in the cache
                                                index = i;
                                                is_cache_full = 0;
                                                break;
                                        }else{
                                                is_cache_full = 1;
                                        }
                                }
                                if(is_cache_full == 1){
                                       if(reset == 0){
                                                query_generated_count= 0;
                                                cache_hit_count = 0;
                                                cache_hit_ratio = 0;
                                                query_delay = 0;
                                                number_of_queries_served = 0;
                                                reset = 1;
                                        }
                                        all_valid_bits = 1;
                                        index = 0;
                                        oldest = c[l].last_accessed_time[0];
                                        for(i = 0; i < CACHE_SIZE ; i++){
                                               if(c[l].valid_bit[i] == 0){
                                                       if(c[l].last_accessed_time[i] < oldest){
                                                                oldest = c[l].last_accessed_time[i];  // find the smallest
                                                                all_valid_bits = 0;
                                                                index = i;
                                                        }
                                                }
                                        }
                                        if(oldest == c[l].last_accessed_time[0] && c[l].valid_bit[i] == 0){
                                                all_valid_bits = 0;
                                                index = i;
                                        }
                                        if(all_valid_bits == 1){
                                                oldest_valid = c[l].last_accessed_time[0];
                                                for(i = 0; i < CACHE_SIZE ; i++){
                                                      if(c[l].last_accessed_time[i] < oldest_valid){
                                                                oldest_valid = c[l].last_accessed_time[i];  // find the smallest
                                                                index = i;
                                                        }
                                                }

                                        }
                                }
       // printf("\n index value is %ld\n",index);
                                c[l].data_item_id[index] = m->data_id;
                                y =(long) simtime();
                                c[l].last_update_time[index] = y;
                                c[l].last_accessed_time[index] = y;
                                c[l].valid_bit[index] = 1;
//                                printf("\n client %ld recieved the requested id:%ld:%ld from the server and added to cache at %6.3f seconds\n", l,m->data_id,c[l].data_item_id[index],clock);
                        }


         /*               for(i = 0; i < CACHE_SIZE ; i++){
                                printf("\n in cache %ld: data id:%ld and valid bit: %ld,\n",i,c[l].data_item_id[i],c[l].valid_bit[i]);
                        }*/
                }
//      clear(ir_event);
        clear(server_sent_msg);
        }
}

