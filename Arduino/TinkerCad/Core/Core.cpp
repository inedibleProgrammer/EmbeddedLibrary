/*
    Description: Description Here

    Date: 11 January 2020
*/
#include <stdint.h>

//Classes__________________________________________________________________________
class Singleton
{
private: /* Private constructor: */
    Singleton(){}
public: /* Singleton setup: */
    static Singleton & Instance();
public: /* Methods */
private: /* Member Variables */
private: /* Registers */
};
/* Instance: It is necessary to bring this outside of the class for TinkerCAD to compile */
Singleton & Singleton::Instance()
{
    static Singleton instance;
    return instance;
}
// End Singleton



//Data Declarations________________________________________________________________

//Function Prototypes______________________________________________________________
void SimpleTimerInit();

int main()
{
    
    while(1)
    {

    }
    return 0;
}


//Function Definitions______________________________________________________________