#include "gnss_processor.h"

#include <cstdlib>
#include <iostream>
#include <limits>

#define CHECK(name, expression) \
    do { \
        if (expression) { \
            std::cout << "[PASS] " << name << '\n'; \
        } else { \
            std::cerr << "[FAIL] " << name << '\n'; \
            return EXIT_FAILURE; \
        } \
    } while (false)

int main() {
    { GnssProcessor p; CHECK("normal frames", p.process({1000,12.9716,77.5946,1.2f,8,1}) && p.process({1100,12.971601,77.594601,1.1f,9,1})); }
    { GnssProcessor p; CHECK("high HDOP rejected", !p.process({1000,12.9,77.5,6.5f,8,1})); }
    { GnssProcessor p; CHECK("no fix rejected", !p.process({1000,12.9,77.5,1.2f,8,0})); }
    { GnssProcessor p; CHECK("latitude above range", !p.process({1000,91,77.5,1.2f,8,1})); }
    { GnssProcessor p; CHECK("latitude below range", !p.process({1000,-91,77.5,1.2f,8,1})); }
    { GnssProcessor p; CHECK("longitude above range", !p.process({1000,12.9,181,1.2f,8,1})); }
    { GnssProcessor p; CHECK("longitude below range", !p.process({1000,12.9,-181,1.2f,8,1})); }
    { GnssProcessor p; CHECK("too few satellites", !p.process({1000,12.9,77.5,1.2f,3,1})); }
    { GnssProcessor p; CHECK("gap over limit rejected", p.process({1000,12.9,77.5,1.2f,8,1}) && !p.process({2601,12.9,77.5,1.2f,8,1})); }
    { GnssProcessor p; CHECK("exact gap limit accepted", p.process({1000,12.9,77.5,1.2f,8,1}) && p.process({2500,12.9,77.5,1.2f,8,1})); }
    { GnssProcessor p; CHECK("duplicate timestamp rejected", p.process({1000,12.9,77.5,1.2f,8,1}) && !p.process({1000,12.9,77.5,1.2f,8,1})); }
    { GnssProcessor p; CHECK("out of order rejected", p.process({1100,12.9,77.5,1.2f,8,1}) && !p.process({1000,12.9,77.5,1.2f,8,1})); }
    { GnssProcessor p; CHECK("NaN latitude rejected", !p.process({1000,std::numeric_limits<double>::quiet_NaN(),77.5,1.2f,8,1})); }
    { GnssProcessor p; CHECK("infinite longitude rejected", !p.process({1000,12.9,std::numeric_limits<double>::infinity(),1.2f,8,1})); }
    { GnssProcessor p; CHECK("infinite HDOP rejected", !p.process({1000,12.9,77.5,std::numeric_limits<float>::infinity(),8,1})); }
    { GnssProcessor p; CHECK("negative HDOP rejected", !p.process({1000,12.9,77.5,-0.1f,8,1})); }
    { GnssProcessor p; CHECK("unsupported fix rejected", !p.process({1000,12.9,77.5,1.2f,8,3})); }
    { GnssProcessor p; CHECK("inclusive coordinate and HDOP boundaries", p.process({1000,-90,-180,0.0f,4,1}) && p.process({1100,90,180,5.0f,4,2})); }
    { GnssProcessor p; CHECK("invalid quality advances received timestamp", !p.process({1000,12.9,77.5,6.0f,8,1}) && p.process({1100,12.9,77.5,1.2f,8,1})); }
    { GnssProcessor p; CHECK("quality rejection does not poison recovery", p.process({1000,12.9,77.5,1.2f,8,1}) && !p.process({1100,91,77.5,1.2f,8,1}) && p.process({1200,12.9,77.5,1.2f,8,1})); }
    { GnssProcessor p; CHECK("first frame can have zero timestamp", p.process({0,12.9,77.5,1.2f,8,1})); }
    { GnssProcessor p; CHECK("long-gap baseline recovery", p.process({1000,12.9,77.5,1.2f,8,1}) && !p.process({2600,12.9,77.5,1.2f,8,1}) && p.process({2700,12.9,77.5,1.2f,8,1})); }
    { GnssProcessor p; CHECK("Null Island placeholder rejected", !p.process({1000,0.0,0.0,1.2f,8,1})); }
    { GnssProcessor p({5.0f,4,1500,false}); CHECK("Null Island accepted when configured", p.process({1000,0.0,0.0,1.2f,8,1})); }
    { GnssProcessor p; CHECK("equator and prime meridian coordinates remain valid", p.process({1000,0.0,77.5,1.2f,8,1}) && p.process({1100,12.9,0.0,1.2f,8,1})); }
    { GnssProcessor p({3.0f,4,1500}); CHECK("custom HDOP limit", !p.process({1000,12.9,77.5,4.0f,8,1})); }
    { GnssProcessor p({5.0f,6,1500}); CHECK("custom minimum satellite count", !p.process({1000,12.9,77.5,1.2f,5,1})); }
    { GnssProcessor p({5.0f,4,500}); CHECK("custom maximum timestamp gap", p.process({1000,12.9,77.5,1.2f,8,1}) && !p.process({1601,12.9,77.5,1.2f,8,1})); }
    { GnssProcessor p; const bool first = p.process({2000,12.9,77.5,1.2f,8,1}); p.reset(); CHECK("reset starts a fresh timestamp session", first && p.process({1000,12.9,77.5,1.2f,8,1})); }
    std::cout << "All 29 tests passed.\n";
    return EXIT_SUCCESS;
}
