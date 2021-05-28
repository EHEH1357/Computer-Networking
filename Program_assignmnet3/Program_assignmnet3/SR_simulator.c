/*2016112106 김태용*/
#define _CRT_SECURE_NO_WARNINGS

#include <stdio.h>
#include <stdlib.h>

/* ******************************************************************
 ALTERNATING BIT AND GO-BACK-N NETWORK EMULATOR: VERSION 1.1  J.F.Kurose

   This code should be used for PA2, unidirectional or bidirectional
   data transfer protocols (from A to B. Bidirectional transfer of data
   is for extra credit and is not required).  Network properties:
   - one way network delay averages five time units (longer if there
	   are other messages in the channel for GBN), but can be larger
   - packets can be corrupted (either the header or the data portion)
	   or lost, according to user-defined probabilities
   - packets will be delivered in the order in which they were sent
	   (although some can be lost).
**********************************************************************/

#define BIDIRECTIONAL 0    /* change to 1 if you're doing extra credit */
/* and write a routine called B_output */

/* a "msg" is the data unit passed from layer 5 (teachers code) to layer  */
/* 4 (students' code).  It contains the data (characters) to be delivered */
/* to layer 5 via the students transport level protocol entities.         */
struct msg {
	char data[20];
};

/* a packet is the data unit passed from layer 4 (students code) to layer */
/* 3 (teachers code).  Note the pre-defined packet structure, which all   */
/* students must follow. */
struct pkt {
	int seqnum;
	int acknum;
	int checksum;
	char payload[20];
};

/********* STUDENTS WRITE THE NEXT SEVEN ROUTINES *********/


/* comm defines */
#define   A    0
#define   B    1
/* timeout for the timer */
#define INTERVAL 1.0
#define TIMEOUT 20.0
/* A가 data package를 B로 보낼때,
acknum을 설정할 필요가 없기때문에 
default number로 아래처럼 설정.*/
#define DEFAULT_ACK 111
   /* window size의 최대값 정의*/
#define N 1000
	/* winodw size의 default값 정의*/
int window_size = 1000;

struct ring_buf {
	/* receiver가 0일때: acceptable, receiver가 1일때:data received.
	   sender가	0일때: acceptable, sender가 1일때:data sended, 
	   sender가 2일때:ACK received */
	int flag;
	struct pkt packet;
	float timeout;	/* packet가 보낸 시간 */
};
float g_cur_time = 0.0;

/* receiver 정의*/
int B_base = 0;
int B_ring_start = 0;
struct ring_buf B_buf[N];

/* statistic 정의*/
int A_from_layer5 = 0;
int A_to_layer3 = 0;
int B_from_layer3 = 0;
int B_to_layer5 = 0;

/* sender 정의*/
int A_base = 0;
int A_ring_start = 0;
int A_nextseq = 0;
/* 모든 window의 packets을 위한 ring buffer*/
struct ring_buf A_buf[N];

/* window가 가득찻을때 사용할 extra buffer*/
struct node {
	struct msg message;
	struct node *next;
};
struct node *list_head = NULL;
struct node *list_end = NULL;
int Extra_BufSize = 0;
void append_msg(struct msg *m)
{
	int i;
	/*memory 할당*/
	struct node *n = malloc(sizeof(struct node));
	if (n == NULL) {
		printf("No enough memory");
		return;
	}
	n->next = NULL;
	/*copy packet*/
	for (i = 0; i < 20; ++i) {
		n->message.data[i] = m->data[i];
	}

	/* list가 비어있을때 바로 list에 추가*/
	if (list_end == NULL) {
		list_head = n;
		list_end = n;
		++Extra_BufSize;
		return;
	}
	/* list가 비어있지 않을때 끝에 추가*/
	list_end->next = n;
	list_end = n;
	++Extra_BufSize;
}
struct node *pop_msg()
{
	struct node *p;
	/* list가 비어있을때 NULL반환*/
	if (list_head == NULL) {
		return NULL;
	}

	/* 첫번쨰 node 재탐색*/
	p = list_head;
	list_head = p->next;
	if (list_head == NULL) {
		list_end = NULL;
	}
	--Extra_BufSize;

	return p;
}
/*
checksum의 구현. data packet과 ACK packet은 동일한 방법사용.
carry flow 없음
Seqnum < 1000;
acknum < 1000;
각 payload < 255;
따라서 checksum의 최대는 MAX_INT보다 작다.

seqnum과 acknum의 추가 이유 : 
checksum은 packet이 손상되지 않고 올바른지 확인하는 데 사용.
Seqnum과 acknum도 변질될 수 있으니 seqnum과 acknum을 추가.
*/
int calc_checksum(struct pkt *p)
{
	int i;
	int checksum = 0; /*0으로 초기화*/
	if (p == NULL)
	{
		return checksum;
	}
	/*payload에 있는 모든 문자 추가*/
	for (i = 0; i < 20; i++)
	{
		checksum += (unsigned char)p->payload[i];
	}
	/*seqnum 추가*/
	checksum += p->seqnum;
	/*acknum 추가*/
	checksum += p->acknum;
	/*최종 checknum 반환*/
	return checksum;
}
void Debug_Log(int AorB, char *msg, struct pkt *p, struct msg *m)
{
	char ch = (AorB == A) ? 'A' : 'B';
	if (AorB == A)
	{
		if (p != NULL) {
			printf("[%c] %s. Window[(%d,%d) Packet[seq=%d,ack=%d,check=%d,data=%c..]\n", ch, msg,
				A_base, A_nextseq, p->seqnum, p->acknum, p->checksum, p->payload[0]);
		}
		else if (m != NULL) {
			printf("[%c] %s. Window[(%d,%d) Message[data=%c..]\n", ch, msg, A_base, A_nextseq, m->data[0]);
		}
		else {
			printf("[%c] %s.Window[(%d,%d)\n", ch, msg, A_base, A_nextseq);
		}
	}
	else
	{
		if (p != NULL) {
			printf("[%c] %s. Base[%d] Packet[seq=%d,ack=%d,check=%d,data=%c..]\n", ch, msg,
				B_base, p->seqnum, p->acknum, p->checksum, p->payload[0]);
		}
		else if (m != NULL) {
			printf("[%c] %s. Base[%d] Message[data=%c..]\n", ch, msg, B_base, m->data[0]);
		}
		else {
			printf("[%c] %s. Base[%d]\n", ch, msg, B_base);
		}
	}
}
/* window가 가득찼는지 확인하는 함수 */
int window_isfull()
{
	if (A_nextseq >= A_base + window_size)
		return 1;
	else
		return 0;
}
/* copy packet */
void copy_packet(struct pkt *dst, struct pkt *src)
{
	int i;
	if (dst == NULL || src == NULL)
		return;
	dst->seqnum = src->seqnum;
	dst->acknum = src->acknum;
	dst->checksum = src->checksum;
	for (i = 0; i < 20; ++i)
	{
		dst->payload[i] = src->payload[i];
	}
}
/* B를 위한 함수 */
struct ring_buf *get_ring_buf(int AorB, int seqnum)
{
	int cur_index = 0;
	if (AorB == A)
	{
		if (seqnum < A_base || seqnum >= A_base + window_size)
		{
			return NULL;
		}

		cur_index = (A_ring_start + seqnum - A_base) % window_size;
		return &(A_buf[cur_index]);
	}
	else
	{
		if (seqnum < B_base || seqnum >= B_base + window_size)
		{
			return NULL;
		}

		cur_index = (B_ring_start + seqnum - B_base) % window_size;
		return &(B_buf[cur_index]);
	}
}

/* free send buffer가 필요한지 확인 */
void free_send_buf()
{
	int i;
	int count = 0;
	/* receive window 확인 */
	for (i = A_base; i < A_base + window_size; ++i)
	{
		struct ring_buf *p = get_ring_buf(A, i);
		/* sequence가 아닐때 검색 중지 */
		if (p == NULL || p->flag != 2)
			break;
		/* layer 5 에 packet 보내기 */
		p->flag = 0;
		++count;
	}

	/* 예상 number와 ring base 조정 */
	A_base += count;
	A_ring_start += count;

	/* window size가 줄어들을 때 추가 message를 보내야하는지 extra buffer 확인 */
	while (count > 0) {
		struct node *n = pop_msg();
		if (n == NULL) {
			break;
		}
		A_output(n->message);
		free(n);
		count--;
	}
}

/* called from layer 5, passed the data to be sent to other side */
/* 매순간 새로운 packet이 들어옴,
 * a) extra buffer끝에 packet 추가.
 * b) window가 가득찼는지 아닌지 확인; 만약 window가 가득찼으면 packet을 extra buffer에 냅둠;
 * c) 만약 winodw가 가득차지 않았으면, extra buffer를 시작할 때 하나의 packet을 검색하여 처리한다.
 */
A_output(message)
struct msg message;
{
	printf("================================ Inside A_output===================================\n");
	int i;
	int checksum = 0;
	struct ring_buf *p = NULL;
	struct node *n;

	/* buffer에 메시지 추가 */
   /*Step (a)*/
	append_msg(&message);

	/* 마지막 packet이 ACK가 아니면, message drop하기. */
	/*Step (b)*/
	if (window_isfull())
	{
		Debug_Log(A, "Drop this message, the window is full already", NULL, &message);
		return;
	}

	/* extra buffer에서 message pop하기 */
	/* Step(c)*/
	n = pop_msg();
	if (n == NULL)
	{
		printf("No message need to process\n");
		return 0;
	}

	/* buffer에서 빈 packet 가져오기 */
	p = get_ring_buf(A, A_nextseq);
	if (p == NULL || p->flag == 1)
	{
		Debug_Log(A, "BUG! The window is full already", NULL, &message);
		return;
	}
	++A_from_layer5;
	Debug_Log(A, "Receive a message from layer5", NULL, &message);
	/* msg에서 pkt으로 데이터 복사 */
	for (i = 0; i < 20; i++)
	{
		p->packet.payload[i] = n->message.data[i];
	}
	/* free node */
	free(n);

	/* 현재 seqnum 설정 */
	p->packet.seqnum = A_nextseq;
	/* package를 보내기때문에, acknum를 설정할 이유가없음 */
	p->packet.acknum = DEFAULT_ACK;

	/* check sum을 seqnum과 acknum을 포함하여 계산 */
	checksum = calc_checksum(&(p->packet));
	/* check sum 설정 */
	p->packet.checksum = checksum;

	p->flag = 1;
	p->timeout = g_cur_time + TIMEOUT;

	/* layer 3에 packet 보내기*/
	tolayer3(A, p->packet);
	++A_to_layer3;
	++A_nextseq;

	Debug_Log(A, "Send packet to layer3", &(p->packet), &message);

	return 0;
	printf("================================ Outside A_output===================================\n");
}

B_output(message)  /* need be completed only for extra credit */
struct msg message;
{
}

/* called from layer 3, when a packet arrives for layer 4 */
A_input(packet)
struct pkt packet;
{
	printf("================================ Inside A_input===================================\n");
	struct ring_buf *p;

	Debug_Log(A, "Receive ACK packet from layer3", &packet, NULL);

	/* checksum 체크, 손상됐으면 아무것도 하지않기 */
	if (packet.checksum != calc_checksum(&packet))
	{
		Debug_Log(A, "ACK packet is corrupted", &packet, NULL);
		return;
	}

	/* Duplicate ACKs, 아무것도 하지않기 */
	if (packet.acknum < A_base)
	{
		Debug_Log(A, "Receive duplicate ACK", &packet, NULL);
		return;
	}

	p = get_ring_buf(A, packet.acknum);
	if (p == NULL)
	{
		Debug_Log(A, "BUG: receive ACK of future packets", &packet, NULL);
		return;
	}

	p->flag = 2;

	/* 다음 seq으로 가고, timer 멈추기 */
	free_send_buf();

	Debug_Log(A, "ACK packet process successfully accomplished!!", &packet, NULL);
	printf("================================ Outside A_input===================================\n");
}

/* A의 timer 꺼질때 호출 */
A_timerinterrupt()
{
	int i;

	g_cur_time += INTERVAL;

	/* 현재 package가 timeout일때 재전송 */
	for (i = A_base; i < A_nextseq; ++i)
	{
		struct ring_buf *p = get_ring_buf(A, i);
		if (p != NULL && p->flag == 1 && g_cur_time > p->timeout)
		{
			tolayer3(A, p->packet);
			++A_to_layer3;
			/* 헌재 packet의 timeout을 재설정 */
			p->timeout = (g_cur_time + TIMEOUT);
			Debug_Log(A, "Timeout! Send out the package again", &(p->packet), NULL);
		}
	}

	/* timer 재시작 */
	starttimer(A, INTERVAL);
}

A_init()
{
	int i;
	for (i = 0; i < window_size; ++i)
	{
		A_buf[i].flag = 0;
	}

	/* timer시작, timeout은 TIMEOUT이 아니고 INTERVAL이다 */
	starttimer(A, INTERVAL);
}


/* layer5에 data를 보내야하는지 check. */
void free_recv_buf()
{
	int i;
	int count = 0;
	/* receive window 확인 */
	for (i = B_base; i < B_base + window_size; ++i)
	{
		struct ring_buf *p = get_ring_buf(B, i);
		/* sequent가 아닐때, 검색중지 */
		if (p == NULL || p->flag != 1)
			break;
		/* layer 5에 packet 보내기 */
		tolayer5(B, p->packet.payload);
		++B_to_layer5;
		Debug_Log(B, "Send packet to layer5", &(p->packet), NULL);
		p->flag = 0;
		++count;
	}

	/* 예상 number와 ring base 조정 */
	B_base += count;
	B_ring_start += count;
}

/* called from layer 3, when a packet arrives for layer 4 at B*/
B_input(packet)
struct pkt packet;
{
	printf("================================ Inside B_input===================================\n");
	Debug_Log(B, "Receive a packet from layer3", &packet, NULL);
	++B_from_layer3;

	/* checksum 체크, 손상됐으면 packet drop하기 */
	if (packet.checksum != calc_checksum(&packet))
	{
		Debug_Log(B, "Packet is corrupted", &packet, NULL);
		return;
	}

	/* window의 범주 외에 있는 packet은 무조건 error packet */
	if (packet.seqnum >= B_base + window_size)
	{
		Debug_Log(B, "BUG! receive packet with large seqnum", &packet, NULL);
		return;
	}

	struct ring_buf *p = get_ring_buf(B, packet.seqnum);
	/* duplicate package, ACK에 다시보내기.
	   (dublicate package discard하기)
	   case 1: seqnum < B_base;
	   case 2: the packet already buffered  */
	if (p == NULL || p->flag == 1)
	{
		Debug_Log(B, "Duplicate packet detected", &packet, NULL);
		packet.acknum = packet.seqnum;
		packet.checksum = calc_checksum(&packet);
		tolayer3(B, packet);
		Debug_Log(B, "Send ACK packet to layer3", &packet, NULL);
		return;
	}

	/* 현재 packet buffer하기*/
	p->flag = 1;
	copy_packet(&p->packet, &packet);

	/* ack를 sender에 보내기 */
	packet.acknum = packet.seqnum;
	packet.checksum = calc_checksum(&packet);
	tolayer3(B, packet);
	Debug_Log(B, "Send ACK packet to layer3", &packet, NULL);

	/* layer5에 data를 보내야 하는지 체크 */
	free_recv_buf();
	printf("================================ Outside B_input(packet) =========================\n");
}

/* called when B's timer goes off */
B_timerinterrupt()
{
}

B_init()
{
	int i;
	for (i = 0; i < window_size; ++i)
	{
		B_buf[i].flag = 0;
	}
}

/*****************************************************************
***************** NETWORK EMULATION CODE STARTS BELOW ***********
The code below emulates the layer 3 and below network environment:
  - emulates the tranmission and delivery (possibly with bit-level corruption
	and packet loss) of packets across the layer 3/4 interface
  - handles the starting/stopping of a timer, and generates timer
	interrupts (resulting in calling students timer handler).
  - generates message to be sent (passed from later 5 to 4)
THERE IS NOT REASON THAT ANY STUDENT SHOULD HAVE TO READ OR UNDERSTAND
THE CODE BELOW.  YOU SHOLD NOT TOUCH, OR REFERENCE (in your code) ANY
OF THE DATA STRUCTURES BELOW.  If you're interested in how I designed
the emulator, you're welcome to look at the code - but again, you should have
to, and you defeinitely should not have to modify
******************************************************************/

struct event {
	float evtime;           /* event time */
	int evtype;             /* event type code */
	int eventity;           /* entity where event occurs */
	struct pkt *pktptr;     /* ptr to packet (if any) assoc w/ this event */
	struct event *prev;
	struct event *next;
};
struct event *evlist = NULL;   /* the event list */

/* possible events: */
#define  TIMER_INTERRUPT 0
#define  FROM_LAYER5     1
#define  FROM_LAYER3     2

#define  OFF             0
#define  ON              1
#define   A    0
#define   B    1



int TRACE = 1;             /* for my debugging */
int nsim = 0;              /* number of messages from 5 to 4 so far */
int nsimmax = 0;           /* number of msgs to generate, then stop */
float time = 0.000;
float lossprob;            /* probability that a packet is dropped  */
float corruptprob;         /* probability that one bit is packet is flipped */
float lambda;              /* arrival rate of messages from layer 5 */
int   ntolayer3;           /* number sent into layer 3 */
int   nlost;               /* number lost in media */
int ncorrupt;              /* number corrupted by media*/

main()
{
	struct event *eventptr;
	struct msg  msg2give;
	struct pkt  pkt2give;

	int i, j;
	char c;

	init();
	A_init();
	B_init();

	while (1) {
		eventptr = evlist;            /* get next event to simulate */
		if (eventptr == NULL)
			goto terminate;
		evlist = evlist->next;        /* remove this event from event list */
		if (evlist != NULL)
			evlist->prev = NULL;
		if (TRACE >= 2) {
			printf("\nEVENT time: %f,", eventptr->evtime);
			printf("  type: %d", eventptr->evtype);
			if (eventptr->evtype == 0)
				printf(", timerinterrupt  ");
			else if (eventptr->evtype == 1)
				printf(", fromlayer5 ");
			else
				printf(", fromlayer3 ");
			printf(" entity: %d\n", eventptr->eventity);
		}
		time = eventptr->evtime;        /* update time to next event time */
		if (nsim == nsimmax)
			break;                        /* all done with simulation */
		if (eventptr->evtype == FROM_LAYER5) {
			generate_next_arrival();   /* set up future arrival */
			/* fill in msg to give with string of same letter */
			j = nsim % 26;
			for (i = 0; i < 20; i++)
				msg2give.data[i] = 97 + j;
			if (TRACE > 2) {
				printf("          MAINLOOP: data given to student: ");
				for (i = 0; i < 20; i++)
					printf("%c", msg2give.data[i]);
				printf("\n");
			}
			nsim++;
			if (eventptr->eventity == A)
				A_output(msg2give);
			else
				B_output(msg2give);
		}
		else if (eventptr->evtype == FROM_LAYER3) {
			pkt2give.seqnum = eventptr->pktptr->seqnum;
			pkt2give.acknum = eventptr->pktptr->acknum;
			pkt2give.checksum = eventptr->pktptr->checksum;
			for (i = 0; i < 20; i++)
				pkt2give.payload[i] = eventptr->pktptr->payload[i];
			if (eventptr->eventity == A)     /* deliver packet by calling */
				A_input(pkt2give);            /* appropriate entity */
			else
				B_input(pkt2give);
			free(eventptr->pktptr);          /* free the memory for packet */
		}
		else if (eventptr->evtype == TIMER_INTERRUPT) {
			if (eventptr->eventity == A)
				A_timerinterrupt();
			else
				B_timerinterrupt();
		}
		else {
			printf("INTERNAL PANIC: unknown event type \n");
		}
		free(eventptr);
	}

terminate:
	printf(" Simulator terminated at time %f\n after sending %d msgs from layer5\n", time, nsim);
	printf("===========================================================================\n");
	printf("[%u] of packets sent from the Application Layer of Sender A\n", A_from_layer5);
	printf("[%u] of packets sent from the Transport Layer of Sender A\n", A_to_layer3);
	printf("[%u] packets received at the Transport layer of Receiver B\n", B_from_layer3);
	printf("[%u] of packets received at the Application layer of Receiver B\n", B_to_layer5);
	printf("Total time: [%f] time units\n", time);
	printf("Throughput = [%f] packets/time units\n", B_to_layer5 / time);
}



init()                         /* initialize the simulator */
{
	int i;
	float sum, avg;
	float jimsrand();


	printf("-----  Stop and Wait Network Simulator Version 1.1 -------- \n\n");
	printf("Enter the number of messages to simulate: ");
	scanf("%d", &nsimmax);
	printf("Enter  packet loss probability [enter 0.0 for no loss]:");
	scanf("%f", &lossprob);
	printf("Enter window size [default : 1000, max : 1000]:");
	scanf("%d", &window_size);
	printf("Enter packet corruption probability [0.0 for no corruption]:");
	scanf("%f", &corruptprob);
	printf("Enter average time between messages from sender's layer5 [ > 0.0]:");
	scanf("%f", &lambda);
	printf("Enter TRACE:");
	scanf("%d", &TRACE);

	srand(9999);              /* init random number generator */
	sum = 0.0;                /* test random number generator for students */
	for (i = 0; i < 1000; i++)
		sum = sum + jimsrand(); /* jimsrand() should be uniform in [0,1] */
	avg = sum / 1000.0;
	if (avg < 0.25 || avg > 0.75) {
		printf("It is likely that random number generation on your machine\n");
		printf("is different from what this emulator expects.  Please take\n");
		printf("a look at the routine jimsrand() in the emulator code. Sorry. \n");
		exit(0);
	}

	ntolayer3 = 0;
	nlost = 0;
	ncorrupt = 0;

	time = 0.0;                  /* initialize time to 0.0 */
	generate_next_arrival();     /* initialize event list */
}

/****************************************************************************/
/* jimsrand(): return a float in range [0,1].  The routine below is used to */
/* isolate all random number generation in one location.  We assume that the*/
/* system-supplied rand() function return an int in therange [0,mmm]        */
/****************************************************************************/
float jimsrand()
{
	double mmm = 32767;   	   /* largest int  - MACHINE DEPENDENT!!!!!!!!   */
	float x;                   /* individual students may need to change mmm */
	x = rand() / mmm;          /* x should be uniform in [0,1] */
	return (x);
}

/********************* EVENT HANDLINE ROUTINES *******/
/*  The next set of routines handle the event list   */
/*****************************************************/

generate_next_arrival()
{
	double x, log(), ceil();
	struct event *evptr;
	char *malloc();
	float ttime;
	int tempint;

	if (TRACE > 2)
		printf("          GENERATE NEXT ARRIVAL: creating new arrival\n");

	x = lambda * jimsrand() * 2; /* x is uniform on [0,2*lambda] */
	/* having mean of lambda        */
	evptr = (struct event *)malloc(sizeof(struct event));
	evptr->evtime = time + x;
	evptr->evtype = FROM_LAYER5;
	if (BIDIRECTIONAL && (jimsrand() > 0.5))
		evptr->eventity = B;
	else
		evptr->eventity = A;
	insertevent(evptr);
}


insertevent(p)
struct event *p;
{
	struct event *q, *qold;

	if (TRACE > 2) {
		printf("            INSERTEVENT: time is %lf\n", time);
		printf("            INSERTEVENT: future time will be %lf\n", p->evtime);
	}
	q = evlist;     /* q points to header of list in which p struct inserted */
	if (q == NULL) { /* list is empty */
		evlist = p;
		p->next = NULL;
		p->prev = NULL;
	}
	else {
		for (qold = q; q != NULL && p->evtime > q->evtime; q = q->next)
			qold = q;
		if (q == NULL) { /* end of list */
			qold->next = p;
			p->prev = qold;
			p->next = NULL;
		}
		else if (q == evlist) { /* front of list */
			p->next = evlist;
			p->prev = NULL;
			p->next->prev = p;
			evlist = p;
		}
		else {     /* middle of list */
			p->next = q;
			p->prev = q->prev;
			q->prev->next = p;
			q->prev = p;
		}
	}
}

printevlist()
{
	struct event *q;
	int i;
	printf("--------------\nEvent List Follows:\n");
	for (q = evlist; q != NULL; q = q->next) {
		printf("Event time: %f, type: %d entity: %d\n", q->evtime, q->evtype, q->eventity);
	}
	printf("--------------\n");
}



/********************** Student-callable ROUTINES ***********************/

/* called by students routine to cancel a previously-started timer */
stoptimer(AorB)
int AorB;  /* A or B is trying to stop timer */
{
	struct event *q, *qold;

	if (TRACE > 2)
		printf("          STOP TIMER: stopping timer at %f\n", time);
	/* for (q=evlist; q!=NULL && q->next!=NULL; q = q->next)  */
	for (q = evlist; q != NULL; q = q->next)
		if ((q->evtype == TIMER_INTERRUPT && q->eventity == AorB)) {
			/* remove this event */
			if (q->next == NULL && q->prev == NULL)
				evlist = NULL;       /* remove first and only event on list */
			else if (q->next == NULL) /* end of list - there is one in front */
				q->prev->next = NULL;
			else if (q == evlist) { /* front of list - there must be event after */
				q->next->prev = NULL;
				evlist = q->next;
			}
			else {     /* middle of list */
				q->next->prev = q->prev;
				q->prev->next = q->next;
			}
			free(q);
			return;
		}
	printf("Warning: unable to cancel your timer. It wasn't running.\n");
}


starttimer(AorB, increment)
int AorB;  /* A or B is trying to stop timer */
float increment;
{

	struct event *q;
	struct event *evptr;
	char *malloc();

	if (TRACE > 2)
		printf("          START TIMER: starting timer at %f\n", time);
	/* be nice: check to see if timer is already started, if so, then  warn */
	/* for (q=evlist; q!=NULL && q->next!=NULL; q = q->next)  */
	for (q = evlist; q != NULL; q = q->next)
		if ((q->evtype == TIMER_INTERRUPT && q->eventity == AorB)) {
			printf("Warning: attempt to start a timer that is already started\n");
			return;
		}

	/* create future event for when timer goes off */
	evptr = (struct event *)malloc(sizeof(struct event));
	evptr->evtime = time + increment;
	evptr->evtype = TIMER_INTERRUPT;
	evptr->eventity = AorB;
	insertevent(evptr);
}


/************************** TOLAYER3 ***************/
tolayer3(AorB, packet)
int AorB;  /* A or B is trying to stop timer */
struct pkt packet;
{
	struct pkt *mypktptr;
	struct event *evptr, *q;
	char *malloc();
	float lastime, x, jimsrand();
	int i;


	ntolayer3++;

	/* simulate losses: */
	if (jimsrand() < lossprob) {
		nlost++;
		if (TRACE > 0)
			printf("          TOLAYER3: packet being lost\n");
		return;
	}

	/* make a copy of the packet student just gave me since he/she may decide */
	/* to do something with the packet after we return back to him/her */
	mypktptr = (struct pkt *)malloc(sizeof(struct pkt));
	mypktptr->seqnum = packet.seqnum;
	mypktptr->acknum = packet.acknum;
	mypktptr->checksum = packet.checksum;
	for (i = 0; i < 20; i++)
		mypktptr->payload[i] = packet.payload[i];
	if (TRACE > 2) {
		printf("          TOLAYER3: seq: %d, ack %d, check: %d ", mypktptr->seqnum,
			mypktptr->acknum, mypktptr->checksum);
		for (i = 0; i < 20; i++)
			printf("%c", mypktptr->payload[i]);
		printf("\n");
	}

	/* create future event for arrival of packet at the other side */
	evptr = (struct event *)malloc(sizeof(struct event));
	evptr->evtype = FROM_LAYER3;   /* packet will pop out from layer3 */
	evptr->eventity = (AorB + 1) % 2; /* event occurs at other entity */
	evptr->pktptr = mypktptr;       /* save ptr to my copy of packet */
	/* finally, compute the arrival time of packet at the other end.
	   medium can not reorder, so make sure packet arrives between 1 and 10
	   time units after the latest arrival time of packets
	   currently in the medium on their way to the destination */
	lastime = time;
	/* for (q=evlist; q!=NULL && q->next!=NULL; q = q->next) */
	for (q = evlist; q != NULL; q = q->next)
		if ((q->evtype == FROM_LAYER3 && q->eventity == evptr->eventity))
			lastime = q->evtime;
	evptr->evtime = lastime + 1 + 9 * jimsrand();



	/* simulate corruption: */
	if (jimsrand() < corruptprob) {
		ncorrupt++;
		if ((x = jimsrand()) < .75)
			mypktptr->payload[0] = 'Z'; /* corrupt payload */
		else if (x < .875)
			mypktptr->seqnum = 999999;
		else
			mypktptr->acknum = 999999;
		if (TRACE > 0)
			printf("          TOLAYER3: packet being corrupted\n");
	}

	if (TRACE > 2)
		printf("          TOLAYER3: scheduling arrival on other side\n");
	insertevent(evptr);
}

tolayer5(AorB, datasent)
int AorB;
char datasent[20];
{
	int i;
	if (TRACE > 2) {
		printf("          TOLAYER5: data received: ");
		for (i = 0; i < 20; i++)
			printf("%c", datasent[i]);
		printf("\n");
	}

}