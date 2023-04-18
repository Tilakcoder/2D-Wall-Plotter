#include <SPI.h>
#include <SD.h>
//#include <AccelStepper.h>

const int d1 = 2, d2 = 4;
const int s1 = 3, s2 = 5;
const int en = 12;

const int switchs = 6, up = 7, down = 8, left = 10, right = 11;
const int led = 13;
int lineCount = 0;

bool printState = false;

double MINstep = 40;

class Gcode {
  public:
    // Width 1160(580)  height 860
    double M1X = -580, M1Y = 430, M2X = 580, M2Y = 430, gap = 0, newX, newY;
    double CX = 0, CY = 0;
    double preZ = 90;
    double G, X = 0, Y = 0, Z = 90, F, I, J, M, T;
    bool absolute = false;
    double FeedRate = 100;

    double rou(double n, String ms) {
      return n * MINstep;
    }

    void Read(String GCC) {
      String GC = GCC + " ";
      char GCODE[GC.length()];
      //      Serial.println(GC);
      GC.toCharArray(GCODE, GC.length());
      char *name = NULL;
      name = strtok(GCODE, " ;\n");
      while (name != NULL)
      {
        String d = String(name);
        float tmp = d.substring(1, d.length()).toFloat();
        switch (name[0]) {
          case ('G'):
            G = tmp;
            break;
          case ('X'):
            X = tmp;
            break;
          case ('Y'):
            Y = tmp;
            break;
          case ('Z'):
            Z = tmp;
            break;
          case ('I'):
            I = tmp;
            break;
          case ('J'):
            J = tmp;
            break;
          case ('F'):
            F = tmp;
            break;
          case ('M'):
            M = tmp;
            G = -10;
            break;
          case ('T'):
            T = tmp;
          default:
            Serial.println(name[0]);
            Serial.println("IS not included");
            break;
        }

        name = strtok(NULL, " ");
      }
    }

    double distance(double x1, double y1, double x2, double y2) {
      return sqrt(((x2 - x1) * (x2 - x1)) + ((y2 - y1) * (y2 - y1)));
    }

    void Run(double one, double two) {
      double r1 = rou(one, "one"), r2 = rou(two, "two");
      double speeds = 700;


      double TS = (abs(r1) >= abs(r2)) ? r1 : r2;
      bool ine = (abs(r1) >= abs(r2)) ? true : false;
      double Sm = (abs(r1) <= abs(r2)) ? r1 : r2;
      Sm = round(abs(TS) / abs(Sm));

      for (int i = 1; i <= abs(TS); i++) {
        if (ine) {
          digitalWrite(d1, (r1 > 0) ? 1 : 0);
          digitalWrite(s1, HIGH);
          delayMicroseconds(speeds);
          digitalWrite(s1, LOW);
          delayMicroseconds(speeds);
          r1 -= (r1 > 0) ? 1 : -1;
        }
        else {
          digitalWrite(d2, (r2 > 0) ? 1 : 0);
          digitalWrite(s2, HIGH);
          delayMicroseconds(speeds);
          digitalWrite(s2, LOW);
          delayMicroseconds(speeds);
          r2 -= (r2 > 0) ? 1 : -1;
        }

        if (i % (int)Sm == 0) {
          if (ine) {
            digitalWrite(d2, (r2 > 0) ? 1 : 0);
            digitalWrite(s2, HIGH);
            delayMicroseconds(speeds);
            digitalWrite(s2, LOW);
            delayMicroseconds(speeds);
            r2 -= (r2 > 0) ? 1 : -1;
          }
          else {
            digitalWrite(d1, (r1 > 0) ? 1 : 0);
            digitalWrite(s1, HIGH);
            delayMicroseconds(speeds);
            digitalWrite(s1, LOW);
            delayMicroseconds(speeds);
            r1 -= (r1 > 0) ? 1 : -1;
          }
        }
      }



      digitalWrite(d1, (r1 > 0) ? 1 : 0);
      for (int x = 0; x < abs(r1); x++)
      {
        digitalWrite(s1, HIGH);
        delayMicroseconds(speeds);
        digitalWrite(s1, LOW);
        delayMicroseconds(speeds);
      }
      digitalWrite(d2, (r2 > 0) ? 1 : 0);
      for (int x = 0; x < abs(r2); x++)
      {
        digitalWrite(s2, HIGH);
        delayMicroseconds(speeds);
        digitalWrite(s2, LOW);
        delayMicroseconds(speeds);
      }
    }

    void Goabsolute(double x2, double y2) {
      double cX = x2 - CX;
      double cY = y2 - CY;
      double steps = (abs(cX) > abs(cY)) ? abs(cX) : abs(cY);
      for (int i = 1; i <= steps; i++) {
        newX = CX + (cX / steps);
        newY = CY + (cY / steps);


        double thresh = 3;
        double far = abs(newX) / (abs(M1X) + 1250);
        double thisY = newY - (thresh * far);

        // Fixing Y axis
        //        double TY = 10;
        //        double Yfar =

        double M1D = distance(M1X, M1Y, CX - (gap / 2), CY);
        double M2D = distance(M2X, M2Y, CX + (gap / 2), CY);
        //        double new1 = distance(M1X, M1Y, newX-(gap/2), thisY);
        //        double new2 = distance(M2X, M2Y, newX+(gap/2), thisY);

        double new1 = distance(M1X, M1Y, newX - (gap / 2), newY);
        double new2 = distance(M2X, M2Y, newX + (gap / 2), newY);


        //        Run(M1D - new1, M2D - new2);
        Run(new1 - M1D, new2 - M2D);
        CX = newX;
        CY = newY;
      }
    }

    void Gorelative(double x2, double y2) {
      Goabsolute(X + x2, Y + y2);
    }

    void compile() {
      if (G == -10) {
        Serial.println("It's M");
      }
      else if (G == 90) {
        absolute = true;
      }
      else if (G == 0 || G == 1) {
        // effect Z first
        analogWrite(9, (Z > 50) ? 200 : 100);
        if (preZ != Z) {
          delay(500);
        }
        preZ = Z;
        delay(200);
        if (absolute) {
          Goabsolute(X, Y);
        }
        else {
          Gorelative(X, Y);
        }
        Serial.println("C");
      }
      else if (G == 2) {
        // Minimum Angle
        double OrX = CX + I, OrY = CY + J;
        double r = distance(OrX, OrY, CX, CY);
        double angle = (asin((MINstep / 2) / r) * 180 / 3.14) * 2;

        // Number of steps

      }
    }
};


void autohome();
Gcode MYG;


void control() {
  int f, b, l, r;
  f = digitalRead(up);
  b = digitalRead(down);
  l = digitalRead(left);
  r = digitalRead(right);

}

void blinks(int pin, int wait, int count) {
  for (int i = 0; i < count; i++) {
    digitalWrite(pin, HIGH);
    delay(50);
    digitalWrite(pin, LOW);
    delay(50);
  }
  delay(wait);
}

void setup()
{
  Serial.begin(9600);

  pinMode(d1, OUTPUT);
  pinMode(s1, OUTPUT);
  pinMode(d2, OUTPUT);
  pinMode(s2, OUTPUT);
  pinMode(9, OUTPUT);
  pinMode(switchs, INPUT);
  pinMode(up, INPUT);
  pinMode(down, INPUT);
  pinMode(left, INPUT);
  pinMode(right, INPUT);
  pinMode(led, OUTPUT);
  pinMode(en, OUTPUT);
  
  digitalWrite(en, LOW);


  analogWrite(9, 200);

  int scon = digitalRead(switchs);
  while (scon == 1) {
    blinks(led, 1000, 5);
    scon = digitalRead(switchs);
  }

  if (!SD.begin(10))
  {
    Serial.println("Card failed");
    while (!SD.begin(10)) {
      digitalWrite(led, HIGH);
      delay(500);
      digitalWrite(led, LOW);
      delay(500);
    }
  }

  digitalWrite(led, HIGH);


  File dataFile = SD.open("G2.ngc");
  Serial.println("CARD passed");

  if (dataFile)
  {
    digitalWrite(en, HIGH);
    while (dataFile.available())
    {
      scon = digitalRead(switchs);
      if (scon == 1) {
        String a = dataFile.readStringUntil(';');
        MYG.Read(a);
        MYG.compile();
        lineCount += 1;
      }
    }
    dataFile.close();
    digitalWrite(en, LOW);
  }
  else
  {
    Serial.println("error opening file");
    while(true){
      blink(led, 1000, 2);
    }
  }
}

void loop()
{
}

void autohome()
{
  // code to home machine
}

void gotoLocation(double x, double y)
{
  // code to run machine to location
}
