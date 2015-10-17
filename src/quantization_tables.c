/* Tables of rational numbers */
#include "quantization_tables.h" 

MMSample eventDeltaQuantTable[] = {
    1./4.,
    2./4.,
    3./4.,
    5./4.,
    6./4.,
    7./4.,
    8./4.,
    /*
    9./4.,
    10./4.,
    11./4.,
    12./4.,
    13./4.,
    14./4.,
    15./4.,
    16./4.
    */
};

int get_eventDeltaQuantTable_length(void)
{
    return sizeof(eventDeltaQuantTable) / sizeof(MMSample);
}
