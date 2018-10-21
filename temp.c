/*Naasa Fikri
LM75B
Heavily based on Alexander Rüedlinger's LM75 driver
*/

#include <stdio.h>
#include <linux/i2c-dev.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <fcntl.h>
#include <unistd.h>
#include <time.h>

/*
	LM75B structure definition
*/
typedef struct { 
	int file; //file descriptor
	int address; //i2c device address
	char *i2c_device; //i2c device file path
} lm75b_t;

/*
	cast into lm75b object
*/
#define TO_S(x) (lm75b_t*) x 

/*
	set the address of the i2c device
*/
int lm75b_set_addr(void *_s){
	lm75b_t* s = TO_S(_s);
	int error;
	if((error = ioctl(s->file, I2C_SLAVE, s->address)) < 0){
		printf("ioctl failed\n");
	}
	return error;
}

/*
	cleanup when the divice initialization failed
*/
void lm75b_init_error_cleanup(void *_s){
	lm75b_t* s = TO_S(_s);
	if(s->i2c_device != NULL){
		free(s->i2c_device);
		s->i2c_device = NULL;
	}

	free(s);
	s = NULL;
}

/*
	initializes the lm75b structure
*/
void *lm75b_init(int address, const char* i2c_device_filepath){
	void *_s = malloc(sizeof(lm75b_t));
	if(_s == NULL){
		printf("malloc error\n");
		return NULL;
	}
	lm75b_t *s = TO_S(_s);
	s->address = address;
	s->i2c_device = (char*) malloc(strlen(i2c_device_filepath) * sizeof(char));
	if(s->i2c_device == NULL){
		printf("i2c not found\n");
		return NULL;
	}
	strcpy(s->i2c_device, i2c_device_filepath);

	int file;
	if((file = open(s->i2c_device, O_RDWR)) < 0){
		printf("fail to open i2c\n");
		lm75b_init_error_cleanup(s);
		return NULL;
	}

	s->file = file;
	if(lm75b_set_addr(s) < 0){
		lm75b_init_error_cleanup(s);
		printf("ioctl failed\n");
		return NULL;
	}

	printf("device OK\n");
	return _s;
}

/*
	get the temperature form the device
*/
float lm75b_temperature(void *_s) {
	lm75b_t *lm = TO_S(_s);
	uint16_t data = (uint16_t) i2c_smbus_read_word_data(lm->file, 0); //the data is <LSB><MSB>. need to reverse it.
	uint8_t msb, lsb;
	msb = data & 0x00FF; //get the most significant byte
	lsb = (data & 0xFF00) >> 8; //get the least significant byte
	data = ((uint16_t)msb << 8) | lsb; //cast the most significant byte. OR it with the least significant byte.
	data = data >> 5; //ignore 5 on the least significant bit to get 11 bit of data.
	printf("dec: %d hex: %x ", data, data);
	float temperature = data * 0.125; //convert the data to degrees C. pg. 9 of the datasheet.
	return temperature;
}

/*
	clean-up to close the device
*/
void lm75b_close(void *_s){
	if(_s == NULL){
		return;
	}
	lm75b_t *s = TO_S(_s);

	if(close(s->file) < 0)
		printf("close failed\n");
	free(s->i2c_device);
	s->i2c_device = NULL;
	free(s);
	_s = NULL;
}

int main(){
	char *i2c_device = "/dev/i2c-1"; //i2c filepath for Raspberry Pi 2
	int address = 0x48; //LM75B i2c address
	int i;
	void *lm = lm75b_init(address, i2c_device);
	if(lm != NULL){
		for(i = 0; i < 20; i++){
			printf("%f\n", lm75b_temperature(lm));
			usleep(2000000);
		}
		lm75b_close(lm);
	}

	return 0;
}
