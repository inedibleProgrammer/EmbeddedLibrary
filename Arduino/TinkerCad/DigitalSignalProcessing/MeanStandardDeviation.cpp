/*
    Description: This is me testing grabbing the mean and standard deviation from something
    using the ATTiny45.

    Date: 11 Jan. 2020
*/

//Classes__________________________________________________________________________
/*
    Name: Timer0Controller
    Description: 
    * This class is a singleton to control
    the timer0 module of the ATTiny45.
    * Some functionality is partially unnecessary.
*/
class Timer0Controller
{
public: // Singleton Setup:
    Timer0Controller(){}
    static Timer0Controller & Instance();
public: // Methods:
    void Init()
    {
        // Set the main task loop:
        m_taskLoop = false;

        // Assign the registers their addresses:
        ControlA = (uint8_t*)(0x2A + 0x20); // TCCR0A
        ControlB = (uint8_t*)(0x33 + 0x20); // TCCR0B
        Counter  = (uint8_t*)(0x23 + 0x20); // TCNT0
        OutputCompareA = (uint8_t*)(0x29 + 0x20); // OCR0A
        OutputCompareB = (uint8_t*)(0x28 + 0x20); // OCR0B
        InterruptMask  = (uint8_t*)(0x39 + 0x20); // TIMSK

        // CTC Mode: (WGM0[2:0] = 2)
        (*ControlA) = (*ControlA) | (1 << 1); // Set WGM01

        // Set OCR0A to 125 to get the 1 ms interrupt period
        (*OutputCompareA) = 125;

        // Set OC0A on Compare Match
        // (*ControlA) = (*ControlA) | (1 << 7); // Set COM0A1
        // (*ControlA) = (*ControlA) | (1 << 6); // Set COM0A0

        // Prescale: 1/64
        (*ControlB) = (*ControlB) | (1 << 0); // Set CS00
        (*ControlB) = (*ControlB) | (1 << 1); // Set CS01

        // Enable compare match interrupt
        (*InterruptMask) = (*InterruptMask) | (1 << 4); // Set OCIE0A.

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
    volatile uint8_t * ControlA;
    volatile uint8_t * ControlB;
    volatile uint8_t * Counter;
    volatile uint8_t * OutputCompareA;
    volatile uint8_t * OutputCompareB;
    volatile uint8_t * InterruptMask;
};
Timer0Controller & Timer0Controller::Instance()
{
    static Timer0Controller instance;
    return instance;
}

// end Timer0Controller

/*
    Name: Button
    Description: Simple method of controlling a button.
*/
class Button
{
public:
    Button(uint8_t* port, uint8_t pin)
    : m_port(port + 0x20)
    , m_pin(pin)
    {
        // Set the DDxn to input:
        *(m_port - 1) = (*(m_port - 1)) & ~(1 << m_pin);
        *(m_port) = (*m_port) | (1 << m_pin); // Activate pullup resistor
    }

    void Process()
    {
        static uint8_t onTime;
        static uint8_t offTime;

        if(this->Read())
        {
            onTime++;
            offTime = 0;
        }
        else
        {
            offTime++;
            onTime = 0;
        }

        if(onTime >= 50)
        {
            m_state = true;
            onTime = 51;
        }
        else
        {
            m_state = false;
            offTime = 51;
        }
    }

    bool GetState()
    {
        return m_state;
    }
private: // Private Methods:
    bool Read()
    {
        if((*(m_port - 2) & (1 << m_pin)) == (1 << m_pin)) // Read the PIN register
        {
            return true;
        }
        else
        {
            return false;
        }
    }
private: // Member variables:
    bool m_state;

private: // Registers:
    volatile uint8_t* m_port;
    volatile uint8_t  m_pin;
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
    , m_port(port + 0x20)
    , m_pin(pin)
    {
        // Set the DDxn to output:
        *(m_port - 1) = (*(m_port - 1)) | (1 << m_pin);

        // Initialize the port to off:
        (*m_port) = (*m_port) & ~(1 << m_pin);
    }
    void Toggle()
    {
        (*m_port) = (*m_port) ^ (1 << m_pin);
    }
    void Activate()
    {
        if(m_state == true)
        {

        }
        else
        {
            (*m_port) = (*m_port) | (1 << m_pin);
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
            (*m_port) = (*m_port) & ~(1 << m_pin);
            m_state = false;
        }
    }
    void OneSecondBlink()
    {
        static int blinkTime;

        if(blinkTime >= 1000)
        {
            this->Toggle();
            blinkTime = 0;
        }
        blinkTime++;
    }
private: // Member variables:
    bool m_state;
private: // Registers:
    volatile uint8_t* m_port;
    volatile uint8_t  m_pin;
};
// end Led

//Data Declarations________________________________________________________________
//Function Prototypes______________________________________________________________
//Function Definitions______________________________________________________________
int main()
{
    Timer0Controller::Instance().Init();

    // Enable Interrupts:
    sei();

    // Create a test Led:
    Led testLed((uint8_t*)0x18, 2); // PORTB, PIN0
    Button testButton((uint8_t*)0x18, 1); // PORTB, PIN1

    while(1)
    {
        // testLed.OneSecondBlink();
        testButton.Process();

        if(testButton.GetState())
        {
            testLed.Activate();
        }
        else
        {
            testLed.Deactivate();
        }

        Timer0Controller::Instance().WaitTaskLoop();
    }
    return 0;
}

//Interrupt Service Routines________________________________________________________
ISR(TIMER0_COMPA_vect)
{
    Timer0Controller::Instance().InterruptRoutine();
}

// ISR(TIMER0_OVF_vect)
// {
//     Timer0Controller::Instance().InterruptRoutine();
// }