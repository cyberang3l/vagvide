#include "network.h"
/* All the web page strings that I serve from Arduino */
#include "web_page_strings.h"
#include "temperature.h"

// Variable to store the previous TCP sequence number
// and compare it with the TCP sequence of a newly arrived
// packet in order to check if the packet is a duplicate
static unsigned int PREV_TCP_SEQ_NUM = 0;
// Used as cursor while filling the buffer
static BufferFiller bfill;
// TCP/IP send/receive buffer
byte Ethernet::buffer[2000];
//
byte myip[4], gwip[4], dnsip[4], netmask[4], mymac[6];

void resetEepromNetworkConfig() {
  // To reset the ip configuration, write the
  // value 0 to NET_EEPROM_OFFSET.
  EEPROM.write(NET_EEPROM_OFFSET, 0);
}

void initNetworkModule() {
  NetEeprom.init(mymac);

  Serial.print("MAC: ");
  print_macAddress(mymac);
  Serial.println();

  while (ether.begin(sizeof Ethernet::buffer, mymac, ETH_SPI_CHIP_SELECT_PIN) == 0)
  {
    /* TODO: If the EN28J60 module is not powered up, the begin() function will never
     *       return because the initialize() function in the file enc28j60.cpp that
     *       is called by the begin function is running in an etternal while loop
     *       until it gets a response from the hardware. We should have a way to
     *       notify the user in the LCD that there is a problem with the Ethernet in
     *       case the ether.begin takes a very long time.
     */
    Serial.println( "Failed to access Ethernet controller");
    delay(5000);
  }
}

void initNetworkAddr() {
  if (NetEeprom.isDhcp())
  {
    Serial.println("Try DHCP");
    if (!ether.dhcpSetup())
      Serial.println( "DHCP failed");
  }
  else
  {
    NetEeprom.readIp(myip);
    NetEeprom.readGateway(gwip);
    NetEeprom.readDns(dnsip);
    NetEeprom.readSubnet(netmask);
    Serial.println("Static IP");
    ether.staticSetup(myip, gwip, dnsip, netmask);
  }

  ether.printIp("My IP: ", ether.myip);
  ether.printIp("Netmask: ", ether.netmask);
  ether.printIp("GW IP: ", ether.gwip);
  ether.printIp("DNS IP: ", ether.dnsip);
}

void processEthernetPacket(IN uint16_t payload_pos) {
  if (payload_pos) {
    /* Get the current TCP seq number */
    unsigned int CURRENT_TCP_SEQ_NUM = get_TCP_seq(Ethernet::buffer);

    /* Store the received request data in the *data pointer */
    char *data = (char *) Ethernet::buffer + payload_pos;
    /* Store the IP of the connect client in the *clientIP pointer */
    byte *clientIP = (byte *) Ethernet::buffer + IP_SRC_P;

    /* bfill stores a Pointer to the start of TCP payload. */
    bfill = ether.tcpOffset();

    /* Compare the current TCP seq number with the previous one.
     * If it is the same, just reply back with an "HTTP OK".
     *
     * I do this because in most cases, browsers will send duplicate
     * HTTP requests when Arduino takes a relatively long time to reply,
     * e.g. because it is waiting for temperature conversion.
     */
    if (PREV_TCP_SEQ_NUM != CURRENT_TCP_SEQ_NUM) {
      PREV_TCP_SEQ_NUM = CURRENT_TCP_SEQ_NUM;
      /* Use the 'hostname_client_connected' array to store the hostname that the client is using
       * and use this hostname for the links.
       * Initially I was using the IP of the ENC28J60 module, but I found out that if I connect
       * from the Internet using a dyndns, this doesn't work because the IP is usually in a private
       * 192.168.x.x range, which is not publicly routable.
       */
      char hostname_client_connected[HOSTNAME_MAX_SIZE];
      memset(hostname_client_connected, '\0', sizeof(char) * HOSTNAME_MAX_SIZE);

      ether.printIp("Got connection from: ", clientIP);

      /* Temporary string to store the values read from the http request */
      char str_temp[20];
      if (strncmp("GET /", data, 5) == 0)
      {
        get_hostname_from_http_request(data, hostname_client_connected, HOSTNAME_MAX_SIZE);
        Serial.println(hostname_client_connected);

        data += 5;
        if (data[0] == ' ')
        {
          Serial.println("HTTP:Main page...");
          bfill.emit_p(http_OK_200);
          bfill.emit_p(webpage_main,
                       hostname_client_connected,
                       hostname_client_connected
                       );
        }
        else if (strncmp( "temp ", data, 5 ) == 0)
        {
          Serial.println("HTTP:Requesting Temperatures...");
          readAllTemperatures();
          bfill.emit_p(http_OK_200);
          for (int i = 0; i < numSensors; i++) {
            tempSensorDesc[i].toCharArray(str_temp, TEMPSENSOR_DESC_STR_LENGTH);
            bfill.emit_p(webpage_temperature,
                         str_temp,
                         dtostrf(temperature[i], 4, 2, str_temp + TEMPSENSOR_DESC_STR_LENGTH));
            //printTemperature(temperature[i], tempSensorDesc[i], oneWirePins[i]);
          }
        }
        else if (strncmp( "ipconfig ", data, 9 ) == 0)
        {
          Serial.println("HTTP:IP Configuration...");
          NetEeprom.readIp(myip);
          NetEeprom.readGateway(gwip);
          NetEeprom.readDns(dnsip);
          NetEeprom.readSubnet(netmask);

          bfill.emit_p(http_OK_200);
          char checked_dhcp[8] = "";
          if (NetEeprom.isDhcp())
            strcat(checked_dhcp, "checked\0");

          bfill.emit_p(webpage_ipconfig,
                       hostname_client_connected,
                       checked_dhcp,
                       myip[0], myip[1], myip[2], myip[3],
                       netmask[0], netmask[1], netmask[2], netmask[3],
                       gwip[0], gwip[1], gwip[2], gwip[3],
                       dnsip[0], dnsip[1], dnsip[2], dnsip[3]);

        }
        else
        {
          for (unsigned int i = 0; i < sizeof(str_temp) - 1; i++ )
          {
            if (data[i] == ' ')
            {
              str_temp[i] = '\0';
              break;
            }
            str_temp[i] = data[i];
          }
          bfill.emit_p(http_not_found_404);
          bfill.emit_p(webpage_not_found, str_temp, ether.myip[0], ether.myip[1], ether.myip[2], ether.myip[3]);
        }
      }
      else if (strncmp("POST /ipconfig ", data, 14) == 0)
      {
        Serial.println("HTTP:IP Configuration set...");
        int i, j = 0, datalen = strlen(data);
        /* if read_var_name == true, then we parse the name of the variable
         * we want to read i.e. ip, dns, subnet or gw
         * if read_var_name == false, then we parse the value for the
         * previously read var_name
         */
        bool read_var_name = true;
        /* If the configuration parameters are ok,
         * set the corresponding IP addresses
         */
        bool static_conf_ok = true;
        /* Buffer for storing */
        char value[20];

        get_hostname_from_http_request(data, hostname_client_connected, HOSTNAME_MAX_SIZE);

        /* Find the offset that the POSTed data start */
        while (data[datalen--] != '\n')
          continue;

        data += (datalen += 2);

        /* I noticed that sometimes the actual data is not getting received in
         * the POST packet, so if the length of the data is zero, receive the
         * next packet here.
         */
        if (strlen(data) == 0) {
          uint16_t pos = ether.packetLoop(ether.packetReceive());
          data = (char *) Ethernet::buffer + pos;
          // Serial.print("Reading second packet: ");
          // Serial.println(data);
        }

        for ( i = 0; i <= strlen(data); i++ )
        {
          /* if we read '=', then the value should follow */
          if (data[i] == '=')
          {
            /* so add a string termination character i nthe str_temp */
            str_temp[j] = '\0';
            j = 0;
            /* set the read_var_name=false */
            read_var_name = false;
            continue;
          }

          /* if the current character is '&', or the string termination character '\0'
           * which marks the end of the available data, then the value reading has finished
           * and we can start reading the next variable (if more variables exist) and process
           * the current one
           */
          if (data[i] == '&' || data[i] == '\0')
          {
            /* so terminate the value string */
            value[j] = '\0';
            j = 0;
            /* enable the flag read_var_name so that we can start reading a
             * variable name in the next loop
             */
            read_var_name = true;
            if (strncmp(str_temp, "dhcp", 4) == 0)
            {
              // Serial.print("dhcp: ");
              // Serial.println(value);
              /* If it is not set already to DHCP, do it now.
                 and set the static_conf_ok = false since we
                 configure the ip address setting by using DHCP
              */
              static_conf_ok = false;
              if (!NetEeprom.isDhcp())
              {
                NetEeprom.writeDhcpConfig(mymac);

                bfill.emit_p(http_OK_200);
                bfill.emit_p(webpage_please_connect_manually);
                ether.httpServerReply(bfill.position());

                software_Reset();
              }
            }
            else if (strncmp(str_temp, "ip", 2) == 0)
            {
              //Serial.print("ip: ");
              //Serial.println(value);
              if (ether.parseIp(myip, value) != 0)
              {
                static_conf_ok = false;
                break;
              }
            }
            else if (strncmp(str_temp, "gw", 2) == 0)
            {
              //Serial.print("gw: ");
              //Serial.println(value);
              if (ether.parseIp(gwip, value) != 0)
              {
                static_conf_ok = false;
                break;
              }
            }
            else if (strncmp(str_temp, "dns", 3) == 0)
            {
              //Serial.print("dns: ");
              //Serial.println(value);
              if (ether.parseIp(dnsip, value) != 0)
              {
                static_conf_ok = false;
                break;
              }
            }
            else if (strncmp(str_temp, "subnet", 6) == 0)
            {
              //Serial.print("subnet: ");
              //Serial.println(value);
              if (ether.parseIp(netmask, value) != 0)
              {
                static_conf_ok = false;
                break;
              }
              else
              {
                //Serial.println("subnet is a valid IP");
                /* It is not enough for the subnet mask to be a valid IP
                 * address. It needs to follows some additional rules, so
                 * call the function subnet_mask_valid().
                 */
                if (!subnet_mask_valid(netmask))
                {
                  Serial.println("subnet is not a valid mask!");
                  static_conf_ok = false;
                  break;
                }

              }
            }

            if (data[i] == ' ')
              break;
            else
              continue;
          }

          if (data[i] != '=')
          {
            if (read_var_name)
              str_temp[j++] = data[i];
            else
              value[j++] = data[i];
          }
        }

        if (static_conf_ok)
        {
          NetEeprom.writeManualConfig(mymac, myip, gwip, netmask, dnsip);

          bfill.emit_p(http_OK_200);
          bfill.emit_p(webpage_please_connect_manually);
          ether.httpServerReply(bfill.position());

          software_Reset();
        }

        NetEeprom.readIp(myip);
        NetEeprom.readGateway(gwip);
        NetEeprom.readDns(dnsip);
        NetEeprom.readSubnet(netmask);

        bfill.emit_p(http_OK_200);
        char checked_dhcp[8] = "";
        if (NetEeprom.isDhcp())
          strcat(checked_dhcp, "checked\0");

        bfill.emit_p(webpage_ipconfig,
                     hostname_client_connected,
                     checked_dhcp,
                     myip[0], myip[1], myip[2], myip[3],
                     netmask[0], netmask[1], netmask[2], netmask[3],
                     gwip[0], gwip[1], gwip[2], gwip[3],
                     dnsip[0], dnsip[1], dnsip[2], dnsip[3]);

      }
      else
      {
        bfill.emit_p(http_unauthorized_401);
        bfill.emit_p(webpage_unauthorized);
      }
      /* Send a response to an HTTP request. */
      //Serial.println("Sending response back to the client...");
      ether.httpServerReply(bfill.position());
    } else {
      //Serial.println("Duplicate request. The received TCP packet has already been served.");
      bfill.emit_p(http_OK_200);
      ether.httpServerReply(bfill.position());
    }
  }
}

bool subnet_mask_valid(IN byte subnet_mask[])
{
  byte i, j, test_mask;
  bool found_zero = false;

  for (i = 0; i < 4; i++)
  {
    test_mask = 0x80; //0b10000000

    for (j = 0; j < 8; j++)
    {
      //Serial.print("test_mask = ");
      //Serial.println(test_mask, HEX);
      /* If the bit is zero */
      if ((subnet_mask[i] & test_mask) == 0)
      {
        //Serial.print("0");
        /* If a zero hasn't been found yet */
        if (!found_zero)
          /* Once we found a 0, the found_zero flag
             is set to true. From now on, the of the
             bits should be zero.
          */
          found_zero = true;

        test_mask = (test_mask >> 1);
      }
      else
      {
        //Serial.print("1");
        /* if we run in this "else" clause
           it means that the current bit is "1"
           So if a zero has already been found,
           return false (not a valid subnet mask)
        */
        if (found_zero)
          return false;

        test_mask = (test_mask >> 1);
      }
    }
    //Serial.print(" ");
  }
  //Serial.println("");

  /* If the execution reached the end, it is a valid subnet mask*/
  return true;
}

void get_hostname_from_http_request(IN char data[],
                                    OUT char hostname[],
                                    IN int hostname_size)
{
  unsigned i;
  /* Whenever a new line is found
     end_of_line becomes true;

     The hostname should start right after
     a '\r\n' in the beginning of a new line.
  */
  bool end_of_line = false;

  for (i = 0; i < strlen(data); i++)
  {
    /* If we are in the middle of a line,
       keep on searching until a \r\n is found.
    */
    if (!end_of_line)
    {
      if (data[i] == '\r')
      {
        if (data[++i] == '\n')
        {
          /* if found, set end_of_line = true*/
          end_of_line = true;
        }
      }
    }
    else
    {
      /* The code runs in this else case whenever a new line is starting */
      end_of_line = false;
      /* If the line starts with 'Host: ', then it should be our line! */
      if (data[i] == 'H' &&
          data[i + 1] == 'o' &&
          data[i + 2] == 's' &&
          data[i + 3] == 't' &&
          data[i + 4] == ':' &&
          data[i + 5] == ' ')
      {
        i += 6;
        int j;
        for (j = 0; j < hostname_size; j++)
        {
          /* So read the hostname and store it in the hostname char array
             until the next line comes,
          */
          if (data[i] == '\r' && data[i + 1] == '\n')
          {
            hostname[j] = '\0';
            return;
          }


          hostname[j] = data[i++];
        }
      }
    }
  }
}

bool eth_link_state_up() {
  return ether.isLinkUp();
}

unsigned int get_TCP_seq(IN byte *ethBuf) {
  unsigned int seq = 0;

  seq = (unsigned int)ethBuf[TCP_SEQ_H_P] << 24 |
        (unsigned int)ethBuf[TCP_SEQ_H_P+1] << 16 |
        (unsigned int)ethBuf[TCP_SEQ_H_P+2] << 8 |
        (unsigned int)ethBuf[TCP_SEQ_H_P+3];

  return seq;
}

void print_macAddress(IN byte mymac[]) {
  for (byte i = 0; i < 6; ++i)
  {
    Serial.print(mymac[i], HEX);
    if (i < 5)
      Serial.print(':');
  }
}
