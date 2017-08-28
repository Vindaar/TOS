#ifndef TOS_PROTOCOL_HPP
#define TOS_PROTOCOL_HPP

// MAC-Adressen

#define MAC_ADDR "AA:BA:DD:EC:AD:E2"    // Xilinx
// #define MAC_ADDR "00:0A:35:01:E3:21" // SRS

// IP-Adressen

#define IP_ADRESSE "10.1.2.2"    // Xilinx
// #define IP_ADRESSE "10.0.1.2" // SRS
// #define IP_ADRESSE "134.93.131.42" // Uni Mainz

/** packet length in bytes
 * kann bis 1500-X erhoeht werden PLen=1024 ergibt PQueue=113 => 113 Pakete
 *
 * take care> no packet should be shorter than 43 byte date such that in FPGA
 * tx counter goes higher than 103 to start D2 signal to timepix AND PLen has
 * to be dividable by 4 as one hit in 0suppressed data has 4 byte.
 * Otherwise there will be an error cassed by
 * FPGA::SaveData(int hit_x_y_val[12288] ,int NumHits) in fpga.cpp.
 */
#define PLen 1400

/** 256 pixels per x and y dimension of timepix chip
 */
#define PIXPD 256
#define BIT_PER_PIX 14

/** Anzahl der Pakete
 * (a+b-1)/b liefert immer aufgerundete Division z.B. a=5,b=3 => (5+ 3-1)/3=2
 */
// PQueue is 82 in case of PLen == 1400 and 83 for PLen == 1399
#define PQueue (PIXPD * PIXPD * BIT_PER_PIX / 8 + 33 + PLen - 1)/PLen //pre and postload> 256bit+8bit=264bit=33byte



#endif
