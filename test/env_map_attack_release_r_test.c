#include <stdio.h> 
#include <math.h> 
#include "env_map.h"

int main (void)
{
    MMSample x, a, r;
    for (x = 0.; x < 2.*M_PI; x += 0.01) {
        env_map_attack_release_f(&a,&r,x,0.1,2.0,0.1,2.0);
        fwrite(&x,sizeof(MMSample),1,stdout);
        fwrite(&a,sizeof(MMSample),1,stdout);
        fwrite(&r,sizeof(MMSample),1,stdout);
    }
    return(0);
}
