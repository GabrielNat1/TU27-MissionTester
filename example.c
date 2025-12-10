#include "types.h"
#include "constants.h"
#include "intrinsics.h"
#include "natives.h"
#include "common.h"

bool showHelp = true;
bool alreadyPressed = false;

enum Buttons
{
    Button_A = 0xC1,
    Button_X = 0xC2
};

void FloatingHelpText(const char* text)
{
    BEGIN_TEXT_COMMAND_DISPLAY_HELP("STRING");
    ADD_TEXT_COMPONENT_SUBSTRING_PLAYER_NAME(text);
    END_TEXT_COMMAND_DISPLAY_HELP(0, 0, 1, -1);
}

void main()
{
    NETWORK_SET_SCRIPT_IS_SAFE_FOR_NETWORK_GAME();

    while (true)
    {
        if (showHelp)
        {
            FloatingHelpText("Press ~INPUT_FRONTEND_X~ + ~INPUT_FRONTEND_A~");
        }
        
        if (!alreadyPressed &&
            IS_CONTROL_PRESSED(0, Button_X) &&
            IS_CONTROL_PRESSED(0, Button_A))
        {
            alreadyPressed = true;
            showHelp = false;

            _SET_NOTIFICATION_TEXT_ENTRY("STRING");
            ADD_TEXT_COMPONENT_SUBSTRING_PLAYER_NAME("~g~Comando ativado com sucesso!");
            _DRAW_NOTIFICATION(5000, 1);
        }

        WAIT(0);
    }
}