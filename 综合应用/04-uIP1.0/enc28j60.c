//===========================================================================//
//                                                                           //
// 文件：  ENC28J60.C                                                        //
// 说明：  BW-DK5438开发板以太网ENC28J60驱动程序                             //
//         由Microchip ENC28J60 Ethernet Interface Driver ATMEGA8版移植      //
// 编译：  IAR Embedded Workbench IDE for MSP430 v4.21                       //
// 版本：  v1.1                                                              //
// 编写：  JASON.ZHANG                                                       //
// 版权：  北京拓普博维电子技术有限公司                                      //
// 日期：  2010.09.25                                                        //
//                                                                           //
//===========================================================================//
#include "msp430x54x.h"
#include <stdlib.h>
#include <stdio.h>
#include "PIN_DEF.H"

#include "enc28j60.h"
//#include "timeout.h"

static uint8_t Enc28j60Bank                 ;
static uint16_t NextPacketPtr               ;

void Init_J60SPI(void)                        // ENC28J60占用SPI端口
{  
  P3OUT    |=  TP_CS                        ; // 关闭触摸屏
  P3DIR    |=  TP_CS                        ;
  P5OUT    |=  J60_CS                       ;
  P5DIR    |=  J60_CS                       ;
  P10DIR   &= ~PMISO                        ;
  P10REN   |=  PMISO                        ;
  P10DIR   |=  PMOSI+PSCK                   ;
  P10OUT   &=~(PMOSI+PSCK)                  ;
  __delay_cycles(20)                        ;  
}

void Init_TPSPI(void)                         // 触摸屏占用SPI端口
{  
  P10OUT   &= PSCK                          ;
  P10DIR   |= PMOSI+PSCK+PNSS               ;
  P10DIR   &=~PMISO                         ;
  P10REN   |= PMISO                         ;
  P3OUT    |= TP_CS                         ;  
  P3DIR    |= TP_CS                         ;  
  P5DIR    &=~J60_CS                        ; // 关ENC28J60片选
}

unsigned char SPI_RW_Char(unsigned char data)
{
  unsigned char temp,i,cRec=0               ;
  temp = data                               ;
  for(i=0;i<8;i++)
  {
    if(P10IN&PMISO)
      cRec    |= (1<<(7-i))                 ;        
    if(temp&0x80)
      P10OUT  |= PMOSI                      ;
    else             
      P10OUT  &=~PMOSI                      ;
    P10OUT    |= PSCK                       ;
    P10OUT    &=~PSCK                       ;
    temp     <<=  1                         ;
  }  
  return cRec                               ;
}

uint8_t enc28j60ReadOp(uint8_t op, uint8_t address)
{
  unsigned char data                          ;  
  CLR_J60_CS                                  ;                         
  SPI_RW_Char(op | (address & ADDR_MASK))     ;
  data=SPI_RW_Char(0x00)                      ;
  if(address & 0x80)
  {
    data = SPI_RW_Char(0x00)                  ;
  }
  SET_J60_CS                                  ;                         
  return data                                 ;
}

void enc28j60WriteOp(uint8_t op, uint8_t address, uint8_t data)
{
  CLR_J60_CS                                  ;                         
  SPI_RW_Char(op | (address & ADDR_MASK))     ;
  SPI_RW_Char(data)                           ;
  SET_J60_CS                                  ;                         
}

void enc28j60ReadBuffer(uint16_t len, uint8_t* data)
{
  CLR_J60_CS                                  ;                         
  SPI_RW_Char(ENC28J60_READ_BUF_MEM)          ;
  while(len--)
  {
    *data++   = SPI_RW_Char(0x00)             ;
  }		
  *data  = 0                                  ;
  SET_J60_CS                                  ;                         
}

void enc28j60WriteBuffer(uint16_t len, uint8_t* data)
{
  CLR_J60_CS                                  ;                         
  SPI_RW_Char(ENC28J60_WRITE_BUF_MEM)         ;
  while(len--)
  {
    SPI_RW_Char(*data++)                      ;
  }
  SET_J60_CS                                  ;                         
}

         // set the bank (if needed)
void enc28j60SetBank(uint8_t address)
{
  if((address & BANK_MASK) != Enc28j60Bank)
  {
    enc28j60WriteOp(ENC28J60_BIT_FIELD_CLR, ECON1, (ECON1_BSEL1|ECON1_BSEL0));
    enc28j60WriteOp(ENC28J60_BIT_FIELD_SET, ECON1, (address & BANK_MASK)>>5);
    Enc28j60Bank = (address & BANK_MASK);
  }
}

uint8_t enc28j60Read(uint8_t address)
{
  // set the bank
  enc28j60SetBank(address);
        // do the read
  return enc28j60ReadOp(ENC28J60_READ_CTRL_REG, address);
}

void enc28j60Write(uint8_t address, uint8_t data)
{
        // set the bank
  enc28j60SetBank(address);
        // do the write
  enc28j60WriteOp(ENC28J60_WRITE_CTRL_REG, address, data);
}

void enc28j60PhyWrite(uint8_t address, uint16_t data)
{
        // set the PHY register address
   enc28j60Write(MIREGADR, address);
        // write the PHY data
   enc28j60Write(MIWRL, data);
   enc28j60Write(MIWRH, data>>8);
        // wait until the PHY write completes
   while(enc28j60Read(MISTAT) & MISTAT_BUSY){
                __delay_cycles(3000);
        }
}


void enc28j60Init(uint8_t* macaddr)
{
// perform system reset
  enc28j60WriteOp(ENC28J60_SOFT_RESET, 0, ENC28J60_SOFT_RESET);
  delay_ms(50);
	// check CLKRDY bit to see if reset is complete
        // The CLKRDY does not work. See Rev. B4 Silicon Errata point. Just wait.
	//while(!(enc28j60Read(ESTAT) & ESTAT_CLKRDY));
	// do bank 0 stuff
	// initialize receive buffer
	// 16-bit transfers, must write low byte first
	// set receive buffer start address
  NextPacketPtr = RXSTART_INIT;  // Rx start
        
  enc28j60Write(ERXSTL, RXSTART_INIT&0xFF);// set receive pointer address
  enc28j60Write(ERXSTH, RXSTART_INIT>>8);	
  enc28j60Write(ERXRDPTL, RXSTART_INIT&0xFF);// RX end
  enc28j60Write(ERXRDPTH, RXSTART_INIT>>8);
  enc28j60Write(ERXNDL, RXSTOP_INIT&0xFF);
  enc28j60Write(ERXNDH, RXSTOP_INIT>>8);	
  enc28j60Write(ETXSTL, TXSTART_INIT&0xFF);// TX start
  enc28j60Write(ETXSTH, TXSTART_INIT>>8);
  enc28j60Write(ETXNDL, TXSTOP_INIT&0xFF);// TX end
  enc28j60Write(ETXNDH, TXSTOP_INIT>>8);
	// do bank 1 stuff, packet filter:
        // For broadcast packets we allow only ARP packtets
        // All other packets should be unicast only for our mac (MAADR)
        //
        // The pattern to match on is therefore
        // Type     ETH.DST
        // ARP      BROADCAST
        // 06 08 -- ff ff ff ff ff ff -> ip checksum for theses bytes=f7f9
        // in binary these poitions are:11 0000 0011 1111
        // This is hex 303F->EPMM0=0x3f,EPMM1=0x30
  enc28j60Write(ERXFCON, ERXFCON_UCEN|ERXFCON_CRCEN|ERXFCON_PMEN);
  enc28j60Write(EPMM0, 0x3f);
  enc28j60Write(EPMM1, 0x30);
  enc28j60Write(EPMCSL, 0xf9);
  enc28j60Write(EPMCSH, 0xf7);
        //
        //
	// do bank 2 stuff
	// enable MAC receive
  enc28j60Write(MACON1, MACON1_MARXEN|MACON1_TXPAUS|MACON1_RXPAUS);
	// bring MAC out of reset
  enc28j60Write(MACON2, 0x00);
	// enable automatic padding to 60bytes and CRC operations
  enc28j60WriteOp(ENC28J60_BIT_FIELD_SET, MACON3, MACON3_PADCFG0|MACON3_TXCRCEN|MACON3_FRMLNEN);
	// set inter-frame gap (non-back-to-back)
  enc28j60Write(MAIPGL, 0x12);
  enc28j60Write(MAIPGH, 0x0C);
	// set inter-frame gap (back-to-back)
  enc28j60Write(MABBIPG, 0x12);
	// Set the maximum packet size which the controller will accept
        // Do not send packets longer than MAX_FRAMELEN:
  enc28j60Write(MAMXFLL, MAX_FRAMELEN&0xFF);	
  enc28j60Write(MAMXFLH, MAX_FRAMELEN>>8);
	// do bank 3 stuff
        // write MAC address
        // NOTE: MAC address in ENC28J60 is byte-backward
  enc28j60Write(MAADR5, macaddr[0]);
  enc28j60Write(MAADR4, macaddr[1]);
  enc28j60Write(MAADR3, macaddr[2]);
  enc28j60Write(MAADR2, macaddr[3]);
  enc28j60Write(MAADR1, macaddr[4]);
  enc28j60Write(MAADR0, macaddr[5]);
	// no loopback of transmitted frames
  enc28j60PhyWrite(PHCON2, PHCON2_HDLDIS);
	// switch to bank 0
  enc28j60SetBank(ECON1);
	// enable interrutps
  enc28j60WriteOp(ENC28J60_BIT_FIELD_SET, EIE, EIE_INTIE|EIE_PKTIE);
	// enable packet reception
  enc28j60WriteOp(ENC28J60_BIT_FIELD_SET, ECON1, ECON1_RXEN);
}

// read the revision of the chip:
uint8_t enc28j60getrev(void)
{
  return(enc28j60Read(EREVID));
}

void enc28j60PacketSend(uint16_t len, uint8_t* packet)
{
	// Set the write pointer to start of transmit buffer area
	enc28j60Write(EWRPTL, TXSTART_INIT&0xFF);
	enc28j60Write(EWRPTH, TXSTART_INIT>>8);
	// Set the TXND pointer to correspond to the packet size given
	enc28j60Write(ETXNDL, (TXSTART_INIT+len)&0xFF);
	enc28j60Write(ETXNDH, (TXSTART_INIT+len)>>8);
	// write per-packet control byte (0x00 means use macon3 settings)
	enc28j60WriteOp(ENC28J60_WRITE_BUF_MEM, 0, 0x00);
	// copy the packet into the transmit buffer
	enc28j60WriteBuffer(len, packet);
	// send the contents of the transmit buffer onto the network
	enc28j60WriteOp(ENC28J60_BIT_FIELD_SET, ECON1, ECON1_TXRTS);
        // Reset the transmit logic problem. See Rev. B4 Silicon Errata point 12.
	if( (enc28j60Read(EIR) & EIR_TXERIF) ){
                enc28j60WriteOp(ENC28J60_BIT_FIELD_CLR, ECON1, ECON1_TXRTS);
        }
}

// Gets a packet from the network receive buffer, if one is available.
// The packet will by headed by an ethernet header.
//      maxlen  The maximum acceptable length of a retrieved packet.
//      packet  Pointer where packet data should be stored.
// Returns: Packet length in bytes if a packet was retrieved, zero otherwise.
uint16_t enc28j60PacketReceive(uint16_t maxlen, uint8_t* packet)
{
	uint16_t rxstat;
	uint16_t len;
	// check if a packet has been received and buffered
	//if( !(enc28j60Read(EIR) & EIR_PKTIF) ){
        // The above does not work. See Rev. B4 Silicon Errata point 6.
	if( enc28j60Read(EPKTCNT) ==0 ){
		return(0);
        }

	// Set the read pointer to the start of the received packet
	enc28j60Write(ERDPTL, (NextPacketPtr));
	enc28j60Write(ERDPTH, (NextPacketPtr)>>8);
	// read the next packet pointer
	NextPacketPtr  = enc28j60ReadOp(ENC28J60_READ_BUF_MEM, 0);
	NextPacketPtr |= enc28j60ReadOp(ENC28J60_READ_BUF_MEM, 0)<<8;
	// read the packet length (see datasheet page 43)
	len  = enc28j60ReadOp(ENC28J60_READ_BUF_MEM, 0);
	len |= enc28j60ReadOp(ENC28J60_READ_BUF_MEM, 0)<<8;
        len-=4; //remove the CRC count
	// read the receive status (see datasheet page 43)
	rxstat  = enc28j60ReadOp(ENC28J60_READ_BUF_MEM, 0);
	rxstat |= enc28j60ReadOp(ENC28J60_READ_BUF_MEM, 0)<<8;
	// limit retrieve length
        if (len>maxlen-1){
                len=maxlen-1;
        }
        // check CRC and symbol errors (see datasheet page 44, table 7-3):
        // The ERXFCON.CRCEN is set by default. Normally we should not
        // need to check this.
        if ((rxstat & 0x80)==0){
                // invalid
                len=0;
        }else{
                // copy the packet from the receive buffer
                enc28j60ReadBuffer(len, packet);
        }
	// Move the RX read pointer to the start of the next received packet
	// This frees the memory we just read out
	enc28j60Write(ERXRDPTL, (NextPacketPtr));
	enc28j60Write(ERXRDPTH, (NextPacketPtr)>>8);
	// decrement the packet counter indicate we are done with this packet
	enc28j60WriteOp(ENC28J60_BIT_FIELD_SET, ECON2, ECON2_PKTDEC);
	return(len);
}

void ENC_SLEEP(void)
{
  enc28j60WriteOp(ENC28J60_BIT_FIELD_CLR, ECON1, ECON1_RXEN) ;
  enc28j60WriteOp(ENC28J60_BIT_FIELD_SET, ECON2, ECON2_VRPS) ;
  enc28j60WriteOp(ENC28J60_BIT_FIELD_SET, ECON2, ECON2_PWRSV);  
}

void ENC_WAKEUP(void)
{
  enc28j60WriteOp(ENC28J60_BIT_FIELD_CLR, ECON2, ECON2_PWRSV);
  delay_ms(1)                                                ;  
  enc28j60WriteOp(ENC28J60_BIT_FIELD_SET, ECON1, ECON1_RXEN) ;  
}

void delay_ms(unsigned int ms)
{
  unsigned int i                                            ;
  for(i=0;i<ms;i++)
    __delay_cycles(20000)                                    ;
}
