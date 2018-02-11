class HallSensor{
  byte analog_output;
  int zero_level;
  int prev_value;
  
public:
  HallSensor(byte = A0);
  int getRawOutput() const;
  int calculateZeroLevel();
  int getZeroLevel() const;
  int getMinOutput();
  int getOutputFrequency();
};

HallSensor::HallSensor(byte analog_out){
  analog_output = analog_out;
  zero_level = 0;
  prev_value = 0;
}

int HallSensor::getRawOutput() const{
  return analogRead(analog_output);
}

int HallSensor::calculateZeroLevel(){
  long long acc = 0;
  for(int i = 0; i < 1000; i++){
    acc += analogRead(analog_output);
  }
  zero_level = acc/1000;
  return zero_level;
}

int HallSensor::getZeroLevel() const{
  return zero_level;
}

int HallSensor::getMinOutput(){
  int raw_value = analogRead(analog_output);
  char sign =  //above or below zero
  sign = (prev_value < -1 || prev_value > 1) ? 1 : prev_value;
  bool current_sign = false;
  if(zero_level < raw_value)
  {
    sign = 1;
    if(current_sign)
      current_sign = false;
  }
  else
  {    
    sign = -1;
    if(!current_sign)
      current_sign = true;
  }
  prev_value = sign;
  return sign;
}

int HallSensor::getOutputFrequency(){
  int raw_value = 0;
  unsigned int peak_counter = 0;
  bool current_sign = false;
  unsigned int time = millis()%1000;
  while(1)
  {
    if(millis()%1000 - time >= 50 )
    {    
      peak_counter = (peak_counter+prev_value)/2;
      peak_counter = peak_counter < 3 ? 0 : peak_counter;
      prev_value = peak_counter;
      return peak_counter;
    }
    raw_value = analogRead(analog_output);
    if(zero_level < raw_value && current_sign)
    {
      current_sign = false;
      peak_counter++;      
    }else
      if(zero_level > raw_value && !current_sign)
      {  
        current_sign = true;
        peak_counter++;
      }
  }
}


class Motor{
  byte enable_input;
  byte rotation_input;
  bool left_rotation; //0 or 1
  bool is_on;
  bool is_left_rotate;
  HallSensor *sensor;
  
public:
  Motor(byte = A0, byte = 5, byte = 4, bool = false);
  ~Motor();
  void on();
  void off();
  void setRotation();
  void setLeftRotation();
  void setRightRotation();
  void changeRotation();
  bool isStopped() const;
  bool isOn() const;
  HallSensor* getSensor() const;
};

Motor::Motor(byte analog_out, byte enable_in, byte rotation_in, bool left_rotation_sign){
  sensor = new HallSensor(analog_out);
  sensor->calculateZeroLevel();
  enable_input = enable_in;
  rotation_input = rotation_in;
  left_rotation = left_rotation_sign;
  off();
}

void Motor::on(){
  is_on = true;
  digitalWrite(enable_input, HIGH);
}

void Motor::off(){
  is_on = false;
  digitalWrite(enable_input, LOW);
}

void Motor::setRotation(){
  digitalWrite(rotation_input, is_left_rotate ? left_rotation : !left_rotation);
}

void Motor::setLeftRotation(){
  is_left_rotate = true;
  setRotation();
}

void Motor::setRightRotation(){
  is_left_rotate = false;
  setRotation();
}

void Motor::changeRotation(){
  is_left_rotate = !is_left_rotate;
  setRotation();
}

bool Motor::isStopped() const{
  if(is_on && sensor->getOutputFrequency() == 0)
    return true;
  return false;
}

bool Motor::isOn() const{
  return is_on;
}

HallSensor* Motor::getSensor() const{
  return sensor;
}

class Finger{
  int current_position;
  unsigned int full_bend;
  int new_position;
  unsigned long step;
  bool is_bend;
  bool is_move;
  Motor *motor;
public:
  Finger(byte = A0, byte = 5, byte = 4, bool = false);
  void bend();
  void straight();
  unsigned int getPosition() const;
  void setPosition(unsigned int);
  unsigned int getFullBendValue() const;
  void setStartPosition();
  unsigned int getNewPosition() const;
  void move();
  void stop();
  void isArrived();
  void isStopped();
  void changeCurrentPosition();
  bool isBend() const;
  Motor* getMotor() const;
  bool isMove() const;
  
  unsigned int setup();
};

Finger::Finger(byte analog_out, byte enable_in, byte rotation_in, bool left_rotation_sign){
  motor = new Motor(analog_out, enable_in, rotation_in, left_rotation_sign);
}

void Finger::bend(){
  is_bend = true;
  motor->setLeftRotation();
  move();
}

void Finger::straight(){
  is_bend = false;
  motor->setRightRotation();
  move();
}

unsigned int Finger::getPosition() const{
  return current_position;
}

void Finger::setPosition(unsigned int new_pos){
  new_position = new_pos*step;
  int cur_pos = current_position/step;
  if(cur_pos - new_pos > -1 || cur_pos - new_pos < 1){
    if(motor->isOn())
      stop();
    return;
  }
  if(new_position > current_position)
    bend();
  else
    straight();  
}

unsigned int Finger::getFullBendValue() const{
  return full_bend;
}

void Finger::setStartPosition(){
  straight();
}

unsigned int Finger::getNewPosition() const{
  return new_position;
}

void Finger::move(){
  is_move = true;
  motor->on();
}

void Finger::stop(){
  is_move = false;
  motor->off();
}

void Finger::isArrived(){
  if(!is_move) return;
  isStopped();
  if( (is_bend && new_position - current_position < 10) ||
      (!is_bend && current_position - new_position < 10))
    stop();
}

void Finger::isStopped(){
  changeCurrentPosition();
  if(motor->isStopped()){
    stop();
    if(!is_bend){
      current_position = 0;  
    }
  }
}

void Finger::changeCurrentPosition(){
  if(is_move)
    if(is_bend)
      current_position += motor->getSensor()->getOutputFrequency();
    else
      current_position -= motor->getSensor()->getOutputFrequency();
}

bool Finger::isBend() const{
  return is_bend;
}

unsigned int Finger::setup(){
  unsigned int tmp_position = 0;
  straight();
  while(is_move)
    isStopped();
    
  current_position = 0;
  bend();
  while(is_move)
    isStopped();
  
  tmp_position = current_position;
  straight();
  while(is_move)
    isStopped();
    
  full_bend = tmp_position - current_position/2;
  step = full_bend/19;
  return full_bend;
}

Motor* Finger::getMotor() const{
  return motor;
}

bool Finger::isMove() const{
  return is_move;
}

int getCmd(){
  char *string = new char[5];
  int i = 1;
  byte ex = 1;
  string[0] = Serial.read();
  if(string[0] != 's')  return 0;
  while(string[i-1] != 'f' && ex++)
    if(Serial.available())
      string[i++] = Serial.read();
  if(string[3] != 'f') return 0;
  i = (string[1]-'0')*100 + string[2]-'0';
  delete(string);
  return i;
}

Finger *hand;
void setup() {
  pinMode(5, OUTPUT);
  pinMode(4, OUTPUT);  
  pinMode(A4, INPUT);  
  
  pinMode(10, OUTPUT);
  pinMode(7, OUTPUT);  
  pinMode(A3, INPUT);
 
  pinMode(9, OUTPUT);
  pinMode(8, OUTPUT);  
  pinMode(A2, INPUT);
  
  pinMode(6, OUTPUT);
  pinMode(12, OUTPUT);  
  pinMode(A1, INPUT);
  
  pinMode(11, OUTPUT);
  pinMode(13, OUTPUT);  
  pinMode(A0, INPUT);
  Serial.begin(115200);
  hand = new Finger[5];
  hand[4] = Finger(A4, 5, 4, false);
  hand[3] = Finger(A3, 10, 7, true);
  hand[2] = Finger(A2, 9, 8, true);
  hand[1] = Finger(A1, 6, 12, true);
  hand[0] = Finger(A0, 11, 13, false);
  Serial.println(hand[4].setup());
  Serial.println(hand[3].setup());
  Serial.println(hand[2].setup());
  Serial.println(hand[1].setup());
  Serial.println(hand[0].setup());
}

int cmd = 0;
void loop()
{
  if(Serial.available()){
    cmd = getCmd();
    hand[cmd/100].setPosition(cmd%100);
  }
  for(byte num = 0; num < 5; num++)
    hand[num].isArrived();
}


