# Wiring Notes — Two-Zone Security Alarm

```
Arduino Uno              4x4 Matrix Keypad
+-----------+           +--------------------+
|        D2 |-----------| Row 1              |
|        D3 |-----------| Row 2              |
|        D4 |-----------| Row 3              |
|        D5 |-----------| Row 4              |
|        D6 |-----------| Col 1              |
|        D7 |-----------| Col 2              |
|        D8 |-----------| Col 3              |
|        D9 |-----------| Col 4              |
+-----------+           +--------------------+

Zone 1 (door reed switch):
   5V --- reed switch --- A0 --- 10k resistor --- GND
   (switch closed = A0 reads HIGH = door open)

Zone 2 (PIR motion):
   Arduino 5V/GND -> PIR VCC/GND
   PIR OUT -> A1

     D10 ----------------- Siren/buzzer + (- to GND)
     D11-[220ohm]--|>|-- Red LED   --GND (armed)
     D12-[220ohm]--|>|-- Green LED --GND (disarmed)
```

- The keypad library treats rows/columns purely as digital pins scanned in a matrix pattern — no resistors needed, the library handles pin mode switching internally.
- Zone 1 uses a pull-down so an open circuit (door open, switch not touching) reads LOW consistently rather than floating.
