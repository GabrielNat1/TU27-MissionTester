#include "types.h"
#include "constants.h"
#include "intrinsics.h"
#include "natives.h"
#include "common.h"

bool isNotifyActive = true;
bool isNotifyTeleport = false;

int bankActive = 0;
int inRange = 0;
int robberyActivated = 0;
int vaultUnlocked = 0;
int bankLocked = 0;
int scriptLoaded = 0;

#define DPAD_UP 12
#define DPAD_RIGHT 13
#define DPAD_DOWN 14
#define DPAD_LEFT 15
#define MAX_CIVILS 10
#define BANK_LOCK_TIME_MS (24*60*60*1000) // 24h

// distance
float spawnDistance = 50.0f;
float despawnDistance = 70.0f;

// coordinates
float bankX = -1212.0f;
float bankY = -330.0f;
float bankZ = 37.0f;

// mini-game
int sequence[4] = { DPAD_UP, DPAD_RIGHT, DPAD_DOWN, DPAD_LEFT };
int step = 0;
int lastInputTick = 0;

// lock
int bankLockStart = 0;

// npcs
int guard1 = 0;
int guard2 = 0;
int manager = 0;
int civils[MAX_CIVILS];
int numCivils = 0;

#define ACT_BTN_X 179  // INPUT_FRONTEND_X
#define ACT_BTN_B 178  // INPUT_FRONTEND_CANCEL (B) 


enum ScaleformButtons
{
    ARROW_UP, ARROW_DOWN, ARROW_LEFT, ARROW_RIGHT,
    BUTTON_DPAD_UP, BUTTON_DPAD_DOWN, BUTTON_DPAD_RIGHT, BUTTON_DPAD_LEFT,
    BUTTON_DPAD_BLANK, BUTTON_DPAD_ALL, BUTTON_DPAD_UP_DOWN, BUTTON_DPAD_LEFT_RIGHT,
    BUTTON_LSTICK_UP, BUTTON_LSTICK_DOWN, BUTTON_LSTICK_LEFT, BUTTON_LSTICK_RIGHT,
    BUTTON_LSTICK, BUTTON_LSTICK_ALL, BUTTON_LSTICK_UP_DOWN, BUTTON_LSTICK_LEFT_RIGHT, BUTTON_LSTICK_ROTATE,
    BUTTON_RSTICK_UP, BUTTON_RSTICK_DOWN, BUTTON_RSTICK_LEFT, BUTTON_RSTICK_RIGHT,
    BUTTON_RSTICK, BUTTON_RSTICK_ALL, BUTTON_RSTICK_UP_DOWN, BUTTON_RSTICK_LEFT_RIGHT, BUTTON_RSTICK_ROTATE,
    BUTTON_A, BUTTON_B, BUTTON_X, BUTTON_Y, BUTTON_LB, BUTTON_LT, BUTTON_RB, BUTTON_RT,
    BUTTON_START, BUTTON_BACK, RED_BOX, RED_BOX_1, RED_BOX_2, RED_BOX_3,
    LOADING_HALF_CIRCLE_LEFT, ARROW_UP_DOWN, ARROW_LEFT_RIGHT, ARROW_ALL,
    LOADING_HALF_CIRCLE_LEFT_2, SAVE_HALF_CIRCLE_LEFT, LOADING_HALF_CIRCLE_RIGHT
};

enum Buttons
{
    Button_A = 0xC1,
    Button_B = 0xC3,
    Button_X = 0xC2,
    Button_Y = 0xC0,
    Button_Back = 0xBF,
    Button_LB = 0xC4,
    Button_LT = 0xC6,
    Button_LS = 0xC8,
    Button_RB = 0xC5,
    Button_RT = 0xC7,
    Button_RS = 0xC9,
    Dpad_Up = 0xCA,
    Dpad_Down = 0xCB,
    Dpad_Left = 0xCC,
    Dpad_Right = 0xCD
};


void FloatingHelpText(const char* text)
{
    BEGIN_TEXT_COMMAND_DISPLAY_HELP("STRING");
    ADD_TEXT_COMPONENT_SUBSTRING_PLAYER_NAME(text);
    END_TEXT_COMMAND_DISPLAY_HELP(0, 0, 1, -1);
}

void SendPhoneMessage(const char* text) {
    FloatingHelpText(text);
}

void LoadModelSafe(Hash model)
{
    if (!HAS_MODEL_LOADED(model)) {
        REQUEST_MODEL(model);
        int timeout = GET_GAME_TIMER() + 5000; 
        while (!HAS_MODEL_LOADED(model) && GET_GAME_TIMER() < timeout) {
            WAIT(0);
        }
    }
}

int CREATE_PED_SAFE(int pedType, Hash model, float x, float y, float z, float heading)
{
    vector3 pos;
    pos.x = x;
    pos.y = y;
    pos.z = z;

    LoadModelSafe(model);

    if (!HAS_MODEL_LOADED(model))
        return 0;

    return CREATE_PED(pedType, model, pos, heading, true, true);
}

int CREATE_VEHICLE_SAFE(Hash model, float x, float y, float z, float heading)
{
    vector3 pos;
    pos.x = x;
    pos.y = y;
    pos.z = z;

    LoadModelSafe(model);

    if (!HAS_MODEL_LOADED(model))
        return 0;

    return CREATE_VEHICLE(model, pos, heading, true, true);
}

int CREATE_OBJECT_SAFE(Hash model, float x, float y, float z)
{
    LoadModelSafe(model);
    if (!HAS_MODEL_LOADED(model)) return 0;
    return CREATE_OBJECT(model, x, y, z, 0, 0, 0);
}

float Dist3D(float ax, float ay, float az, float bx, float by, float bz)
{
    vector3 a;
    a.x = ax;
    a.y = ay;
    a.z = az;

    vector3 b;
    b.x = bx;
    b.y = by;
    b.z = bz;

    return GET_DISTANCE_BETWEEN_COORDS(a, b, true);
}

void SpawnNPCs()
{
    float px, py, pz;
    px = bankX + 1.0f; py = bankY + 1.0f; pz = bankZ;
    Hash guardHash = GET_HASH_KEY("s_m_m_security_01");
    Hash managerHash = GET_HASH_KEY("s_m_m_bank_01");
    Hash civilHash = GET_HASH_KEY("a_m_m_business_01");

    guard1 = CREATE_PED_SAFE(26, guardHash, px, py, pz, 0.0f);
    px = bankX - 1.0f; py = bankY + 1.0f;
    guard2 = CREATE_PED_SAFE(26, guardHash, px, py, pz, 0.0f);

    px = bankX; py = bankY + 3.0f;
    manager = CREATE_PED_SAFE(26, managerHash, px, py, bankZ, 0.0f);

    numCivils = 0;
    for (int i = 0; i < 4; i++)
    {
        float cx = bankX - 2.0f + (float)i;
        float cy = bankY + 2.0f;
        float cz = bankZ;
        int civil = CREATE_PED_SAFE(26, civilHash, cx, cy, cz, 0.0f);
        if (civil && numCivils < MAX_CIVILS)
        {
            civils[numCivils++] = civil;
            TASK_START_SCENARIO_IN_PLACE(civil, "WORLD_HUMAN_STAND_MOBILE", 0, 1);
        }
    }

    if (DOES_ENTITY_EXIST(guard1)) TASK_START_SCENARIO_IN_PLACE(guard1, "WORLD_HUMAN_GUARD_STAND", 0, 1);
    if (DOES_ENTITY_EXIST(guard2)) TASK_START_SCENARIO_IN_PLACE(guard2, "WORLD_HUMAN_GUARD_STAND", 0, 1);
    if (DOES_ENTITY_EXIST(manager)) TASK_START_SCENARIO_IN_PLACE(manager, "WORLD_HUMAN_CLIPBOARD", 0, 1);
}

void DespawnNPCs()
{
    if (DOES_ENTITY_EXIST(guard1)) { DELETE_PED(&guard1); guard1 = 0; }
    if (DOES_ENTITY_EXIST(guard2)) { DELETE_PED(&guard2); guard2 = 0; }
    if (DOES_ENTITY_EXIST(manager)) { DELETE_PED(&manager); manager = 0; }

    for (int i = 0; i < numCivils; i++) {
        if (DOES_ENTITY_EXIST(civils[i])) { DELETE_PED(&civils[i]); civils[i] = 0; }
    }
    numCivils = 0;
}

void SpawnMoneyBags()
{
    float vx = bankX;
    float vy = bankY + 6.0f;
    float vz = bankZ;

    float offsets[6][2] = {
        {0.0f, 0.0f}, {1.0f, 1.0f}, {-1.0f, -1.0f},
        {1.0f, -1.0f}, {-1.0f, 1.0f}, {0.5f, -0.5f}
    };

    for (int i = 0; i < 6; i++) {
        float x = vx + offsets[i][0];
        float y = vy + offsets[i][1];
        CREATE_OBJECT_SAFE(GET_HASH_KEY("prop_money_bag_01"), x, y, vz);
    }
}

void SpawnSWAT()
{
    Hash carHash = GET_HASH_KEY("riot");
    Hash swatHash = GET_HASH_KEY("s_m_y_swat_01");

    int carOffsets[3][2] = { {10,20},{-10,20},{0,25} };
    for (int i = 0; i < 3; i++) {
        float cx = bankX + (float)carOffsets[i][0];
        float cy = bankY + (float)carOffsets[i][1];
        float cz = bankZ;
        int car = CREATE_VEHICLE_SAFE(carHash, cx, cy, cz, 0.0f);
        if (car) {
            LoadModelSafe(swatHash);
            int ped = CREATE_PED_INSIDE_VEHICLE(car, 26, swatHash, -1, 1, 1);
            if (DOES_ENTITY_EXIST(ped)) TASK_COMBAT_PED(ped, PLAYER_PED_ID(), 0, 16);
        }
    }
}

void OpenVaultDoor()
{
    vector3 pos;
    pos.x = bankX;
    pos.y = bankY + 6.0f;
    pos.z = bankZ;
    Hash doorHash = GET_HASH_KEY("v_ilev_bk_vaultdoor");

    int door = GET_CLOSEST_OBJECT_OF_TYPE(pos, 3.0f, doorHash, false, false, false);

    if (DOES_ENTITY_EXIST(door))
    {
        for (float angle = 0.0f; angle <= 120.0f; angle += 5.0f)
        {
            // x360, 4º parameter rotationOrder
            // 5º parameter is p4 (bool)
            SET_ENTITY_ROTATION(door, 0.0f, 0.0f, angle, 2, false);

            WAIT(30);
        }
    }
}
void CivilsRunAway()
{
    for (int i = 0; i < numCivils; i++) {
        if (DOES_ENTITY_EXIST(civils[i])) {
            //any fleeTime = -1;
            //fleeTime.i = -1;

            TASK_SMART_FLEE_PED(civils[i], PLAYER_PED_ID(), 50.0f, (any)-1, true, true);
        }
    }
}

void AddCashToPlayer(int amount)
{
    FloatingHelpText("You picked up the money from the vault! (simulated)");
}

void AddBankBlip()
{
    Blip b = ADD_BLIP_FOR_COORD(bankX, bankY, bankZ);
    SET_BLIP_SPRITE(b, 108);
    SET_BLIP_COLOUR(b, 2);
    SET_BLIP_SCALE(b, 0.8f);
    SET_BLIP_AS_SHORT_RANGE(b, 1);
    BEGIN_TEXT_COMMAND_SET_BLIP_NAME("STRING");
    ADD_TEXT_COMPONENT_SUBSTRING_PLAYER_NAME("Fleeca Bank");
    END_TEXT_COMMAND_SET_BLIP_NAME(b);
}

void CheckProximity()
{
    int gameTime = GET_GAME_TIMER();

    if (bankLocked) {
        if ((int)(gameTime - bankLockStart) >= BANK_LOCK_TIME_MS) {
            bankLocked = 0;
            FloatingHelpText("The bank is operational again!");
            SendPhoneMessage("The bank is open again!");
        }
        return;
    }

    float px = 0.0f, py = 0.0f, pz = 0.0f;
    vector3 ppos = GET_ENTITY_COORDS(PLAYER_PED_ID(), 1);
    px = ppos.x; py = ppos.y; pz = ppos.z;

    float dist = Dist3D(px, py, pz, bankX, bankY, bankZ);

    if (dist < spawnDistance && !bankActive) {
        inRange = 1;
        if (isNotifyActive)
            FloatingHelpText("Press ~INPUT_FRONTEND_X~ + ~INPUT_FRONTEND_CANCEL~ to activate the mod near the bank.");
        else {
            SpawnNPCs();
            AddBankBlip();
            bankActive = 1; inRange = 1;
            FloatingHelpText("You are near the bank. Aim at the guards to start the heist.");
        }
    }
    else if (dist > despawnDistance && bankActive) {
        DespawnNPCs();
        bankActive = 0; inRange = 0; robberyActivated = 0; vaultUnlocked = 0; step = 0;
        bankLocked = 1; bankLockStart = gameTime;
    }
}

void CheckAggro()
{
    if (!robberyActivated && !bankLocked && bankActive) {
        bool aiming = false;
        if (DOES_ENTITY_EXIST(guard1) && IS_PLAYER_FREE_AIMING_AT_ENTITY(PLAYER_PED_ID(), guard1)) aiming = true;
        if (DOES_ENTITY_EXIST(guard2) && IS_PLAYER_FREE_AIMING_AT_ENTITY(PLAYER_PED_ID(), guard2)) aiming = true;
        if (DOES_ENTITY_EXIST(manager) && IS_PLAYER_FREE_AIMING_AT_ENTITY(PLAYER_PED_ID(), manager)) aiming = true;
        if (IS_PED_SHOOTING(PLAYER_PED_ID())) aiming = true;

        if (aiming) {
            robberyActivated = 1;
            SET_MAX_WANTED_LEVEL(5);
            SET_PLAYER_WANTED_LEVEL(PLAYER_PED_ID(), 5, 0);
            SET_PLAYER_WANTED_LEVEL_NOW(PLAYER_PED_ID(), 0);
    
            if (PREPARE_ALARM("BANK_ALARM")) START_ALARM("BANK_ALARM", true);
            CivilsRunAway();
            FloatingHelpText("Heist started! Go to the vault and press the correct button sequence.");
        }
    }
}

int IS_ANY_BUTTON_PRESSED_WRONG()
{
    for (int i = 0; i < 4; i++) if (IS_CONTROL_JUST_PRESSED(0, sequence[i]) && i != step) return 1;
    return 0;
}

void VaultMiniGame()
{
    if (!robberyActivated || vaultUnlocked || !bankActive) return;

    vector3 vaultPos = { bankX, bankY + 6.0f, bankZ };
    vector3 playerPos = GET_ENTITY_COORDS(PLAYER_PED_ID(), 1);

    if (Dist3D(vaultPos.x, vaultPos.y, vaultPos.z, playerPos.x, playerPos.y, playerPos.z) < 2.0f) {
        BEGIN_TEXT_COMMAND_DISPLAY_HELP("STRING");
        ADD_TEXT_COMPONENT_SUBSTRING_PLAYER_NAME("Press the correct button sequence!");
        END_TEXT_COMMAND_DISPLAY_HELP(0, 0, 1, -1);

        if (IS_CONTROL_JUST_PRESSED(0, sequence[step])) {
            int now = GET_GAME_TIMER();
            if (now - lastInputTick > 100) {
                step++;
                lastInputTick = now;
                if (step == 4) {
                    FloatingHelpText("Vault open! Take the money!");
                    SpawnMoneyBags();
                    OpenVaultDoor();
                    AddCashToPlayer(50000);
                    vaultUnlocked = 1;
                    SpawnSWAT();
                }
            }
        }
        else if (IS_ANY_BUTTON_PRESSED_WRONG()) {
            step = 0;
        }
    }
}

void OtherLoop()
{
    if (isNotifyActive) {
        FloatingHelpText("Press ~INPUT_FRONTEND_X~ + ~INPUT_FRONTEND_CANCEL~ to enable!");
    }
    if (isNotifyTeleport) {
        FloatingHelpText("Press ~INPUT_FRONTEND_RS~ to teleport!");
    }
}

void Hook()
{
    OtherLoop();

    if (inRange && isNotifyActive) {
        if (IS_CONTROL_PRESSED(0, ACT_BTN_X) && IS_CONTROL_PRESSED(0, ACT_BTN_B)) {
            isNotifyActive = false;
            isNotifyTeleport = true;

            SpawnNPCs();
            AddBankBlip();
            bankActive = 1;
            FloatingHelpText("Mod activated! Aim at the guards to start the heist.");
            WAIT(200);
        }
    }

    if (isNotifyTeleport && IS_CONTROL_JUST_PRESSED(0, Button_RS)) {
        isNotifyTeleport = false;
        int handle = PLAYER_PED_ID();
        if (IS_PED_IN_ANY_VEHICLE(handle, 0)) handle = GET_VEHICLE_PED_IS_IN(handle, 0);
        SET_ENTITY_COORDS(handle, bankX + 5.0f, bankY + 5.0f, bankZ + 1.0f, 0, 0, 0, 1);
    }

    // checks
    CheckProximity();
    CheckAggro();
    VaultMiniGame();
}

void main()
{
    _SET_NOTIFICATION_TEXT_ENTRY("STRING");
    ADD_TEXT_COMPONENT_SUBSTRING_PLAYER_NAME("~s~~b~Bank Heist Mod Loaded!");
    _DRAW_NOTIFICATION(3000, 1);

    NETWORK_SET_SCRIPT_IS_SAFE_FOR_NETWORK_GAME();
    scriptLoaded = 1;
    FloatingHelpText("Bank Heist script started.");

    while (true)
    {
        Hook();
        WAIT(0);
    }
}
