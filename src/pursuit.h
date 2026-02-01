/* File: pursuit.h */

#ifndef PURSUIT_H
#define PURSUIT_H

#include "angband.h"

/*
 * Prepare to bring a monster to the next level.
 * Called when the player uses stairs.
 */
void prepare_staircase_pursuit(void);

/*
 * Spawn the pursuing monster on the new level.
 * Called after level generation.
 */
void execute_staircase_pursuit(void);

/*
 * Prepare to bring adjacent monsters to town via recall.
 * Called when Word of Recall activates.
 */
void prepare_recall_ambush(void);

/*
 * Spawn the ambushers in town.
 * Called after level (town) generation.
 */
void execute_recall_ambush(void);

/*
 * Reset dread timers.
 * Called at level generation.
 */
void reset_dread(void);

/*
 * Process atmospheric dread messages.
 * Called every turn.
 */
void process_dread(void);

/*
 * Process breathing walls effect.
 * Called every turn.
 */
void process_breathing_walls(void);

#endif /* PURSUIT_H */
