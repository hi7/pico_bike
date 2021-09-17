#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "pico/binary_info.h"
#include "hardware/uart.h"

#define UART_ID uart1
#define BAUD_RATE 9600

//#define UART_TX_GPIO 4
#define UART_RX_GPIO 5

const uint LED_GPIO = 25;

int checksum(const char *s) {
    int c = 0;
    while(*s)
        c ^= *s++;
    return c;
}

int fromHex(char c) {
    if(c >= 'a') {
        return 10 + (c - 'a');
    }
    if(c >= 'A') {
        return 10 + (c - 'A');
    }
    return c - '0';
}

bool check(const char *line) {
    char *found = strchr(line, '*');
    if(found != NULL) {
        int len = strlen(line);
        const char *checkString = &line[len-2];
        found[0] = 0; // cut rest of string
        int sum = fromHex(checkString[0])*16 + fromHex(checkString[1]);
        if(sum != checksum(line)) printf("%d %d %s\n", sum, checksum(line), line);
        return (sum == checksum(line));
    }
    return true;
}

int readWord(const char *line, int startIndex, char *word) {
    char *found = strchr(&line[startIndex], ',');
    if(found != NULL) {
        int len = found - &line[startIndex];
        memcpy(word, &line[startIndex], len);
        word[len] = '\0';
    }
}

bool parse(const char *line) {
    const char *found = strchr(line, ',');
    char token[10];
    int startIndex = 0;
    readWord(line, startIndex, token);
    if(strcmp(token, "GNGLL") == 0 || strcmp(token, "GPGGA") == 0) { 
        //geographic position, latitude / longitude
        printf("%s\n", line);
    } else if(strcmp(token, "GNGGA") == 0 || strcmp(token, "GPGGA") == 0) { 
        //global positioning system fix data
        printf("%s\n", line);
    /*} else if(strcmp(token, "GNGSA") == 0) { //GPS DOP and active satelites
        printf("%s\n", line);
    } else if(strcmp(token, "GPGSV") == 0) { //GPS satelites in view
        printf("%s\n", line);*/
    } else if(strcmp(token, "GNRMC") == 0 || strcmp(token, "GPRMC") == 0) { 
        //recommended minimum specific GPS/transit data
        printf("%s\n", line);
    } else if(strcmp(token, "GNRMC") == 0 || strcmp(token, "GPVTG") == 0) { 
        //track made good and ground speed
        printf("%s\n", line);
    }
}

int main() {
    bi_decl(bi_program_description("bike"));
    bi_decl(bi_1pin_with_name(UART_RX_GPIO, "GPS UART TX/PICO RX"));

    stdio_init_all();
    gpio_init(LED_GPIO);
    gpio_set_dir(LED_GPIO, GPIO_OUT);
    uart_init(UART_ID, BAUD_RATE);
    //gpio_set_function(UART_TX_GPIO, GPIO_FUNC_UART);
    gpio_set_function(UART_RX_GPIO, GPIO_FUNC_UART);
    char line[] = "012345678901234567890123456789012345678901234567890123456789";
    int index = 0;
    bool listen = false;
    while(1 /*uart_is_readable(UART_ID)*/) {
        const char c = uart_getc(UART_ID);
        //putc(c, stdout);
        if(listen && (c == 13 || c == 10)) { // CR or LF
            line[index] = 0;
            if(check(line)) {
                gpio_put(LED_GPIO, false); // turn off LED
                parse(line);
            } else {
                printf("checksum error: %s\n", line);
                gpio_put(LED_GPIO, true); // turn on LED
            }
            listen = false;
        }
        if(listen) {
            line[index++] = c;
        }
        if(!listen && c == '$') { // start
            listen = true;
            index = 0;
        }
        //gpio_put(LED_GPIO, 1);
    }
    puts("UART is not readable!");
    return 0;
}
