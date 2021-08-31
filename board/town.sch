EESchema Schematic File Version 4
EELAYER 30 0
EELAYER END
$Descr A4 11693 8268
encoding utf-8
Sheet 1 1
Title ""
Date ""
Rev ""
Comp ""
Comment1 ""
Comment2 ""
Comment3 ""
Comment4 ""
$EndDescr
$Comp
L Connector_Generic:Conn_02x05_Odd_Even EXP1
U 1 1 610C7AEC
P 6250 2250
F 0 "EXP1" H 6300 2667 50  0000 C CNN
F 1 "Panel Connector" H 6300 2576 50  0000 C CNN
F 2 "Connector_PinHeader_2.54mm:PinHeader_2x05_P2.54mm_Vertical" H 6250 2250 50  0001 C CNN
F 3 "~" H 6250 2250 50  0001 C CNN
	1    6250 2250
	1    0    0    -1  
$EndComp
$Comp
L Connector_Generic:Conn_02x05_Odd_Even EXP2
U 1 1 610C83EE
P 6250 3150
F 0 "EXP2" H 6300 3567 50  0000 C CNN
F 1 "Panel Connector" H 6300 3476 50  0000 C CNN
F 2 "Connector_PinHeader_2.54mm:PinHeader_2x05_P2.54mm_Vertical" H 6250 3150 50  0001 C CNN
F 3 "~" H 6250 3150 50  0001 C CNN
	1    6250 3150
	1    0    0    -1  
$EndComp
$Comp
L Connector:Conn_01x03_Female J2
U 1 1 610D19E0
P 4100 4650
F 0 "J2" H 4128 4676 50  0000 L CNN
F 1 "LED Strip Connector" H 4128 4585 50  0000 L CNN
F 2 "Connector_JST:JST_XH_S3B-XH-A-1_1x03_P2.50mm_Horizontal" H 4100 4650 50  0001 C CNN
F 3 "~" H 4100 4650 50  0001 C CNN
	1    4100 4650
	1    0    0    -1  
$EndComp
$Comp
L Connector:Conn_01x05_Female J1
U 1 1 610C9A8F
P 2150 4700
F 0 "J1" H 2178 4726 50  0000 L CNN
F 1 "Charger Connector" H 2178 4635 50  0000 L CNN
F 2 "Connector_JST:JST_PH_B5B-PH-K_1x05_P2.00mm_Vertical" H 2150 4700 50  0001 C CNN
F 3 "~" H 2150 4700 50  0001 C CNN
	1    2150 4700
	1    0    0    -1  
$EndComp
$Comp
L power:GND #PWR0101
U 1 1 610D3484
P 1300 4800
F 0 "#PWR0101" H 1300 4550 50  0001 C CNN
F 1 "GND" V 1305 4672 50  0000 R CNN
F 2 "" H 1300 4800 50  0001 C CNN
F 3 "" H 1300 4800 50  0001 C CNN
	1    1300 4800
	0    1    1    0   
$EndComp
$Comp
L Device:R R1
U 1 1 610D6D94
P 2100 6150
F 0 "R1" H 2170 6196 50  0000 L CNN
F 1 "100K" H 2170 6105 50  0000 L CNN
F 2 "Resistor_SMD:R_0603_1608Metric_Pad0.98x0.95mm_HandSolder" V 2030 6150 50  0001 C CNN
F 3 "~" H 2100 6150 50  0001 C CNN
	1    2100 6150
	1    0    0    -1  
$EndComp
$Comp
L Device:R R2
U 1 1 610D7360
P 2100 6650
F 0 "R2" H 2170 6696 50  0000 L CNN
F 1 "100K" H 2170 6605 50  0000 L CNN
F 2 "Resistor_SMD:R_0603_1608Metric_Pad0.98x0.95mm_HandSolder" V 2030 6650 50  0001 C CNN
F 3 "~" H 2100 6650 50  0001 C CNN
	1    2100 6650
	1    0    0    -1  
$EndComp
$Comp
L Device:C C1
U 1 1 610D799F
P 1600 6650
F 0 "C1" H 1715 6696 50  0000 L CNN
F 1 "100n" H 1715 6605 50  0000 L CNN
F 2 "Capacitor_SMD:C_0603_1608Metric_Pad1.08x0.95mm_HandSolder" H 1638 6500 50  0001 C CNN
F 3 "~" H 1600 6650 50  0001 C CNN
	1    1600 6650
	1    0    0    -1  
$EndComp
Wire Wire Line
	1300 6400 1600 6400
Wire Wire Line
	1600 6400 1600 6500
Connection ~ 1600 6400
Wire Wire Line
	2100 6950 2100 6800
Wire Wire Line
	2100 6400 2100 6500
Wire Wire Line
	1600 6400 2100 6400
Wire Wire Line
	2100 6300 2100 6400
Connection ~ 2100 6400
Wire Wire Line
	1600 6800 1600 6950
Wire Wire Line
	1600 6950 2100 6950
Wire Wire Line
	2100 6950 2300 6950
Connection ~ 2100 6950
Wire Wire Line
	2100 6000 2100 5900
Wire Wire Line
	2100 5900 2300 5900
$Comp
L Connector:Conn_01x14_Female H1
U 1 1 610F2F28
P 1050 2600
F 0 "H1" H 942 3385 50  0000 C CNN
F 1 "Header" H 942 3294 50  0000 C CNN
F 2 "Connector_PinHeader_2.54mm:PinHeader_1x14_P2.54mm_Vertical" H 1050 2600 50  0001 C CNN
F 3 "~" H 1050 2600 50  0001 C CNN
	1    1050 2600
	-1   0    0    -1  
$EndComp
Wire Wire Line
	1250 2100 1950 2100
Wire Wire Line
	1250 2200 1950 2200
Wire Wire Line
	1250 2300 1950 2300
Wire Wire Line
	1250 2400 1950 2400
Wire Wire Line
	1250 2500 1950 2500
Wire Wire Line
	1250 2600 1950 2600
Wire Wire Line
	1950 2700 1250 2700
Wire Wire Line
	1250 2800 1950 2800
Wire Wire Line
	1950 2900 1250 2900
Wire Wire Line
	1250 3000 1950 3000
Wire Wire Line
	1950 3100 1250 3100
Wire Wire Line
	1250 3200 1950 3200
Wire Wire Line
	1950 3300 1250 3300
Text Label 1900 2000 2    50   ~ 0
GND
Text Label 3950 2800 0    50   ~ 0
SW_KILL
Text Label 3950 2500 0    50   ~ 0
STRIP_LED_DIN
Text Label 1900 2600 2    50   ~ 0
PANEL_LED_DIN
Text Label 1900 2700 2    50   ~ 0
BEEP
Text Label 1900 2800 2    50   ~ 0
LCD_RST
Text Label 1900 2900 2    50   ~ 0
LCD_A0
Text Label 1900 3000 2    50   ~ 0
LCD_CS
Text Label 1900 3100 2    50   ~ 0
SDCARD_CS
Text Label 1900 3200 2    50   ~ 0
MOSI
Text Label 1900 3300 2    50   ~ 0
MISO
$Comp
L Connector:Conn_01x14_Female H2
U 1 1 61106453
P 4850 2700
F 0 "H2" H 4742 3485 50  0000 C CNN
F 1 "Header" H 4742 3394 50  0000 C CNN
F 2 "Connector_PinHeader_2.54mm:PinHeader_1x14_P2.54mm_Vertical" H 4850 2700 50  0001 C CNN
F 3 "~" H 4850 2700 50  0001 C CNN
	1    4850 2700
	1    0    0    1   
$EndComp
Text Label 3950 3200 0    50   ~ 0
SCK
Text Label 3950 3100 0    50   ~ 0
BTN_EN1
Text Label 3950 3000 0    50   ~ 0
BTN_EN2
Text Label 3950 2700 0    50   ~ 0
BTN_ENC
Text Label 3950 2000 0    50   ~ 0
VIN
Text Label 3950 2600 0    50   ~ 0
BATTMON
Text Label 1300 6400 2    50   ~ 0
BATTMON
Text Label 2300 5900 0    50   ~ 0
VLIPO
Text Label 2300 6950 0    50   ~ 0
GND
Text Label 3900 4750 2    50   ~ 0
VIN
Text Label 3900 4650 2    50   ~ 0
STRIP_LED_DIN
Text Label 3900 4550 2    50   ~ 0
STRIP_LED_GND
Text Label 1850 4500 2    50   ~ 0
PGOOD
Text Label 1850 4600 2    50   ~ 0
CHG
Text Label 1850 4700 2    50   ~ 0
VIN
Text Label 1850 4800 2    50   ~ 0
GND
Text Label 1850 4900 2    50   ~ 0
VLIPO
Text Label 6050 2950 2    50   ~ 0
SW_KILL
Text Label 6050 3050 2    50   ~ 0
SW_RST
Text Label 6050 3150 2    50   ~ 0
MOSI
Text Label 6050 3250 2    50   ~ 0
SDCARD_CS
Text Label 6050 3350 2    50   ~ 0
SCK
Text Label 6550 2950 0    50   ~ 0
GND
Text Label 6550 3050 0    50   ~ 0
SDCARD_CD
Text Label 6550 3150 0    50   ~ 0
BTN_EN2
Text Label 6550 3250 0    50   ~ 0
BTN_EN1
Text Label 6550 3350 0    50   ~ 0
MISO
Text Label 6050 2050 2    50   ~ 0
VIN
Text Label 6050 2250 2    50   ~ 0
PANEL_LED_DIN
Text Label 6050 2350 2    50   ~ 0
LCD_A0
Text Label 6050 2450 2    50   ~ 0
BTN_ENC
Text Label 6550 2050 0    50   ~ 0
GND
Text Label 6550 2250 0    50   ~ 0
LCD_RST
Text Label 6550 2350 0    50   ~ 0
LCD_CS
Text Label 6550 2450 0    50   ~ 0
BEEP
Text Label 1900 2500 2    50   ~ 0
PGOOD
Text Label 1900 2400 2    50   ~ 0
CHG
NoConn ~ 6050 2150
NoConn ~ 6550 2150
$Comp
L power:VBUS #PWR0102
U 1 1 6113FA91
P 1300 4700
F 0 "#PWR0102" H 1300 4550 50  0001 C CNN
F 1 "VBUS" V 1315 4827 50  0000 L CNN
F 2 "" H 1300 4700 50  0001 C CNN
F 3 "" H 1300 4700 50  0001 C CNN
	1    1300 4700
	0    -1   -1   0   
$EndComp
Wire Wire Line
	1950 4600 1850 4600
Wire Wire Line
	1850 4900 1950 4900
Wire Wire Line
	1250 2000 1950 2000
Wire Wire Line
	1850 4500 1950 4500
Wire Wire Line
	1300 4700 1400 4700
Wire Wire Line
	1300 4800 1400 4800
Text Label 3950 2900 0    50   ~ 0
SDCARD_CD
NoConn ~ 5600 3050
Wire Wire Line
	5600 3050 6050 3050
$Comp
L power:PWR_FLAG #FLG0101
U 1 1 611568B8
P 1400 4700
F 0 "#FLG0101" H 1400 4775 50  0001 C CNN
F 1 "PWR_FLAG" H 1400 4873 50  0000 C CNN
F 2 "" H 1400 4700 50  0001 C CNN
F 3 "~" H 1400 4700 50  0001 C CNN
	1    1400 4700
	1    0    0    -1  
$EndComp
Connection ~ 1400 4700
Wire Wire Line
	1400 4700 1950 4700
$Comp
L power:PWR_FLAG #FLG0102
U 1 1 6115A846
P 1400 4800
F 0 "#FLG0102" H 1400 4875 50  0001 C CNN
F 1 "PWR_FLAG" H 1400 4973 50  0000 C CNN
F 2 "" H 1400 4800 50  0001 C CNN
F 3 "~" H 1400 4800 50  0001 C CNN
	1    1400 4800
	-1   0    0    1   
$EndComp
Connection ~ 1400 4800
Wire Wire Line
	1400 4800 1950 4800
$Comp
L Device:Q_NMOS_GSD Q1
U 1 1 6115D668
P 4500 6400
F 0 "Q1" H 4705 6446 50  0000 L CNN
F 1 "G6N02L" H 4705 6355 50  0000 L CNN
F 2 "Package_TO_SOT_SMD:SOT-23_Handsoldering" H 4700 6500 50  0001 C CNN
F 3 "~" H 4500 6400 50  0001 C CNN
	1    4500 6400
	1    0    0    -1  
$EndComp
Wire Wire Line
	4600 6200 4600 6050
Wire Wire Line
	4600 6050 4850 6050
Wire Wire Line
	4600 6600 4600 6750
Wire Wire Line
	4600 6750 4850 6750
Text Label 4850 6750 0    50   ~ 0
GND
Text Label 4850 6050 0    50   ~ 0
STRIP_LED_GND
$Comp
L Device:R R3
U 1 1 6116BDC8
P 4050 6400
F 0 "R3" V 3843 6400 50  0000 C CNN
F 1 "1K" V 3934 6400 50  0000 C CNN
F 2 "Resistor_SMD:R_0603_1608Metric_Pad0.98x0.95mm_HandSolder" V 3980 6400 50  0001 C CNN
F 3 "~" H 4050 6400 50  0001 C CNN
	1    4050 6400
	0    1    1    0   
$EndComp
$Comp
L Device:R R4
U 1 1 6116CC8A
P 3800 6600
F 0 "R4" H 3870 6646 50  0000 L CNN
F 1 "1M" H 3870 6555 50  0000 L CNN
F 2 "Resistor_SMD:R_0603_1608Metric_Pad0.98x0.95mm_HandSolder" V 3730 6600 50  0001 C CNN
F 3 "~" H 3800 6600 50  0001 C CNN
	1    3800 6600
	1    0    0    -1  
$EndComp
Wire Wire Line
	3800 6750 4600 6750
Connection ~ 4600 6750
Wire Wire Line
	3800 6450 3800 6400
Wire Wire Line
	3800 6400 3900 6400
Wire Wire Line
	4200 6400 4300 6400
Wire Wire Line
	3800 6400 3600 6400
Connection ~ 3800 6400
Text Label 3600 6400 2    50   ~ 0
STRIP_LED_EN
Text Label 1900 2300 2    50   ~ 0
STRIP_LED_EN
Text Label 3950 2100 0    50   ~ 0
AGND
Text Label 1900 2200 2    50   ~ 0
DOOR2
Text Label 1900 2100 2    50   ~ 0
DOOR1
Text Label 3950 2200 0    50   ~ 0
VCC
Text Label 3950 2300 0    50   ~ 0
X_A9
Text Label 3950 2400 0    50   ~ 0
X_A8
$Comp
L Connector:Conn_01x02_Female J3
U 1 1 611CA057
P 5650 4600
F 0 "J3" H 5678 4576 50  0000 L CNN
F 1 "Door Sensor" H 5678 4485 50  0000 L CNN
F 2 "Connector_JST:JST_XH_S2B-XH-A-1_1x02_P2.50mm_Horizontal" H 5650 4600 50  0001 C CNN
F 3 "~" H 5650 4600 50  0001 C CNN
	1    5650 4600
	1    0    0    -1  
$EndComp
Text Label 5450 4600 2    50   ~ 0
GND
Text Label 5450 4700 2    50   ~ 0
DOOR1
$Comp
L Connector:Conn_01x02_Female J4
U 1 1 611D52DC
P 6900 4600
F 0 "J4" H 6928 4576 50  0000 L CNN
F 1 "Door Sensor" H 6928 4485 50  0000 L CNN
F 2 "Connector_JST:JST_XH_S2B-XH-A-1_1x02_P2.50mm_Horizontal" H 6900 4600 50  0001 C CNN
F 3 "~" H 6900 4600 50  0001 C CNN
	1    6900 4600
	1    0    0    -1  
$EndComp
Text Label 6700 4600 2    50   ~ 0
GND
Text Label 6700 4700 2    50   ~ 0
DOOR2
Text Label 3950 3300 0    50   ~ 0
X_D13
$Comp
L teensy:Teensy3.2_HeadersOnly U1
U 1 1 610C56FD
P 2950 3350
F 0 "U1" H 2950 4987 60  0000 C CNN
F 1 "Teensy3.2" H 2950 4881 60  0000 C CNN
F 2 "teensy:Teensy32_HeadersOnly" H 2950 3150 60  0000 C CNN
F 3 "" H 2950 2600 60  0000 C CNN
	1    2950 3350
	1    0    0    -1  
$EndComp
Wire Wire Line
	3900 2000 4650 2000
Wire Wire Line
	4650 2100 3900 2100
Wire Wire Line
	3900 2200 4650 2200
Wire Wire Line
	4650 2300 3900 2300
Wire Wire Line
	3900 2400 4650 2400
Wire Wire Line
	3900 2500 4650 2500
Wire Wire Line
	4650 2600 3900 2600
Wire Wire Line
	3900 2700 4650 2700
Wire Wire Line
	4650 2800 3900 2800
Wire Wire Line
	3900 2900 4650 2900
Wire Wire Line
	4650 3000 3900 3000
Wire Wire Line
	3900 3100 4650 3100
Wire Wire Line
	4650 3200 3900 3200
Wire Wire Line
	3900 3300 4650 3300
$EndSCHEMATC
