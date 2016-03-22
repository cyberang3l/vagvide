#ifndef network_h
#define network_h
#ifdef __cplusplus

#include "common.h"
/* Use EtherCard to enable ENC28J20 ethernet module
 * Documentation: http://jeelabs.net/pub/docs/ethercard/classEtherCard.html
 */
#include "EtherCard.h"
#include "net.h"
/* Use EEPROM and NetEEPROM to store the network configuration */
#include "EEPROM.h"
#include "NetEEPROM.h"

#define ETH_SPI_CHIP_SELECT_PIN 53
#define HOSTNAME_MAX_SIZE 50

/***f* subnet_mask_valid
 *
 * Returns true if the subnet_mask is valid, false otherwise.
 *
 * A subnet mask must be composed of a sequence of one's (1)
 * starting from the MSB, followed by a sequence of zeros (0).
 * It can be something like 255.255.255.0, but not
 * 255.254.255.0. In the latter case, 254 equals to
 * to the binary number 1111 1110 which is followed
 * by 1111 1111 (255).
 */
bool subnet_mask_valid(IN byte subnet_mask[]);

/***f* get_hostname_from_http_request
 *
 * Reads the data from an HTTP request, and extracts the
 * hostname that the client used to connect to the Arduino server.
 * The extracted hostname is stored in the hostname char array.
 * If the client used our IP to connect to us, this function will
 * store just an IP address, however, if the client used a domain
 * name to connect to us, this function will return the domain name.
 */
void get_hostname_from_http_request(IN char data[],
                                    OUT char hostname[],
                                    IN int hostname_size);


/***f* get_TCP_seq
 *
 * Extracts the TCP sequence number from the ethernet buffer ethBuf
 */
unsigned int get_TCP_seq(IN byte *ethBuf);


/***f* print_macAddress
 *
 * A function to print the MAC address.
 */
void print_macAddress(IN byte mymac[]);

#endif // endif __cpluscplus
#endif // endif network_h
