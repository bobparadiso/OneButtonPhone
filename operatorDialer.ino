//-code to answer/hang up phone and optionally dial operator via a single switch
//-running on Arduino Nano (w/ Optiboot) wired to Comdial 2500 phone via relays
//-any model phone could be used in concept, as long as relay(s) are in place for hook and a relay for '0' key, then SetLineOpen function is modified appropriately

//for Comdial 2500 hook wiring:
//for hook down: yellow->gray, black->red
//for hook up: yellow->brown, green->white

#include <avr/wdt.h>
#include <TimerOne.h>

#define HOOK0_PIN 2
#define HOOK1_PIN 3
#define HOOK2_PIN 4
#define DIAL_0_PIN 5
#define SWITCH_PIN 6 //PD6 PCINT22
#define LED_PIN 12 //off-hook indicator LED

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
uint8_t lineOpen = 0;
void SetLineOpen(uint8_t val)
{
	//open line
	if (val)
	{
		digitalWrite(HOOK0_PIN, LOW);
		digitalWrite(HOOK1_PIN, LOW);
		digitalWrite(HOOK2_PIN, LOW);
		digitalWrite(LED_PIN, HIGH);
		Serial.println("line open");
		lineOpen = 1;
	}
	//hang up
	else
	{
		digitalWrite(HOOK0_PIN, HIGH);
		digitalWrite(HOOK1_PIN, HIGH);
		digitalWrite(HOOK2_PIN, HIGH);
		digitalWrite(LED_PIN, LOW);
		Serial.println("line closed");
		lineOpen = 0;
	}
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
		
	digitalWrite(LED_PIN, LOW);
	digitalWrite(HOOK0_PIN, HIGH);
	digitalWrite(HOOK1_PIN, HIGH);
	digitalWrite(HOOK2_PIN, HIGH);
	digitalWrite(DIAL_0_PIN, HIGH);
	
	pinMode(LED_PIN, OUTPUT);
	pinMode(HOOK0_PIN, OUTPUT);
	pinMode(HOOK1_PIN, OUTPUT);
	pinMode(HOOK2_PIN, OUTPUT);
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
					digitalWrite(DIAL_0_PIN, LOW);
					delayWhileSwitchNotPressed(500);
					digitalWrite(DIAL_0_PIN, HIGH);				

					delayWhileSwitchNotPressed(18000);

					Serial.println("menu, dialing 0");
					digitalWrite(DIAL_0_PIN, LOW);
					delayWhileSwitchNotPressed(500);
					digitalWrite(DIAL_0_PIN, HIGH);
					
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
