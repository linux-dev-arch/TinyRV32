#include "defs/cpu.c"
int ram_index=0;
int start=0;
int current_time=0;
int ledpin=13;
int freq=4;
int prog=2;
int syspin=3;//for future update
unsigned long cpu_freq=0;
unsigned long instr_count=0;
uint8_t pinmap[]={2,3,4,5,6,7,8,9,10,11,12,13};
void print_ram(){
  Serial.print("[");
  for(int i =0;i<sizeof(ram);i++){
    Serial.print(",0x");
    Serial.print(ram[i],HEX);
  }
  Serial.println("]");
}
void upload(){
  while(1){
    if(Serial.available()){
      int x=Serial.read();
      Serial.write(x);
        if(ram_index < 512){
          ram[ram_index]=x;
          ram_index++;
        }
        else{
          break;
        }
    }
  }
}
void setup() {
  // put your setup code here, to run once:
  pinMode(prog,INPUT);
  pinMode(freq,INPUT);
  pinMode(syspin,INPUT);
  pinMode(ledpin,OUTPUT);
  digitalWrite(syspin,1);
  digitalWrite(prog,1);
  digitalWrite(freq,1);
  Serial.begin(115200);
  Serial.println("Checking Status pin");
  delay(1000);
  if (digitalRead(prog) == 0){
    for(int i=0;i<sizeof(ram);i++){
      ram[i]=0;//clear ram
    }
    upload();
  } 
  print_ram();
  Serial.println("running application!");
  start=millis();
}

void loop() {
  // put your main code here, to run repeatedly:
cpu_step(get_instr(pc));
instr_count++;
current_time=millis();
if ((current_time-start)>2000){
  cpu_freq = instr_count /((current_time-start)/2000);
  instr_count=0;
  start=current_time;
}
while (Serial.available()) {
    uint8_t next = (uart_rx_head + 1) % 20;

    // Only add if buffer isn't full
    if (next != uart_rx_tail) {
        uart_rx_buf[uart_rx_head] = Serial.read();
        uart_rx_head = next;
    } else {
        // Buffer full, discard the byte
        Serial.read();
    }
}
if (digitalRead(freq)==0){
  Serial.print("CPU_FREQ=");
  Serial.println(cpu_freq);
}
if(uart_tx != 0){
    Serial.write(uart_tx);
    uart_tx=0;
  }
if(led == 0xFF){
  digitalWrite(ledpin,1);
}
else if(led ==0){
  digitalWrite(ledpin,0);
}
}
