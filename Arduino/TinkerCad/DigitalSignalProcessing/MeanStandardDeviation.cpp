/*
    Description: This is me testing grabbing the mean and standard deviation from something
    using the ATTiny45.

    Date: 22 Feb. 2020
*/
//Include files____________________________________________________________________
#include <stdint.h>
#include <avr/interrupt.h>

//Classes__________________________________________________________________________
/*
    Name: Timer0Controller
    Description: 
    * This class is a singleton to control
    the timer0 module of the ATTiny45.
    * Some functionality is partially unnecessary.
    * This timer acts as a 1 ms counter.
*/
class Timer0Controller
{
private: // Singleton Setup:
    Timer0Controller(){}
public: // Singleton Setup:
    static Timer0Controller & Instance();

public: // Methods:
    void Init()
    {
        // Set the main task loop:
        m_taskLoop = false;

        // Assign the registers their addresses:
        r_controlA = (uint8_t*)(0x2A + 0x20); // TCCR0A
        r_controlB = (uint8_t*)(0x33 + 0x20); // TCCR0B
        r_counter  = (uint8_t*)(0x23 + 0x20); // TCNT0
        r_outputCompareA = (uint8_t*)(0x29 + 0x20); // OCR0A
        r_outputCompareB = (uint8_t*)(0x28 + 0x20); // OCR0B
        r_interruptMask  = (uint8_t*)(0x39 + 0x20); // TIMSK

        // CTC Mode: (WGM0[2:0] = 2)
        (*r_controlA) = (*r_controlA) | (1 << 1); // Set WGM01

        // Set OCR0A to 125 to get the 1 ms interrupt period
        (*r_outputCompareA) = 125;

        // Set OC0A on Compare Match
        // (*r_controlA) = (*r_controlA) | (1 << 7); // Set COM0A1
        // (*r_controlA) = (*r_controlA) | (1 << 6); // Set COM0A0

        // Prescale: 1/64
        (*r_controlB) = (*r_controlB) | (1 << 0); // Set CS00
        (*r_controlB) = (*r_controlB) | (1 << 1); // Set CS01

        // Enable compare match interrupt
        (*r_interruptMask) = (*r_interruptMask) | (1 << 4); // Set OCIE0A.

    }
    void WaitTaskLoop()
    {
        while(!m_taskLoop){} // wait until main task loop
        m_taskLoop = false;
    }
    void InterruptRoutine()
    {
        m_counter++;
        m_taskLoop = true;
    }
private: // Member Variables:
    uint16_t       m_counter;
    volatile bool  m_taskLoop;
private: // Registers:
    volatile uint8_t * r_controlA;
    volatile uint8_t * r_controlB;
    volatile uint8_t * r_counter;
    volatile uint8_t * r_outputCompareA;
    volatile uint8_t * r_outputCompareB;
    volatile uint8_t * r_interruptMask;
};
/* I needed to bring this method out for TinkerCAD to compile it correctly. */
Timer0Controller & Timer0Controller::Instance()
{
    static Timer0Controller instance;
    return instance;
}
// end Timer0Controller


/*
    Name: Analog
    Description:
        * This class is a singleton.
*/
class Analog
{
private: // Singleton Setup:
    Analog(){}
public: // Singleton Setup:
    static Analog & Instance();
public:
    void Init()
    {
        /* Initialize registers: */
        r_multiplexerSelection = (uint8_t*)(0x07 + 0x20);
        r_controlStatusA       = (uint8_t*)(0x06 + 0x20);
        r_dataH                = (uint8_t*)(0x05 + 0x20);
        r_dataL                = (uint8_t*)(0x04 + 0x20);
        r_controlStatusB       = (uint8_t*)(0x03 + 0x20);
        r_digitalInputDisable  = (uint8_t*)(0x14 + 0x20);

        /* Set voltage reference to Vcc: */
            // Vcc is set by default, so do nothing

        /* Set single channel: */
            // By default, ADC5 (PB0), is selected, so do nothing

        /* Set the ADEN bit in the ADCSRA register to enable the ADC */
        (*r_controlStatusA) = (*r_controlStatusA) | (1 << 7);
    }

    uint16_t StartConversion()
    {
        uint16_t adcResult = 0;

        /* 
            * Start the conversion by setting the ADC Start Conversion Bit (ADSC) 
                * This bit stays high for as long as the conversion, and is cleared
                by hardware when the conversion is over.
                * Use IsConversionFinished() to detect if the bit has been cleared.
        */
        (*r_controlStatusA) = (*r_controlStatusA) | (1 << 6);

        /* Wait until conversion is finished. */
        while(!IsConversionFinished())
        {
            // Do nothing
        }

        /* Merge ADCH and ADCL into one register: */
        adcResult = (*r_dataH << 8) | (*r_dataL); 
    }

private: // Private methods:
    bool IsConversionFinished()
    {
        /* 
            * Test the ADC Start Conversion bit.
                * if it is set, return false.
                * if it is cleared, return true.
        */
        return ( (*r_controlStatusA) & (1 << 6) ) == 0;
    }

private: // Member variables:
private: // Registers:
    volatile uint8_t * r_multiplexerSelection;  /* ADMUX */
    volatile uint8_t * r_controlStatusA;        /* ADCSRA */
    volatile uint8_t * r_dataH;                 /* ADCH */
    volatile uint8_t * r_dataL;                 /* ADCH */
    volatile uint8_t * r_controlStatusB;        /* ADCSRB */
    volatile uint8_t * r_digitalInputDisable;   /* DIDR0 */

};
/* I needed to bring this method out for TinkerCAD to compile it correctly. */
Analog & Analog::Instance()
{
    static Analog instance;
    return instance;
}
// end Analog

/*
    Name: Button
    Description: Simple method of controlling a button.
    * An event is set when the button goes from uncertainPressed -> pressed
*/
class Button
{
public:
    #define DEBOUNCE_TIME 20

    typedef enum
    {
        released,
        uncertainPressed,
        pressed,
        uncertainReleased
    } ButtonState;
public:
    Button(uint8_t* port, uint8_t pin)
    : m_buttonEvent(false)
    , m_state(released)
    , r_port(port + 0x20)
    , m_pin(pin)
    {
        *(r_port - 1) = (*(r_port - 1)) & ~(1 << m_pin); // Set the DDxn to input
        *(r_port) = (*r_port) | (1 << m_pin); // Activate pullup resistor
    }

    void Process()
    {
        switch(m_state)
        {
            case released:
                if(!Read())
                {
                    m_state = uncertainPressed;
                }
                else
                {
                    m_buttonEvent = false;
                }
                break;
            case uncertainPressed:
                {
                    static uint8_t pressedCount;
                    if(Read())
                    {
                        pressedCount = 0;
                        m_state = released;
                    }
                    else
                    {
                        pressedCount++;
                    }

                    if(pressedCount >= DEBOUNCE_TIME)
                    {
                        m_state = pressed;
                        m_buttonEvent = true;
                        pressedCount = 0;
                    }
                }
                break;
            case pressed:
                if(Read())
                {
                    m_state = uncertainReleased;
                }
                break;
            case uncertainReleased:
                {
                    static uint8_t releasedCount;
                    if(!Read())
                    {
                        releasedCount = 0;
                        m_state = pressed;
                    }
                    else
                    {
                        releasedCount++;
                    }

                    if(releasedCount >= DEBOUNCE_TIME)
                    {
                        m_state = released;
                        releasedCount = 0;
                    }
                }
                break;
            default:
                break;
        }
    } // end Process()

    bool GetEvent()
    {
        bool ret;
        ret = m_buttonEvent;
        m_buttonEvent = false;
        return ret;
    }
private: // Private Methods:
    inline bool Read()
    {
        if((*(r_port - 2) & (1 << m_pin)) == (1 << m_pin)) // Read the PIN register
        {
            return true;
        }
        else
        {
            return false;
        }
    }
private: // Member variables:
    bool m_buttonEvent;
    ButtonState m_state;
    volatile uint8_t m_pin;
private: // Registers:
    volatile uint8_t * r_port;
};
// end Button

/*
    Name: Led
    Description: 
    * Simple method of controlling an Led.
    * Parameters:
        * Port: The address of the Port according to the datasheet. (the class adds 0x20 for you)
        * Pin:  The relevant pin
*/
class Led
{
public:
    Led(uint8_t * port, uint8_t pin)
    : m_state(false)
    , r_port(port + 0x20)
    , m_pin(pin)
    {
        // Set the DDxn to output:
        *(r_port - 1) = (*(r_port - 1)) | (1 << m_pin);

        // Initialize the port to off:
        (*r_port) = (*r_port) & ~(1 << m_pin);
    }
    void Toggle()
    {
        (*r_port) = (*r_port) ^ (1 << m_pin);
    }
    void Activate()
    {
        if(m_state == true)
        {

        }
        else
        {
            (*r_port) = (*r_port) | (1 << m_pin);
            m_state = true;
        }
    }
    void Deactivate()
    {
        if(m_state == false)
        {

        }
        else
        {
            (*r_port) = (*r_port) & ~(1 << m_pin);
            m_state = false;
        }
    }
    void OneSecondBlink()
    {
        static uint16_t blinkTime;

        if(blinkTime >= 1000)
        {
            this->Toggle();
            blinkTime = 0;
        }
        blinkTime++;
    }
private: // Member variables:
    bool m_state;
    volatile uint8_t m_pin;
private: // Registers:
    volatile uint8_t* r_port;
};
// end Led

//Data Declarations________________________________________________________________
//Function Prototypes______________________________________________________________
//Function Definitions______________________________________________________________
int main()
{
    #define NUM_TRIALS 10

    Timer0Controller::Instance().Init();
    Analog::Instance().Init();

    // Enable Interrupts:
    sei();

    Led setFinishedLed((uint8_t*)0x18, 2); // PORTB, PIN2
    Button sampleStartButton((uint8_t*)0x18, 1); // PORTB, PIN1

    uint8_t breakpointDummy = 0;
    uint8_t currentTrial;
    bool collectingSamples = true;
    uint16_t experimentSamples[NUM_TRIALS];

    for(int i = 0; i < NUM_TRIALS; i++)
    {
        experimentSamples[i] = 0;
    }

    while(1)
    {
        sampleStartButton.Process(); // Poll to update button state

        if(collectingSamples)
        {
            if(sampleStartButton.GetEvent())
            {
                experimentSamples[currentTrial] = Analog::Instance().StartConversion();
                currentTrial++;
            }
            if(currentTrial >= NUM_TRIALS)
            {
                collectingSamples = false;
            }
        }
        else
        {
            setFinishedLed.Activate();
            if(sampleStartButton.GetEvent()) 
            {
                setFinishedLed.Deactivate();
                // Calculate the mean:
                uint32_t total = 0;
                float average = 0;
                for(int i = 0; i < NUM_TRIALS; i++)
                {
                    total = total + experimentSamples[i];
                }

                average = ((float)total) / ((float)NUM_TRIALS);

                // Restart the experiment:
                collectingSamples = true;
                currentTrial = 0;
                // The array does not need to be initialized to 0 because we're going to overwrite every value anyways
            }
        }

        Timer0Controller::Instance().WaitTaskLoop();
    }
    return 0;
} // end int main()

//Interrupt Service Routines________________________________________________________
ISR(TIMER0_COMPA_vect)
{
    Timer0Controller::Instance().InterruptRoutine();
} // end ISR(TIMER0_COMPA_vect)
