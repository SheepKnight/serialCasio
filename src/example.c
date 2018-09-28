#include <fxcg/display.h>
#include <fxcg/keyboard.h>
#include <fxcg/serial.h>
#include <fxcg/system.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
//#include <color.h>

int keyGetter(){
	unsigned char buffer[12];
	PRGM_GetKey_OS( buffer );
	return ( buffer[1] & 0x0F ) * 10 + ( ( buffer[2]  & 0xF0 )  >> 4 );
}

int main() {
	
	
	if (Serial_IsOpen() != 1) {
		unsigned char mode[6] = {0, 5, 0, 0, 0, 0};    // 9600 bps 8n1
		Serial_Open(mode);
		Serial_ClearRX();
		Serial_ClearTX();
	}
	
	int shouldStop = 0;
	
	Bdisp_AllClr_VRAM();
	locate_OS(1,1);
	int y = 1;
	int x = 1;
	Print_OS(">", 0, 0);
    
	while (shouldStop == 0) {
        unsigned char in;
		int status = Serial_ReadSingle(&in);
		while(status==0){
			
			//Serial_WriteSingle(in);
			if(in == '\n'){
			
				y++;
				locate_OS(1,y);
			
			}else{
				
				x++;
				if(x==21){
				
					x = 1;
					y++;
					if(y== 8){
					
						y = 1;
						x = 1;
						
					}
					locate_OS(1,y);
				
				}
				char p[2];
				p[0] = in;
				p[1] = '\0'; 
				const char * printer = p;
				Print_OS(printer, 0, 0);
			
			}
			Bdisp_PutDisp_DD();
			status = Serial_ReadSingle(&in);
		}

		int key = keyGetter();
		//GetKey(&key);
		
		switch (key) {
			
			case KEY_PRGM_F1:
			{
				
				Bdisp_Fill_VRAM(COLOR_WHITE, 1);
				locate_OS(1,1);
				char message[50];
				memset(message, 0, sizeof message);
				int i = 0;
				int typeKey;
				GetKey(&typeKey);
				
				while(typeKey != KEY_CTRL_F2 && i < 50){
					
					switch(typeKey){
						case KEY_CTRL_F3:
							if(GetSetupSetting((unsigned int)0x14) == 0x88 ){
								SetSetupSetting((unsigned int)0x14, 0x84);
							}
							if(GetSetupSetting((unsigned int)0x14) == 0x84 ){
								SetSetupSetting((unsigned int)0x14, 0x88);
							}
							break;
						
						case KEY_CTRL_F4:
							{
								char entree = CharacterSelectDialog();
								if(entree != 0){
									message[i] = entree;
									i++;
								}
								break;
							}
						case KEY_CTRL_LEFT:
							message[i] = '\0';
							i--;
							break;

						case KEY_CTRL_VARS:
							message[i] = ':';
							i++;
							break;
						
						case KEY_CTRL_OPTN:
							message[i] = '+';
							i++;
							break;
						
						default:
						
							if(typeKey != KEY_CTRL_F1 && typeKey != KEY_CTRL_ALPHA && typeKey != KEY_CTRL_SHIFT){
						
								message[i] = typeKey;
								i++;
							
							}
							break;
							
					}
					Bdisp_Fill_VRAM(COLOR_WHITE, 1);
					char buffer[52];
					strcpy(buffer,"  ");
					strcpy(buffer+2,message);
					
					PrintXY( 1, 2, buffer, TEXT_MODE_NORMAL, TEXT_COLOR_BLACK);
					
					GetKey(&typeKey);
					
				}
				i++;
				message[i] = '\n';
				const unsigned char * messageN = message;
				Serial_ClearTX();
				Serial_Write(messageN, i);
				memset(message, 0, sizeof message);
				Bdisp_Fill_VRAM(COLOR_WHITE, 1);
				locate_OS(1,1);
				break;
			}
			case KEY_PRGM_MENU:
				shouldStop = 0;
				Serial_Close(1);
				while(1){
					int p = 0;
					GetKey(&p);
				}
				break;
			
			default :  
				//Serial_WriteSingle((unsigned char)key);
				break;
			
		}

		Bdisp_PutDisp_DD();
	}
 
	return 0;
}