#include <Arduino.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#define TAILLE_TABLEAU 40

/*
Programme de monitoring de la chaudière
voir détails dans joplin

16.01.21 DB
*/


//-- capteur tension              {pellet , pompe L, pompe C}
const String  etiquetteCpt  [3] = {"CCT"  , "CLP"  ,"CCP"   };
const byte    indexCapt     [3] = {13     , 11     , 12     }; //no de index
const byte    pinCapt       [3] = {2      , 4      , 5      };  //pin
boolean       actif         [3];  //sortie active
unsigned long antiRebCapt       =  500;                         //anti rebond
//-- capteur de tension - alimentation pellets --
const byte    pinCCA      = 3;     //pin
boolean       epCCA       = true;  //état précédent
unsigned long chronoCCA   = 0;     //chronomètre
unsigned long antiRebCCA  = 0;     //anti rebond
//-- vannes                       {Logement    , Chaudière   }
const String  etiquette     [2] = {"CLY"       , "CCY"       };
const byte    index         [2] = {31          , 32          };  // no de index
unsigned long valeur        [2] = {102000      , 102000      }; //position vanne (1/2 au départ)
//                                {ouv L, fer L, ouv C, fer C} 
const byte    j             [4] = {0    , 0    , 1    , 1    }; //curseur valeurs communes
const byte    k             [4] = {1    , 0    , 3    , 2    }; //désactivation extremité sens inverse
const byte    pin           [4] = {6    , 7    , 8    , 9    }; //pin
const boolean valAugmente   [4] = {true , false, true , false}; //valeur augmente
boolean       vanneEnMouv   [4] = {false, false, false, false}; //état précédent
boolean       extremAtteint [4] = {false, false, false, false}; //a atteint extremité
unsigned long antiRebVanne  [4] = {0    , 0    , 0    , 0    }; //anti rebond
unsigned long chrono        [4] = {0    , 0    , 0    , 0    }; //chronomètre
// -- capteur température
const byte    pinI2C   = 11;    //pin
OneWire oneWire(pinI2C);
DallasTemperature capteursTemp(&oneWire);
DeviceAddress TCO, TCI, TLO, TLI;
const unsigned long iTC0 = 300000; //intervalle entre deux envoi de température [ms]
const unsigned long iTC1 = 60000;  //intervalle entre deux envoi de température chauf, actif [ms]
unsigned long       mpTC = 0;      //millis précédent
// -- constantes --
const unsigned long cycleY  = 204000; //temps cycle complet overture ou fermeture vanne [ms]
const unsigned long antiRebond = 200;  //anti rebond [ms]
const String cJD1 = "SendToHTTP 192.168.1.109,8121,device=%sysname%&taskid=";
const String cJD2 = "&cmd=";
const String cJD3 = "&value=";
const unsigned long intervalEnvois = 1000;  //1s entre chaque envoi http
// -- tableau tampon fifo --
byte curseurEcritureTableau = 0;  
byte curseurLectureTableau = 0;
const byte tailleTableau = TAILLE_TABLEAU;
unsigned long chronoTableau = 0;
String  tableauCodes   [TAILLE_TABLEAU]; 
byte    tableauIndexes [TAILLE_TABLEAU];
float   tableauValeurs [TAILLE_TABLEAU];
byte    tableauDcm     [TAILLE_TABLEAU];

void Envoi() {
  if (millis() - chronoTableau > intervalEnvois && curseurEcritureTableau != curseurLectureTableau) {
    chronoTableau = millis();
    String txt;
    txt = cJD1 + tableauIndexes[curseurLectureTableau] + cJD2 + tableauCodes[curseurLectureTableau] + cJD3; 
    Serial.print(txt);
    Serial.println(tableauValeurs[curseurLectureTableau], tableauDcm[curseurLectureTableau]);
    //Serial.print("curseur lecture ");
    //Serial.println(curseurLectureTableau);
    curseurLectureTableau++;  
    if(curseurLectureTableau >= TAILLE_TABLEAU) curseurLectureTableau = 0;
  }
}

void Donnees(String code, byte idx, float val, int dcm) {
  tableauCodes[curseurEcritureTableau] = code;
  tableauIndexes[curseurEcritureTableau] = idx;
  tableauValeurs[curseurEcritureTableau] = val;
  tableauDcm[curseurEcritureTableau] = dcm;
  //Serial.print("curseur ecriture ");
  //Serial.println(curseurEcritureTableau);    
  curseurEcritureTableau++;  
  if(curseurEcritureTableau >= TAILLE_TABLEAU) curseurEcritureTableau = 0;
}

void CaptTension(int i) {
  if(digitalRead(pinCapt[i]) != actif[i] && millis() - antiRebond > antiRebCapt) {
    antiRebCapt = millis();
    actif[i] = !actif[i];    
    Donnees(etiquetteCpt[i], indexCapt[i], !actif[i], 0);
  }
}

void CaptAlimentation() {
//alimentation pellets
  if(digitalRead(pinCCA) == false && epCCA == true && millis() > (antiRebond + antiRebCCA)) { //activé
    epCCA = false;
    chronoCCA = millis();
    antiRebCCA = millis(); 
    digitalWrite(LED_BUILTIN, HIGH);   
  }
  if(digitalRead(pinCCA) == true && epCCA == false && millis() > (antiRebond + antiRebCCA)) { //désactivé
    epCCA = true;
    antiRebCCA = millis();
    digitalWrite(LED_BUILTIN, LOW);
    //Serial.print("CCA off ");
    unsigned long tCCA = (millis() - chronoCCA);
    if(tCCA > antiRebond + 1) Donnees("CCA", 21, float(tCCA) / 1000, 2);        
  }
}

void CaptVanne(int i) {
  int valeurInt;
  //activé  
  if(!digitalRead(pin[i]) && !vanneEnMouv[i] && millis() - antiRebVanne[i] > antiRebond && !extremAtteint[i]) { 
    vanneEnMouv[i] = true;
    chrono[i] = millis();
    antiRebVanne[i] = millis();  
    digitalWrite(LED_BUILTIN, HIGH);
  }
  //désactivé
  if(digitalRead(pin[i]) && vanneEnMouv[i] && millis() > (antiRebond + antiRebVanne[i]) && !extremAtteint[i]) { 
    vanneEnMouv[i] = false;
    antiRebVanne[i] = millis();
    digitalWrite(LED_BUILTIN, LOW);
    if (valAugmente[i] ) valeur[j[i]] = valeur[j[i]] + millis() - chrono[i];
    else valeur[j[i]] = valeur[j[i]] + chrono[i] - millis();
    valeurInt = map(valeur[j[i]] , 0, cycleY, 0, 1000);
    Donnees(etiquette[j[i]], index[j[i]], float(valeurInt) / 1000, 3);
    extremAtteint[k[i]] = false;
  }
  //a atteint extremité
  boolean execExtrem = false;
  if(vanneEnMouv[i] && !extremAtteint[i] && (millis() - chrono[i] >= cycleY - valeur[j[i]]) && valAugmente[i]) { //max
    valeur[j[i]] = cycleY;
    execExtrem = true;
  }
  if(vanneEnMouv[i] && !extremAtteint[i] && (millis() - chrono[i] >= valeur[j[i]]) && !valAugmente[i]) {  //min
    valeur[j[i]] = 0;
    execExtrem = true;
  }
  if(execExtrem) {
    execExtrem = false;
    valeurInt = map(valeur[j[i]], 0, cycleY, 0, 1000);
    Donnees(etiquette[j[i]], index[j[i]], float(valeurInt) / 1000, 3);
    extremAtteint[i] = true;
    vanneEnMouv[i] = false;
    digitalWrite(LED_BUILTIN, LOW);
  }
}

void CaptTemperature(unsigned long interval) {  
  if(interval < millis() - mpTC) {
    mpTC = millis();
    float tmpCT;
    capteursTemp.requestTemperatures();
    tmpCT = capteursTemp.getTempC(TLO);
    if(tmpCT != -127) Donnees("TLO", 1, tmpCT, 1); 
    else Donnees("MCC", 99, 81, 0);
    tmpCT = capteursTemp.getTempC(TLI);
    if(tmpCT != -127) Donnees("TLI", 2, tmpCT, 1); 
    else Donnees("MCC", 99, 82, 0);
    tmpCT = capteursTemp.getTempC(TCO);
    if(tmpCT != -127) Donnees("TCO", 3, tmpCT, 1); 
    else Donnees("MCC", 99, 83, 0);   
    tmpCT = capteursTemp.getTempC(TCI);    
    if(tmpCT != -127) Donnees("TCI", 4, tmpCT, 1); 
    else Donnees("MCC", 99, 84, 0);  
    //Serial.println();
    //Serial.print("temp interval ");
    //Serial.println(interval);   
  }
}

void setup() {
  Serial.begin(9600);
  //Serial.println("----------------- SETUP -------------------");
  for (int i = 0; i < 3; i++)  pinMode(pinCapt[i], INPUT);
  pinMode(pinCCA,  INPUT);
  for (int i = 0; i < 4; i++)  pinMode(pin[i], INPUT);
  pinMode(LED_BUILTIN, OUTPUT);   
  for (int i = 0; i < 10; i++) {      //pause en attendant boot esp
    digitalWrite(LED_BUILTIN, HIGH);
    delay(500);
    digitalWrite(LED_BUILTIN, LOW);
    delay(500);
  }
  capteursTemp.begin();
  if (capteursTemp.getDeviceCount() != 4) Donnees("MCC", 99, 8, 0); //vérifie si il y a bien 4 capteurs
  if (!capteursTemp.getAddress(TLO, 0)) Donnees("MCC", 99, 8.1, 1); //erreur si pas trouvé
  if (!capteursTemp.getAddress(TLI, 3)) Donnees("MCC", 99, 8.2, 1); //    -
  if (!capteursTemp.getAddress(TCO, 1)) Donnees("MCC", 99, 8.3, 1); //    -
  if (!capteursTemp.getAddress(TCI, 2)) Donnees("MCC", 99, 8.4, 1); //    -
  CaptTemperature(0);
  for (int i = 0; i < 3; i++) {
    actif[i] = !digitalRead(pinCapt[i]);
    delay(200);
    Donnees(etiquetteCpt[i], indexCapt[i], actif[i], 0);
  }
  Donnees("MCC", 99, 0, 0);
  //Serial.println("--------------- Fin SETUP -----------------");
}

void loop() {
  for (int i = 0; i < 3; i++) CaptTension(i); //capt tension
  CaptAlimentation();
  for (int i = 0; i < 4; i++) CaptVanne(i); //vannes 4x (2 directions x 2 vannes)
  Envoi();  //envoi les données régulierement
  if(!actif[0]) CaptTemperature(iTC1);  //envoi température plus rapproché si chauff. en cours
  else CaptTemperature(iTC0);  
}