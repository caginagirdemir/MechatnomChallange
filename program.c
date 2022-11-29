#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>

unsigned int *gpio_port1;

#define UART_INT_TX     0x08
#define UART_INT_RX     0x40

#define P1P8_ON         0x80
#define P1P8_OFF        0x40

#define DATA_READY      0x10
#define XMT_FIFO_FULL   0x40
#define XMT_FIFIO_EMPTY 0x80

#define UART_BUF_SIZE   1024

struct interrupt_registers
{
    unsigned char *intr_mask; //interrupt mask
    unsigned char *intr_status; //source of interrupts
    unsigned char *intr_ack;
};

struct uart_registers
{
    unsigned int    *control; //serial configuartion register
    unsigned int    *status; //status
    unsigned char   *tx_data; //transmit data(with UART hardware fifo!)
    unsigned char   *rx_data; //recieve data(with UART hardware fifo!)
};

struct uart_registers *uart_registers_ptr;              //pointer to uart hardware registers
struct interrupt_registers *interrupt_registers_ptr;    //pointer to interupt hardware registers

unsigned char uart_tx[UART_BUF_SIZE]; //trasmit fifo buffer
unsigned char uart_rx[UART_BUF_SIZE]; //receive fifo buffer

//fifo control structure
struct _arm_uart_buf
{
    unsigned char *beg;         //begin of fifo buffer
    unsigned char *end;         //end of fifo buffer
    unsigned char *read_ptr;    //tail
    unsigned char *write_ptr;   //head
} arm_uart_bufs[2];

#define TX_FIFO     0
#define RX_FIFO     1

unsigned int on_time[1];
unsigned int off_time[1];

bool echo_flag = 1;

//function declarations
unsigned char * next_ptr(int index);
void            init_(void);
void            uart_irq_interrupt(void);
void            _uart_rx(void);
void            _uart_tx(void);
int             bufRead(unsigned char *buf, int index);
void            task1();
void            task2();
int             uart_send(unsigned char *buf, int len);
int             ft_strlen(char *str);

void init_(void) //memory assignemnts
{
    int i;
    struct _arm_uart_buf *sptr;

    uart_registers_ptr->control = (unsigned int *)0x80030000;
    uart_registers_ptr->status = (unsigned int *)0x80030002;
    uart_registers_ptr->tx_data = (unsigned char *)0x80030004;
    uart_registers_ptr->rx_data = (unsigned char *)0x80030006;
    interrupt_registers_ptr->intr_mask = (unsigned char *)0x800a002c;
    interrupt_registers_ptr->intr_status = (unsigned char *)0x800a0030;
    interrupt_registers_ptr->intr_ack = (unsigned char *)0x800a0034;

    for (i = 0; i < 2; i++)
    {
        sptr = &arm_uart_bufs[i];
        if(i)
        {
            sptr->beg = sptr->write_ptr = sptr->read_ptr = uart_tx;
            sptr->end = uart_tx + 1023;
        }
        else
        {
            sptr->beg = sptr->write_ptr = sptr->read_ptr = uart_rx;
            sptr->end = uart_rx + 1023;
        }
    }

    gpio_port1 = (unsigned int *)0x600b0030; //gpio address assignemnt

    *on_time = 3 * 1000; // for microsecond function
    *off_time = 5 * 1000; // for microsecond function
}

//interrupt service routine: will be called by receive or transmit intrrupt
void uart_irq_interrupt(void)
{
    unsigned char status;

    status = *interrupt_registers_ptr->intr_status;      //read interrupt status

    if(status & UART_INT_RX)
    {
        *interrupt_registers_ptr->intr_ack |= UART_INT_RX;
        _uart_rx();
    }
    if(status & UART_INT_TX)
    {
        *interrupt_registers_ptr->intr_ack |= UART_INT_TX;
        _uart_tx();
    }
}

//receive subroutine of interrupt service
void    _uart_rx(void)
{
    unsigned char *rx_ptr;

    //as long as hardware receive buffer if filled
    while(*uart_registers_ptr->status & DATA_READY)
    {
        //get pointer to next write item in receive fifo (overrun check!)
        rx_ptr = next_ptr(RX_FIFO);
        *rx_ptr = *uart_registers_ptr->rx_data; //write item to receive fifo
    }
    rx_ptr = next_ptr(RX_FIFO);
    *rx_ptr = '\t';
}

//transmit subroutine of interrupt service
void _uart_tx(void)
{
    int ret;
    unsigned char c;

    ret = 1;
    if(*uart_registers_ptr->status & XMT_FIFIO_EMPTY) //is transmit buffer empty
    {
        //as long as transmit fifo in hardware is not full and software fifo not empty
        while(!(*uart_registers_ptr->status & XMT_FIFO_FULL) && (ret))
        {
            ret = bufRead(&c, TX_FIFO);             //read char from software fifo
            if(ret)                                 //not empty?
                *uart_registers_ptr->tx_data = c;   //copy item to transmit register
        }
        // software transmit fifo empty? This code not have to one time due to hardware limitations.
        // Code can turn back therefore if ret not 0 then keep TX interruption open.
        if (ret == 0)
            *interrupt_registers_ptr->intr_mask &= ~UART_INT_TX; //disable transmit
    }
}

unsigned char *next_ptr(int index) //get next pointer, provide circularity and check overwrite control
{
    unsigned char *rtp;
    unsigned char *next;

    rtp = arm_uart_bufs[index].write_ptr;
    next = rtp + 1;
    if(next >= arm_uart_bufs[index].end) //provides circularity for next pointer
        next = arm_uart_bufs[index].beg;

    if(arm_uart_bufs[index].write_ptr == arm_uart_bufs[index].end) //provides circularity
        arm_uart_bufs[index].write_ptr = arm_uart_bufs[index].beg;
    else if (next != arm_uart_bufs[index].read_ptr) //overwrite control
    {
        arm_uart_bufs[index].write_ptr++;
    }
    else
    {
        //Out of Space state
    }
    return(rtp);
}

int bufRead(unsigned char *buf, int index) //get data from ring buffers
{
    //is receive fifo empty?
    if(arm_uart_bufs[index].read_ptr == arm_uart_bufs[index].write_ptr)
    {
        //fifo is empty
        *buf = *arm_uart_bufs[index].read_ptr;
        //*arm_uart_bufs[index].read_ptr = 0x0; //interrupt problem!!!
        return(0);
    }

    //fifo is not empty
    *buf = *arm_uart_bufs[index].read_ptr;
    *arm_uart_bufs[index].read_ptr = 0x0;

    //is read pointer at the end?
    if(arm_uart_bufs[index].read_ptr == arm_uart_bufs[index].end)
    {
        //wrap around read pointer to begin
        arm_uart_bufs[index].read_ptr = arm_uart_bufs[index].beg;
    }
    else
    {
        //increment read pointer to next item
        arm_uart_bufs[index].read_ptr++;
    }
    return(1);
}

int uart_send(unsigned char *buf, int len)
{
    unsigned char *txptr;


    while(len--)
    {
        txptr = next_ptr(TX_FIFO);
        *txptr = *buf;
        buf++;
    }
    _uart_tx();
    return 1;
}

int ft_strlen(char *str)
{
    int i = -1;
    while(str[++i] != '\0') ;
    return i;
}

void task2() //infinity loop
{
        int ret = 1;
        int i = -1;
        unsigned char c = '\0';
        char buf[15];

        while(c != '\t') // tab non-printable delimeter character in this program
        {
            ret = bufRead(&c, RX_FIFO);
            if(ret)
                buf[++i] = c;
        }
        buf[++i] = '\0';
        if(strcmp(buf, "stop"))
            echo_flag = 0;
        else if(strcmp(buf, "start"))
            echo_flag = 1;
        else if(strncmp(buf,"ledon", 5))
            *on_time = atoi(strchr(buf, '=')+1) * 1000;
        else if(strncmp(buf,"ledoff", 6))
            *off_time = atoi(strchr(buf, '=')+1) * 1000;
        else if(echo_flag)
            uart_send((unsigned char *)buf, ft_strlen(buf));
}

void task1() //infinity loop
{
    if(echo_flag)
    {
        usleep(1000000);
        *gpio_port1 |= P1P8_ON;
        usleep(1000000);
        *gpio_port1 |= P1P8_OFF;
    }
    else
    {
        usleep(*off_time);
        *gpio_port1 |= P1P8_ON;
        usleep(*on_time);
        *gpio_port1 |= P1P8_OFF;
    }
}

int main()
{
    init_();
    // thread_start(task1); 
    // thread_start(task2);
    pause(); 
}
