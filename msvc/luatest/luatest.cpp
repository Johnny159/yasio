#include "../../src/lyasio.h"
#include "../../src/string_view.hpp"
#include "../../src/sol.hpp"
#include <thread>
#pragma comment(lib, "lua51.lib")

int main(int argc, char** argv)
{
    sol::state s;
    s.open_libraries();
    luaopen_yasio(s.lua_state());

    s.script_file("example.lua");

    do {
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    } while (!s["global_update"].call(50.0 / 1000));

    return 0;
}
