#include <Arduino.h>

/*
Programme de monitoring de la chaudière
voir détails dans  joplin

8.10.20 DB
*/

//-- trappe pellets --
const int     pCCT   = 2;     //pin
boolean       epCCT  = true;  //état précédent
unsigned long arCCT  = 0;     //anti rebond
//-- alimentation pellets --
const int     pCCA   = 3;     //pin
boolean       epCCA  = true;  //état précédent
unsigned long crCCA  = 0;     //chronomètre
unsigned long arCCA  = 0;     //anti rebond
//-- pompes --
const int     pCLP   = 4;     //pin pompe logement
boolean       epCLP  = true;  //état précédent
unsigned long arCLP  = 0;     //anti rebond
const int     pCCP   = 5;     //pin pompe chaudière
boolean       epCCP  = true;  //tpat précédent
unsigned long arCCP  = 0;     //anti rebond
//-- vannes --
int           CLY    = 0;     //position vanne logement (0-100)
const int     pCLYo  = 6;     //vanne logement ouverture
boolean       epCLYo = true;  //état précédent
boolean       mCLYo  = false; //a atteint le maxi
unsigned long arCLYo = 0;     //anti rebond
unsigned long crCLYo = 0;     //chronomètre
const int     pCLYf  = 7;     //vanne logement fermeteure
boolean       epCLYf = true;  //état précédent
boolean       mCLYf  = false; //a atteint le mini
unsigned long arCLYf = 0;     //anti rebond
unsigned long crCLYf = 0;     //chronomètre
int           CCY    = 0;     //position vanne chaudière (0-100)
const int     pCCYo  = 8;     //vanne chaudière ouverture
boolean       epCCYo = true;  //état précédent
boolean       mCCYo  = false; //a atteint le maxi
unsigned long arCCYo = 0;     //anti rebond
unsigned long crCCYo = 0;     //chronomètre
const int     pCCYf  = 9;     //vanne chaudière fermeteure
boolean       epCCYf = true;  //état précédent
boolean       mCCYf  = false; //a atteint le mini
unsigned long arCCYf = 0;     //anti rebond
unsigned long crCCYf = 0;     //chronomètre
// -- constantes --
const int     tdY    = 30000; //temps de déplacement vanne [ms]
const unsigned long ar = 50;  //anti rebond [ms]
const unsigned long iP = 3600000; //intervalle entre 2 Ping [ms]
const String cJD1 = "SendToHTTP 192.168.1.109,8121,device=%sysname%&taskid=1&cmd=";
const String cJD2 = "&value=";
// -- variables --
unsigned long tpP    = 0;     //temps précédent Ping

void Envoi(String code, int val) {
  String txt;
  txt = cJD1 + code + cJD2 + val;
  Serial.println(txt);  
  return;
}
void Ping() {
  if(millis() - tpP > iP) {
    Envoi("MCC", 1);
    tpP = millis();
  }
  return;
}

void setup() {
  Serial.begin(9600);
  //Serial.println("----------------- SETUP -------------------");
  pinMode(pCCT,  INPUT);
  pinMode(pCCA,  INPUT);
  pinMode(pCLP,  INPUT);
  pinMode(pCCP,  INPUT);
  pinMode(pCLYo, INPUT);
  pinMode(pCLYf, INPUT);
  pinMode(pCCYo, INPUT);
  pinMode(pCCYf, INPUT);
  pinMode(LED_BUILTIN, OUTPUT); 
  //Serial.println("--------------- Fin SETUP -----------------");
  Envoi("MCC", 0);
}

void loop() {
  Ping();
//trappe pellets
  if(digitalRead(pCCT) != epCCT && millis() > (ar + arCCT)) {
    arCCT = millis();
    epCCT = !epCCT;
    Envoi("CCT", !epCCT);
  }
//pompe logement
  if(digitalRead(pCLP) != epCLP && millis() > (ar + arCLP)) {
    arCLP = millis();
    epCLP = !epCLP;
    Envoi("CLP", !epCLP);
  }
//pompe chaudière
  if(digitalRead(pCCP) != epCCP && millis() > (ar + arCCP)) {
    arCCP = millis();
    epCCP = !epCCP;
    Envoi("CCP", !epCCP);
  }  
//alimentation pellets
  if(digitalRead(pCCA) == false && epCCA == true && millis() > (ar + arCCA)) { //activé
    epCCA = false;
    crCCA = millis();
    arCCA = millis(); 
    digitalWrite(LED_BUILTIN, HIGH);   
  }
  if(digitalRead(pCCA) == true && epCCA == false && millis() > (ar + arCCA)) { //désactivé
    epCCA = true;
    arCCA = millis();
    digitalWrite(LED_BUILTIN, LOW);
    //Serial.print("CCA off ");
    int tCCA = millis() - crCCA;
    Envoi("CCA", tCCA);        
  }
//-----------------------------------------------------------------------------------------------------------
//ouverture vanne logement
  if(digitalRead(pCLYo) == false && epCLYo == true && millis() > (ar + arCLYo)  && mCLYo == false) { //activé
    epCLYo = false;
    crCLYo = millis();
    arCLYo = millis();  
    digitalWrite(LED_BUILTIN, HIGH);
  }   
  if(epCLYo == false && mCLYo == false && (millis() - crCLYo >= tdY - CLY)) { //a atteint le maxi
      CLY = tdY;
      Envoi("CLY", CLY / (tdY / 100));
      mCLYo = true;
      epCLYo = true;
      digitalWrite(LED_BUILTIN, LOW);
  }
  if(digitalRead(pCLYo) == true && epCLYo == false && millis() > (ar + arCLYo) && mCLYo == false) { //désactivé
    epCLYo = true;
    arCLYo = millis();
    digitalWrite(LED_BUILTIN, LOW);
    int tCLYo = millis() - crCLYo;
    CLY = CLY + tCLYo;
    Envoi("CLY", CLY / (tdY / 100));
    mCLYf = false;        
  }
//fermeture vanne logement
  if(digitalRead(pCLYf) == false && epCLYf == true && millis() > (ar + arCLYf) && mCLYf == false) { //activé
    epCLYf = false;
    crCLYf = millis();
    arCLYf = millis();
    digitalWrite(LED_BUILTIN, HIGH);
  }
  if(epCLYf == false && mCLYf == false && (millis() - crCLYf >= CLY)) { //a atteint le mini
      CLY = 0;
      Envoi("CLY", CLY / (tdY / 100));
      mCLYf = true;
      epCLYf = true;
      digitalWrite(LED_BUILTIN, LOW);
  }
  if(digitalRead(pCLYf) == true && epCLYf == false && millis() > (ar + arCLYf) && mCLYf == false) { //désactivé
    epCLYf = true;
    arCLYf = millis();
    digitalWrite(LED_BUILTIN, LOW);
    int tCLYf = millis() - crCLYf;
    CLY = CLY - tCLYf;
    Envoi("CLY", CLY / (tdY / 100));  
    mCLYo = false;         
  }
//-----------------------------------------------------------------------------------------------------------
//ouverture vanne chaudière
  if(digitalRead(pCCYo) == false && epCCYo == true && millis() > (ar + arCCYo)  && mCCYo == false) { //activé
    epCCYo = false;
    crCCYo = millis();
    arCCYo = millis();  
    digitalWrite(LED_BUILTIN, HIGH);
  }   
  if(epCCYo == false && mCCYo == false && (millis() - crCCYo >= tdY - CCY)) { //a atteint le maxi
      CCY = tdY;
      Envoi("CCY", CCY / (tdY / 100));
      mCCYo = true;
      epCCYo = true;
      digitalWrite(LED_BUILTIN, LOW);
  }
  if(digitalRead(pCCYo) == true && epCCYo == false && millis() > (ar + arCCYo) && mCCYo == false) { //désactivé
    epCCYo = true;
    arCCYo = millis();
    digitalWrite(LED_BUILTIN, LOW);
    int tCCYo = millis() - crCCYo;
    CCY = CCY + tCCYo;
    Envoi("CCY", CCY / (tdY / 100));
    mCCYf = false;        
  }
//fermeture vanne chaudière
  if(digitalRead(pCCYf) == false && epCCYf == true && millis() > (ar + arCCYf) && mCCYf == false) { //activé
    epCCYf = false;
    crCCYf = millis();
    arCCYf = millis();
    digitalWrite(LED_BUILTIN, HIGH);
  }
  if(epCCYf == false && mCCYf == false && (millis() - crCCYf >= CCY)) { //a atteint le mini
      CCY = 0;
      Envoi("CCY", CCY / (tdY / 100));
      mCCYf = true;
      epCCYf = true;
      digitalWrite(LED_BUILTIN, LOW);
  }
  if(digitalRead(pCCYf) == true && epCCYf == false && millis() > (ar + arCCYf) && mCCYf == false) { //désactivé
    epCCYf = true;
    arCCYf = millis();
    digitalWrite(LED_BUILTIN, LOW);
    int tCCYf = millis() - crCCYf;
    CCY = CCY - tCCYf;
    Envoi("CCY", CCY / (tdY / 100));  
    mCCYo = false;         
  }
//-----------------------------------------------------------------------------------------------------------
}

