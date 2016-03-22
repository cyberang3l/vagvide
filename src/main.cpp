/* Common things between files such as generic helper
 * functions and definitions
 */
#include "common.h"
/* Custom network functions */
#include "network.h"
/* Custom functions/data-structs related to the temperature sensors */
#include "temperature.h"
/* All the web page strings that I serve from Arduino */
#include "web_page_strings.h"
/* The PID and PID Autotune library */
#include <PID_v1.h>
#include <PID_AutoTune_v0.h>

/* Includes for the LCD */
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

#define TIME_BETWEEN_MEASUREMENTS 1000 // In milliseconds

#define LCD_I2C_ADDR 0x27
#define LCD_BACKLIGHT_PIN 3
#define LCD_RS_PIN 0
#define LCD_RW_PIN 1
#define LCD_EN_PIN 2
#define LCD_D4_PIN 4
#define LCD_D5_PIN 5
#define LCD_D6_PIN 6
#define LCD_D7_PIN 7

static unsigned int PREV_TCP_SEQ_NUM = 0;

/* PID variables */
/* Define Variables we'll be connecting to */
double PID_Output;
/* Specify the links and initial tuning parameters */
PID SousPID(&current_temperature, &PID_Output, &desired_temperature, 850, 0.5, 0.1, DIRECT);
//PID SousPID(&current_temperature, &PID_Output, &desired_temperature, 2, 5, 1, DIRECT);

LiquidCrystal_I2C lcd(LCD_I2C_ADDR, LCD_EN_PIN, LCD_RW_PIN,
  LCD_RS_PIN, LCD_D4_PIN, LCD_D5_PIN, LCD_D6_PIN,
  LCD_D7_PIN, LCD_BACKLIGHT_PIN, POSITIVE);

// Array to store ethernet interface ip address
// Will be read by NetEEPROM
static byte myip[4];
// Array to store gateway ip address
// Will be read by NetEEPROM
static byte gwip[4];
// Array to store DNS IP Address
// Will be read by NetEEPROM
static byte dnsip[4];
// Array to store netmask
// Will be read by NetEEPROM
static byte netmask[4];
// Array to store ethernet interface mac address
// Will be read by NetEEPROM
static byte mymac[6];

// Used as cursor while filling the buffer
static BufferFiller bfill;

// TCP/IP send and receive buffer
byte Ethernet::buffer[2000];

/***f* setup
 *
 * Default Arduino setup function
 */
void setup() {
  // define the degree symbol

  pinMode(PUSH_BTN_MENU_BACK, INPUT);
  pinMode(PUSH_BTN_MENU_OK, INPUT);
  pinMode(PUSH_BTN_MENU_DOWN, INPUT);
  pinMode(PUSH_BTN_MENU_UP, INPUT);
  pinMode(SSR_PIN, OUTPUT);
  pinMode(PUMPRELAY_PIN, OUTPUT);
  pump_operate(false);
  pinMode(FLOAT_SWITCH_PIN, INPUT);
  ssr_operate(0);

  initTempSensors();

  // To reset the ip configuration, write the value 0 to NET_EEPROM_OFFSET.
  // 0 is the offset
  //EEPROM.write(NET_EEPROM_OFFSET, 0);

  NetEeprom.init(mymac);

  Serial.print("MAC: ");
  print_macAddress(mymac);
  Serial.println();

  while (ether.begin(sizeof Ethernet::buffer, mymac, ETH_SPI_CHIP_SELECT_PIN) == 0)
  {
    Serial.println( "Failed to access Ethernet controller");
    delay(5000);
  }

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

//  while (ether.clientWaitingGw())
//    ether.packetLoop(ether.packetReceive());
//  Serial.println("Gateway found");
//  Serial.println("");
  //tell the PID to range between 0 and the TIME_BETWEEN_MEASUREMENTS
  //SousPID.SetOutputLimits(0, TIME_BETWEEN_MEASUREMENTS);

  //turn the PID on
  SousPID.SetMode(AUTOMATIC);

  lcd.begin(16, 2); //  <<----- My LCD is 16x2, or 20x4
  lcd.createChar(0, degree_symbol); // Store in byte 0 in LCD the degree_symbol (available bytes are 0-7)

  // Switch on the backlight
  lcd.setBacklight(HIGH);
  //lcd.setBacklight(LOW);
  lcd.home (); // go home

  lcd.print("Xinoula Sola PSI!!");
  lcd.setCursor(0, 1);
  lcd.print("Vangelis");
  lcd.setCursor(9, 1);
  lcd.print("C");
  lcd.write(byte(0));
  //lcd.print(char(0)); // Write this symbol
}

void loop() {
  uint8_t float_switch;
  float_switch = digitalRead(FLOAT_SWITCH_PIN);

  // Serial.print("Button OK: ");
  // Serial.println(digitalRead(PUSH_BTN_MENU_OK));
  // Serial.print("Button BACK: ");
  // Serial.println(digitalRead(PUSH_BTN_MENU_BACK));
  // Serial.print("Button UP: ");
  // Serial.println(digitalRead(PUSH_BTN_MENU_UP));
  // Serial.print("Button DOWN: ");
  // Serial.println(digitalRead(PUSH_BTN_MENU_DOWN));
  // delay(500);

  /* If the float_switch is out of the water, then turn off
   * the pump and SSR
   */
  if (!float_switch)
  {
    pump_operate(false);
    ssr_operate(0);
  }

  desired_temperature = 36.6;

  /* If the elapsed time since the last measurement is greater than
   * TIME_BETWEEN_MEASUREMENTS, send a new requestTemperature to all
   * of the sensors. Since we do not wait for conversion in this spot
   * (in the setup function I disable the waitForConversion) in order
   * to avoid blocking the program, before we read the actual
   * temperatures and use them, we have to make sure that the conversion
   * has finished.
   */
  if (timeElapsedSinceLastMeasurement > TIME_BETWEEN_MEASUREMENTS) {
    for (int i = 0; i < numSensors; i++)
      temp_sensor[i].requestTemperatures();
    timeElapsedSinceLastMeasurement = 0;
  }

  /* If the floating switch is on, meaning that the Sous Vide is in the water,
   * only then allow the water pump and the immersion heaters to work.
   */
  if(float_switch) {
    pump_operate(true);
    readAllTemperatures();
    Serial.print("Average temperature: ");
    Serial.println(avg_temperature);
    current_temperature = avg_temperature;
    lcd.setCursor(11, 1);
    lcd.print(current_temperature, DEC);
    SousPID.Compute();
    Serial.print("PID Output: ");
    Serial.println(PID_Output);
    ssr_operate(PID_Output);
    // if (PID_Output && (abs(desired_temperature - current_temperature) > 0.))
    //   pump_operate(true);
    // else
    //   pump_operate(false);
  }
  //}
  /* Copy received packets to data buffer Ethernet::buffer
   * and return the uint16_t Size of received data (which is needed by ether.packetLoop).
   */
  uint16_t len = ether.packetReceive();
  /* Parse received data and return the uint16_t Offset of TCP payload data
   * in data buffer Ethernet::buffer, or zero if packet processed
   */
  uint16_t pos = ether.packetLoop(len);

  if (pos) {
    /* Get the current TCP seq number */
    unsigned int CURRENT_TCP_SEQ_NUM = get_TCP_seq(Ethernet::buffer);

    // Serial.print("PREV TCP SEQ: ");
    // Serial.print(PREV_TCP_SEQ_NUM);
    // Serial.print(", CURRENT_TCP_SEQ_NUM: ");
    // Serial.println(CURRENT_TCP_SEQ_NUM);

    /* Store the received request data in the *data pointer */
    char *data = (char *) Ethernet::buffer + pos;
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
          int i;
          for ( i = 0; i < sizeof(str_temp) - 1; i++ )
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
          len = ether.packetReceive();
          pos = ether.packetLoop(len);
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
