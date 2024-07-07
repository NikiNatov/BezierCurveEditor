#include "application.h"

int main(int argc, char** argv)
{
#if defined(_DEBUG)
    // Memory leaks check
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif

    Application app(1280, 720);
    app.Run();
}