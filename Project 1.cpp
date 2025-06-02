#include<ctime>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>
#include "CC212SGL.h"
#include <windows.h>      // Required for Windows API functions
#include <mmsystem.h>     // Required for multimedia functions like PlaySound
#include <iostream>

#pragma comment(lib, "winmm.lib")  // Link the winmm.lib library for multimedia functions


#pragma comment(lib, "CC212SGL.lib")
CC212SGL gl;


bool debug = false;

//Input Handling
bool _keys[256] = { false };

//This function will check whether a key has been pressed (once)
//bool returns either true or false
bool isKeyPressedOnce(int key) // from friday session
{
    SHORT keyState = GetAsyncKeyState(key);
    bool isCurrentlyPressed = keyState & 0x8000; //if true, then MSB is set
    bool result = false;

    if (isCurrentlyPressed && _keys[key] == false)
    {
        result = true;
        _keys[key] = true;
    }
    else if (!isCurrentlyPressed)
    {
        _keys[key] = false;
    }
    return result;
}




void disableQuickEditMode() //from GPT: fix left click removing the graphics
{
    HANDLE hInput = GetStdHandle(STD_INPUT_HANDLE);
    DWORD mode;
    GetConsoleMode(hInput, &mode);
    mode &= ~ENABLE_QUICK_EDIT_MODE;
    mode |= ENABLE_MOUSE_INPUT;
    SetConsoleMode(hInput, mode);
}

class Objects  //from friday session 
{
public:
    static enum BOXES { L = 0, R = 1, U = 2, Lower = 3 };
    static enum Animations {
        Boss_idle, Boss_shoot, Boss_reload, Boss_jump, Boss_hurt, Boss_dead, LeftBullet, Left, NPCLeft, NPCRight, Bullet, Right, Jump, NPCJump, LNPCJump, LeftJump, NPCDeath, Death, RIdle, LIdle, Damage, Shooting, LShooting, Reloading, LReloading, background, Custom2, NPCMeleeAttack, MeleeAttack, LMeleeAttack, LNPCMeleeAttack, Meele2, LMeele2, Meele3, LMeele3, run, Rrun,
    };

    int sprites[30][60];
    int totalFramesCount[50] = { -1 };
    float current_frame = 0;
    int frame_dir = 1;
    Animations activeAnimation;
    int x = 0, y = 0;

    int box[4] = { 0 };

    void setBoundingBox(int left, int right, int upper, int lower) {
        box[0] = left;
        box[1] = right;
        box[2] = upper;
        box[3] = lower;
    }

    int getLeftEdge()
    {
        return x + box[0];
    }
    int getRightEdge() {
        return x + box[1];
    }
    int getUpperEdge() {
        return y + box[2];
    }
    int getLowerEdge() {
        return y + box[3];
    }
    int getCenterX() {
        return (getLeftEdge() + getRightEdge()) / 2;
    }

    //from friday session
    void loadFrames(const char path[], Animations anim, int frames, bool isFullScreen = false) {
        for (int i = 0; i < frames; i++)
        {

            char final_loc[100]; //maximum path size is 100 chars
            char num[50];

            itoa(i, num, 10);	//Convert from int to string (ascii)

            strcpy(final_loc, path);	//final_loc = path
            strcat(final_loc, num);	//final_loc = final_loc+num
            strcat(final_loc, ".png");	//final_loc = final_loc+num

            sprites[anim][i] = gl.loadImage(final_loc);
            if (isFullScreen == true)
                gl.resizeImage(sprites[anim][i], gl.getWindowWidth(),
                    gl.getWindowHeight());
        }
        totalFramesCount[anim] = frames;
    }

    void animate(float speed = 4.0f, bool cycle = true) // from friday session
    {
        if (cycle)
        {
            //Processing
            if (current_frame <= 0)
                frame_dir = 1;
            else if (current_frame >= totalFramesCount[activeAnimation] - 1)
                frame_dir = 0;

            if (frame_dir == 1)	//Upward direction
                current_frame += 1.0 / speed;
            else if (frame_dir == 0)
                current_frame -= 1.0 / speed;
        }
        else
        {
            frame_dir = 1;
            current_frame += 1.0 / speed;
            if (current_frame >= totalFramesCount[activeAnimation] - 1)
                current_frame = 0.0f;
        }

    }

    void setActiveAnimation(Animations active)
    {
        activeAnimation = active;
        current_frame = 0;
    }

    virtual void render() {
        gl.drawImage(sprites[activeAnimation][(int)current_frame], x, y, gl.generateFromRGB(255, 255, 255));

        current_frame += 0.2f * frame_dir;
        if (current_frame >= totalFramesCount[activeAnimation]) current_frame = 0;
        if (debug) {
            int left = getLeftEdge();
            int top = getUpperEdge();
            int width = getRightEdge() - left;
            int height = getLowerEdge() - top;

            gl.setDrawingColor(COLORS::RED);
            gl.drawRectangle(left, top, width, height);
        }
    }
};
class Player : public Objects // some things are from friday sessions
{
public:
    int currentAmmo = 10;    // Current bullets in magazine
    bool needsReload = false;
    int dirx = 0;
    int diry = 0;
    int hp = 100;
    bool facingright = true;



    // Jumping and Movement Variables
    float gravity = 0.98f;
    float jumpStrength = 20.0f;
    float yVelocity = 0.0f;
    float jumpMomentum = 2.0f;    // Horizontal speed during jump
    int holdJumpDirection = 0;
    bool isAttacking = false;
    bool isJumping = false;
    bool isShooting = false;
    bool isMeleeAttack = false;
    bool isReloading = false;
    bool isDamaged = false;

    void movement()
    {
        // Allow horizontal movement even while jumping
        if (!isReloading && !isMeleeAttack)  // Only restrict movement during reload or melee
        {
            if (isJumping)
            {
                // Use jumpMomentum for air movement
                x += dirx * jumpMomentum;
            }
            else
            {
                x += dirx;  // Normal ground movement speed
            }
        }
    }
    virtual void Jumping(bool falling = false)
    {
        if (!isJumping)
        {
            isJumping = true;
            yVelocity = jumpStrength * !falling;
            holdJumpDirection = dirx;  // Store the direction when jump starts
            facingright ? setActiveAnimation(Objects::Jump) : setActiveAnimation(Objects::LeftJump);

        }
    }

    virtual void Physics()
    {
        // Apply gravity and vertical movement
        yVelocity -= gravity;
        y -= yVelocity;

        // Horizontal movement during jump uses holdJumpDirection
        if (isJumping)
        {
            // Use the stored jump direction for consistent momentum
            x += holdJumpDirection * jumpMomentum;
        }

        // Check if the player has landed
        if (y >= gl.getWindowHeight() - 200)
        {
            y = gl.getWindowHeight() - 200;
            yVelocity = 0;

            // Reset animation only if not in another action state
            if (isJumping && !isReloading && !isShooting && !isMeleeAttack)
            {
                facingright ? setActiveAnimation(Objects::RIdle) : setActiveAnimation(Objects::LIdle);
            }
            isJumping = false;
        }
    }


    void PlayerAllAnimations() {
        loadFrames("Characters\\Gangster 1\\Short_Idle", Objects::RIdle, 5);
        loadFrames("Characters\\Gangster 1\\Walking", Objects::Right, 9);
        loadFrames("Characters\\Gangster 1\\Jumping", Objects::Jump, 9);
        loadFrames("Characters\\Gangster 1\\Melee_attack", Objects::MeleeAttack, 3);
        loadFrames("Characters\\Gangster 1\\Death", Objects::Death, 4);
        loadFrames("Characters\\Gangster 1\\Damage", Objects::Damage, 4);
        loadFrames("Characters\\Gangster 1\\Shooting", Objects::Shooting, 3);
        loadFrames("Characters\\Gangster 1\\Reloading", Objects::Reloading, 16);

        loadFrames("Characters\\Gangster 1\\LWalking", Objects::Left, 9);
        loadFrames("Characters\\Gangster 1\\LShort_Idle", Objects::LIdle, 5);
        loadFrames("Characters\\Gangster 1\\LJumping", Objects::LeftJump, 9);
        loadFrames("Characters\\Gangster 1\\LMelee_attack", Objects::LMeleeAttack, 3);
        loadFrames("Characters\\Gangster 1\\LShooting", Objects::LShooting, 3);
        loadFrames("Characters\\Gangster 1\\LReloading", Objects::LReloading, 16);
    }
    void reload() {
        if (currentAmmo < 10) {
            isReloading = true;
            setActiveAnimation(facingright ? Objects::Reloading : Objects::LReloading);
            currentAmmo = 10;
        }
    }
};
void drawAmmoCounter(const Player& player) {
    gl.setDrawingColor(WHITE);
    char ammoText[20];
    sprintf(ammoText, "Ammo: %d/10", player.currentAmmo);
    gl.drawText(20, 60, ammoText);
}

class NPC : public Player {
public:
    int Bhp = 1000;
    bool isBossDead = false;
    bool BossdeathAnimationComplete = false;
    bool isbossfalling = false;
    void BossAllAnimations() {
        loadFrames("Characters\\NPC\\Idle", Objects::Boss_idle, 13);
        loadFrames("Characters\\NPC\\Dead", Objects::Boss_dead, 4);
        loadFrames("Characters\\NPC\\Hurt", Objects::Boss_hurt, 3);
        loadFrames("Characters\\NPC\\Reloading", Objects::Boss_reload, 5);
        loadFrames("Characters\\NPC\\Shooting", Objects::Boss_shoot, 11);
        loadFrames("Characters\\NPC\\Jump", Objects::Boss_jump, 9);
    }
    void updateBoundingBox() {
        if (isBossDead) {
            setBoundingBox(0, 0, 0, 0); // Remove bounding box
        }
    }
    void Jumping(bool falling = false)
    {
        if (!isJumping)
        {
            isJumping = true;
            yVelocity = jumpStrength * !falling;
            holdJumpDirection = dirx;  // Store the direction when jump starts
            facingright ? setActiveAnimation(Objects::Boss_jump) : setActiveAnimation(Objects::Boss_jump);

        }
    }
    void Physics()
    {
        // Apply gravity and vertical movement
        yVelocity -= gravity;
        y -= yVelocity;

        // Horizontal movement during jump uses holdJumpDirection
        if (isJumping)
        {
            // Use the stored jump direction for consistent momentum
            x += holdJumpDirection * jumpMomentum;
        }

        // Check if the player has landed
        if (y >= gl.getWindowHeight() - 200)
        {
            y = gl.getWindowHeight() - 200;
            yVelocity = 0;

            // Reset animation only if not in another action state
            if (isJumping && !isReloading && !isShooting && !isMeleeAttack)
            {
                setActiveAnimation(Objects::Boss_idle);
            }
            isJumping = false;
        }
    }
    void render() override {
        if (isBossDead && BossdeathAnimationComplete) {
            // Render the last frame of the death animation
            gl.drawImage(sprites[Objects::Boss_dead][totalFramesCount[Objects::Boss_dead] - 1], x, y, gl.generateFromRGB(255, 255, 255));
        }
        else {
            Objects::render();
            if (isBossDead && current_frame >= totalFramesCount[Objects::Boss_dead] - 1) {
                BossdeathAnimationComplete = true;
            }
        }
    }

};
class NPC2 : public Player
{
public:
    bool isDead = false;
    bool deathAnimationComplete = false;
    bool isAttacking = false;
    float lastAttackTime = 0;
    const float ATTACK_COOLDOWN_DURATION = 1.0f;
    int currentAttack = 0;
    int speed = 2.0f;
    float jumpMomentum = 4.0f;    // Horizontal speed during jump
    int jumpDirection = 0;        // Store jump direction
    void npc2Animations()
    {
        loadFrames("Characters\\NPC2\\Idle", Objects::LIdle, 6);
        loadFrames("Characters\\NPC2\\RDead", Objects::NPCDeath, 4);

        loadFrames("Characters\\NPC2\\MeeleAttack", Objects::NPCMeleeAttack, 5);
        loadFrames("Characters\\NPC2\\2MeeleAttack", Objects::Meele2, 5);
        loadFrames("Characters\\NPC2\\3MeeleAttack", Objects::Meele3, 5);

        loadFrames("Characters\\NPC2\\LMeeleAttack", Objects::LNPCMeleeAttack, 5);
        loadFrames("Characters\\NPC2\\L2MeeleAttack", Objects::LMeele2, 5);
        loadFrames("Characters\\NPC2\\L3MeeleAttack", Objects::LMeele3, 5);
        loadFrames("Characters\\NPC2\\Lrun", Objects::run, 9);
        loadFrames("Characters\\NPC2\\Rrun", Objects::Rrun, 9);
        loadFrames("Characters\\NPC2\\LWalk", Objects::NPCLeft, 9);
        loadFrames("Characters\\NPC2\\RWalk", Objects::NPCRight, 9);
        loadFrames("Characters\\NPC2\\RJump", Objects::NPCJump, 9);
        loadFrames("Characters\\NPC2\\LJump", Objects::LNPCJump, 9);
    }
    void moveTowardsPlayer(Player& player) //from GPT: Make the NPC track the player
    {
        if (!isDead) {
            // Calculate direction to player
            int distanceToPlayer = player.x - x;
            bool inMeleeRange = abs(distanceToPlayer) < 40 && abs(player.y - y) < 50;
            float currentTime = clock() / (float)CLOCKS_PER_SEC;
            facingright = distanceToPlayer > 0;
            // Check for melee range and attack
            if (inMeleeRange) {
                if (!isAttacking) {
                    if ((currentTime - lastAttackTime) >= ATTACK_COOLDOWN_DURATION) {
                        isAttacking = true;
                        lastAttackTime = currentTime;
                        // Choose random attack animation
                        currentAttack = rand() % 2;
                        switch (currentAttack) {
                        case 0:
                            setActiveAnimation(facingright ? Objects::NPCMeleeAttack : Objects::LNPCMeleeAttack);
                            break;
                        case 1:
                            setActiveAnimation(facingright ? Objects::Meele2 : Objects::LMeele2);
                            break;
                        case 2:
                            setActiveAnimation(facingright ? Objects::Meele3 : Objects::LMeele3);
                            break;
                        }
                    }
                    else {
                        // Set to idle while waiting for cooldown
                        setActiveAnimation(Objects::LIdle);
                    }
                }
            }
            // Move towards player if not in melee range and not attacking
            else if (!isAttacking && !inMeleeRange && !isJumping) {  // Don't change direction while jumping
                if (abs(distanceToPlayer) > 30) {
                    if (distanceToPlayer > 0) {
                        x += speed;
                        facingright = true;
                        if (activeAnimation != Objects::NPCRight && !isJumping) {
                            setActiveAnimation(Objects::NPCRight);
                        }
                    }
                    else {
                        x -= speed;
                        facingright = false;
                        if (activeAnimation != Objects::NPCLeft && !isJumping) {
                            setActiveAnimation(Objects::NPCLeft);
                        }
                    }
                }
            }

            // Apply jump momentum if jumping
            if (isJumping) {
                x += jumpDirection * jumpMomentum;
            }

            // Handle damage dealing during attack animations
            if (isAttacking && inMeleeRange) {
                bool shouldDealDamage = false;
                int damageFrame = 3;

                if ((activeAnimation == Objects::NPCMeleeAttack || activeAnimation == Objects::LNPCMeleeAttack) &&
                    (int)current_frame == damageFrame) {
                    shouldDealDamage = true;
                }
                else if ((activeAnimation == Objects::Meele2 || activeAnimation == Objects::LMeele2) &&
                    (int)current_frame == damageFrame) {
                    shouldDealDamage = true;
                }
                else if ((activeAnimation == Objects::Meele3 || activeAnimation == Objects::LMeele3) &&
                    (int)current_frame == damageFrame) {
                    shouldDealDamage = true;
                }

                if (shouldDealDamage) {
                    player.hp -= 5;
                    player.isDamaged = true;
                    player.setActiveAnimation(Objects::Damage);
                }
            }
        }
    }

    
    void updateAnimationStates() 
    {
        // Handle jumping animation end
        if (isJumping && current_frame >= totalFramesCount[Objects::NPCJump] - 1) {
            isJumping = false;
            setActiveAnimation(facingright ? Objects::NPCRight : Objects::NPCLeft);
        }

        // Handle attack animation end with proper frame checking
        if (isAttacking) {
            int currentAnimationFrames = 0;

            // Determine the correct frame count based on current animation
            if (activeAnimation == Objects::NPCMeleeAttack || activeAnimation == Objects::LNPCMeleeAttack) {
                currentAnimationFrames = totalFramesCount[Objects::NPCMeleeAttack];
            }
            else if (activeAnimation == Objects::Meele2 || activeAnimation == Objects::LMeele2) {
                currentAnimationFrames = totalFramesCount[Objects::Meele2];
            }
            else if (activeAnimation == Objects::Meele3 || activeAnimation == Objects::LMeele3) {
                currentAnimationFrames = totalFramesCount[Objects::Meele3];
            }

            // Check if current animation has completed
            if (current_frame >= currentAnimationFrames - 1) {
                isAttacking = false;
                setActiveAnimation(facingright ? Objects::NPCRight : Objects::NPCLeft);
            }
        }
    }
    void updateBoundingBox() {
        if (isDead) {
            setBoundingBox(0, 0, 0, 0); // Remove bounding box
        }
    }
    void Jumping(bool falling = false)
    {
        if (!isJumping && !isAttacking)
        {
            isJumping = true;
            yVelocity = jumpStrength * !falling;

            // Store the current direction at jump start
            jumpDirection = facingright ? 1 : -1;

            // Set jump animation based on current facing direction
            setActiveAnimation(facingright ? Objects::NPCJump : Objects::LNPCJump);

            // Apply initial horizontal movement away from bullet
            x += jumpDirection * 40;  // Initial dodge movement
        }
    }
    void render() override {
        if (isDead && deathAnimationComplete) {
            // Render the last frame of the death animation
            gl.drawImage(sprites[Objects::NPCDeath][totalFramesCount[Objects::NPCDeath] - 1], x, y, gl.generateFromRGB(255, 255, 255));
        }
        else {
            Objects::render();
            if (isDead && current_frame >= totalFramesCount[Objects::NPCDeath] - 1) {
                deathAnimationComplete = true;
            }
        }
    }
};

class Bullet : public Objects //from gpt: create a bullet class
{
public:
    bool isActive = false; 
    int speed = 7;         
    int direction = 1;      



    // Method to initialize the bullet
    void fire(int startX, int startY, int dir, bool facingRight) {
        x = startX;
        y = startY;
        direction = dir;
        isActive = true; // Bullet is now active
        setActiveAnimation(facingRight ? Objects::Bullet : Objects::LeftBullet);
    }

    // Method to update the bullet's position
    void update()
    {
        if (isActive)
        {
            x += speed * direction; // Move the bullet in the specified direction

            // Check if the bullet goes off-screen and deactivate it
            if (x < 0 || x > gl.getWindowWidth()) {
                isActive = false; // Bullet is deactivated once it goes off-screen
            }
        }
    }


};

void drawHealthBars(const Player& player, const NPC& Boss, const NPC2& Npc) //from GPT: Put the Health bars in a function
{
    int hpWidth = 200;
    int hpHeight = 30;
    int Position = 20;

    // Player health bar - draw from left to right
    gl.setDrawingColor(WHITE);
    gl.drawRectangle(Position, Position, hpWidth, hpHeight);
    gl.drawText(Position, Position - 20, "Player");

    gl.setDrawingColor(RED);
    float PlayerHP = (player.hp / 100.0f) * hpWidth;
    gl.drawSolidRectangle(Position, Position, (int)PlayerHP, hpHeight);

    // Boss health bar - draw from right to left
    gl.setDrawingColor(WHITE);
    int BossHB = gl.getWindowWidth() - hpWidth - Position;
    gl.drawRectangle(BossHB, Position, hpWidth, hpHeight);
    gl.drawText(BossHB + hpWidth - 203, Position - 20, "BOSS");

    gl.setDrawingColor(RED);
    float BossHP = (Boss.Bhp / 1000.0f) * hpWidth;
    gl.drawSolidRectangle(BossHB + (hpWidth - (int)BossHP), Position, (int)BossHP, hpHeight);

    // NPC health bar
    gl.setDrawingColor(WHITE);
    int NPCHB = gl.getWindowWidth() - hpWidth - Position;
    int NPCPositionY = Position + hpHeight + 30;
    gl.drawRectangle(NPCHB, NPCPositionY, hpWidth, hpHeight);
    gl.drawText(NPCHB + hpWidth - 203, NPCPositionY - 20, "NPC");

    gl.setDrawingColor(RED);
    float NPCHP = (Npc.hp / 100.0f) * hpWidth;
    gl.drawSolidRectangle(NPCHB, NPCPositionY, (int)NPCHP, hpHeight);
}

void updateBullets(Bullet bullets[], int& numBullets)
{
    for (int i = 0; i < numBullets; i++) {
        if (bullets[i].isActive)
        {

            bullets[i].update();

        }

    }
}
void renderBullets(Bullet bullets[], int numBullets) {
    for (int i = 0; i < numBullets; i++) {
        if (bullets[i].isActive) {
            bullets[i].render();
        }
    }
}
void cleanupBullets(Bullet bullets[], int& numBullets) //from GPT: Fix shooting more than 60 bullets will crash the code
{
    for (int i = 0; i < numBullets; i++) {
        if (!bullets[i].isActive) {
            for (int j = i; j < numBullets - 1; j++) {
                bullets[j] = bullets[j + 1];
            }
            numBullets--;
            i--;
        }
    }
}
void handleBossShooting(NPC& Boss, Bullet bullets[], int& numBullets, int& bossTimer, bool& bossShooting, bool& bulletFired)  //from gpt: put it in a function
{
    // Start shooting animation every 2 seconds
    if (!bossShooting && (clock() - bossTimer) / CLOCKS_PER_SEC >= 2) {
        Boss.setActiveAnimation(Objects::Boss_shoot); // Start shooting animation
        bossShooting = true;
        bulletFired = false; // Reset bullet fired flag
    }

    // Fire a bullet on the 8th frame
    if (bossShooting && (int)Boss.current_frame == 8 && !bulletFired)
    {
        if (numBullets < 60) { 
            bullets[numBullets].fire(
                Boss.getLeftEdge() - 20,
                Boss.getUpperEdge() + (Boss.getLowerEdge() - Boss.getUpperEdge()) / 5, -1, false);
            numBullets++;
        }
        bulletFired = true; // Mark bullet as fired
    }

    // Return to idle after the shooting animation completes
    if (bossShooting && Boss.current_frame >= Boss.totalFramesCount[Objects::Boss_shoot] - 1)
    {
        Boss.setActiveAnimation(Objects::Boss_idle);
        bossShooting = false;
        bossTimer = clock(); // Reset timer for the next shooting cycle
    }
}
// Update Boss jump animation
void handleBossJump(NPC& Boss)
{
    if (Boss.activeAnimation == Objects::Boss_jump) {
        // Apply actual vertical movement
        static float jumpVelocity = -20.0f;
        static float gravity = 0.98f;

        Boss.y += jumpVelocity;
        jumpVelocity += gravity;

        // Reset when landing
        if (Boss.y >= gl.getWindowHeight() - 200)
        {
            Boss.y = gl.getWindowHeight() - 200;
            jumpVelocity = -20.0f;
            Boss.setActiveAnimation(Objects::Boss_idle);
        }
    }
}
void handleNPCJump(NPC2& Npc) 
{
    if (Npc.isJumping) {
       
        Npc.yVelocity -= Npc.gravity;
        Npc.y -= Npc.yVelocity;

       
        Npc.x += Npc.jumpDirection * Npc.jumpMomentum;

       
        if (Npc.y >= gl.getWindowHeight() - 200) {
            Npc.y = gl.getWindowHeight() - 200;
            Npc.yVelocity = 0;
            Npc.isJumping = false;

           
            if (!Npc.isAttacking) {
                Npc.setActiveAnimation(Npc.facingright ? Objects::NPCRight : Objects::NPCLeft);
            }
        }
    }
}


void checkCollisions(Player& player, NPC& Boss, NPC2& Npc, Bullet bullets[], Bullet NPCbullets[], int numBullets, int numNPCBullets)
{
    // Check player melee attack collisions with Boss
    if (player.isMeleeAttack && player.current_frame >= 1) {  // Check on frame 1 of melee animation
        int meleeRange = 40;  
        bool inRange = false;

        if (player.facingright) {
            inRange = (Boss.getLeftEdge() - player.getRightEdge()) < meleeRange;
        }
        else {
            inRange = (player.getLeftEdge() - Boss.getRightEdge()) < meleeRange;
        }

        if (inRange) {
            Boss.Bhp -= 40;  // Melee damage
            if (Boss.Bhp < 0) Boss.Bhp = 0;
        }
    }

    // Check player bullet collisions with Boss
    for (int i = 0; i < numBullets; i++) {
        if (bullets[i].isActive) {
            bool hitBoss = bullets[i].getLeftEdge() < Boss.getRightEdge() &&
                bullets[i].getRightEdge() > Boss.getLeftEdge() &&
                bullets[i].getUpperEdge() < Boss.getLowerEdge() &&
                bullets[i].getLowerEdge() > Boss.getUpperEdge();

            bool hitNpc = bullets[i].getLeftEdge() < Npc.getRightEdge() &&
                bullets[i].getRightEdge() > Npc.getLeftEdge() &&
                bullets[i].getUpperEdge() < Npc.getLowerEdge() &&
                bullets[i].getLowerEdge() > Npc.getUpperEdge();

            if (hitBoss)
            {
                Boss.Bhp -= 10;  
                if (Boss.Bhp < 0) Boss.Bhp = 0;

                bullets[i].isActive = false;
            }
            if (hitNpc) {
                Npc.hp -= 5; 
                if (Npc.hp < 0) Npc.hp = 0;
                bullets[i].isActive = false;
            }
        }
    }

    // Check NPC bullet collisions with player
    for (int i = 0; i < numNPCBullets; i++) {
        if (NPCbullets[i].isActive) {
            bool hitPlayer = NPCbullets[i].getLeftEdge() < player.getRightEdge() &&
                NPCbullets[i].getRightEdge() > player.getLeftEdge() &&
                NPCbullets[i].getUpperEdge() < player.getLowerEdge() &&
                NPCbullets[i].getLowerEdge() > player.getUpperEdge();

            if (hitPlayer) {
                player.hp -= 10;  
                player.isDamaged = true;
                player.setActiveAnimation(Objects::Damage);
                if (player.hp <= 0) {
                    player.hp = 0;
                    player.setActiveAnimation(Objects::Death);
                }
                NPCbullets[i].isActive = false;
            }
        }
    }
}
void showGameOver(bool playerWon)
{
    int victoryImage = gl.loadImage("victory.png");
    int defeatImage = gl.loadImage("Lost.png");
    gl.resizeImage(victoryImage, gl.getWindowWidth(), gl.getWindowHeight());
    gl.resizeImage(defeatImage, gl.getWindowWidth(), gl.getWindowHeight());

    int startTime = clock();

    while ((clock() - startTime) / CLOCKS_PER_SEC < 5) { 
        gl.beginDraw();
        if (playerWon) {
            gl.drawImage(victoryImage, 0, 0, gl.generateFromRGB(0, 0, 0));
        }
        else {
            gl.drawImage(defeatImage, 0, 0, gl.generateFromRGB(0, 0, 0));
        }
        gl.endDraw();
    }
}

void waitFor(int start, int threshold) //from friday sessions
{
    while (true)
    {
        float _t = (clock() / (float)CLOCKS_PER_SEC * 1000.0f - start / (float)CLOCKS_PER_SEC * 1000.0f);
        if (_t >= threshold)
            break;
    }
}

void Game()
{
    PlaySound(TEXT("C:\\Users\\PC\\source\\repos\\Graphics session Game\\Graphics session Game\\tolbawar.wav"), NULL, SND_FILENAME | SND_LOOP | SND_ASYNC); //from youtube : How to add music in visual studio
    Objects background;
    Player player;
    NPC Boss;
    NPC2 Npc;
    Bullet bullets[60], NPcbullets[60];  

    player.hp = 100;  
    Boss.Bhp = 1000;  

    int numBullets = 0;
    int numNPCBullets = 0;
    bool bulletActive = false;
    int bulletSpeed = 1;      

    int bossTimer = clock(); 
    bool bossShooting = false; 
    bool bulletFired = false;    
    int npcAttackTimer = clock();
    int npcJumpTimer = clock();
    int bossJumpTimer = clock();
    bool bossCanJump = true;
    bool npcCanJump = true;

   
    for (int i = 0; i < 60; i++)
    {
        bullets[i].loadFrames("bullet\\bullets", Objects::Bullet, 12);
        bullets[i].loadFrames("bullet\\Lbullets", Objects::LeftBullet, 12);

        bullets[i].setActiveAnimation(player.facingright ? Objects::Bullet : Objects::LeftBullet);
        bullets[i].y = gl.getWindowHeight() - 125;

        NPcbullets[i].loadFrames("bullet\\Lbullets", Objects::LeftBullet, 12);
        NPcbullets[i].setActiveAnimation(Objects::LeftBullet);
        NPcbullets[i].x = Boss.x;
        NPcbullets[i].y = Boss.y;
    }

    
    background.loadFrames("bg", Objects::background, 4, true);
    background.setActiveAnimation(Objects::background);


    player.PlayerAllAnimations();
    Boss.BossAllAnimations();
    Npc.npc2Animations();

    
    player.setBoundingBox(43, 80, 55, 130);
    Boss.setBoundingBox(53, 84, 55, 130);
    Npc.setBoundingBox(53, 84, 55, 130);

    for (int i = 0; i < 60; i++)
    {
        bullets[i].setBoundingBox(9, 25, 25, 10);
        NPcbullets[i].setBoundingBox(9, 25, 25, 10);

    }

   
    player.setActiveAnimation(Objects::RIdle);
    player.x = 100;
    player.y = gl.getWindowHeight() - 200;
    player.hp = 100;

    Boss.setActiveAnimation(Objects::Boss_idle);
    Boss.x = gl.getWindowWidth() - 100;
    Boss.y = gl.getWindowHeight() - 200;
    Boss.hp = 100;

    Npc.setActiveAnimation(Objects::LIdle);
    Npc.x = gl.getWindowWidth() - 200;
    Npc.y = gl.getWindowHeight() - 200;
    Npc.hp = 100;



    // Game loop
    while (true) {
        int frameStart = clock();
        if (player.x < -70 || player.x >= Boss.x)
            player.x = 0;

        // Handle debug toggle
        if (isKeyPressedOnce('D')) {
            debug = !debug;
        }

        // Handle movement and animation
        if (isKeyPressedOnce(VK_RIGHT)) {
            player.dirx = 1;
            player.facingright = true;
            if (!player.isReloading) {
                player.setActiveAnimation(Objects::Right);
            }
        }
        else if (isKeyPressedOnce(VK_LEFT)) {
            player.dirx = -1;
            player.facingright = false;
            if (!player.isReloading) {
                player.setActiveAnimation(Objects::Left);
            }
        }
        // Handle jumping
        else if (isKeyPressedOnce(VK_SPACE) && player.y >= gl.getWindowHeight() - 200)
        {
            player.Jumping();
        }




        // Handle shooting
        else if (isKeyPressedOnce('C')) {
            if (!player.isReloading && player.currentAmmo > 0 && numBullets < 60) {
                player.currentAmmo--;
                player.isShooting = true;
                player.setActiveAnimation(player.facingright ? Objects::Shooting : Objects::LShooting);

                if (player.facingright) {
                    bullets[numBullets].fire(player.getCenterX(), player.getUpperEdge() + (player.getLowerEdge() - player.getUpperEdge()) / 3.5, 1, true);
                }
                else {
                    bullets[numBullets].fire(player.getCenterX(), player.getUpperEdge() + (player.getLowerEdge() - player.getUpperEdge()) / 3.5, -1, false);
                }
                numBullets++;

                if (player.currentAmmo == 0) {
                    player.needsReload = true;
                }
            }
        }

        // Handle reloading
        else if (isKeyPressedOnce('R'))
        {
            if (!player.isShooting) {
                player.isReloading = true;
                player.setActiveAnimation(player.facingright ? Objects::Reloading : Objects::LReloading);
                player.currentAmmo = 10;
            }
            else {
                player.isShooting = false;
                player.isReloading = true;
                player.setActiveAnimation(player.facingright ? Objects::Reloading : Objects::LReloading);
            }
        }
        // Handle melee attack
        else if (isKeyPressedOnce('V')) {
            if (!player.isReloading && !player.isShooting) {
                player.isMeleeAttack = true;
                player.setActiveAnimation(player.facingright ? Objects::MeleeAttack : Objects::LMeleeAttack);
            }
        }

        //friday sessions
        if (GetAsyncKeyState(VK_RIGHT) & 0x8000)
        {
       
            player.movement();
        }
        else if (GetAsyncKeyState(VK_LEFT) & 0x8000)
        {
            player.movement();
        }

       
        if (!GetAsyncKeyState(VK_LEFT) && !GetAsyncKeyState(VK_RIGHT)) {
            if (player.dirx != 0 && !player.isReloading && !player.isShooting && !player.isJumping)
            {
                player.setActiveAnimation(player.facingright ? Objects::RIdle : Objects::LIdle);
            }

            player.dirx = 0;
        }

        // Ensure reloading and shooting stop correctly

        if (player.isReloading && player.current_frame >= player.totalFramesCount[Objects::Reloading] - 1)
        {
            player.isReloading = false;
            player.setActiveAnimation(player.facingright ? Objects::RIdle : Objects::LIdle);
        }

        if (player.isShooting && player.current_frame >= player.totalFramesCount[Objects::Shooting] - 1) {
            player.isShooting = false;
            if (GetAsyncKeyState(VK_RIGHT) & 0x8000 || GetAsyncKeyState(VK_LEFT) & 0x8000) {
                player.setActiveAnimation(player.facingright ? Objects::Right : Objects::Left);
            }
            else {
                player.setActiveAnimation(player.facingright ? Objects::RIdle : Objects::LIdle);
            }
        }
        if (player.isJumping && player.current_frame >= player.totalFramesCount[Objects::Jump] - 1) {
            player.isJumping = false;
            if (GetAsyncKeyState(VK_RIGHT) & 0x8000 || GetAsyncKeyState(VK_LEFT) & 0x8000) {
                player.setActiveAnimation(player.facingright ? Objects::Right : Objects::Left);
            }
            else {
                player.setActiveAnimation(player.facingright ? Objects::RIdle : Objects::LIdle);
            }
        }

        if (player.isMeleeAttack && player.current_frame >= player.totalFramesCount[Objects::MeleeAttack] - 1)
        {
            player.isMeleeAttack = false;
            player.setActiveAnimation(player.facingright ? Objects::RIdle : Objects::LIdle);
        }

        //Boss dodge logic
        for (int i = 0; i < numBullets; i++)
        {
            if (bullets[i].isActive && bossCanJump)
            {
                float bulletDistance = abs(bullets[i].x - Boss.x);
                if (bulletDistance < 200 && bulletDistance > 50)
                {
                    Boss.setActiveAnimation(Objects::Boss_jump);
                    Boss.Jumping();
                    bossCanJump = false;
                    bossJumpTimer = clock();
                }
            }
        }
        //Reset boss jump ability
        if (!bossCanJump && (clock() - bossJumpTimer) / CLOCKS_PER_SEC >= 2) {
            bossCanJump = true;
        }

        // NPC dodge logic
        for (int i = 0; i < numBullets; i++) {
            if (bullets[i].isActive && npcCanJump && !Npc.isDead && !Npc.isAttacking) {
                float bulletDistance = abs(bullets[i].x - Npc.x);
                if (bulletDistance < 200 && bulletDistance > 80) {
                    
                    Npc.facingright = (bullets[i].x > Npc.x);  

                    // Trigger jump
                    Npc.Jumping();

                    npcCanJump = false;
                    npcJumpTimer = clock();
                    break;  
                }
            }
        }
        // Reset NPC jump ability
        if (!npcCanJump && (clock() - npcJumpTimer) / CLOCKS_PER_SEC >= 1)
        {
            npcCanJump = true;
        }
        // Check if NPC is dead
        if (Npc.hp <= 0 && !Npc.isDead) {
            Npc.isDead = true;
            Npc.setActiveAnimation(Objects::NPCDeath);
            Npc.updateBoundingBox();
        }
        // Check if Boss is dead
        if (Boss.Bhp <= 0 && !Boss.isBossDead)
        {
            Boss.isBossDead = true;
            Boss.setActiveAnimation(Objects::Boss_dead);
        }
        if (!Npc.isDead) {
            Npc.moveTowardsPlayer(player); 
            Npc.updateAnimationStates();
            Npc.Physics();
        }
        if (player.hp <= 0)
            player.setActiveAnimation(Objects::Death);
        // Handle boss shooting logic and npc logic

        handleBossJump(Boss);
        handleNPCJump(Npc);


        handleBossShooting(Boss, NPcbullets, numNPCBullets, bossTimer, bossShooting, bulletFired);
        checkCollisions(player, Boss, Npc, bullets, NPcbullets, numBullets, numNPCBullets);
        // Update physics and bullets

        player.Physics();
        Boss.Physics();
        updateBullets(bullets, numBullets);
        updateBullets(NPcbullets, numNPCBullets);



        gl.beginDraw();
        background.render();
        player.render();
        Boss.render();
        Npc.render();
        drawAmmoCounter(player);
        renderBullets(bullets, numBullets);
        renderBullets(NPcbullets, numNPCBullets);
        drawHealthBars(player, Boss, Npc);
        gl.endDraw();

        cleanupBullets(bullets, numBullets);
        cleanupBullets(NPcbullets, numNPCBullets);


        if (player.hp <= 0 || Boss.Bhp <= 0)
        {
            showGameOver(player.hp > 0);
            return;
        }

        waitFor(frameStart, 1000 / 60);	//Time Control End
    }

}

class Button  //from gpt: create 2 buttons for the user to click
{
private:
    int x, y, width, height;
    const char* label;
    void (*onClick)();

public:
    // Constructor
    Button(int _x, int _y, int _width, int _height, const char* _label, void (*_onClick)())
    {
        x = _x;
        y = _y;
        width = _width;
        height = _height;
        label = _label;
        onClick = _onClick;
    }

    void draw(COLORS bgColor, COLORS textColor) {
        gl.setDrawingColor(bgColor);
        gl.drawSolidRectangle(x, y, width, height);
        gl.setDrawingColor(textColor);
        gl.drawText(x + 10, y + height / 4, label);
    }

    bool isButtonClicked(int px, int py) {
        return px >= x && px <= x + width && py >= y && py <= y + height;
    }

    void click()
    {
        if (onClick)
            onClick();
    }
};

void exitGame()
{
    exit(0);
}

void handleMenuInput(Button& playButton, Button& exitButton) //from Gpt
{
    if (GetAsyncKeyState(VK_LBUTTON) & 0x8000) {
        POINT p;
        GetCursorPos(&p);
        ScreenToClient(GetConsoleWindow(), &p);

        if (playButton.isButtonClicked(p.x, p.y)) playButton.click();
        if (exitButton.isButtonClicked(p.x, p.y)) exitButton.click();
    }
}

int main() {
    gl.setup();
    gl.setFullScreenMode();
    gl.hideCursor();
    disableQuickEditMode();

    int screenWidth = gl.getWindowWidth();
    int screenHeight = gl.getWindowHeight();


    Button playButton(screenWidth / 2 - 100, screenHeight / 2 - 100, 200, 50, "Play", Game);
    Button exitButton(screenWidth / 2 - 100, screenHeight / 2, 200, 50, "Exit", exitGame);

    int UI = gl.loadImage("UI.png");
    gl.resizeImage(UI, screenWidth, screenHeight);

    while (true) {

        gl.beginDraw();
        gl.drawImage(UI, 0, 0, gl.generateFromRGB(0, 0, 0));
        playButton.draw(CYAN, BLACK);
        exitButton.draw(RED, BLACK);
        gl.endDraw();

        handleMenuInput(playButton, exitButton);

    }

    return 0;
}