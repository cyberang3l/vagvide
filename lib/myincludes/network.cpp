#include "network.h"

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
