# scull
LDD3 scull device source code as per book examples

Installation:

1) make
2) sudo ./scull_load.sh

After successful load it will create below files in filesystem:
1) Char device file: /dev/scull0
2) Scull debugging file: /proc/scullmem

To remove scull when you are done playing:
1) sudo ./scull_unload.sh

Note: Use it on your own risk. In case of any issue please feel free to raise issue.

All license are as per original scull license and GPLv3
