#include <M5Unified.h>

void setup()
{
    auto cfg = M5.config();
    M5.begin(cfg);

    M5.Display.setTextSize(3);
    M5.Display.print("Hello World!!");
}

void loop()
{
}
