#!/bin/bash
./main.out test/test1.inp test/test1.out
python scripts/PostProcess.py test/test1.inp test/test1.out initial1.png deformed1.png

./main.out test/test2.inp test/test2.out
python scripts/PostProcess.py test/test2.inp test/test2.out initial2.png deformed2.png

./main.out test/test4.inp test/test4.out
python scripts/PostProcess.py test/test3.inp test/test3.out initial3.png deformed3.png

./main.out test/test4.inp test/test4.out
python scripts/PostProcess.py test/test4.inp test/test4.out initial4.png deformed4.png
