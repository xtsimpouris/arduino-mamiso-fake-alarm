# arduino-mamiso-fake-alarm
A fake alarm based on arduino

## Features
Tries its best to work without knowing what the current time is. The idea is to follow how bright it is in order to identify night/day and based on predefined delays provide the essance of "armed" or "not armed" alarm.

## Patterns
A pattern is selected randomly from the following cases and stays the same for `PATTERN_KEEP_FOR` amount of time (2 hours).
All patterns are based on the `MAIN_DELAY` betwen frames, currently 900ms.

Pattern 01
![Pattern 01](images/pattern_01.gif)

Pattern 02
![Pattern 02](images/pattern_02.gif)

Pattern 03
![Pattern 03](images/pattern_03.gif)

Pattern 04
![Pattern 04](images/pattern_04.gif)

Pattern 05 - The most classic one
![Pattern 05](images/pattern_05.gif)

Pattern 06
![Pattern 06](images/pattern_06.gif)

Pattern 07
![Pattern 07](images/pattern_07.gif)
