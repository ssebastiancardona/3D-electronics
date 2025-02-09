#include <EEPROM.h>

#define cortesia 34
#define altas 23
#define explo_R 25
#define explo_L 26
#define pito 13
#define corneta 12
#define freno 35
#define outDir_R 27
#define outDir_L 14
#define inDir_R 32
#define inDir_L 33

bool exploradorasEncendidas = false;
int cambiosCortesia = 0;
unsigned long timePulse = 0;
const unsigned long debounceTime = 100; // 100ms de filtro de rebote

int intEx = 128; // Intensidad inicial (mitad del rango 0-255)
unsigned long lastCortesiaTime = 0;
const unsigned long maxTimeBetweenPulses = 2000; // 2s para detectar 5 cambios
bool controlIntensidad = false;

void setup()
{
  pinMode(cortesia, INPUT);
  pinMode(freno, INPUT);
  pinMode(explo_R, OUTPUT);
  pinMode(explo_L, OUTPUT);
  digitalWrite(explo_R, LOW);
  digitalWrite(explo_L, LOW);
  Serial.begin(115200);

  EEPROM.begin(4);
  intEx = EEPROM.read(0);
  if (intEx < 0 || intEx > 255)
    intEx = 128;
}

void loop()
{
  static int estadoInicial = digitalRead(cortesia);
  static int estadoAnterior = estadoInicial;
  int estadoActual = digitalRead(cortesia);
  static bool cambioDetectado = false;
  int estadoFreno = digitalRead(freno);

  if (estadoActual != estadoAnterior && (millis() - timePulse) > debounceTime)
  {
    timePulse = millis();
    if (estadoActual != estadoInicial)
    {
      cambioDetectado = true;
    }
    else if (cambioDetectado)
    {
      cambiosCortesia++;
      cambioDetectado = false;
      lastCortesiaTime = millis();
      Serial.print("Cambios detectados: ");
      Serial.println(cambiosCortesia);
    }
  }
  estadoAnterior = estadoActual;

  // Encender y apagar exploradoras con 3 cambios mientras freno está en 1
  if (estadoFreno == 1 && cambiosCortesia >= 3)
  {
    exploradorasEncendidas = !exploradorasEncendidas;
    // intEx = exploradorasEncendidas ? intEx : 0;
    analogWrite(explo_R, intEx);
    analogWrite(explo_L, intEx);
    cambiosCortesia = 0;
    if (exploradorasEncendidas == false)
    {
      analogWrite(explo_R, 0);
      analogWrite(explo_L, 0);
    }
    else
    {
      analogWrite(explo_R, intEx);
      analogWrite(explo_L, intEx);
    }
    Serial.println(exploradorasEncendidas ? "Exploradoras encendidas." : "Exploradoras apagadas.");
  }

  // Activar función intensidadExploradoras con 5 cambios en menos de 2 segundos mientras freno está en 0
  if (!controlIntensidad && estadoFreno == 0 && cambiosCortesia >= 5 && (millis() - lastCortesiaTime) <= maxTimeBetweenPulses)
  {
    controlIntensidad = true;
    cambiosCortesia = 0;
    intensidadExploradoras();
  }
}

// Función para controlar la intensidad de las exploradoras
void intensidadExploradoras()
{
  Serial.println("Modo de intensidad activado.");
  unsigned long lastActivity = millis();

  while (true)
  {
    analogWrite(explo_R, intEx);
    analogWrite(explo_L, intEx);

    if (digitalRead(cortesia) == 0)
    {
      while (digitalRead(cortesia) == 0 && intEx < 255)
      {
        intEx++;
        analogWrite(explo_R, intEx);
        analogWrite(explo_L, intEx);
        delay(10);
      }

      // Parpadeo al máximo
      if (intEx == 255)
      {
        for (int i = 0; i < 2; i++)
        {
          analogWrite(explo_R, 0);
          analogWrite(explo_L, 0);
          delay(200);
          analogWrite(explo_R, 255);
          analogWrite(explo_L, 255);
          delay(200);
        }
      }

      while (digitalRead(cortesia) == 0 && intEx > 0)
      {
        intEx--;
        analogWrite(explo_R, intEx);
        analogWrite(explo_L, intEx);
        delay(10);
      }

      lastActivity = millis();
    }

    // Si pasan 5s sin actividad, guardar y salir
    if ((millis() - lastActivity) > 5000)
    {
      Serial.print("Guardando valor en EEPROM: ");
      Serial.println(intEx);
      EEPROM.write(0, intEx);
      EEPROM.commit();

      // Parpadeo final antes de salir
      analogWrite(explo_R, 0);
      analogWrite(explo_L, 0);
      delay(200);
      analogWrite(explo_R, intEx);
      analogWrite(explo_L, intEx);
      delay(200);

      break;
    }

    delay(10);
  }

  controlIntensidad = false;
  Serial.println("Modo de intensidad desactivado.");
}
