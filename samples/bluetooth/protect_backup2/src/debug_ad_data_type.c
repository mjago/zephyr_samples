const char * ad_data_type_str(int data_type)
{
    const char * ret;
    const char * str[] =
        {
            "Flags"                                        ,
            "Incomplete List of 16-bit Service Class UUIDs",
            "Complete List of 16-bit Service Class UUIDs"  ,
            "Incomplete List of 32-bit Service Class UUIDs",
            "Complete List of 32-bit Service Class UUIDs  ",
            "Incomplete List of 128-bit Service Class UUID",
            "Complete List of 128-bit Service Class UUIDs ",
            "Shortened Local Name"                         ,
            "Complete Local Name"                          ,
            "Tx Power Level"                               ,
            "Class of Device"                              ,
            "Simple Pairing Hash C"                        ,
            "Simple Pairing Randomizer R"                  ,
            "Device ID"                                    ,
            "Security Manager Out of Band Flags"           ,
            "Slave Connection Interval Range"              ,
            "List of 16-bit Service Solicitation UUIDs"    ,
            "List of 128-bit Service Solicitation UUIDs"   ,
            "Service Data"                                 ,
            "Public Target Address"                        ,
            "Random Target Address"                        ,
            "Appearance"                                   ,
            "Advertising Interval"                         ,
            "LE Bluetooth Device Address"                  ,
            "LE Role"                                      ,
            "Simple Pairing Hash C-256"                    ,
            "Simple Pairing Randomizer R-256"              ,
            "List of 32-bit Service Solicitation UUIDs"    ,
            "Service Data - 32-bit UUID"                   ,
            "Service Data - 128-bit UUID"                  ,
            "LE Secure Connections Confirmation Value"     ,
            "LE Secure Connections Random Value"           ,
            "URI"                                          ,
            "Indoor Positioning"                           ,
            "Transport Discovery Data"                     ,
            "LE Supported Features"                        ,
            "Channel Map Update Indication"                ,
            "PB-ADV"                                       ,
            "Mesh Message"                                 ,
            "Mesh Beacon"                                  ,
            "BIGInfo"                                      ,
            "Broadcast_Code"                               ,
            "3D Information Data"                          ,
            "Manufacturer Specific Data"                   ,
            "Unknown"
        };

    switch(data_type)
    {
    case 0x01: ret = str[ 0]; break;
    case 0x02: ret = str[ 1]; break;
    case 0x03: ret = str[ 2]; break;
    case 0x04: ret = str[ 3]; break;
    case 0x05: ret = str[ 4]; break;
    case 0x06: ret = str[ 5]; break;
    case 0x07: ret = str[ 6]; break;
    case 0x08: ret = str[ 7]; break;
    case 0x09: ret = str[ 8]; break;
    case 0x0A: ret = str[ 9]; break;
    case 0x0D: ret = str[10]; break;
    case 0x0E: ret = str[11]; break;
    case 0x0F: ret = str[12]; break;
    case 0x10: ret = str[13]; break;
    case 0x11: ret = str[14]; break;
    case 0x12: ret = str[15]; break;
    case 0x14: ret = str[16]; break;
    case 0x15: ret = str[17]; break;
    case 0x16: ret = str[18]; break;
    case 0x17: ret = str[19]; break;
    case 0x18: ret = str[20]; break;
    case 0x19: ret = str[21]; break;
    case 0x1A: ret = str[22]; break;
    case 0x1B: ret = str[23]; break;
    case 0x1C: ret = str[24]; break;
    case 0x1D: ret = str[25]; break;
    case 0x1E: ret = str[26]; break;
    case 0x1F: ret = str[27]; break;
    case 0x20: ret = str[28]; break;
    case 0x21: ret = str[29]; break;
    case 0x22: ret = str[30]; break;
    case 0x23: ret = str[31]; break;
    case 0x24: ret = str[32]; break;
    case 0x25: ret = str[33]; break;
    case 0x26: ret = str[34]; break;
    case 0x27: ret = str[35]; break;
    case 0x28: ret = str[36]; break;
    case 0x29: ret = str[37]; break;
    case 0x2A: ret = str[38]; break;
    case 0x2B: ret = str[39]; break;
    case 0x2C: ret = str[40]; break;
    case 0x2D: ret = str[41]; break;
    case 0x3D: ret = str[42]; break;
    case 0xFF: ret = str[43]; break;
    default:   ret = str[44]; break;
    }
    return ret;
}
