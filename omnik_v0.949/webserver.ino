void handleRoot(const char *db_name, const char *sql) {
  
  String temp;
  int sec = millis() / 1000;
  int min = sec / 60;
  int hr = min / 60;

  temp = "<html><head>\
   <title>Omnik Solar v0.947</title>\
      <style>\
      body { background-color: #cccccc; font-family: Verdana, Helvetica, Sans-Serif; font-size: large; Color: #000088; }\
      <meta name='viewport' content='width=device-width, initial-scale=1'>\
      </style>\
      </head>\
      <body>\
      <h1>Omnik solar v 0.947</h1>";
  
           
            // Display the HTML web page
  //          client.println("<!DOCTYPE html><html>");
  //        client.println("<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">");
  //          client.println("<link rel=\"icon\" href=\"data:,\">");
            // CSS to style the on/off buttons 
    
            // Web Page Heading
            
            temp +="</head>"; 
            temp +="<body>";          
            temp +="<h2>";
            temp +="<table>";
            temp +="<tr>";
            temp +="<th></th>";
            temp +="<th> </th>";
            temp +="<th> </th>";
            temp +="</tr>";
      
            temp +="<tr>";
            // column 1 
            temp +="<td>";           
            temp +="<table style='text-align: left'>";
            temp +="<tr><td>Temperatuur</td></tr>";
            temp +="<tr><td>Spanning</td></tr>";
            temp +="<tr><td>Stroom</td></tr>";
            temp +="<tr><td>Vermogen</td></tr>";         
            temp +="<tr><td>Energie vandaag</td></tr>";
            temp +="<tr><td>Totaal geleverd</td></tr>";   
            temp +="</table>";
            temp +="</td>";
 
            //column 2
            temp +="<td>";           
            temp +="<table style='text-align: right'>";
            temp +="<tr><td>";
            temp +=PVtemp;
            temp +="</td></tr>";
            temp +="<tr><td>";
            temp +=PVvoltagedc;
            temp +="</td></tr>";
            temp +="<tr><td>";
            temp +=PVcurrentdc;
            temp +="</td></tr>";
            temp +="<tr><td>";
            temp +=PVpower;
            temp +="</td></tr>";
            temp +="<tr><td>";
            temp +=PVtoday;
            temp +="</td></tr>";
            temp +="<tr><td>";
            temp +=PVtotal;
            temp +="</td></tr>";
            temp +="</table>";
            temp +="</td>";

          //column 3
            temp +="<td>";           
            temp +="<table style='text-align: left'>";
            temp +="<tr><td>";
            temp +="C";
            temp +="</td></tr>";
            temp +="<tr><td>";
            temp +="V";
            temp +="</td></tr>";
            temp +="<tr><td>";
            temp +="A";
            temp +="</td></tr>";
            temp +="<tr><td>";
            temp +="W";
            temp +="</td></tr>";
            temp +="<tr><td>";
            temp +="KWh";
            temp +="</td></tr>";
            temp +="<tr><td>";
            temp +="KWh";
            temp +="</td></tr>";
            temp +="</table>";
            temp +="</td>";

            temp +="</tr>";
            
            temp +="<tr>";          
   
            temp +="</table>";
            temp += "</h2>";
            temp +="</body></html>";
            
 /*
            
           temp += "<form name='params' method='POST' action='exec_sql'>\
      <textarea style='font-size: medium; width:100%' rows='4' placeholder='Enter SQL Statement' name='sql'>";
  if (sql != NULL)
    temp += sql;
  temp += "</textarea> \
      <br>File name (prefix with /spiffs/ or /sd/ or /sdcard/):<br/><input type=text size='50' style='font-size: small' value='";
  if (db_name != NULL)
    temp += db_name;
  temp += "' name='db_name'/> \
      <br><br><input type=submit style='font-size: large' value='Execute'/>\
      </form><hr/>";
*/
  wserver.send ( 200, "text/html", temp.c_str());
  db_exec("select * from pvdata where pvtime1 = date('now','localtime');");
  db_exec("select datetime('now','localtime')");
  update_display();
}

void handleNotFound() {
  
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += wserver.uri();
  message += "\nMethod: ";
  message += ( wserver.method() == HTTP_GET ) ? "GET" : "POST";
  message += "\nArguments: ";
  message += wserver.args();
  message += "\n";

  for ( uint8_t i = 0; i < wserver.args(); i++ ) {
      message += " " + wserver.argName ( i ) + ": " + wserver.arg ( i ) + "\n";
  }

  wserver.send ( 404, "text/plain", message );
}



int openDb(const char *filename) {
  if (strncmp(filename, current_db, sizeof(current_db)) == 0)
    return 0;
  else
    sqlite3_close(db);
  int rc = sqlite3_open(filename, &db);
  if (rc) {
      Serial.printf("Can't open database: %s\n", sqlite3_errmsg(db));
      memset(current_db, '\0', sizeof(current_db));
      return rc;
  } else {
      Serial.printf("Opened database successfully\n");
      strcpy(current_db, filename);
  }
  return rc;
}
