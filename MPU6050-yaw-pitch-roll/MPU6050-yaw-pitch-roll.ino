/*
 * CREATED BY RANICK PATRA 11.01.2018
 * 
 * Terms of use
 * 
 * THE SOFTWARE IS PROVIDED WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY
 */



#include <Wire.h>

int temp;
double gyro_cal[3], gravity, gyro[3], acc[3], angle[3];
unsigned long now;
int count;

void setup() {

  Wire.begin();           //Initiate the Wire library and join the I2C bus as a master
  /*set clockspeed 400KHz for fast access default is 100 KHz
   * if you want to use default one just comment out line 27
  */
  Wire.setClock(400000);
  //wait for some time to get ready MPU6050
  delay(1000);

  Serial.begin(230400); // start serial communication high baudrate to avoid delay in serial print and read

  pinMode(13, OUTPUT);  //declare pin 13 as OUTPUT
  digitalWrite(13, LOW);//set pin 13 state LOW

  //set up the MPU6050
  Wire.beginTransmission(0x68); //start communicating with mpu6050
  Wire.write(0x6B);             //we want to write in 0x6B register which is powermanagement register
  Wire.write(0x00);             //write 0x00 to that register
  Wire.endTransmission();       //end the transmission

  Wire.beginTransmission(0x68); //start communicating with mpu6050
  Wire.write(0x1B);             //we want to write in 0x1B which is gyro configaration register
  Wire.write(0x08);             //set the gyro for 500 deg pre sec full scale
  Wire.endTransmission();       //end the transmission

  Wire.beginTransmission(0x68); //start communicating with mpu6050
  Wire.write(0x1C);             //we want to write in 0x1C register which is Accelerometer Configuration register
  Wire.write(0x10);             //set the accelerometer with +-8g 
  Wire.endTransmission();       //end the transmission

  Wire.beginTransmission(0x68); //start communicating with mpu6050
  Wire.write(0x1A);             //we want to write in 0x1A which is used to gyro and accelerometer synchronizing
  Wire.write(0x03);             //write 0x03 in that register
  Wire.endTransmission();       // end the transmission


  // random check registers wheather the values is written correctly or not
  Wire.beginTransmission(0x68);
  Wire.write(0x1B);
  Wire.endTransmission();
  Wire.beginTransmission(0x68);
  Wire.requestFrom(0x68, 1);
  while (Wire.available() < 1);
  byte data = Wire.read();
  Wire.endTransmission();
  if (data != 0x08) {
    digitalWrite(13, HIGH);
    while (true)            //if not written this loop continues untill a next reset takes place
      delay(10);
  }
// random check registers wheather the values is written correctly or not
  Wire.beginTransmission(0x68);
  Wire.write(0x1C);
  Wire.endTransmission();
  Wire.beginTransmission(0x68);
  Wire.requestFrom(0x68, 1);
  while (Wire.available() < 1);
  data = Wire.read();
  Wire.endTransmission();
  if (data != 0x10) {
    digitalWrite(13, HIGH);
    while (true)            //if not written this loop continues untill a next reset takes place
      delay(10);
  }

  //set the values 0
  gyro_cal[0] = gyro_cal[1] = gyro_cal[2] = 0;
  acc[0] = acc[1] = acc[2] = 0;
  angle[0] = angle[1] = angle[2] = 0;

  //calibrating the gyro
  //dont move the gyro during calibration
  for (int i = 0; i < 2000; i++) {
    Wire.beginTransmission(0x68); //start communicating with MPU6050
    Wire.write(0x43);             //start reading from 0x43 register
    Wire.endTransmission();       //end the trasnmission
    Wire.requestFrom(0x68, 6);    //requesting 6 bytes of data
    while (Wire.available() < 6); //wait until 6 bytes are receive
    //add a particular axis data to its corrosponding variable
    gyro_cal[0] += Wire.read() << 8 | Wire.read();    //reading gyro data
    gyro_cal[1] += Wire.read() << 8 | Wire.read();    //reading gyro data
    gyro_cal[2] += Wire.read() << 8 | Wire.read();    //reading gyro data
    if (!(i % 25)) {
      digitalWrite(13, !digitalRead(13));   //blink the led during calibration
    }
    delayMicroseconds(4000);      //wait for some times say 4000 µs
  }

  //devide the total value by 2000 to get the average value
  gyro_cal[0] /= 2000;
  gyro_cal[1] /= 2000;
  gyro_cal[2] /= 2000;

  for (int i = 0; i < 200; i++) {
    Wire.beginTransmission(0x68);   //start communicating with MPU6050
    Wire.write(0x3B);               //start reading from 0x3B register
    Wire.endTransmission();         //end the transmission
    Wire.requestFrom(0x68, 6);      //requesting 6 bytes of data
    while (Wire.available() < 6);   //wait until 6 bytes are receive
    //add corrosponding acc data to its corrosponding variable
    acc[0] += Wire.read() << 8 | Wire.read();
    acc[1] += Wire.read() << 8 | Wire.read();
    acc[2] += Wire.read() << 8 | Wire.read();
    if (!(i % 25)) {
      digitalWrite(13, !digitalRead(13)); //blink the led
    }
    delayMicroseconds(4000);      //wait for some times say 4000 µs
  }

  digitalWrite(13, LOW);          // turn of the led

  //take the average of acc data
  acc[0] /= 200;
  acc[1] /= 200;
  acc[2] /= 200;

  //get the graviry vector
  gravity = sqrt(acc[0] * acc[0] + acc[1] * acc[1] + acc[2] * acc[2]);
  angle[0] = asin(acc[0] / gravity) * 57.295779;    //set the initial pitch angle
  angle[1] = asin(acc[1] / gravity) * 57.295779;    //set the initial roll angle
  angle[2] = 0;//acos(acc[2] / gravity)*57.295779;  //set the initial yaw angle to 0
  
  while (!Serial);    //wait for serial port to ready
  count = 0;          //set the variable count to zero which used to print data in serial

}

void loop() {

  if (Serial.available() > 0) {   //if data available in serial port
    if (Serial.read() == 'k')     //if the data is 'k' then
      angle[2] = 0;               //reset the yaw angle to zero
  }

  now = micros() + 4000;          //take corrent time in µs and add 4000µs which is out loop time or refresh time we want to refresh 250 times in a second
  Wire.beginTransmission(0x68);   //start communicating with MPU6050
  Wire.write(0x3B);               //start reading from 0x3B
  Wire.endTransmission();         //end the transmission
  Wire.requestFrom(0x68, 14);     //request 14 bytes
  while (Wire.available() < 14);  //wait until 14 bytes are received
  acc[0] = Wire.read() << 8 | Wire.read();    //read acc x data
  acc[1] = Wire.read() << 8 | Wire.read();    //read acc y data
  acc[2] = Wire.read() << 8 | Wire.read();    //read acc z data
  temp = Wire.read() << 8 | Wire.read();      //read temperature data [we don't use it here]
  gyro[1] = Wire.read() << 8 | Wire.read();   //read gyro pitch data
  gyro[0] = Wire.read() << 8 | Wire.read();   //read gyro roll data
  gyro[2] = Wire.read() << 8 | Wire.read();   //read gyro yaw data

  //substract the calibration value to get proper data
  gyro[0] -= gyro_cal[1];
  gyro[1] -= gyro_cal[0];
  gyro[2] -= gyro_cal[2];

  /*
   *     1
   *  --------- = 0.0000611   ///// 65.5 is the value for the gyro for 500°/sec configure and rotate 1°/sec and 250 is refresh rate
   *  65.5*250
   */
  gyro[0] *= 0.0000611;   //we dont need the raw value again so we can change the original data
  gyro[1] *= 0.0000611;   //we dont need the raw value again so we can change the original data
  gyro[2] *= 0.0000611;   //we dont need the raw value again so we can change the original data

  gravity = sqrt(acc[0] * acc[0] + acc[1] * acc[1] + acc[2] * acc[2]);    //calculate the gravity
  acc[0] = asin(acc[0] / gravity) * 57.295779;                            //get pitch angle
  acc[1] = asin(acc[1] / gravity) * 57.295779;                            //get roll angle

  /*
   *     1
   *  --------- = 0.0000611   ///// 65.5 is the value for the gyro for 500°/sec configure and rotate 1°/sec and 250 is refresh rate
   *  65.5*250
   */
  angle[0] = 0.96 * (angle[0] + gyro[0]) + 0.04 * acc[0];   //take a larger portion of gyro data and smaller portion of accelerometer data to avoid drift
  angle[1] = 0.96 * (angle[1] + gyro[1]) + 0.04 * acc[1];   //take a larger portion of gyro data and smaller portion of accelerometer data to avoid drift
  angle[2] = angle[2] + gyro[2];       //yaw angle gyro based only it may drift

  /*    π
   * -------- = 0.0174532925      //arduino sin() takes radian // we convert rolto pitch and pitch to rol because MPU6050 may be yawed inclimbed
   *   180
   */
  angle[0] -= angle[1] * sin(gyro[2] * 0.0174532925);
  angle[1] += angle[0] * sin(gyro[2] * 0.0174532925);

  count++;
  if (count == 100) {         //print data in serial every 0.1s
    Serial.print("ypr = ");
    Serial.print(angle[2]); Serial.print(", ");
    Serial.print(angle[0]); Serial.print(", ");
    Serial.println(angle[1]);
    count = 0;
  }

  //wait until 4000 µs passed
  delayMicroseconds(now - micros());

}
