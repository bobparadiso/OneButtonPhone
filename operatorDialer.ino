//-code to answer/hang up phone and optionally dial operator via a single switch
//-running on Arduino Nano (w/ Optiboot) wired to Comdial 2500 phone via SSRs
//-any model phone could be used in concept, as long as relay(s) are in place for hook and a relay for '0' key, then SetLineOpen function is modified appropriately

//more info: https://bobparadiso.com/2016/03/26/one-button-operator-dialer/


/*
Comdial 2500 Hook Switch Wiring
Going from lever end to wire end the reeds (grouped as switches) are as follows:
*Black(GN), Red(R)
*White(J1), Green(L1)
*Brown(J2), Yellow(L2), Gray(L2)

The order of events for hook up is:
*break Yellow – Gray
*make White – Green
*make Yellow – Brown
*break Black – Red

The order of events for hook down is the opposite and reverse:
*make Black – Red
*break Yellow – Brown
*break White – Green
*make Yellow – Gray
*/

#include <avr/wdt.h>
#include <TimerOne.h>

#define HOOK_BLACK_RED_PIN 6
#define HOOK_WHITE_GREEN_PIN 5
#define HOOK_YELLOW_GRAY_PIN 3
#define HOOK_YELLOW_BROWN_PIN 2
#define DIAL_0_PIN 4
#define SWITCH_PIN A1
#define LED_PIN A0 //off-hook indicator LED

#define BTN_POLL_INTERVAL 10 //ms
#define DEBOUNCE_TIME 100 //ms
#define DEBOUNCE_ITERATIONS (DEBOUNCE_TIME / BTN_POLL_INTERVAL)

//
volatile uint16_t buttonDebounceCount = 0;
volatile uint8_t lastButtonState = 0;
volatile uint8_t debouncedButtonState = 0;
void updateBtnState()
{
	uint8_t buttonState = digitalRead(SWITCH_PIN) == LOW;
	//increment count if state is maintained
	if (buttonState == lastButtonState)
	{
		buttonDebounceCount++;
		if (buttonDebounceCount == DEBOUNCE_ITERATIONS)
			debouncedButtonState = lastButtonState;
	}
	//store last button state and reset count
	else
	{
		buttonDebounceCount = 0;
		lastButtonState = buttonState;
	}
}

//
void delayWhileSwitchNotPressed(uint32_t duration)
{
	uint32_t start = millis();
	while ((millis() - start) < duration && !debouncedButtonState)
		wdt_reset();
}

//
#define HOOK_STEP_DELAY 10
uint8_t lineOpen = 0;
void SetLineOpen(uint8_t val)
{
	//open line (hook up)
	if (val)
	{
		digitalWrite(HOOK_YELLOW_GRAY_PIN, LOW);
		delay(HOOK_STEP_DELAY);
		digitalWrite(HOOK_WHITE_GREEN_PIN, HIGH);
		delay(HOOK_STEP_DELAY);
		digitalWrite(HOOK_YELLOW_BROWN_PIN, HIGH);
		delay(HOOK_STEP_DELAY);
		digitalWrite(HOOK_BLACK_RED_PIN, LOW);
		digitalWrite(LED_PIN, HIGH);
		Serial.println("line open");
		lineOpen = 1;
	}
	//hang up (hook down)
	else
	{
		digitalWrite(HOOK_BLACK_RED_PIN, HIGH);
		delay(HOOK_STEP_DELAY);
		digitalWrite(HOOK_YELLOW_BROWN_PIN, LOW);
		delay(HOOK_STEP_DELAY);
		digitalWrite(HOOK_WHITE_GREEN_PIN, LOW);
		delay(HOOK_STEP_DELAY);
		digitalWrite(HOOK_YELLOW_GRAY_PIN, HIGH);
		digitalWrite(LED_PIN, LOW);
		Serial.println("line closed");
		lineOpen = 0;
	}
}

//
void dial_0()
{
	digitalWrite(DIAL_0_PIN, HIGH);
	delayWhileSwitchNotPressed(500);
	digitalWrite(DIAL_0_PIN, LOW);				
}

//
#define OPERATOR_THRESHOLD 1000
void setup()
{
	wdt_disable();
	wdt_enable(WDTO_4S);

	Serial.begin(115200);

	Timer1.initialize(BTN_POLL_INTERVAL * 1000);
	Timer1.attachInterrupt(updateBtnState); 
	
	SetLineOpen(0);
 	
	pinMode(LED_PIN, OUTPUT);
	pinMode(HOOK_BLACK_RED_PIN, OUTPUT);
	pinMode(HOOK_WHITE_GREEN_PIN, OUTPUT);
	pinMode(HOOK_YELLOW_BROWN_PIN, OUTPUT);
	pinMode(HOOK_YELLOW_GRAY_PIN, OUTPUT);
	pinMode(DIAL_0_PIN, OUTPUT);
	pinMode(SWITCH_PIN, INPUT_PULLUP);
	
	Serial.println("ready");

	//main loop
	while (1)
	{
		wdt_reset();

		if (debouncedButtonState)
		{
			if (!lineOpen)
			{
				SetLineOpen(1);

				uint32_t start = millis();
				Serial.println("waiting for switch release");
				while (debouncedButtonState)
					wdt_reset(); //wait while switch is still activated
				Serial.println("ready");
				uint32_t elapsed = millis() - start;
				
				//if long press, then get to operator
				if (elapsed > OPERATOR_THRESHOLD)
				{
					delayWhileSwitchNotPressed(100); //give line extra time to open
					
					//get to a live operator by dialing 0 through each menu			
					Serial.println("dial tone, dialing 0");
					dial_0();

					delayWhileSwitchNotPressed(18000);

					Serial.println("menu, dialing 0");
					dial_0();
					
					if (debouncedButtonState)
						SetLineOpen(0);
				}
			}
			else
			{
				SetLineOpen(0);
			}

			Serial.println("waiting for switch release");
			while (debouncedButtonState)
				wdt_reset(); //wait while switch is still activated
			Serial.println("ready");
		}
	}
}

//
void loop() {}
