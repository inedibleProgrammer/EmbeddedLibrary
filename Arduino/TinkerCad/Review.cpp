#include <stdint.h>

namespace att45
{
class IoController
{
public:
    IoController(volatile uint8_t * port, uint8_t pin)
    : m_port_x(port)
    , m_pin(pin)
    {
        m_ddr_x = (port - 1);
    }

    inline void SetOutput()
    {
        (*m_ddr_x) = (*m_ddr_x) | (1 << m_pin);
    }

    inline void Toggle()
    {
        (*m_port_x) ^= (1 << m_pin);
    }

    uint8_t Read()
    {

    }

private:
    volatile uint8_t * m_port_x;
    volatile uint8_t * m_ddr_x;
    volatile uint8_t * m_pin_x;
    uint8_t m_pin;
};
}

namespace att45
{
class Timer_0_Controller
{
public:
    Timer_0_Controller()
    {
        TCCR0B = TCCR0B | (1 << CS02); // prescale 256
    }
private:

};
}

uint16_t timer_ovf_ticks = 0;

int main()
{
    ::att45::IoController port_b_pin_0(&PORTB, 0);
    port_b_pin_0.SetOutput();

    ::att45::Timer_0_Controller timer0;
    while(1)
    {
        // port_b_pin_0.Toggle();
    }
}

ISR(TIMER0_OVF_vect)
{
    timer_ovf_ticks = timer_ovf_ticks + 1;
}