/*
 * 
 * settare filtro scs per non ricevere ack e msg doppi
 * 
 * 
  UDPknxgate - gateway between KNXgate/SCSgate and ethernet UDP application
  V 2.0 - per solo SCS risponde anche a comandi HTTP, tipo:

                      http://192.168.2.230/gate.htm?type=12&from=01&to=31&cmd=01&resp=y

          la porta di ricezione Ã¨ quella prevista in configurazione
          la pagina di comando si chiama gate.htm
          i parametri possibili sono  type=   12 (comando, byte 4 del telegramma) default 12
                                      from=   NN  mio indirizzo hex - default 01 hex
                                      to=     NN  indirizzo hex del device destinatario
                                      cmd=    NN  hex comando da mandare
                                      resp=   y     "y" request to send a response, otherwise no response
                                      
          Ã¨ possibile utilizzare una finestra di aiuto con
                      http://192.168.2.230/request

          se si usa l'opzione resp=y attiva il log breve e per ogni telegramma di ritorno (l=4) effettua una chiamata al client:
          http://<ipaddress>:8080/json.htm?type=command&param=udevices&script=scsgate_json.lua&type=<byte[3]>&from=<byte[2]>&to=<byte[1]>&cmd=<byte[4]>
          è parametrizzabile la parte:
                            :8080/json.htm?type=command&param=udevices&script=scsgate_json.lua
                            (esempio conforme a DOMOTICZ e script  SCSGAtE_JSON.LUA


          serial DEBUG: 4abcd   a destinazione    b provenienza    c tipo    d comando 
                        42220   off
                        42221   on
                        42222   more
                        42223   less
                        43334   alza
                        43335   abbassa
                      
           
  V 1.6 - reset/restart se non aggancia il router con partenza normale (senza jumper) - OBBLIGATORIO IL JUMPER PER PROGRAMMARE *********************
  V 1.5 - aggiunta parametrizzazione numero porta udp da usare (default 52056)
  V 1.4 - ottimizzazione tempi di attesa
  V 1.3 - in modalitÃ  di connessione client NON attiva il server http per migliorare i tempi di risposta
*/
/*
#define _SW_VERSION "VER 2.0"

#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ESP8266HTTPClient.h>
//#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <fs.h>
#include <EEPROM.h>

extern "C"
{
#include "user_interface.h"
}

// =======================================================================================================================
#define DEBUG
//#define DEBUG_UDP
//#define NO_WEB_CLIENT
//#define ECHO
//#define KNX
#define SCS
#define HTTP_PORT 80
// =======================================================================================================================

#define INNERWAIT 80  // inner loop delay
#define OUTERWAIT 100 // outer loop delay

#ifdef KNX
#define _MODO "KNX"
#define _modo "knx"
#endif

#ifdef SCS
#define _MODO "SCS"
#define _modo "scs"
#endif

char echoOn = 0;

ESP8266WebServer server(HTTP_PORT);
HTTPClient httpClient;

bool testWifi(void);
void setupAP(void);
void launchWeb(int webtype);
void createWebServer(int webtype);

const char *ssid = "ESP_" _MODO "GATE";
const char *passphrase = _modo "gate1";
IPAddress local_ip(0, 0, 0, 0);
IPAddress router_ip(0, 0, 0, 0);
IPAddress remote_ip;
unsigned int remote_port;
char webon = 0;
char forceAP = 0;
String st;
String content;
String callback;
int statusCode;
char udpopen;
String httpResp = ""; // resp= nessuna risposta      resp=i conferma immediata       resp=a risposta "get" con caratteri SCSGATE      resp=y  entrambe

char packetBuffer[255];
char requestBuffer[32];
unsigned char requestLen;
char replyBuffer[255];
unsigned char replyLen;
unsigned char firstTime = 0;

unsigned int localPort = 52056;
WiFiUDP port;
int dots;
int lines;

// -----------------------------------------------------------------------------------------------------------------------------------------------
#define E_SSID 0
#define E_PASSW 32
#define E_IPADDR 100
#define E_ROUTIP 104
#define E_PORT 108
#define E_CALLBACK 127
// -----------------------------------------------------------------------------------------------------------------------------------------------
bool testWifi(void)
{
	int c = 0;
#ifdef DEBUG
	Serial.println("Waiting for Wifi to connect");
#endif
	while (c < 22)
	{
		if (WiFi.status() == WL_CONNECTED)
		{
#ifdef DEBUG
			Serial.println("Connected!");
#endif
			return true;
		}
		delay(500);
#ifdef DEBUG
		Serial.print(WiFi.status());
#endif
		c++;
	}
#ifdef DEBUG
	Serial.println("");
	Serial.println("Connect timed out, opening AP");
#endif
	return false;
}

// -----------------------------------------------------------------------------------------------------------------------------------------------
void launchWeb(int webtype)
{ // webtype=1 : AP       webtype=0 : WIFI connected to router
#ifdef DEBUG
	Serial.println("");
	Serial.println("WiFi connected");
	Serial.print("Local IP: ");
	Serial.println(WiFi.localIP());
	Serial.print("SoftAP IP: ");
	Serial.println(WiFi.softAPIP());
#endif

	if (webtype == 0)
	{
		//      delete server;
		//      server = new ESP8266Webserver(localport);
	}

#ifdef NO_WEB_CLIENT
	if (webtype == 1)
#endif
	{
		createWebServer(webtype);
		// Start the server
		server.begin();
		webon = 1;
#ifdef DEBUG
		Serial.println("Server started");
#endif
	}
}

// -----------------------------------------------------------------------------------------------------------------------------------------------
void setupAP(void)
{
	WiFi.mode(WIFI_STA);
	WiFi.disconnect();
	delay(100);
	int n = WiFi.scanNetworks();
#ifdef DEBUG
	Serial.println("scan done");
	if (n == 0)
		Serial.println("no networks found");
	else
	{
		Serial.print(n);
		Serial.println(" networks found");
		for (int i = 0; i < n; ++i)
		{
			// Print SSID and RSSI for each network found
			Serial.print(i + 1);
			Serial.print(": ");
			Serial.print(WiFi.SSID(i));
			Serial.print(" (");
			Serial.print(WiFi.RSSI(i));
			Serial.print(")");
			Serial.println((WiFi.encryptionType(i) == ENC_TYPE_NONE) ? " " : "*");
			delay(10);
		}
	}
	Serial.println("");
#endif
	st = "<ol>";
	for (int i = 0; i < n; ++i)
	{
		// Print SSID and RSSI for each network found
		st += "<li>";
		st += WiFi.SSID(i);
		st += " (";
		st += WiFi.RSSI(i);
		st += ")";
		st += (WiFi.encryptionType(i) == ENC_TYPE_NONE) ? " " : "*";
		st += "</li>";
	}
	st += "</ol>";
	delay(100);

	WiFi.mode(WIFI_AP);
	WiFi.softAP(ssid, passphrase, 8);
#ifdef DEBUG
	Serial.println("softap");
#endif
	launchWeb(1);
#ifdef DEBUG
	Serial.println("open UDP port");
	Serial.println("UDP server started");
#endif
	port.begin(localPort);
	udpopen = 1;
#ifdef DEBUG
	Serial.println("over");
#endif
}

// =============================================================================================
void handleNotFound()
{
	String message = "File non trovato\n\n";
	message += "URI: ";
	message += server.uri();
	message += "\nMethod: ";
	message += (server.method() == HTTP_GET) ? "GET" : "POST";
	message += "\nArguments: ";
	message += server.args();
	message += "\n";
	for (uint8_t i = 0; i < server.args(); i++)
	{
		message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
	}
	server.send(404, "text/plain", message);
}

// =============================================================================================
char aConvert(String aData)
{
	char str[4];
	char *ptr;
	long ret;
	aData.toCharArray(str, 4);
	str[2] = 0;
	ret = strtoul(str, &ptr, 16);
	return (char)ret;
}
// =============================================================================================
void handleRequest()
{
	IPAddress ip = WiFi.softAPIP();

#ifdef SCS
	content = "<!DOCTYPE HTML>\r\n<html>Hello from ESP_" _MODO "GATE " _SW_VERSION;
	content += "<p>";
	//  content += st;
	content += "</p><form method='get' action='gate'>\
    <label>Req Type: </label><input name='type' maxlength=2 value='12' style='width:25px'>\
    <label>From: </label><input name='from' maxlength=2 value='01' style='width:25px'>\
    <label>to:</label><input name='to' maxlength=2 style='width:25px'>\
    <label>command</label><input name='cmd' maxlength=2 style='width:25px'>\
    <label>reply</label><input name='resp' maxlength=1 value='y' style='width:15px'>\
    <input type='submit'></form>";
	content += "</html>";
	server.send(500, "text/html", content);
#endif
}
// =============================================================================================
void handleGate() //  ipaddress/request?type=x&from=xx&to=yy&cmd=zz&resp=y
//          i parametri possibili sono  type=   12 (comando, byte 4 del telegramma) default 12
//                                      from=   NN  mio indirizzo hex - default 01 hex
//                                      to=     NN  indirizzo hex del device destinatario
//                                      cmd=    NN  hex comando da mandare
//                                      resp=   y     "y" request to send a response, otherwise no response
{

	//    if(!server.authenticate(www_username, www_password))
	//        return server.requestAuthentication();

#ifdef SCS
	String type;
	char ctype;
	String from;
	char cfrom;
	String to;
	char cto;
	String cmd;
	char ccmd;

	//    remote_ip = {127,0,0,1}; // server.remoteIP();
	WiFiClient client = server.client();
	//    if ((client.available()) && (client.connected()))
	if (client.connected())
	{
#ifdef DEBUG
		Serial.println("");
		Serial.print("New client:");
		Serial.print(client.remoteIP());
		Serial.print(" port:");
		Serial.println(client.remotePort());
#endif
		remote_ip = client.remoteIP();
		remote_port = client.remotePort();
	}

	if (server.hasArg("type"))
	{
		type = server.arg("type");
		ctype = aConvert(type);
		if (ctype == 0)
			ctype = 0x12;
	}
	else
		ctype = 0x12;
	if (server.hasArg("from"))
	{
		from = server.arg("from");
		cfrom = aConvert(from);
	}
	else
		cfrom = 0x00;
	if (server.hasArg("to"))
	{
		to = server.arg("to");
		cto = aConvert(to);
	}
	else
		cto = 0x00;
	if (server.hasArg("cmd"))
	{
		cmd = server.arg("cmd");
		ccmd = aConvert(cmd);
	}
	else
		ccmd = 0x01;

	if (server.hasArg("resp"))
	{
		httpResp = server.arg("resp");
	}
	else
		httpResp = "";

#ifdef DEBUG
	Serial.println(" ");
	Serial.print(" t=");
	if (server.hasArg("type"))
	{
		Serial.print(type);
		Serial.print(":");
	}
	Serial.println(ctype, HEX);

	Serial.print(" f=");
	if (server.hasArg("from"))
	{
		Serial.print(from);
		Serial.print(":");
	}
	Serial.println(cfrom, HEX);

	Serial.print(" a=");
	if (server.hasArg("to"))
	{
		Serial.print(to);
		Serial.print(":");
	}
	Serial.println(cto, HEX);

	Serial.print(" c=");
	if (server.hasArg("cmd"))
	{
		Serial.print(cmd);
		Serial.print(":");
	}
	Serial.println(ccmd, HEX);
#endif
	{
		if ((firstTime == 0) && ((httpResp == "y") || (httpResp == "a")))
		{
			requestBuffer[0] = '@';
			requestBuffer[1] = 0x15; // evita memo in eeprom

			requestBuffer[2] = '@';
			requestBuffer[3] = 'M';
			requestBuffer[4] = 'X';
			requestBuffer[5] = ' ';

			requestBuffer[6] = '@';
			requestBuffer[7] = 'Y';
			requestBuffer[8] = '1';
			requestBuffer[9] = ' ';

			requestBuffer[10] = '@';
			requestBuffer[11] = 'F';
			requestBuffer[12] = '3';

			requestBuffer[13] = ' ';

			requestBuffer[14] = '@';
			requestBuffer[15] = 'l';

			requestBuffer[16] = '@';
			requestBuffer[17] = 'y';
			requestBuffer[18] = cto;
			requestBuffer[19] = cfrom;
			requestBuffer[20] = ctype;
			requestBuffer[21] = ccmd;
			requestLen = 22;
			firstTime = 1;
		}
		else
		{
			requestBuffer[0] = '@';
			requestBuffer[1] = 'y';
			requestBuffer[2] = cto;
			requestBuffer[3] = cfrom;
			requestBuffer[4] = ctype;
			requestBuffer[5] = ccmd;
			requestLen = 6;
		}
		if ((httpResp == "y") || (httpResp == "i"))
		{
			content = "{\"status\":\"OK\"}";
			statusCode = 200;
		
			server.send(statusCode, "text/html", content);
		}
	}
#endif
}
// =============================================================================================
void handleScan()
{
	IPAddress ip = WiFi.softAPIP();
	String ipStr = String(ip[0]) + '.' + String(ip[1]) + '.' + String(ip[2]) + '.' + String(ip[3]);
	content = "<!DOCTYPE HTML>\r\n<html>Hello from ESP_" _MODO "GATE " _SW_VERSION " at ";
	content += ipStr;
	content += "<p>";
	content += st;
	content += "</p><form method='get' action='setting'><label>SSID: </label><input name='ssid' length=32>\
    <label>PSW: </label><input name='pass' length=64><label>IP address:<input name='ip' length=16></label>\
    <label>Gateway IP<input name='rip' length=16></label></label><label>UDP port<input name='uport' length=5></label>\
    <input type='submit'></form>";
	content += "</html>";
	server.send(200, "text/html", content);
}
// =============================================================================================
void handleSetting()
{
	String qsid = server.arg("ssid");
	String qpass = server.arg("pass");
	String qip = server.arg("ip");
	String qrip = server.arg("rip");
	String qport = server.arg("uport");
	local_ip.fromString(qip);
	router_ip.fromString(qrip);
	localPort = qport.toInt();
	if (localPort == 0)
		localPort = 52056;
	if (qsid.length() > 0 && qpass.length() > 0)
	{
#ifdef DEBUG
		Serial.println("clearing eeprom");
#endif
		for (int i = 0; i < 96; ++i)
		{
			EEPROM.write(i, 0);
		}
#ifdef DEBUG
		Serial.println(qsid);
		Serial.println("");
		Serial.println(qpass);
		Serial.println("");
		Serial.println("writing eeprom ssid:");
#endif
		for (int i = 0; i < qsid.length(); ++i)
		{
			EEPROM.write(E_SSID + i, qsid[i]);
			//              Serial.print("Wrote: ");
			//              Serial.println(qsid[i]);
		}
#ifdef DEBUG
		Serial.println("writing eeprom pass:");
#endif
		for (int i = 0; i < qpass.length(); ++i)
		{
			EEPROM.write(E_PASSW + i, qpass[i]);
			//              Serial.print("Wrote: ");
			//              Serial.println(qpass[i]);
		}
#ifdef DEBUG
		Serial.println("writing ipaddress:");
		String ipStr1;
#endif
		for (int i = 0; i < 4; ++i)
		{
			EEPROM.write(E_IPADDR + i, local_ip[i]);
#ifdef DEBUG
			ipStr1 = String(local_ip[i]) + '.';
			Serial.println(ipStr1);
#endif
		}

#ifdef DEBUG
		Serial.println("writing gateway ip:");
#endif
		for (int i = 0; i < 4; ++i)
		{
			EEPROM.write(E_ROUTIP + i, router_ip[i]);
#ifdef DEBUG
			ipStr1 = String(router_ip[i]) + '.';
			Serial.println(ipStr1);
#endif
		}
		EEPROM.write(E_PORT, highByte(localPort));
		EEPROM.write(E_PORT + 1, lowByte(localPort));

		EEPROM.commit();
		content = "{\"Success\":\"saved to eeprom... reset to boot into new wifi\"}";
		statusCode = 200;
	}
	server.send(statusCode, "application/json", content);
}
// =============================================================================================
void handleRoot()
{
	IPAddress ip = WiFi.localIP();
	String ipStr = String(ip[0]) + '.' + String(ip[1]) + '.' + String(ip[2]) + '.' + String(ip[3]);
	//    server.send(200, "application/json", "{\"IP\":\"" + ipStr + "\"}");
	content = "<!DOCTYPE HTML>\r\n<html>Hello from ESP_" _MODO "GATE " _SW_VERSION " at ";
	content += ipStr;
	content += "</html>";
	server.send(200, "text/html", content);
}
// =============================================================================================
void handleClear()
{
	content = "<!DOCTYPE HTML>\r\n<html>";
	content += "<p>Clearing the EEPROM - reboot.</p></html>";
	server.send(200, "text/html", content);
#ifdef DEBUG
	Serial.println("clearing eeprom");
#endif
	for (int i = 0; i < 108; ++i)
	{
		EEPROM.write(i, 0);
	}
	EEPROM.commit();
	WiFi.disconnect(); //////////
}

// =============================================================================================
void handleCallback()
{
	content = "<!DOCTYPE HTML>\r\n<html>Hello from ESP_" _MODO "GATE " _SW_VERSION "<p>";
	//    content += st;
	content += "</p><form method='get' action='backsetting'><label>Callback request: </label><input name='callback' length=128 maxlength=128 style='width:800px'>\
    <input type='submit'></form>";
	content += "</html>";
	server.send(200, "text/html", content);
}
// =============================================================================================
void handleBackSetting()
{
	callback = server.arg("callback");

#ifdef DEBUG
	Serial.print("req=");
	Serial.println(callback);
#endif
	int i;
	for (i = 0; i < callback.length(); ++i)
	{
		EEPROM.write(E_CALLBACK + i, callback[i]);
	}
	EEPROM.write(E_CALLBACK + i, 0);
	EEPROM.commit();

	content = "{\"Status\":\"OK\"}";
	statusCode = 200;

	server.send(statusCode, "application/json", content);
}
// =============================================================================================

// =============================================================================================
void handleTest1()
{
	char temp[400];
	int sec = millis() / 1000;
	int min = sec / 60;
	int hr = min / 60;

	snprintf(temp, 400,

			 "<html>\
              <head>\
                <meta http-equiv='refresh' content='5'/>\
                <title>ESP8266 Demo</title>\
                <style>\
                  body { background-color: #cccccc; font-family: Arial, Helvetica, Sans-Serif; Color: #000088; }\
                </style>\
              </head>\
              <body>\
                <h1>Hello from ESP8266!</h1>\
                <p>Uptime: %02d:%02d:%02d</p>\
                <img src=\"/test.svg\" />\
              </body>\
            </html>",

			 hr, min % 60, sec % 60);
	server.send(200, "text/html", temp);
}
// =============================================================================================
void handleTest2()
{
	String out = "";
	char temp[100];
	out += "<svg xmlns=\"http://www.w3.org/2000/svg\" version=\"1.6\" width=\"400\" height=\"150\">\n";
	out += "<rect width=\"400\" height=\"150\" fill=\"rgb(250, 230, 210)\" stroke-width=\"1\" stroke=\"rgb(0, 0, 0)\" />\n";
	out += "<g stroke=\"black\">\n";
	int y = rand() % 130;
	for (int x = 10; x < 390; x += 10)
	{
		int y2 = rand() % 130;
		sprintf(temp, "<line x1=\"%d\" y1=\"%d\" x2=\"%d\" y2=\"%d\" stroke-width=\"1\" />\n", x, 140 - y, x + 10, 140 - y2);
		out += temp;
		y = y2;
	}
	out += "</g>\n</svg>\n";

	server.send(200, "image/svg+xml", out);
}
// =============================================================================================

// -----------------------------------------------------------------------------------------------------------------------------------------------
void createWebServer(int webtype)
// -----------------------------------------------------------------------------------------------------------------------------------------------
{
	if (webtype == 1) // server web AP
	{
		server.on("/", handleScan);
		server.on("/setting", handleSetting);
		//    server.on ("/test1", handleTest1);
		//    server.on ("/test2", handleTest2);
		server.on("/request", handleRequest);
		server.on("/gate.htm", handleGate);
		server.on("/gate", handleGate);
		server.on("/callback", handleCallback);
		server.on("/backsetting", handleBackSetting);
		server.onNotFound(handleNotFound);
	}
	else
		// --------------------------------------------------------------------------------------
		if (webtype == 0) // server web connesso a gateway
	{
		server.on("/", handleRoot);
		server.on("/cleareeprom", handleClear);
		//    server.on ("/test1", handleTest1);
		//    server.on ("/test2", handleTest2);
		server.on("/request", handleRequest);
		server.on("/gate.htm", handleGate);
		server.on("/gate", handleGate);
		server.on("/callback", handleCallback);
		server.on("/backsetting", handleBackSetting);
		server.onNotFound(handleNotFound);
	}
}

// -----------------------------------------------------------------------------------------------------------------------------------------------
void setup()
{

	wifi_station_set_hostname(_modo "gate");

	pinMode(2, INPUT);
	pinMode(0, OUTPUT);

	// aspetta 15 secondi perche' con l'assorbimento iniziale di corrente esp8266 fa disconnettere l'adattatore seriale
	int pauses = 0;
	while (pauses < 150) // 15 secondi wait
	{
		pauses++;
		delay(100); // wait 100ms
	}

	Serial.begin(115200);
	while (!Serial)
	{
		;
	}
	Serial.flush();

	EEPROM.begin(512);

	digitalWrite(0, LOW); // A0 OUTPUT BASSO
	delay(10);

#ifdef DEBUG
	Serial.println();
	Serial.println();
	Serial.println("Startup");

	Serial.println("Press A for start as AP");
	Serial.setTimeout(10000);
	char serin[4];
	Serial.readBytes(serin, 1);
	if ((serin[0] == 'A') || (serin[0] == 'a'))
	{
		forceAP = 1;
		Serial.println("Forced AP mode");
	}
	Serial.setTimeout(1000);

	// read eeprom for ssid and pass
	Serial.print("Read EEPROM ssid: ");
#endif

	String esid;
	for (int i = E_SSID; i < 32; ++i)
	{
		esid += char(EEPROM.read(i));
	}
#ifdef DEBUG
	Serial.println(esid);
	Serial.print("Read EEPROM pass: ");
#endif

	String epass = "";
	for (int i = E_PASSW; i < 96; ++i)
	{
		epass += char(EEPROM.read(i));
	}
#ifdef DEBUG
	Serial.println(epass);
#endif
	char a;
	for (int i = E_CALLBACK; i < (E_CALLBACK + 120); ++i)
	{
		a = char(EEPROM.read(i));
		if (a == 0)
			i = E_CALLBACK + 121;
		else
			callback += a;
	}
#ifdef DEBUG
	Serial.print("callback=");
	Serial.println(callback);
#endif

	Serial.write("@");  // set led lamps
	Serial.write(0xF0); // set led lamps low-freq (client mode)
	delay(50);			// wait 50ms

#ifdef DEBUG
	Serial.write("@MA"); // set SCSgate/KNXgate ascii mode
#else
	Serial.write("@MX"); // set SCSgate/KNXgate hex mode
#endif
	delay(50); // wait 50ms

	if (Serial.find("k") == 0)
	{
#ifdef DEBUG
//  Serial.print("Gate error ");
#endif
		//  ledError(1);
	}

	Serial.write("@b"); // clear SCSgate/KNXgate buffer

	Serial.setTimeout(10); // timeout is 10mS
	delay(50);			   // wait 50ms
	Serial.flush();

	int i = E_IPADDR;
	for (int j = 0; j < 4; ++j)
	{
		local_ip[j] = char(EEPROM.read(i++));
	}

	i = E_ROUTIP;
	for (int j = 0; j < 4; ++j)
	{
		router_ip[j] = char(EEPROM.read(i++));
	}

	byte high = EEPROM.read(E_PORT);
	byte low = EEPROM.read(E_PORT + 1);
	localPort = word(high, low);
	if ((localPort == 0) || (localPort == 0xFFFF))
		localPort = 52056;

#ifdef DEBUG
	if (digitalRead(2) == 0)
		Serial.println("Pin 2 force AP mode...");
#endif

	if ((esid.length() > 1) && (digitalRead(2) == 1) && (forceAP == 0)) // dati in eeprom, jumper assente, forzatura non digitata
	{
		WiFi.begin(esid.c_str(), epass.c_str());

		if ((local_ip[0] > 0) && (local_ip[0] < 255))
		{
#ifdef DEBUG
			Serial.println(" ");
			Serial.println("static IP");
#endif
			WiFi.config(local_ip, router_ip, IPAddress(255, 255, 255, 0));
		}
#ifdef DEBUG
		else
		{
			Serial.println(" ");
			Serial.println("dynamic IP");
		}
#endif

		if (testWifi()) // wifi connesso
		{
			WiFi.mode(WIFI_STA);
			//      WiFi.disconnect();
			launchWeb(0);
#ifdef DEBUG
			Serial.println("open UDP port");
#endif
			port.begin(localPort);
			udpopen = 1;
#ifdef DEBUG
			Serial.println("opened UDP");
#endif
			Serial.write("@");  // set led lamps
			Serial.write(0xF1); // set led lamps std-freq (client mode)
			return;
		}
		else
		{
			// CONNESSIONE WIFI FALLITA . REBOOT
			ESP.restart();
		}
	}
	// access point mode
	setupAP();
	Serial.write("@");  // set led lamps
	Serial.write(0xF2); // set led lamps high-freq (AP mode)
}

// -----------------------------------------------------------------------------------------------------------------------------------------------
void loop()
{

	if (udpopen == 1)
	{
		// ====================================== receive from UDP - send to SERIAL =====================
		int packetSize = port.parsePacket();

		if (packetSize)
		{
			int len = port.read(packetBuffer, 255);
			packetSize = len;
			remote_ip = port.remoteIP();
			remote_port = port.remotePort();
			packetBuffer[len] = 0;

#ifndef DEBUG_UDP
			httpResp = "";
#endif

#ifdef DEBUG
			Serial.println(port.remoteIP());
			Serial.println(port.remotePort());
			Serial.println(packetBuffer);
			if (strcmp(packetBuffer, "@q") == 0)
			{
				port.beginPacket(port.remoteIP(), port.remotePort());
				port.write("kKNX 18.51");
				Serial.println("kKNX 18.51");
				port.endPacket();
			}
			else
			{
				port.beginPacket(port.remoteIP(), port.remotePort());
				port.write("k");
				Serial.println("k");
				port.endPacket();
			}
#else
if (strcmp(packetBuffer, "@Keep_alive") == 0)
{
}
#ifdef ECHO
else if (packetBuffer[0] == '>')
{
	echoOn = 1;
}
else if (packetBuffer[0] == '<')
{
	echoOn = 0;
}
#endif
else
{
	int s = 0;
	while (s < len)
	{
		Serial.write(packetBuffer[s]); // write on serial KNXgate/SCSgate
		delayMicroseconds(100);
		s++;
	}
}
#endif
}

delay(1); // wait 1 mS (!)

// ====================================== receive from SERIAL - send to UDP =====================
// char replyBuffer[24];
// unsigned char replyLen;
//

replyLen = 0;
if (Serial.available())
{
	while (Serial.available() && (replyLen < 255))
	{
		while (Serial.available() && (replyLen < 255))
		{
			replyBuffer[replyLen++] = Serial.read(); // receive from serial USB
			delayMicroseconds(INNERWAIT);
		}
		delayMicroseconds(OUTERWAIT);
	}
	replyBuffer[replyLen] = 0;
}

if ((remote_ip) && (replyLen))
{
#ifndef DEBUG_UDP
	if (httpResp == "")
#endif
	{
		int success;
		do
		{
			success = port.beginPacket(remote_ip, remote_port);
		} while (!success);

#ifdef DEBUG_UDP
		port.print("fromPic:[");
		char hBuffer[12];
		sprintf(hBuffer, "%02X", replyLen);
		port.write(hBuffer[0]);
		port.write(hBuffer[1]);
		port.write(']');
#endif

		int n = 0;
		// attenzione - ogni carattere sulla seriale ci mette circa 90uS ad arrivare
		while (n < replyLen)
		{
#ifdef DEBUG_UDP
			//            sprintf(hBuffer, "%02X", replyBuffer[n++]);
			//            port.write(hBuffer[0]);
			//            port.write(hBuffer[1]);
			port.write(replyBuffer[n++]);
			//            port.write('.');
#else
			port.write(replyBuffer[n++]);
#endif
		}
		success = port.endPacket();
	}

#ifndef DEBUG_UDP
	else
#endif

		if (((httpResp == "a") || (httpResp == "y")) && (callback[0] == ':'))
	{
#ifdef DEBUG
		Serial.println(replyBuffer);
		if ((replyBuffer[0] == '4') && (replyLen == 5))
#else
		if ((replyBuffer[0] == 4) && (replyLen == 5))
#endif

		// intero  7 A8 32 00 12 01 21 A3
		// ridotto 4 32 00 12 01
		{
			content = "http://";
			content += remote_ip.toString();
			content += callback;
			
char hBuffer[12];
sprintf(hBuffer, "&type=%02X", replyBuffer[3]);
content += hBuffer;
sprintf(hBuffer, "&from=%02X", replyBuffer[2]);
content += hBuffer;
sprintf(hBuffer, "&to=%02X", replyBuffer[1]);
content += hBuffer;
sprintf(hBuffer, "&cmd=%02X", replyBuffer[4]);
content += hBuffer;

#ifdef DEBUG
Serial.println("request ");
Serial.println(content);
httpClient.begin(content);
//      httpClient.begin("http://192.168.2.181/index.htm");
//      httpClient.begin("http://192.168.2.9/test.htm?type=command&param=1");  //Specify request destination
//      httpClient.begin("http://jsonplaceholder.typicode.com/users/1");
//                        http://<domoticz ip>:<domoticz port>/json.htm?type=command&param='''udevices'''&script='''yoctopuce_meteo.lua'''&
//     http://<domoticz ip>:<domoticz port>/json.htm?type=command&param=udevices&script=yoctopuce_meteo.lua&timestamp=1446371516&network=&meteo%23dataLogger=OFF&meteo%23temperature=5.72&meteo%23pressure=889&meteo%23humidity=81.1
Serial.println("get");
int httpCode = httpClient.GET(); //Send the request
if (httpCode > 0)
{											 //Check the returning code
	Serial.println("OK");					 // write on serial KNXgate/SCSgate
	String payload = httpClient.getString(); //Get the request response payload
	Serial.print(payload);					 //Print the response payload - debug
}
else
{
	Serial.print("ERROR ");
	Serial.println(httpCode);
}
httpClient.end(); //Close connection
#else

httpClient.begin(content);
int httpCode = httpClient.GET(); //Send the request
httpClient.end();				 //Close connection
#endif
}
}
}

// =====================================================================================================

#ifdef ECHO
if ((remote_ip) && (packetSize) && (echoOn))
{
	int success;
	do
	{
		success = port.beginPacket(remote_ip, remote_port);
	} while (!success);
	delay(5);					  // wait 5 mS (!)
	port.write('0' + packetSize); // write on serial KNXgate/SCSgate
	int s;
	for (s = 0; s < packetSize; s++)
	{
		port.write(packetBuffer[s]); // write on serial KNXgate/SCSgate
	}
	success = port.endPacket();
}
#endif

// =====================================================================================================
}

// =====================================================================================================
// =====================================================================================================

if (requestLen > 0)
{
#ifdef DEBUG_UDP
	int success;
	do
	{
		success = port.beginPacket(remote_ip, remote_port);
	} while (!success);
	port.print("toPic:[");
	port.write(requestLen);
	port.write(']');
#endif
	// =========================== send control char and data over serial =============================
	int s = 0;
	while (s < requestLen)
	{
		Serial.write(requestBuffer[s]); // write on serial KNXgate/SCSgate
#ifdef DEBUG_UDP
		port.write(requestBuffer[s]);
#endif
		//        delayMicroseconds(INNERWAIT);
		delayMicroseconds(OUTERWAIT * 2);
		s++;
	}
#ifdef DEBUG_UDP
	port.write(']');
	success = port.endPacket();
#endif
	requestLen = 0;
}

// =====================================================================================================
// =====================================================================================================

if (webon == 1)
	server.handleClient();
// =====================================================================================================
// =====================================================================================================

if (webon == 1)
	server.handleClient();
}
*/