#include "graphics.h"
#include "gba.h"
#include "sprites.h"
#include "./images/tennisCourtBackground.h"
#include <string.h>

volatile OamEntry *playerSprite = &shadow[0];
volatile OamEntry *cpuSprite = &shadow[1];
volatile OamEntry *ballSprite = &shadow[2];

void drawBall(Ball ball)
{
    ballSprite->attr0 = (ball.y - 2) | SPRITES_PALETTE_TYPE | TENNIS_BALL_SPRITE_SHAPE;
    ballSprite->attr1 = (ball.x - 2) | TENNIS_BALL_SPRITE_SIZE;
    ballSprite->attr2 = TENNIS_BALL_ID | TENNIS_BALL_PALETTE_ID;
}

int getSwingFrame(int swingFrameCounter)
{
    int frame = (SWING_FRAME_COUNTER_START - swingFrameCounter) / 4;
    if (frame > 2)
    {
        frame = 2;
    }
    return frame;
}

void hideSprites(void)
{
    initializeSprites();
    drawSprites();
}

void drawPlayer(Player player, int currentlyServing)
{
    if (player.racketHitBox.enabled)
    {
        int frame = getSwingFrame(player.swingFrameCounter);
        playerSprite->attr0 = (player.y - 5) | SPRITES_PALETTE_TYPE | SWING_BLUE_SPRITE_SHAPE;
        playerSprite->attr1 = player.x | SWING_BLUE_SPRITE_SIZE;
        playerSprite->attr2 = swing_blue_frames[frame] | swing_blue_palettes[frame];
    }
    else if (currentlyServing)
    {
        playerSprite->attr0 = (player.y - 5) | SPRITES_PALETTE_TYPE | SERVE_BLUE_SPRITE_SHAPE;
        playerSprite->attr1 = player.x | SERVE_BLUE_SPRITE_SIZE;
        playerSprite->attr2 = SERVE_BLUE_ID | SERVE_BLUE_PALETTE_ID;
    }
    else
    {
        playerSprite->attr0 = (player.y - 5) | SPRITES_PALETTE_TYPE | BLUE_RUNNING_FRAME_1_SPRITE_SHAPE;
        playerSprite->attr1 = player.x | BLUE_RUNNING_FRAME_1_SPRITE_SIZE;
        if (player.x % 30 < 15)
        {
            playerSprite->attr2 = BLUE_RUNNING_FRAME_1_ID | BLUE_RUNNING_FRAME_1_PALETTE_ID;
        }
        else
        {
            playerSprite->attr2 = BLUE_RUNNING_FRAME_2_ID | BLUE_RUNNING_FRAME_2_PALETTE_ID;
        }
    }
}

void drawCpu(Player cpu)
{
    if (cpu.racketHitBox.enabled)
    {
        int frame = getSwingFrame(cpu.swingFrameCounter);
        cpuSprite->attr0 = (cpu.y - 5) | SPRITES_PALETTE_TYPE | SWING_RED_SPRITE_SHAPE;
        cpuSprite->attr1 = (cpu.x - PLAYER_WIDTH) | SWING_RED_SPRITE_SIZE;
        cpuSprite->attr2 = swing_red_frames[frame] | swing_red_palettes[frame];
    }
    else
    {
        cpuSprite->attr0 = (cpu.y - 5) | SPRITES_PALETTE_TYPE | RED_RUNNING_FRAME_1_SPRITE_SHAPE;
        cpuSprite->attr1 = (cpu.x - PLAYER_WIDTH) | RED_RUNNING_FRAME_1_SPRITE_SIZE;
        if (cpu.x % 30 < 15)
        {
            cpuSprite->attr2 = RED_RUNNING_FRAME_1_ID | RED_RUNNING_FRAME_1_PALETTE_ID;
        }
        else
        {
            cpuSprite->attr2 = RED_RUNNING_FRAME_2_ID | RED_RUNNING_FRAME_2_PALETTE_ID;
        }
    }
}

void drawScoreBoard(Score score)
{
    char scoreStr[80];
    if ((score.player == 3) && (score.cpu == 3))
    {
        sprintf(scoreStr, "Deuce");
    }
    else
    {
        sprintf(scoreStr, "%s - %s", possibleScores[score.player], possibleScores[score.cpu]);
    }
    drawCenteredString(0, 5, SCREEN_WIDTH, 10, scoreStr, BLACK);
}

void drawMatchStandings(MatchStandings matchStandings, int yOffset)
{
    drawCenteredString(0, yOffset, SCREEN_WIDTH, 10, "S1  S2  S3", WHITE);
    for (int i = 0; i < MATCH_LENGTH; i++)
    {
        u16 color = matchStandings.setWinsByColor[i];
        if (!color)
        {
            color = WHITE;
        }
        char *character = matchStandings.setsCompleted[i];
        drawString(MATCH_STANDINGS_X_OFFSET(i), yOffset + 15, character, color);
    }
}

// This function will be used to draw everything about the app
// including the background and whatnot.
void fullDrawAppState(AppState *state)
{
    drawFullScreenImageDMA(tennis_court_background);
    initializeSprites();
    UNUSED(state);
}

void undrawAppState(AppState *state)
{
    if (state->player.racketHitBox.debugColor)
    {
        //Undraw hitbox when debug color is on
        HitBox hitBox = state->player.racketHitBox;
        drawFullScreenImagePortionDMA(hitBox.x, hitBox.y, hitBox.size, hitBox.size, tennis_court_background);
    }

    if (state->cpu.racketHitBox.debugColor)
    {
        //Undraw hitbox when debug color is on
        HitBox hitBox = state->cpu.racketHitBox;
        drawFullScreenImagePortionDMA(hitBox.x, hitBox.y, hitBox.size, hitBox.size, tennis_court_background);
    }

    if (state->serveStarted || state->textDisplayQueue)
    {
        //Undraw text instructions
        drawFullScreenImagePortionDMA(70, 40, 100, 40, tennis_court_background);
    }

    if (state->ball.landingDebugColor)
    {
        //Undraw ball when debug color is on
        drawFullScreenImagePortionDMA(state->ball.expectedLandingX, GROUND, 5, 5, tennis_court_background);
    }

    // Undraw scoreboard
    drawFullScreenImagePortionDMA(0, 5, SCREEN_WIDTH, 10, tennis_court_background);
}

void drawAppState(AppState *state)
{
    drawPlayer(state->player, (state->playerServing && state->serveStarted));
    drawCpu(state->cpu);
    if (state->textDisplayQueue == NULL)
    {
        drawBall(state->ball);
    }
    drawScoreBoard(state->score);
    drawSprites();

    if (state->textDisplayQueue != NULL)
    {
        TextDisplay textDisplay = *state->textDisplayQueue;
        u16 color = textDisplay.color ? textDisplay.color : BLACK;
        drawCenteredString(0, 41, SCREEN_WIDTH, 10, textDisplay.text, BLACK);
        drawCenteredString(0, 40, SCREEN_WIDTH, 10, textDisplay.text, color);
        if (!strcmp(textDisplay.text, "Match Standings"))
        {
            drawMatchStandings(textDisplay.matchStandings, 55);
        }
    }
    else if (!(state->serveStarted) && (state->playerServing))
    {
        drawCenteredString(0, 41, SCREEN_WIDTH, 10, "Press B to serve", BLACK);
        drawCenteredString(0, 40, SCREEN_WIDTH, 10, "Press B to serve", WHITE);
    }
    if (state->ball.landingDebugColor)
    {
        drawRectDMA(state->ball.expectedLandingX, GROUND, 5, 5, state->ball.landingDebugColor);
    }
}
