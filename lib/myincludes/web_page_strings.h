#include "Arduino.h"

//$D = word data type
//$L = long data type
//$S = c string
//$F = progmem string
//$E = byte from the eeprom.
//$T = float type
const char http_OK_200[] PROGMEM =
  "HTTP/1.0 200 OK\r\n"
  "Content-Type: text/html\r\n"
  "Pragma: no-cache\r\n\r\n"
  ;

const char http_unauthorized_401[] PROGMEM =
  "HTTP/1.0 401 Unauthorized\r\n"
  "Content-Type: text/html\r\n\r\n"
  ;

const char http_not_found_404[] PROGMEM =
  "HTTP/1.0 404 Not Found\r\n"
  "Content-Type: text/html\r\n"
  "Pragma: no-cache\r\n\r\n"
  ;

const char webpage_unauthorized[] PROGMEM =
  "<!DOCTYPE HTML>\r\n"
  "<h1>401 Unauthorized</h1>"
  ;

const char webpage_not_found[] PROGMEM =
  "<!DOCTYPE HTML>\r\n"
  "<html><head>\r\n"
  "<title>404 Not Found</title>\r\n"
  "</head><body>\r\n"
  "<h1>Not Found</h1>\r\n"
  "<p>The requested URL /$S was not found on this server.</p>\r\n"
  "<hr>\r\n"
  "<address>Vaguino Web Server at $D.$D.$D.$D</address>\r\n"
  "</body></html>"
  ;

const char webpage_temperature[] PROGMEM =
  "$S=$S <br>"
  ;

const char webpage_ipconfig[] PROGMEM =
  "<!DOCTYPE HTML>\r\n"
  "<html><head>"
  "<title>IP Configuration</title>"
  "<style type=\"text/css\">\r\n"
  "a {"
  "color: #003399;"
  "background-color: transparent;"
  "font-weight: normal;"
  "text-decoration: none;"
  "}\r\n"
  "</style>\r\n"
  "<script src=\"http://code.jquery.com/jquery-latest.min.js\"></script>"
  "<script type=\"text/javascript\">"
  "$$(function() {"
  "enable_cb();"
  "$$(\"#g\").click(enable_cb);"
  "$$(\"input.g\").prop(\"disabled\", $$(\"#g\").prop('checked'));"
  "});"
  "function enable_cb() {"
  "$$(\"input.g\").prop(\"disabled\", this.checked);"
  "}"
  "</script>"
  "</head><body>"
  "<form method=\"post\">"
  "<table>"
  "<tr><td colspan=\"2\"><a href=\"http://$S\">Home</a></td></tr>"
  "<tr><td colspan=\"2\" align=\"right\"><input type=\"checkbox\" name=\"dhcp\" value=\"1\" id=\"g\" $S>Use DHCP</td></tr>"
  "<tr><td align=\"right\">IP Address:</td><td><input type=\"text\" name=\"ip\" class=\"g\" value=\"$D.$D.$D.$D\"></td></tr>"
  "<tr><td align=\"right\">Subnet mask:</td><td><input type=\"text\" name=\"subnet\" class=\"g\" value=\"$D.$D.$D.$D\"></td></tr>"
  "<tr><td align=\"right\">Gateway:</td><td><input type=\"text\" name=\"gw\" class=\"g\" value=\"$D.$D.$D.$D\"></td></tr>"
  "<tr><td align=\"right\">DNS Server:</td><td><input type=\"text\" name=\"dns\" class=\"g\" value=\"$D.$D.$D.$D\"></td></tr>"
  "<tr><td colspan=\"2\" align=\"right\"><input type=\"submit\" value=\"Submit and save\"></td></tr>"
  "</table>"
  "</form>"
  "</body></html>"
  ;

const char webpage_main[] PROGMEM =
  "<!DOCTYPE HTML>\r\n"
  "<html>"
  "<head>\r\n"
  "<title>Vaguino Sous Vide!</title>\r\n"
  "<style type=\"text/css\">\r\n"
  "::selection{ background-color: #E13300; color: white; }\r\n"
  "::moz-selection{ background-color: #E13300; color: white; }\r\n"
  "::webkit-selection{ background-color: #E13300; color: white; }\r\n"
  "body {"
  "background-color: #fff;"
  "margin: 40px;"
  "font: 15px/20px normal Helvetica, Arial, sans-serif;"
  "color: #4F5155;"
  "}\r\n"
  "a {"
  "color: #003399;"
  "background-color: transparent;"
  "font-weight: normal;"
  "text-decoration: none;"
  "}\r\n"
  "h1 {"
  "color: #444;"
  "background-color: transparent;"
  "border-bottom: 1px solid #D0D0D0;"
  "font-size: 19px;"
  "font-weight: normal;"
  "margin: 0 0 14px 0;"
  "padding: 14px 15px 10px 15px;"
  "}\r\n"
  "#contain {"
  "margin: 10px;"
  "border: 1px solid #D0D0D0;"
  "-webkit-box-shadow: 0 0 8px #D0D0D0;"
  "padding: 14px 15px 10px 15px;"
  "}\r\n"
  "p {"
  "margin: 12px 15px 12px 15px;"
  "}\r\n"
  ".boxed { box-shadow: 0 0 0 1px #eeeeee; font-size: 21px; font-weight: bold; color: #5a9bbb;}"
  "</style>\r\n"
  "</head>\r\n"
  "<body>\r\n"
  "<div id=\"contain\">\r\n"
  "<h1>Welcome to the Super Sous Vide Vaguino webserver</font></h1>\r\n"
  "<p><a href=\"http://$S/ipconfig\">IP Configuration</a></p>"
  "<p><a href=\"http://$S/temp\">Sensor Temperatures</a></p>"
  "</div>\r\n"
  "</body>\r\n"
  "</html>"
  ;

const char webpage_please_connect_manually[] PROGMEM =
  "Please connect manually to the newly configured IP address."
  ;
