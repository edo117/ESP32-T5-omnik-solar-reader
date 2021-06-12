
// tcp verkeer 81 
// webserver reparatie nodig 
// info van 81 overzetten naar 80
// versie 0.944
// 
// versie 947
// display PV items
// versie 948 
// epaper versie server naar nieuw ip adres

#include <config_ext.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sqlite3.h>
#include "SPI.h"
#include "FS.h"
#include "SPIFFS.h"
#include "SD_MMC.h"
#include "SD.h"
#include <GxEPD.h>
#include <GxDEPG0213BN/GxDEPG0213BN.h>    // 2.13" b/w  form DKE GROUP
// FreeFonts from Adafruit_GFX
#include <Fonts/FreeMonoBold9pt7b.h>
#include <Fonts/FreeMonoBold12pt7b.h>
#include <Fonts/FreeMonoBold18pt7b.h>
#include <Fonts/FreeMonoBold24pt7b.h>

#include <GxIO/GxIO_SPI/GxIO_SPI.h>
#include <GxIO/GxIO.h>

#define SPI_MOSI 23
#define SPI_MISO -1
#define SPI_CLK 18

#define ELINK_SS 5
#define ELINK_BUSY 4
#define ELINK_RESET 16
#define ELINK_DC 17
// webserver includes
#include <WiFiClient.h>
#include <WebServer.h>
#include <ESPmDNS.h>
int wrc;
sqlite3_stmt *res;
int rec_count = 0;
const char *tail;
char current_db[255];
const char *skuNum = "Bysoft Omnik v0.948";
bool sdOK = false;
int startX = 40, startY = 10;

#define FORMAT_SPIFFS_IF_FAILED true
#define MAX_FILE_NAME_LEN 100
#define MAX_STR_LEN 500
#include <WiFi.h>
#include "time.h"
#include <WiFiUdp.h>
// Set web server port number to 80

WiFiServer server(81);
WebServer wserver(80);
  
// Variables to save date and time
String formattedDate;
String dayStamp;
String timeStamp;
struct tm timeinfo;
// Variable to store the HTTP request

String header = "";
String PVid= " ";
String PVpower =" ";
String PVtoday = " ";
String PVtotal = " ";
String PVtemp = " ";
String PVvoltagedc = " ";
String PVcurrentdc = " ";


char db_file_name[MAX_FILE_NAME_LEN] = "\0";
char sqlstring[MAX_STR_LEN];
sqlite3 *db = NULL;
int rc;

bool first_time = false;
// Variables to save date and time
const char* ntpServer = "pool.ntp.org";
const long  gmtOffset_sec = 3600;
const int   daylightOffset_sec = 3600;

#define FORMAT_SPIFFS_IF_FAILED true
#define MAX_FILE_NAME_LEN 100
#define MAX_STR_LEN 500
#define SDCARD_SS 13
#define SDCARD_CLK 14
#define SDCARD_MOSI 15
#define SDCARD_MISO 2

#define BUTTON_PIN 39


GxIO_Class io(SPI, /*CS=5*/ ELINK_SS, /*DC=*/ ELINK_DC, /*RST=*/ ELINK_RESET);
GxEPD_Class display(io, /*RST=*/ ELINK_RESET, /*BUSY=*/ ELINK_BUSY);

SPIClass sdSPI(VSPI);

void setup() {
  Serial.begin(115200);
  Serial.println();
  Serial.println("setup");
  
  //  SPI.begin();
  //  display.init(); // enable diagnostic output on Serial
  SPI.begin(SPI_CLK, SPI_MISO, SPI_MOSI, ELINK_SS);
  display.init(); // enable diagnostic output on Serial

  display.setRotation(1);
  display.fillScreen(GxEPD_WHITE);
  display.setTextColor(GxEPD_BLACK);
  display.setFont(&FreeMonoBold9pt7b);
  display.setCursor(0, 0);

  sdSPI.begin(SDCARD_CLK, SDCARD_MISO, SDCARD_MOSI, SDCARD_SS);

  if (!SD.begin(SDCARD_SS, sdSPI)) {
    sdOK = false;
  } else {
    sdOK = true;
  }

  if (sdOK) {
    uint32_t cardSize = SD.cardSize() / (1024 * 1024);
    display.println("SDCard:" + String(cardSize) + "MB");
  } else {
    display.println("SDCard  None");
  }
  
  display.update();
  
  // Connect to Wi-Fi network with SSID and password
  if (!SPIFFS.begin(FORMAT_SPIFFS_IF_FAILED)) {
    Serial.println(F("Failed to mount file Serial"));
    return;
  }
  Serial.print("Connecting to network");
  
  WiFi.begin("********", "*********");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  // Print local IP address and start web server
  Serial.println("");
  Serial.println("WiFi connected.");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  
  server.begin();
  wserver.begin();
  
  Serial.println ( "HTTP server started" );
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  printLocalTime();
  //SPI.begin(SPI_CLK, SPI_MISO, SPI_MOSI, ELINK_SS);
  //SD_MMC.begin();
  //SD.begin();
  sqlite3_initialize();
  db_open();
  if ( MDNS.begin ( "esp32" ) ) {
      Serial.println ( "MDNS responder started" );
  }
//
//  memset(current_db, '\0', sizeof(current_db));


  wserver.on ( "/", []() {
    handleRoot(NULL, NULL);
  });
  wserver.on ( "/exec_sql", []() {
      Serial.println("om sql");
      String db_name = wserver.arg("/sd/pvomnik");
      String sql = wserver.arg("select * from pvdata");
      wserver.setContentLength(CONTENT_LENGTH_UNKNOWN);
      handleRoot(db_name.c_str(), sql.c_str());
      

      rc = sqlite3_prepare_v2(db,sqlstring, 1000, &res, &tail);
      if (rc != SQLITE_OK) {
          String resp = "Failed to fetch data: ";
          resp += sqlite3_errmsg(db);
          resp += "<br><br><a href='/'>back</a>";
          wserver.sendContent(resp);
          Serial.println(resp.c_str());
          return;
      }

      rec_count = 0;
      String resp = "<h2>Result:</h2><h3>";
      resp += sql;
      resp += "</h3><table cellspacing='1' cellpadding='1' border='1'>";
      wserver.sendContent(resp);
      bool first = true;
      while (sqlite3_step(res) == SQLITE_ROW) {
          resp = "";
          if (first) {
            int count = sqlite3_column_count(res);
            if (count == 0) {
                resp += "<tr><td>Statement executed successfully</td></tr>";
                rec_count = sqlite3_changes(db);
                break;
            }
            resp += "<tr>";
            for (int i = 0; i<count; i++) {
                resp += "<td>";
                resp += sqlite3_column_name(res, i);
                resp += "</td>";
            }
            resp += "</tr>";
            first = false;
          }
          int count = sqlite3_column_count(res);
          resp += "<tr>";
          for (int i = 0; i<count; i++) {
              resp += "<td>";
              resp += (const char *) sqlite3_column_text(res, i);
              resp += "</td>";
          }
          resp += "</tr>";
          wserver.sendContent(resp);
          rec_count++;
      }
      resp = "</table><br><br>Number of records: ";
      resp += rec_count;
      resp += ".<br><br><input type=button onclick='location.href=\"/\"' value='back'/>";
      wserver.sendContent(resp);
      sqlite3_finalize(res);
  } );
  wserver.onNotFound ( handleNotFound );
}
//////////////////////////////////////////////////////////////////////////
void loop(){
////////////////////////////////////////////////////////////////////////
  wserver.handleClient();
  if(!getLocalTime(&timeinfo)){
    Serial.println("Failed to obtain time");
    return;
  } 
    WiFiClient client = server.available(); 
    
    
// Listen for incoming clients
// basic readout test, just print the current temp
  
  if (client) {                             // If a new client connects,
    update_display();
    
    Serial.println("New Client.");          // print a message out in the serial port
    String currentLine = "";                // make a String to hold incoming data from the client
    while (client.connected()) {            // loop while the client's connected
      if (client.available()) {             // if there's bytes to read from the client,
        
      char c = client.read();             // read a byte, then
        
      Serial.write(c);                    // print it out the serial monitor
      header += c; 
 
        if (c == '\n') {                    // if the byte is a newline character
          // if the current line is blank, you got two newline characters in a row.
          // that's the end of the client HTTP request, so send a response:
          if (currentLine.length() == 0) {
            // HTTP headers always start with a response code (e.g. HTTP/1.1 200 OK)
            // and a content-type so the client knows what's coming, then a blank line:
            client.println("HTTP/1.1 200 OK");
            client.println("Content-type:text/html");
            client.println("Connection: close");
            client.println();
            
            db_exec("select * from pvdata where pvtime1 = date('now','localtime');");
            db_exec("select datetime('now','localtime')");

           
            // Display the HTML web page
            client.println("<!DOCTYPE html><html>");
            client.println("<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">");
            client.println("<link rel=\"icon\" href=\"data:,\">");
            // CSS to style the on/off buttons 
          
            // Web Page Heading
            
            client.println("</head>"); 
            client.println("<body>");          
           
            client.println("<table>");
            client.println("<tr>");
            client.println("<th></th>");
            client.println("<th> </th>");
            client.println("<th> </th>");
            client.println("</tr>");
      
            client.println("<tr>");
            // column 1 
            client.println("<td>");           
            client.println("<table style='text-align: left'>");
            client.println("<tr><td>PV temp</td></tr>");
            client.println("<tr><td>PV voltagedc</td></tr>");
            client.println("<tr><td>PV currentdc</td></tr>");
            client.println("<tr><td>PV power</td></tr>");         
            client.println("<tr><td>PV today</td></tr>");
            client.println("<tr><td>PV total</td></tr>");   
            client.println("</table>");
            client.println("</td>");
 
            //column 2
            client.println("<td>");           
            client.println("<table style='text-align: right'>");
            client.print("<tr><td>");
            client.print(PVtemp);
            client.println("</td></tr>");
            client.print("<tr><td>");
            client.print(PVvoltagedc);
            client.println("</td></tr>");
            client.print("<tr><td>");
            client.print(PVcurrentdc);
            client.println("</td></tr>");
            client.print("<tr><td>");
            client.print(PVpower);
            client.println("</td></tr>");
            client.print("<tr><td>");
            client.print(PVtoday);
            client.println("</td></tr>");
            client.print("<tr><td>");
            client.print(PVtotal);
            client.println("</td></tr>");
            client.println("</table>");
            client.println("</td>");

          //column 3
            client.println("<td>");           
           client.println("<table style='text-align: left'>");
            client.print("<tr><td>");
            client.print("C");
            client.println("</td></tr>");
            client.print("<tr><td>");
            client.print("V");
            client.println("</td></tr>");
            client.print("<tr><td>");
            client.print("A");
            client.println("</td></tr>");
            client.print("<tr><td>");
            client.print("W");
            client.println("</td></tr>");
            client.print("<tr><td>");
            client.print("KWh");
            client.println("</td></tr>");
            client.print("<tr><td>");
            client.print("KWh");
            client.println("</td></tr>");
            client.println("</table>");
            client.println("</td>");

                        
            client.println("</tr>");
            
            client.println("<tr>");          
   
            client.println("</table>");
            client.println("</body></html>");
            
            // The HTTP response ends with another blank line
            client.println();
            // Break out of the while loop
            break;
          } else { // if you got a newline, then clear currentLine
            currentLine = "";
          }
        } else if (c != '\r') {  // if you got anything else but a carriage return character,
          currentLine += c;      // add it to the end of the currentLine
        }
      }
    }
    // Clear the header variable
    // Close the connection
    // client.stop();
    // bereken waarden als bericht van PV komt
    if (header.indexOf("NLDN202013C94059") > 0) {
      PVid          = header.substring(15,30);
      PVpower       = header[60] + (header[59] * 256);
      PVtoday       = (header[70] + (header[69] * 256))/100;
      PVtotal       = (header[74] + (header[73] * 256) + (header[72] * 65536) + (header[71] * 16777216))/10;
      PVtemp        = (header[32] + (header[31] * 256))/10;
      PVvoltagedc   = (header[34] + (header[33] * 256))/10;
      PVcurrentdc   = (header[40] + (header[39] * 256))/10;
      writedb();
    }
    Serial.println(PVid);
    Serial.print("Power : ");
    Serial.print(PVpower);Serial.println(" W");
    Serial.print("E-today : ");
    Serial.print(PVtoday); Serial.println(" KWh");
    Serial.print("E-total : ");Serial.print(PVtotal); Serial.println(" KWh");   
    Serial.println("Client disconnected.");
    
    header = "";   
  }
   client.stop();     
}
void writedb(){
  //
    String mysql="insert into pvdata(PVTIME, PVTIME1, PVPOWER, PVTODAY, PVTOTAL, PVTEMP, PVVOLTAGEDC, PVCURRENTDC) VALUES (datetime('now','localtime'), date('now','localtime'),";
  mysql = mysql + PVpower + ", " + PVtoday + "," + PVtotal +", " + PVtemp + ", " + PVvoltagedc + ", " + PVcurrentdc + ");";
  Serial.print("SQL: ");
  Serial.println(mysql);
  
  mysql.toCharArray(sqlstring,mysql.length());
 // sqlstring.concat(PVpower);
 // sqlstring.concat("));");
  db_exec(sqlstring);
  //db_exec("insert into pvdata(PVTIME, PVPOWER) VALUES (datetime('now','localtime'), ");
  db_exec("select * from pvdata where pvtime1 = date('now','localtime');");
  db_exec("select datetime('now','localtime')");

}
void printLocalTime(){
  
  if(!getLocalTime(&timeinfo)){
    Serial.println("Failed to obtain time");
    return;
  }
  Serial.println(&timeinfo, "%F %T");
  
}
void update_display() {
    display.fillScreen(GxEPD_WHITE);
    //display.setCursor(1, 11);
    //display.println(skuNum);
    display.setCursor(1, 11);
    display.println(&timeinfo, "%F %T");
    display.setCursor(1, 29);
    display.print("Temperatuur: ");
    display.print(PVtemp);display.print(" C");
    display.setCursor(1, 47);
    display.print("Spanning   : ");
    display.print(PVcurrentdc);display.print(" V");
    display.setCursor(1, 65);
    display.print("Stroom     : ");
    display.print(PVcurrentdc);display.print(" A");
    display.setCursor(1, 83);
    display.print("Vandaag    : ");
    display.print(PVtoday);display.print(" KWh"); 
    display.setCursor(1, 101);
    display.print("Totaal     : ");
    display.print(PVtotal);display.print(" KWh");
    

    display.setCursor(display.width()   / 2 - 40, display.height() - 10);

  //if (sdOK) {
    //uint32_t cardSize = SD.cardSize() / (1024 * 1024);
    //display.println("SDCard:" + String(cardSize) + "MB");
  //} else {
  //  display.println("SDCard  None");
  //} 
  display.update();  
  }
