import sys
import math
MRFI_RADIO_OSC_FREQ=26000000
PHY_PREAMBLE_SYNC_BYTES=8

def calc_data_rate(fg3, fg4):
    # mantissa is in MDMCFG3
    mantissa = 256 + fg3

    # exponent is lower nibble of MDMCFG4. 
    exponent = 28 - (fg4 & 0x0F)

    # we can now get data rate 
    dataRate = mantissa * (MRFI_RADIO_OSC_FREQ>>exponent)
    return dataRate
dataRate=0
bits=0
exponent=0
mantissa=0

if len(sys.argv) == 2:
    target_rate = float(sys.argv[1])
    
    DRATE_E = int(math.log((target_rate * pow(2,20)) / MRFI_RADIO_OSC_FREQ, 2))
    DRATE_M = int(round((target_rate * pow(2,28)) / (MRFI_RADIO_OSC_FREQ * pow(2, DRATE_E)) - 256))

    if DRATE_M > 255:
        DRATE_E = DRATE_E +1
        DRATE_M = 0
        
    MDMCFG3 = DRATE_M
    MDMCFG4 = DRATE_E | 0x80
    
    actual_rate = calc_data_rate(MDMCFG3, MDMCFG4)
    print "For data rate = ", target_rate, "set registers:"
    print "MDMCFG3 = ", hex(MDMCFG3)
    print "MDMCFG4 = ", hex(MDMCFG4)
    print "Actual data rate",actual_rate
    
elif len(sys.argv) == 3:
    SMARTRF_SETTING_MDMCFG3 = int(sys.argv[1], 16)
    SMARTRF_SETTING_MDMCFG4 = int(sys.argv[2], 16)
    dataRate = calc_data_rate(SMARTRF_SETTING_MDMCFG3, SMARTRF_SETTING_MDMCFG4)
    
    print "Calculated dataRate =",dataRate
else:
    print "Provide either one argument (a data rate), which will print the registers you need to set, or the register settings, which will print the datarate."