//스텝 모터 핀 셋팅
//스텝 모터 드라이버 구동을 위해서는 EN, DIR, STEP 핀만 연결 되어 있으면 구동 가능
//StallGuard 사용을 위해서는 TMCStepper https://github.com/teemuatlut/TMCStepper 사용 필요


#define DIR_PIN          10     // Direction
#define STEP_PIN         11    // Step
#define EN_PIN          12      // Enable 

//로타리 엔코더 핀 셋팅

//CLK와 DT는 아두이노 나노의 인터럽트 핀을 사용함
#define rotaryCLK 2
#define rotaryDT 3
#define rotarySW 4

//RGB핀 셋팅

#define bluePIN 6
#define greenPIN 7
#define redPIN 8




//AccelStepper 사용. 
#include <AccelStepper.h>
AccelStepper stepper = AccelStepper(1, STEP_PIN, DIR_PIN);


//로타리 엔코더 스위치를 토글 스위치로 사용하기 위한 것
//소스를 어디서 따왔는지 까먹었 ㅜ.ㅜ
int stepperRunStatus = LOW; //LOW off HIGH On
int rotarySWReading;
unsigned long time = 0;           // the last time the output pin was toggl
unsigned long debounce = 200UL;   // the debounce time, increase if the output flickers


//로타리 엔코더 회전에 따른 상태 확인을 위한 것
//여기서 참조 함 https://m.blog.naver.com/emperonics/222108739792
int spd = 0;           // 모터 회전수 증가를 위한 설정값
int currentStateCLK;       // CLK의 현재 신호상태 저장용 변수
int lastStateCLK = 0;          // 직전 CLK의 신호상태 저장용 변수 
String currentDir ="";      // 현재 회전 방향 출력용 문자열 저장 변수
unsigned long lastButtonPress = 0;     // 버튼 눌림 상태 확인용 변수

//스텝 모터 관련 설정
//maxSpeed 1000을 기준으로 테스트 시 너무 느려서 10000을 기준으로 테스트
//9V 6000 정도를 맥스값으로 주는 것이 좋은듯...
// float maxSpeed = 10000;
// float accelSpeed = 1000;
// float speedStep = 100;


float maxSpeed = 6000;
float accelSpeed = 12000;
float speedStep = 600;


void setup() {



    //로타리 엔코더 핀모드
    pinMode(rotaryCLK, INPUT);
    pinMode(rotaryDT, INPUT);
    pinMode(rotarySW, INPUT_PULLUP);

    //외부 인터럽트 등록, 핀의 상태가 변할 때(HIGH에서 LOW 또는 LOW에서 HIGH) 마다 updateEncoder함수가 실행됨. 
    //인터럽트 0번은 2번핀과 연결되어 있고 1번은 3번 핀과 연결되어 있음
    attachInterrupt(0, updateEncoder, CHANGE);
    attachInterrupt(1, updateEncoder, CHANGE);


    //스템모터 핀모드
    pinMode(EN_PIN, OUTPUT);
    pinMode(STEP_PIN, OUTPUT);
    pinMode(DIR_PIN, OUTPUT);

    //시리얼 통신 관련 셋팅
    Serial.begin(9600);
    while(!Serial);
    Serial.println("Start...");


    //스텝 모터 속도 관련 설정
    stepper.setMaxSpeed(maxSpeed);
    // stepper.setAcceleration(maxSpeed/3);
    stepper.setAcceleration(accelSpeed);

    //스텝 모터 관련 설정
    // stepper.setEnablePin(EN_PIN);
    // stepper.setPinsInverted(false, false, true); //이건 뭐에 쓰는겨?
    stepper.enableOutputs();

    //스텝 모터 전기 차단
    //stepper.setEnablePin(EN_PIN);에서 스텝 모터에 전기가 들어가게 됨
    //하지만 전기가 들어가면 스텝 모터가 정지 상태이고, 이 상태에서는 과열 됨. 따라서 전기를 끊어 줌
    //EN_PIN을 사용 해 전기를 끊어 주게 됨
    //TMC2209의 경우 HIGH가 Off / LOW가 On
    //EN 	Enable Motor Outputs: GND=on, VIO=off
    //https://wiki.fysetc.com/Silent2209/

    digitalWrite(EN_PIN, HIGH);//TMC2209 EN HIGH가 OFF -_-??? 

    pinMode(redPIN, OUTPUT);
    pinMode(greenPIN, OUTPUT);
    pinMode(bluePIN, OUTPUT);

    delay(1000);

    digitalWrite(redPIN, HIGH);

}


void loop()
{
  
 
  tmc2209OnOff();
  //인터럽트 발생시 spd값을 변경 함 //
  stepper.setSpeed(spd);
  stepper.runSpeed();
}

void tmc2209OnOff() {
  rotarySWReading = digitalRead(rotarySW); //로타리 스위치가 눌렸을 때 상태에 따라서 스텝 모터 드라이버 On/Off

  if (rotarySWReading == LOW && millis() - time > debounce)
  {
    if (stepperRunStatus == HIGH) {
      stepperRunStatus = LOW;
        stepper.setSpeed(0); //속도를 0으로 바꿈
        delay(500); 
        digitalWrite(EN_PIN, HIGH); //stepMotor 전기 차단
        stepper.disableOutputs();
        spd = 0;
        digitalWrite(bluePIN, LOW);
        digitalWrite(redPIN, HIGH);
        
        Serial.println("Stepper Power Off / Counter 0");
    }
    else {
      stepperRunStatus = HIGH;
        digitalWrite(EN_PIN, LOW);
        digitalWrite(redPIN, LOW);
        digitalWrite(bluePIN, HIGH);
      Serial.println("Stepper Power On");
    }
    time = millis();
  }

}



void updateEncoder(){   // 인터럽트 발생시 실행되는 함수

	currentStateCLK = digitalRead(rotaryCLK); //CLK의 현재 상태를 읽어옴

	
	if (currentStateCLK != lastStateCLK  && currentStateCLK == 1){// CLK핀의 신호가 바뀌었고(즉, 로터리엔코더의 회전이 발생했했고), 그 상태가 HIGH이면(최소 회전단위의 회전이 발생했다면) 
    if (stepperRunStatus == HIGH)  { //모터 상태가 On 이라면
      if (digitalRead(rotaryDT) != currentStateCLK) {// DT핀의 신호를 확인해서 엔코더의 회전 방향을 확인하고, 회전 속도를 증가 시킴
            if(spd < maxSpeed) {
                spd = spd+speedStep;      
            }                 
            currentDir ="CW";
            // stepper.setSpeed(spd);
            // stepper.runSpeed();

        } else {  
          if(spd > -(maxSpeed)) {
              spd = spd-speedStep;      
          }                                    // 신호가 같다면 반시계방향 회전
          currentDir ="CCW";
          // stepper.setSpeed(spd);
          // stepper.runSpeed();
        }

    } else {
      Serial.println("Plz Turn on Motor");
    }


    Serial.print("Motor Status: ");
		Serial.print(stepperRunStatus);  // 모터 상태 출력
		Serial.print(" | Direction: ");
		Serial.print(currentDir);  // 회전방향 출력
		Serial.print(" | Speed: ");
		Serial.print(spd);  //속도 출력
    Serial.print(" | % : ");
		Serial.print(spd/speedStep);  //속도 출력
    Serial.println("%"); 

	}

	// 마지막 상태 변수 저장
	lastStateCLK = currentStateCLK;
}

