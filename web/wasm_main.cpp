//
// Entry point of the WebAssembly build: the page sends UCI commands through wombat_command,
// engine output arrives line by line on stdout (Module.print).
//

#include <emscripten/emscripten.h>

#include "../src/board.h"
#include "../src/nnue.h"
#include "../src/uci.h"

static Engine *engine = nullptr;

int main() {
    board::init();
    nnue::load_default();
    engine = new Engine();
    return 0; // the runtime stays alive (EXIT_RUNTIME=0)
}

extern "C" EMSCRIPTEN_KEEPALIVE void wombat_command(const char *line) {
    if (engine) engine->command(line);
}
