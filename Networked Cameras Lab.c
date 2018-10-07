#include "basic_io.h"
#include "test.h"
#include "LCD.h"
#include "DM9000A.C"
#include "VGA.C"
#include "system.h"
#include "sys/alt_irq.h"
#include "sys/alt_flash.h"

unsigned int aaa, rx_len, i, j, k, l, packet_num, packet_num_disp;
int value = 0;
unsigned short transmit_flag = 0, L2_flag = 0, rotate90_flag;
unsigned char RXT[1500];
char prep_data[640*480/8];

unsigned short int src_port = 1001;
unsigned short int dest_port = 5000;
unsigned int src_address = (172 << 24) + (16 << 16) + (0 << 8) + 12;
unsigned int dest_address = (172 << 24) + (16 << 16) + (254 << 8) + 1;
//MAC addresses in hex
unsigned char src_MAC[6];
unsigned char dest_MAC[] = {0x01, 0x60, 0x6E, 0x11, 0x02, 0xC2};
unsigned char packet;
unsigned char bin_pix[640*480];

void ethernet_interrupts(){
    char bit_pix;
    int offset = 0;
    int check= 1;
    
    alt_flash_fd* fd;
    fd = alt_flash_open_dev(CFI_FLASH_0_NAME);
    
    packet_num++;
    aaa=ReceivePacket (RXT,&rx_len);
    if(!aaa)
    {
        
        printf("Packet Received\n");
        switch(RXT[1]){ 
            case 0: //get image
               printf("Store\n");
               for(i = 0 ; i < 480 ; i++)
                {
                    for(j = 0 ; j < 640 ; j++)
                       {       
                        alt_read_flash(fd,offset,&bit_pix,1);
                        //printf("%x", bit_pix);
                        bin_pix[offset] = bit_pix;
                        offset++;
                        /*if(bit_pix){
                           Vga_Set_Pixel(VGA_0_BASE,j,i);
                        }
                        else{
                           Vga_Clr_Pixel(VGA_0_BASE,j,i);   
                        }*/
                       }
                }    
               printf("Finished storing\n");
               RXT[1] = 0xF0;
               TransmitPacket(RXT, 0x40);
               msleep(500);
            break;
            case 1:
                transmit_flag = 1;
                RXT[1] = 0xF0;
                TransmitPacket(RXT, 0x40);
                msleep(500);
            break;
            case 2:
                printf("Process 2\n");
                offset = 0;
                value = -1;
                packet = 0;
                for(i = 0; i < 480; i++){
                    for(j = 0; j < 640; j++){ 
                       if(offset%8 == 0){
                            value++; 
                            prep_data[value] = packet;
                            packet = 0x00;
                       }
                       packet = packet + (bin_pix[offset] << (7-(j%8)));
                       offset++;
                    }
                }    
                offset = 0;
                value = -1;
                printf("Finish compression\n");
                RXT[1] = 0xF0;
                TransmitPacket(RXT, 0x40);
                msleep(500);        
            break;
            case 3: //image proccess - Invert
                printf("Process 3\n");
                offset = 0;
                value = -1;
                packet = 0;
                for(i = 0; i < 480; i++){
                    for(j = 0; j < 640; j++){ 
                       if(offset%8 == 0){
                            value++; 
                            prep_data[value] = packet;
                            packet = 0x00;
                       }
                       packet = packet + (~bin_pix[offset] << (7-(j%8)));
                       offset++;
                    }
                }    
                RXT[1] = 0xF0;
                TransmitPacket(RXT, 0x40);
                msleep(500); 
            break;
            case 4: //image proccess - Mirror
                printf("Process 4\n");
                offset = 1;
                value = -1;
                packet = 0;
                for(i = 0; i < 479; i++){
                    offset = 1;
                    for(j = 0; j < 640; j++){ 
                       if(offset%8 == 0){
                            value++; 
                            prep_data[value] = packet;
                            packet = 0x00;
                       }
                       packet = packet + (bin_pix[(i+1)*640 - offset] << (7-(j%8)));
                       offset++;
                    }
                }    
                RXT[1] = 0xF0;
                TransmitPacket(RXT, 0x40);
                msleep(500); 
            break;
            case 5: //image proccess - Flip
                printf("Process 5\n");
                offset = 640*480;
                value = -1;
                packet = 0;
                for(i = 0; i < 479; i++){
                    for(j = 0; j < 640; j++){ 
                       if(offset%8 == 0){
                            value++; 
                            prep_data[value] = packet;
                            packet = 0x00;
                       }
                       packet = packet + (bin_pix[offset] << (7-(j%8)));
                       offset--;
                    }
                }    
                RXT[1] = 0xF0;
                TransmitPacket(RXT, 0x40);
                msleep(500); 
            break;
            case 6:
                rotate90_flag = 1;
                printf("Process 6\n");
                offset = 0;
                value = -1;
                packet = 0;
                for(i = 0; i < 480; i++){
                    for(j = 0; j < 640; j++){ 
                       if(offset%8 == 0){
                            value++; 
                            prep_data[value] = packet;
                            packet = 0x00;
                       }
                       packet = packet + (bin_pix[offset] << (7-(j%8)));
                       offset++;
                    }
                }
                RXT[1] = 0xF0;
                TransmitPacket(RXT, 0x40);
                msleep(500); 
            break;
            case(0xC0):
                if(rotate90_flag == 0){
                    printf("Display\n");
                    RXT[1] = 0xF0;
                    TransmitPacket(RXT, 0x40);
                    msleep(500);
                    
                    for(k=0 ; k<1280 ; k++){
                        unsigned char pix_byte = RXT[43+k];
                        for(j = 0; j < 8; j++){
                            packet = (pix_byte >> (7-(j%8)));
                            bit_pix = (packet & (0x01));
                            if(bit_pix){
                                Vga_Set_Pixel(VGA_0_BASE,value%640,value/640);
                            }
                            else{
                                Vga_Clr_Pixel(VGA_0_BASE,value%640,value/640);   
                            }
                            value++;
                        }
                    }
                }
                else{
                    printf("Display\n");
                    RXT[1] = 0xF0;
                    TransmitPacket(RXT, 0x40);
                    msleep(500);
                    
                    for(k=0 ; k<1280 ; k++){
                        unsigned char pix_byte = RXT[43+k];
                        for(j = 0; j < 8; j++){
                            packet = (pix_byte >> (7-(j%8)));
                            bit_pix = (packet & (0x01));
                            if(bit_pix){
                                Vga_Set_Pixel(VGA_0_BASE,639-(value/640),value%640);
                            }
                            else{
                                Vga_Clr_Pixel(VGA_0_BASE,639-(value/640),value%640);   
                            }
                            value++;
                        }
                    }
                }
            break; 
            case(0xF0):
                printf("ACK sent\n");
                outport(LED_GREEN_BASE, 0xFF);
                if(L2_flag == 1){
                    RXT[1] = 0xF0;
                    TransmitPacket(RXT, 0x40);
                    msleep(100);
                }
                L2_flag = 0;
            break; 
            case(0xFF):
                printf("NAK sent\n");
                outport(LED_RED_BASE, 0x3FFFF);
                L2_flag = 0;
            break;
            default:
                L2_flag = 2;
                printf("Unknown Command\n");
                RXT[1] = 0xFF;
                TransmitPacket(RXT, 0x40);
                msleep(500);
            break;
        }
    }
    msleep(100);
    outport(LED_RED_BASE, 0x0);
    outport(LED_GREEN_BASE, 0x0);
}

int main(void)
{
  VGA_Ctrl_Reg vga_ctrl_set;
 
  vga_ctrl_set.VGA_Ctrl_Flags.RED_ON    = 1;
  vga_ctrl_set.VGA_Ctrl_Flags.GREEN_ON  = 1;
  vga_ctrl_set.VGA_Ctrl_Flags.BLUE_ON   = 1;
  vga_ctrl_set.VGA_Ctrl_Flags.CURSOR_ON = 1;
 
  Vga_Write_Ctrl(VGA_0_BASE, vga_ctrl_set.Value);
  Set_Pixel_On_Color(1023,1023,1023);
  Set_Pixel_Off_Color(0,0,0);
  Set_Cursor_Color(0,1023,0);
  
  unsigned char TXT[1323];
                          
  for (i=0; i<6; i++){
    src_MAC[i] = ior(16+i);
  }
              
  LCD_Test();
  DM9000_init();
  alt_irq_register( DM9000A_IRQ, NULL, (void*)ethernet_interrupts ); 
  packet_num=0;
    int m = 0;
  while (1)
  {
    TXT[0] = 1;
    m = 0; 
    if(transmit_flag == 1){
        printf("Transmit\n");
       for(i=0; i < 30; i++){
           for(j = 43; j < 1323; j++){
               TXT[j] = prep_data[m];  
               m++;
           } 
           TXT[1] = 0xF0;  
           TransmitPacket(TXT, 0x40);
           msleep(2000);
           TXT[1] = 0xC0;  
           TransmitPacket(TXT, 0x52B);
           printf("Transmit %i\n", i);
           msleep(2000);
       }   
       transmit_flag = 0;
       printf("Finish transmit\n");
    }
    
    short cmd;
    printf("\nCommand: ");
    scanf("%d",&cmd);
    unsigned int *sw_val = inport(SWITCH_PIO_BASE);
    
    switch(cmd){
        case 1: 
            TXT[1] = sw_val;
            if(sw_val != 1){
                if(sw_val == 6)
                    rotate90_flag = 1;
                else
                    rotate90_flag = 0;
            }
        break;
        case 2:
            TransmitPacket(TXT,0x40);
            L2_flag = 1;
            msleep(500);
            value = 0;
        break;
        case 3:
            //UDP
            //src
            TXT[3] = src_port;
            TXT[4] = (int) src_port >> 8;
            
            //Destination
            TXT[5] = dest_port;
            TXT[6] = (int) dest_port >> 8;
            
            //Length
            unsigned short int udp_length = 10;
            TXT[7] = udp_length;
            TXT[8] = 0;
            
            //Checksum
            unsigned short int checksum = src_port + dest_port + udp_length;
            checksum = ~checksum + 1;
            TXT[9] = checksum;
            TXT[10] = (int) checksum >> 8;
            
            //IPv4
            //Packet Version + Header Length
            TXT[11] = ((int) 4 << 4) + 5; //69
            
            //Type of Servce
            TXT[12] = 0;
            
            //Total Length
            TXT[13] = 0;
            TXT[14] = 22;
            
            //Identification
            TXT[15] = 0;
            TXT[16] = 0;
            //Flags & Offset
            TXT[17] = 0;
            TXT[18] = 0;
            //Time to Live & Protocol
            TXT[19] = 0;
            TXT[20] = 0;
            
            //Checksum
            short int sa1 = src_address >> 16;
            short int sa2 = src_address;
            short int da1 = dest_address >> 16;
            short int da2 = dest_address;
            int x = (TXT[11] << 8) + 22 + 0 + 0 + 0 + sa1 + sa2 + da1 + da2;
            x = ~x;
            TXT[21] = x;
            TXT[22] = x >> 8;
            
            //src Address
            TXT[23] = sa1;
            TXT[24] = sa1 >> 8;
            TXT[25] = sa2;
            TXT[26] = sa2 >> 8;
            
            //Destination Address
            TXT[27] = da1;
            TXT[28] = da1 >> 8;
            TXT[29] = da2;
            TXT[30] = da2 >> 8;
            
            //EthernetFrame
            for(i = 0; i < 6; i++){
                TXT[31 + i] = dest_MAC[i]; //destination MAC
                TXT[37 + i] = src_MAC[i]; //source MAC
            }
            
            //Ether Type
            TXT[43] = 0;
            TXT[44] = 0;
            //CRC
            TXT[45] = 0;
            TXT[46] = 0;
            TXT[47] = 0;
            TXT[48] = 0;
   
    }
  }
  return 0;
}






