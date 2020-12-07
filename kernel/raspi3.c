extern void raspi3_delay(unsigned long);
extern void raspi3_put32(unsigned long, unsigned int);
extern unsigned int raspi3_get32(unsigned long);

#define PBASE 0x3F000000

#define GPFSEL1 (PBASE + 0x00200004)
#define GPSET0 (PBASE + 0x0020001C)
#define GPCLR0 (PBASE + 0x00200028)
#define GPPUD (PBASE + 0x00200094)
#define GPPUDCLK0 (PBASE + 0x00200098)

#define UART_BASE (PBASE + 0x201000)
#define UART_DR (UART_BASE)
#define UART_FR (UART_BASE + 0x18)
#define UART_IBRD (UART_BASE + 0x24)
#define UART_FBRD (UART_BASE + 0x28)
#define UART_CR (UART_BASE + 0x30)
#define UART_LCRH (UART_BASE + 0x2c)
#define UART_IMSC (UART_BASE + 0x18)

#define AUX_ENABLES (PBASE + 0x00215004)
#define AUX_MU_IO_REG (PBASE + 0x00215040)
#define AUX_MU_IER_REG (PBASE + 0x00215044)
#define AUX_MU_IIR_REG (PBASE + 0x00215048)
#define AUX_MU_LCR_REG (PBASE + 0x0021504C)
#define AUX_MU_MCR_REG (PBASE + 0x00215050)
#define AUX_MU_LSR_REG (PBASE + 0x00215054)
#define AUX_MU_MSR_REG (PBASE + 0x00215058)
#define AUX_MU_SCRATCH (PBASE + 0x0021505C)
#define AUX_MU_CNTL_REG (PBASE + 0x00215060)
#define AUX_MU_STAT_REG (PBASE + 0x00215064)
#define AUX_MU_BAUD_REG (PBASE + 0x00215068)

#define CORE_FREQ 250
#define BAUD_FREQ 115200

void miniuart_init(void) {
    unsigned int selector;  // 32 bits

    selector = raspi3_get32(GPFSEL1);
    selector &= ~(7 << 12);
    selector |= 4 << 12;
    selector &= ~(7 << 15);
    selector |= 4 << 15;
    raspi3_put32(GPFSEL1, selector);

    raspi3_put32(GPPUD, 0);
    raspi3_delay(150);
    raspi3_put32(GPPUDCLK0, (1 << 14) | (1 << 15));
    raspi3_delay(150);
    raspi3_put32(GPPUDCLK0, 0);

    raspi3_put32(UART_CR, 0);
    raspi3_put32(UART_IBRD, 26);
    raspi3_put32(UART_FBRD, 3);
    raspi3_put32(UART_LCRH, (1 << 4) | (3 << 5));
    // raspi3_put32(UART_IMSC, 0);
    raspi3_put32(UART_CR, 1 | (1 << 8) | (1 << 9));
}

void miniuart_send(char c) {
    while (1) {
        if (!(raspi3_get32(UART_FR) & (1 << 5))) break;
    }
    raspi3_put32(UART_DR, c);
}

char miniuart_recv(void) {
    while (1) {
        if (!(raspi3_get32(UART_FR) & (1 << 4))) break;
    }
    return (raspi3_get32(UART_DR) & 0xFF);
}

void miniuart_send_string(char* str) {
    for (int i = 0; str[i] != '\0'; i++) {
        miniuart_send((char)str[i]);
    }
}

void raspi3_c_helloworld(unsigned long id)
{
	if (id == 0) {
		miniuart_init();
	}
	miniuart_send_string("Hello from processor ");
	miniuart_send(id + 48);
	miniuart_send_string(".\r\n");

	while (1) {
		miniuart_send(miniuart_recv());
	}
}
