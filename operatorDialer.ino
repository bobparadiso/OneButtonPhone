//-code to answer/hang up phone and optionally dial operator via a single switch
//-running on Arduino Nano wired to Comdial 2500 phone via relays
//-any model phone could be used in concept, as long as relay(s) are in place for hook and a relay for '0' key, then SetLineOpen function is modified appropriately

//for Comdial 2500 hook wiring:
//for hook down: yellow->gray, black->red
//for hook up: yellow->brown, green->white

#define HOOK0_PIN 2
#define HOOK1_PIN 3
#define HOOK2_PIN 4
#define DIAL_0_PIN 5
#define SWITCH_PIN 6 //PD6 PCINT22
#define LED_PIN 12 //off-hook indicator LED

//state for button debounce
volatile uint32_t lastButtonTime = 0;
volatile uint8_t lastButtonState = 0;
ISR(PCINT2_vect)
{
	lastButtonTime = millis();
	lastButtonState = digitalRead(SWITCH_PIN) == LOW;
}

//state is validated/updated at request time
#define DEBOUNCE_THRESHOLD 25
uint8_t getDebouncedButtonState()
{
	static uint8_t debouncedButtonState = 0;
	
	if (millis() - lastButtonTime > DEBOUNCE_THRESHOLD)
		debouncedButtonState = lastButtonState;

	return debouncedButtonState;
}

//
void delayWhileSwitchNotPressed(uint32_t duration)
{
	uint32_t start = millis();
	while ((millis() - start) < duration && !getDebouncedButtonState());
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
	Serial.begin(115200);
		
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
	
	PCICR = _BV(PCIE2); //enable pin change interrupts
	PCMSK2 = 0b01000000;

	Serial.println("ready");

	//main loop
	while (1)
	{
		if (getDebouncedButtonState())
		{
			if (!lineOpen)
			{
				SetLineOpen(1);

				uint32_t start = millis();
				Serial.println("waiting for switch release");
				while (getDebouncedButtonState()); //wait while switch is still activated
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
					
					if (getDebouncedButtonState())
						SetLineOpen(0);
				}
			}
			else
			{
				SetLineOpen(0);
			}

			Serial.println("waiting for switch release");
			while (getDebouncedButtonState()); //wait while switch is still activated
			Serial.println("ready");
		}
	}
}

//
void loop() {}
