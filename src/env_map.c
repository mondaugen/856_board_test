#include "env_map.h" 
#include <math.h> 

/* given x between 0 and 1, will return an attack and a release time. See
 * scripts/smart_envelope_demo.m to see the shape.
 * amin is the minimum attack time
 * amax is the maximum attack time
 * rmin is the minimum release time
 * rmax is the maximum release time */
void env_map_attack_release_f(MMSample *a, MMSample *r, MMSample x, MMSample amin,
        MMSample amax, MMSample rmin, MMSample rmax)
{
    MMSample _a, _r;
    _a = sinf(x*2.*M_PI - M_PI_4);
    if (_a < -M_SQRT1_2) {
        _a = -M_SQRT1_2;
    }
    if (_a > M_SQRT1_2) {
        _a = M_SQRT1_2;
    }
    _a = _a*M_SQRT2+1.;
    *a = _a*0.5*(amax - amin) + amin;
    _r = sinf(x*2.*M_PI + M_PI_4);
    if (_r < -M_SQRT1_2) {
        _r = -M_SQRT1_2;
    }
    if (_r > M_SQRT1_2) {
        _r = M_SQRT1_2;
    }
    _r = _r*-M_SQRT2+1.;
    *r = _r*0.5*(rmax - rmin) + rmin;
}

