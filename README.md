# Circuito de Carga para Baterias de Chumbo Ácido (Lead Acid Battery Charger)
Esse código é complemento do circuito para carga de baterias de chumbo ácido.

# Componentes
* 1 x CI LM317
* 1 x Sensor INA219
* 1 x Capacitor 0,1uF/25V
* 1 x Resistor 5,1 Ohms 1/2W
* 1 x Resistor 1KOhms 1/4W
* 1 x Resistor 10KOhms 1/4W
* 1 x Resistor 20KOhms 1/4W
* 1 x Potenciômetro 20KOhms
* 1 x Diodo 1N4007 (opcional)
* 1 x Buzzer 12V (opcional)
* 1 x Bateria

Observação: O circuito deve ser projetado de acordo com as características técnicas da bateria. O buzzer e diodo são utilizados como proteção para inversão de polaridade da bateria. O sensor INA219 possibilita a medição da tensão da bateria e corrente de carga. Porém, é possível realizar a medição da tensão com um circuito divisor de tensão e um resistor shut conectado ao polo negativo da bateria.

# Código
## Parâmetros de Ajuste
As seguintes variáveis devem ser ajustadas de acordo com as características do circuito e da bateria utilizada.
```
#define DISCHARGED_BATT_LEVEL   11.5 //tensão mínima de descarga
#define FLOATING_BATT_LEVEL     13.8 //tensão máxima para fim de carga 
#define MINIMUM_BATT_LEVEL      6.0  //tensão mínima aceitável

#define MINIMUM_CHG_CURRENT     0.20 //mA -> corrente mínima para finalizar carga

#define VOLTAGE_FACTOR          0.01955036F //(5V/1023)*(R3/R3+R4) constante do divisor de tensão
```
