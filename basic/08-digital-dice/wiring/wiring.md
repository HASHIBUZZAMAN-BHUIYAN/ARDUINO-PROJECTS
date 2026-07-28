# Wiring Notes — Digital Dice

```
Die face layout (matches physical LED placement on the breadboard):

  [D2]        [D3]
        [D5]
  [D4]        [D6]
  [D7]        [D8]

Each LED: Arduino pin --[220 ohm]--> LED anode; LED cathode -> shared GND rail.

Pushbutton:
  3V3 ---- button pin A
  button pin B ---- D12 ---- 10k ohm ---- GND
  (10k pulls D12 LOW when the button is not pressed; pressing pulls it to 3V3/HIGH)
```

- All logic on the Uno Q is 3.3V; the button's pull-down network above uses 3V3, not 5V.
- LED position numbering in the sketch (`LED[0]`..`LED[6]`) corresponds left-to-right, top-to-bottom in the grid above: 0=top-left, 1=top-right, 2=mid-left, 3=center, 4=mid-right, 5=bottom-left, 6=bottom-right.
