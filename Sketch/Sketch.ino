// Inizio blocco Adafruit
#include <Adafruit_NeoPixel.h>
#ifdef __AVR__
  #include <avr/power.h>
#endif
// Fine blocco Adafruit

// Codici dei pin
#define pinLED 2
#define pinIR1 4
#define pinIR2 6
#define pinSmf1 8
#define pinSmf2 10
#define pinTmpPwr 12
const int sensorTmpPin = A0;

// Variabili sensore di temperatura
int sensorTmpState = 0, lastTmpState = 0;
int sensorVal;
float voltage;
float temperature;
int temperaturaLimite = 24;

// Dichiara l'oggetto striscia di LED
Adafruit_NeoPixel strip = Adafruit_NeoPixel(14, pinLED, NEO_RGBW + NEO_KHZ800);
// Dichiara l'oggetto striscia di LED - Semaforo 1
Adafruit_NeoPixel stripSmf1 = Adafruit_NeoPixel(1, pinSmf1, NEO_RGBW + NEO_KHZ800);
// Dichiara l'oggetto striscia di LED - Semaforo 2
Adafruit_NeoPixel stripSmf2 = Adafruit_NeoPixel(1, pinSmf2, NEO_RGBW + NEO_KHZ800);

// Variabili sensori IR
int sensorIR1State = 0, lastIR1State = 0; // variable for reading the pushbutton status for IR1
int sensorIR2State = 0, lastIR2State = 0; // variable for reading the pushbutton status for IR2

// Variabili in input seriale
// Utilizzato per contenere provvisoriamente il valore intero di input seriale.
int inputSeriale;

// Codice per la comunicazione seriale - inizio
const byte numChars = 64;
char receivedChars[numChars];
char *strtokIndx = NULL;
boolean newData = false;
// Codice per la comunicazione seriale - fine

// Codice per la gestione dell'illuminazione - inizio
boolean vetturaInTransito = false;
int posizioneVetturaIntero = 0;
int numeroLEDFadeIn = 8;
int numeroLEDFadeOut = 7;
int frazioniDiLED = 5;
int numeroLEDStriscia = 14;
// int formaOnda[] = {0, 255, 0, 0, 0, 0, 255, 0, 0, 0, 0, 0, 0, 255, 0};
int formaOnda[] = {0, 128, 255, 255, 255, 255, 255, 255, 255, 128, 0, 0, 0, 0, 0};
int intensitaLED[14];
int intensitaCriterioCostante = 255;
// Codice per la gestione dell'illuminazione - fine

void setup() {
    // initialize the IR sensor pin as an input:
    pinMode(pinIR1, INPUT);
    pinMode(pinIR2, INPUT);
    pinMode(pinTmpPwr, OUTPUT);
    digitalWrite(pinIR1, HIGH); // turn on the pullup on IR1
    digitalWrite(pinIR2, HIGH); // turn on the pullup on IR2
    digitalWrite(pinTmpPwr, HIGH); // Attiva l'alimentazione di tmp
  
    Serial.begin(9600);
    
    // Inizializzazione delle strisce
    strip.begin();
    strip.show(); // Initialize all pixels to 'off'
    stripSmf1.begin();
    stripSmf1.show();
    stripSmf2.begin();
    stripSmf2.show();
}

void loop() {
  // read the state of the IR:
  sensorIR1State = digitalRead(pinIR1);
  sensorIR2State = digitalRead(pinIR2);

  sensorVal = analogRead(sensorTmpPin);
  voltage = (sensorVal / 1024.0) * 5.0;
  temperature = (voltage - .5) * 100;

  // IR1
  if (sensorIR1State && !lastIR1State) {
    Serial.print("0"); // Unbroken IR1
  } 
  if (!sensorIR1State && lastIR1State) {
    Serial.print("1"); // Broken IR2
  }
  lastIR1State = sensorIR1State;

  // IR2
  if (sensorIR2State && !lastIR2State) {
    Serial.print("2"); // Unbroken IR2
  } 
  if (!sensorIR2State && lastIR2State) {
    Serial.print("3"); // Broken IR2
  }
  lastIR2State = sensorIR2State;

  // Tmp
  sensorTmpState = temperature > temperaturaLimite ? 1 : 0;

  if (!sensorTmpState && lastTmpState) {
    Serial.print("4"); // Al di sotto di 25 gradi
  }
  if (sensorTmpState && !lastTmpState) {
    Serial.print("5"); // Al di sopra di 25 gradi
  } 
  lastTmpState = sensorTmpState;

  recvWithStartEndMarkers();
  processNewData();
  
  delay(1);
}

void setStrisciaIntensitaZero() {
  for(int i = 0; i < numeroLEDStriscia; i++) {
    strip.setPixelColor(i, 0, 0, 0, 0);
  }
}

void recvWithStartEndMarkers() {
    static boolean recvInProgress = false;
    static byte ndx = 0;
    char startMarker = '<';
    char endMarker = '>';
    char rc;
 
    while (Serial.available() > 0 && newData == false) {
        rc = Serial.read();

        if (recvInProgress == true) {
            if (rc != endMarker) {
                receivedChars[ndx] = rc;
                ndx++;
                if (ndx >= numChars) {
                    ndx = numChars - 1;
                }
            }
            else {
                receivedChars[ndx] = '\0'; // terminate the string
                recvInProgress = false;
                ndx = 0;
                newData = true;
            }
        }

        else if (rc == startMarker) {
            recvInProgress = true;
        }
    }
}

void processNewData() {
    int comType;
    if (newData == true) {
        char *strtokIndx;
  
        strtokIndx = strtok(receivedChars,",");
        comType = atoi(strtokIndx);

        // Se l'input e' sui semafori
        if (comType == 1) {
            strtokIndx = strtok(NULL, ",");
            inputSeriale = atoi(strtokIndx);
            impostaSemafori(inputSeriale);
        }
        // Se l'input e' su vetturaInTransito
        else if (comType == 2) {
            strtokIndx = strtok(NULL, ",");
            inputSeriale = atoi(strtokIndx);
            
            if (inputSeriale == 0) {
              vetturaInTransito = true;
            }
            if (inputSeriale == 1) {
              vetturaInTransito = false;
            }
        }
        // Se l'input e' sulla posizione della vettura
        else if (comType == 3) {
            strtokIndx = strtok(NULL, ",");
            posizioneVetturaIntero = atoi(strtokIndx);

            calcolaIntensitaLED();
            aggiornaLEDStriscia();
        }
        // Se l'input e' su criterioDinamicoAttivo == false
        else if (comType == 4) {
            strtokIndx = strtok(NULL, ",");
            intensitaCriterioCostante = atoi(strtokIndx);

            impostaCriterioCostante();
            aggiornaLEDStriscia();
        }
        
        newData = false;
    }
}

void impostaSemafori(int inputSeriale) {
    if (inputSeriale == 0)
        stripSmf1.setPixelColor(0, 0, 255, 0, 0);
    if (inputSeriale == 1)
        stripSmf1.setPixelColor(0, 255, 0, 0, 0);
    if (inputSeriale == 2)
        stripSmf2.setPixelColor(0, 0, 255, 0, 0);
    if (inputSeriale == 3)
        stripSmf2.setPixelColor(0, 255, 0, 0, 0);

    stripSmf1.show();
    stripSmf2.show();
}

void calcolaIntensitaLED() {
  int a, b, x, y, j;
  int posizioneLED, posizioneFronte;
  
  azzeraArrayIntensita();
  
  posizioneFronte = posizioneVetturaIntero + numeroLEDFadeIn * frazioniDiLED;
  for (int i = 0; i < (numeroLEDFadeIn + numeroLEDFadeOut - 1) * frazioniDiLED; i++) {
    j = posizioneFronte - i - numeroLEDFadeIn * frazioniDiLED;
    if (j >= 0 && j % frazioniDiLED == 0) {
      posizioneLED = j / frazioniDiLED;
      if (posizioneLED < numeroLEDStriscia) {
        a = i / frazioniDiLED;
        b = a + 1;
        x = i % frazioniDiLED;
        y = (int) ((formaOnda[b] - formaOnda[a]) / (double) frazioniDiLED * x) + formaOnda[a];
        intensitaLED[posizioneLED] = y;
      }
    }
  }
}

void aggiornaLEDStriscia() {
  int val;
  
  for (int i = 0; i < numeroLEDStriscia; i++) {
    val = intensitaLED[i];
    strip.setPixelColor(i, val, val, val, val);
  }

  strip.show();
}

void azzeraArrayIntensita() {
  for (int i = 0; i < numeroLEDStriscia; i++) {
    intensitaLED[i] = 0;
  }
}

void impostaCriterioCostante() {
    for (int i = 0; i < numeroLEDStriscia; i++) {
      intensitaLED[i] = intensitaCriterioCostante;
    }
}
