(Created by G-code exporter)
(Fri Nov  2 00:03:01 2012)
(Units: mm)
(Board size: 50.80 x 25.40 mm)
(Accuracy 600 dpi)
(Tool diameter: 0.200000 mm)
#100=2.000000  (safe Z)
#101=0.000000  (cutting depth)
#102=25.000000  (plunge feedrate)
#103=50.000000  (feedrate)
(no predrilling)
(---------------------------------)
G17 G21 G90 G64 P0.003 M3 S3000 M7
G0 Z#100
(polygon 1)
G0 X22.733000 Y13.546667    (start point)
G1 Z#101 F#102
F#103
G1 X22.521333 Y13.462000
G1 X22.267333 Y13.292667
G1 X2.413000 Y13.292667
G1 X2.159000 Y13.081000
G1 X1.947333 Y12.827000
G1 X1.947333 Y12.530667
G1 X2.159000 Y12.276667
G1 X2.413000 Y12.065000
G1 X22.267333 Y12.065000
G1 X22.606000 Y11.853333
G1 X23.156333 Y11.853333
G1 X23.495000 Y12.065000
G1 X23.706667 Y12.403667
G1 X23.706667 Y12.954000
G1 X23.495000 Y13.292667
G1 X23.156333 Y13.504333
G1 X22.733000 Y13.546667
G0 Z#100
(polygon end, distance 45.38)
(milling distance 45.38mm = 1.79in)
M5 M9 M2
